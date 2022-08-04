#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

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
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
        case OP_CONSTANT: {
            Value val = getConstantAtIndex(&compilingChunk, READ_BYTE());
            push(val);
            break;
        }
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

void push(Value value) {
    *(vm.stackTop++) = value;
    vm.count++;
}

Value pop() {
    vm.count--;
    return *(--vm.stackTop);
}
