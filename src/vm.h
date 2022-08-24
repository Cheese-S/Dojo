#ifndef dojo_vm_h
#define dojo_vm_h

#include "common.h"
#include "hashmap.h"
#include "object.h"
#include "value.h"

#define FRAME_MAX 256
#define STACK_MAX (UINT8_COUNT * 64)

typedef struct {
    ObjClosure *closure;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAME_MAX];
    Value stack[STACK_MAX];
    Value *stackTop;
    Obj *objs;
    Hashmap stringLiterals;
    Hashmap globals;
    ObjUpvalue *openUpvalues;
    bool isREPL;
    int frameCount;
    int count;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpreterResult;

extern VM vm;

InterpreterResult interpret(const char *source);
void initVM(bool isREPL);
void push(Value value);
Value pop();

#endif
