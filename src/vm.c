#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "error.h"
#include "hashmap.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void terminateVM();

static InterpreterResult run();

static Value makeStrTemplate(int numSpans);
static bool call(Value callee, int argCount);
static bool callClosure(ObjClosure *closure, int argCount);
static bool callNativeFn(ObjNativeFn *fn, int argCount);
static void defineNativeFn(const char *name, NativeFn fn);

static void closeUpvalues(Value *last);
static ObjUpvalue *captureUpvalue(Value *local);
static ObjUpvalue *findOpenUpvalueParent(Value *local);

static bool isFalsey();
static CallFrame *lastCallFrame();
static void resetStack();
static void push(Value value);
static Value pop();
static Value peek(int depth);

static Value clockNative(int argCount, Value *args);

VM vm;

InterpreterResult interpret(const char *source) {
    ObjFn *fn = compile(source);
    if (!fn) {
        // TODO: free all the resources
        return INTERPRET_COMPILE_ERROR;
    }
    push(OBJ_VAL(fn));
    ObjClosure *closure = newObjClosure(fn);
    pop();
    push(OBJ_VAL(closure));
    callClosure(closure, 0);
    InterpreterResult res = run();
    return res;
    // terminateVM();
}

void initVM(bool isREPL) {
    resetStack();
    initMap(&vm.stringLiterals);
    initMap(&vm.globals);
    defineNativeFn("clock", clockNative);
    vm.objs = NULL;
    vm.isREPL = isREPL;
}

static void terminateVM() {
    freeObjs(vm.objs);
    freeMap(&vm.stringLiterals);
    freeMap(&vm.globals);
    terminateScanner();
}

static InterpreterResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    register uint8_t *ip = frame->ip;
#define LOAD_IP_REGISTER ip = frame->ip
#define SAVE_IP_REGISTER frame->ip = ip
#define READ_BYTE() (*ip++)
#define READ_CONSTANT()                                                        \
    (getConstantAtIndex(&frame->closure->fn->chunk, READ_BYTE()))
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
#define ARITHEMETIC_BINARY_OP(valueType, op)                                   \
    do {                                                                       \
        double b = AS_NUMBER(pop());                                           \
        double a = AS_NUMBER(pop());                                           \
        push(valueType(a op b));                                               \
    } while (false)

    for (;;) {
        // disassembleInstruction(&frame->closure->fn->chunk,
        //                        (int)(ip - frame->closure->fn->chunk.codes));
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
        case OP_CLOSURE: {
            ObjFn *fn = AS_FN(READ_CONSTANT());
            ObjClosure *closure = newObjClosure(fn);
            push(OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case OP_CALL: {
            int argCount = READ_BYTE();
            SAVE_IP_REGISTER;
            if (!call(peek(argCount), argCount)) {
                runtimeError("Can only call functions and methods");
                return INTERPRET_RUNTIME_ERROR;
            }

            frame = &vm.frames[vm.frameCount - 1];
            LOAD_IP_REGISTER;
            break;
        }
        case OP_RETURN: {
            Value result = pop();
            closeUpvalues(frame->slots);
            vm.frameCount--;
            if (vm.frameCount == 0) {
                pop();
                return INTERPRET_OK;
            }

            vm.stackTop = frame->slots;
            push(result);
            frame = &vm.frames[vm.frameCount - 1];
            LOAD_IP_REGISTER;
            break;
        }
        case OP_DEFINE_GLOBAL: {
            ObjString *name = READ_STRING();
            mapPut(&vm.globals, name, peek(0));
            pop();
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString *name = READ_STRING();
            Value val;
            if (!mapGet(&vm.globals, name, &val)) {
                SAVE_IP_REGISTER;
                runtimeError("Undefined Variable '%.*s'", name->length,
                             name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(val);
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (mapPut(&vm.globals, name, peek(0))) {
                mapDelete(&vm.globals, name);
                SAVE_IP_REGISTER;
                runtimeError("Undefined Variable '%.*s'", name->length,
                             name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_LOCAL: {
            int slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            int slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        case OP_CLOSE_UPVALUE: {
            closeUpvalues(vm.stackTop - 1);
            pop();
            break;
        }
        case OP_LOOP: {
            uint16_t jump = READ_SHORT();
            ip -= jump;
            break;
        }
        case OP_JUMP: {
            uint16_t jump = READ_SHORT();
            ip += jump;
            break;
        }
        case OP_JUMP_IF_TRUE: {
            uint16_t jump = READ_SHORT();
            if (!isFalsey(peek(0))) {
                ip += jump;
            }
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t jump = READ_SHORT();
            if (isFalsey(peek(0))) {
                ip += jump;
            }
            break;
        }
        case OP_PRINT: {
            printValue(pop());
            printf("\n");
            break;
        }
        case OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(a == b));
            break;
        }
        case OP_NOT_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(a != b));
            break;
        }
        case OP_LESS: {
            ARITHEMETIC_BINARY_OP(BOOL_VAL, <);
            break;
        }
        case OP_LESS_EQUAL: {
            ARITHEMETIC_BINARY_OP(BOOL_VAL, <=);
            break;
        }
        case OP_GREATER: {
            ARITHEMETIC_BINARY_OP(BOOL_VAL, >);
            break;
        }
        case OP_GREATER_EQUAL: {
            ARITHEMETIC_BINARY_OP(BOOL_VAL, >=);
            break;
        }
        case OP_ADD: {
            ARITHEMETIC_BINARY_OP(NUMBER_VAL, +);
            break;
        }
        case OP_SUBTRACT: {
            ARITHEMETIC_BINARY_OP(NUMBER_VAL, -);
            break;
        }
        case OP_DIVIDE: {
            ARITHEMETIC_BINARY_OP(NUMBER_VAL, /);
            break;
        }
        case OP_MULTIPLY: {
            ARITHEMETIC_BINARY_OP(NUMBER_VAL, *);
            break;
        }
        case OP_TEMPLATE: {
            int numSpans = READ_BYTE();
            // one span contains an expression and a string litreal
            // we also need to pop the template head
            push(makeStrTemplate(numSpans * 2 + 1));
            break;
        }
        case OP_NEGATE: {
            Value val = pop();
            if (!IS_NUMBER(val)) {
                // TODO: ADD RUNTIME ERROR
            }
            push(NUMBER_VAL(-AS_NUMBER(val)));
            break;
        }
        case OP_NOT: {
            push(BOOL_VAL(isFalsey(pop())));
            break;
        }
        case OP_CONSTANT: {
            Value val = READ_CONSTANT();
            push(val);
            break;
        }
        case OP_NIL:
            push(NIL_VAL);
            break;
        case OP_TRUE:
            push(TRUE_VAL);
            break;
        case OP_FALSE:
            push(FALSE_VAL);
            break;

        case OP_POP:
            pop();
            break;
        case OP_POPN: {
            uint8_t n = READ_BYTE();
            while (n--) {
                pop();
            }
            break;
        }
        }
    }
#undef ARITHEMETIC_BINARY_OP
#undef READ_STRING
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_BYTE
#undef SAVE_IP_REGISTER
#undef LOAD_IP_REGISTER
}

static bool call(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_CLOSURE:
            return callClosure(AS_CLOSURE(callee), argCount);
        case OBJ_NATIVE_FN: {
            return callNativeFn(AS_NATIVE_FN(callee), argCount);
        }
        default:
            break;
        }
    }
    return false;
}

static bool callClosure(ObjClosure *closure, int argCount) {
    ObjFn *fn = closure->fn;
    if (argCount != fn->arity) {
        runtimeError("Expected %d arguments but got %d", fn->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAME_MAX) {
        runtimeError("Stack overflow");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = fn->chunk.codes;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callNativeFn(ObjNativeFn *fn, int argCount) {
    if (argCount != fn->arity) {
        runtimeError("Expected %d arguments but got %d", fn->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAME_MAX) {
        runtimeError("Stack overflow");
        return false;
    }

    Value result = fn->fn(argCount, vm.stackTop - argCount);
    vm.stackTop -= argCount + 1;
    push(result);
    return true;
}

static void defineNativeFn(const char *name, NativeFn native) {
    push(newObjStringInVal(name, (int)strlen(name)));
    push(OBJ_VAL(newObjNativeFn(native)));
    mapPut(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static void closeUpvalues(Value *last) {
    while (vm.openUpvalues && vm.openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        // We retrieve the value from the stack and then modify the location of
        // the value In this way, accessing an open/closed upvalue remains the
        // same to the user
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prev = NULL;
    ObjUpvalue *current = vm.openUpvalues;
    while (current && current->location > local) {
        prev = current;
        current = current->next;
    }

    if (current && current->location == local) {
        return current;
    }

    ObjUpvalue *createdUpvalue = newObjUpvalue(local);
    createdUpvalue->next = current;

    if (prev == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prev->next = createdUpvalue;
    }

    return createdUpvalue;
}

static bool isFalsey(Value val) {
    return IS_NIL(val) || (IS_NUMBER(val) && !AS_NUMBER(val)) ||
           (IS_BOOL(val) && !AS_BOOL(val));
}

static CallFrame *lastCallFrame() {
    return &vm.frames[vm.frameCount - 1];
}

static Value makeStrTemplate(int numSpans) {
    FILE *stream;
    char *buf;
    size_t len;
    stream = open_memstream(&buf, &len);
    if (stream == NULL) {
        fprintf(stderr, "DOJO ERROR: Running out of memory\n");
        exit(1);
    }
    while (numSpans--) {
        printValueToFile(stream, pop());
    }
    fflush(stream);
    fclose(stream);

    ObjString *str = newObjString(buf, len);
    markUsingHeap(str);
    return OBJ_VAL(str);
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

static void push(Value value) {
    *(vm.stackTop++) = value;
    vm.count++;
}

static Value pop() {
    vm.count--;
    return *(--vm.stackTop);
}

static Value peek(int depth) {
    return *(vm.stackTop - depth - 1);
}

static Value clockNative(int argCount, Value *args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}