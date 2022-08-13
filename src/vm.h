#ifndef dojo_vm_h
#define dojo_vm_h

#include "common.h"
#include "hashmap.h"
#include "value.h"

#define FRAME_MAX 256
#define STACK_MAX (UINT8_COUNT * 64)

typedef struct {
    Value stack[STACK_MAX];
    Value *stackTop;
    Hashmap stringLiterals;
    uint8_t *ip;
    int count;
} VM;

extern VM vm;

void interpret(const char *source);
void initVM();

#endif
