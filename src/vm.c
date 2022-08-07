#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

static void push(Value value);
static Value pop();
static Value peek(int depth);

VM vm;
Chunk compilingChunk;

void initVM() {
    vm.stackTop = vm.stack;
    vm.count = 0;
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.count = 0;
}

static void run() {
#define READ_BYTE() (*vm.ip++)
    for (;;) {
        // disassembleInstruction(&compilingChunk, vm.ip -
        // compilingChunk.codes);
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
        case OP_ADD: {
            Value a = pop();
            Value b = pop();
            push(NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b)));
            break;
        }
        case OP_SUBTRACT: {
            Value b = pop();
            Value a = pop();
            push(NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b)));
            break;
        }
        case OP_DIVIDE: {
            Value b = pop();
            Value a = pop();
            push(NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b)));
            break;
        }
        case OP_MULTIPLY: {
            Value b = pop();
            Value a = pop();
            push(NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b)));
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
            Value val = pop();
            push(val != TRUE_VAL ? TRUE_VAL : FALSE_VAL);
            break;
        }
        case OP_CONSTANT: {
            Value val = getConstantAtIndex(&compilingChunk, READ_BYTE());
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
        case OP_RETURN:
            printValue(pop());
            goto end;
            break;
        }
    }
end:
    return;
#undef READ_BYTE
}

void interpret(const char *source) {
    initChunk(&compilingChunk);
    initVM();
    compile(source, &compilingChunk);
    vm.ip = compilingChunk.codes;
    run();
    freeChunk(&compilingChunk);
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
