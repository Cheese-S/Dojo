#ifndef dojo_vm_h
#define dojo_vm_h

#include "common.h"
#include "hashmap.h"
#include "object.h"
#include "value.h"

#define FRAME_MAX 256
#define STACK_MAX (UINT8_COUNT * 64)

typedef struct {
    Value stack[STACK_MAX];
    Value *stackTop;
    Obj *objs;
    uint8_t *ip;
    Hashmap stringLiterals;
    Hashmap globals;
    int count;
    bool isREPL;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpreterResult;

extern VM vm;

InterpreterResult interpret(const char *source);
void initVM(bool isREPL);

#endif
