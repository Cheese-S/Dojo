#include "debug.h"
#include "chunk.h"
#include "object.h"
#include <stdint.h>
#include <stdio.h>

static void printLineInfo(Chunk *chunk, int offset);
static bool isSameLineAsPrevCode(Chunk *chunk, int offset);
static int byteInstruction(const char *name, Chunk *chunk, int offset);
static int jumpInstruction(const char *name, int sign, Chunk *chunk,
                           int offset);
static int invokeInstruction(const char *name, Chunk *chunk, int offset);
static int constantInstruction(const char *name, Chunk *chunk, int offset);
static int simpleInstruction(const char *name, int offset);

void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s == \n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);
    printLineInfo(chunk, offset);
    uint8_t instruction = chunk->codes[offset];
    switch (instruction) {
    case OP_INHERIT:
        return simpleInstruction("OP_INHERIT", offset);
    case OP_CLASS:
        return constantInstruction("OP_CLASS", chunk, offset);
    case OP_METHOD:
        return constantInstruction("OP_METHOD", chunk, offset);
    case OP_CLOSURE: {
        offset++;
        uint8_t constant = chunk->codes[offset++];
        printf("%-16s %4d ", "OP_CLOSURE", constant);
        printValue(chunk->constants.values[constant]);
        printf("\n");
        ObjFn *fn = AS_FN(chunk->constants.values[constant]);
        for (int j = 0; j < fn->upvalueCount; j++) {
            int isLocal = chunk->codes[offset++];
            int index = chunk->codes[offset++];
            printf("%04d      |                     %s %d\n", offset - 2,
                   isLocal ? "local" : "upvalue", index);
        }
        return offset;
    }
    case OP_SUPER_INVOKE:
        return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
    case OP_INVOKE:
        return invokeInstruction("OP_INVOKE", chunk, offset);
    case OP_CALL:
        return byteInstruction("OP_CALL", chunk, offset);
    case OP_DEFINE_GLOBAL:
        return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
        return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
        return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_LOCAL:
        return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
        return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_UPVALUE:
        return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
        return byteInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_CLOSE_UPVALUE:
        return byteInstruction("OP_CLOSE_UPVALUE", chunk, offset);
    case OP_GET_PROPERTY:
        return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
        return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OP_GET_SUPER:
        return constantInstruction("OP_GET_SUPER", chunk, offset);
    case OP_JUMP_IF_TRUE:
        return jumpInstruction("OP_JUMP_IF_TRUE", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
        return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_JUMP:
        return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_LOOP:
        return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_EQUAL:
        return simpleInstruction("OP_EQUAL", offset);
    case OP_NOT_EQUAL:
        return simpleInstruction("OP_NOT_EQUAL", offset);
    case OP_LESS_EQUAL:
        return simpleInstruction("OP_LESS_EQUAL", offset);
    case OP_GREATER_EQUAL:
        return simpleInstruction("OP_GREATER_EQUAL", offset);
    case OP_LESS:
        return simpleInstruction("OP_LESS", offset);
    case OP_GREATER:
        return simpleInstruction("OP_GREATER", offset);
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_TEMPLATE:
        return constantInstruction("OP_TEMPLATE", chunk, offset);
    case OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OP_NOT:
        return simpleInstruction("OP_NOT", offset);
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_FALSE:
        return simpleInstruction("OP_FALSE", offset);
    case OP_TRUE:
        return simpleInstruction("OP_TRUE", offset);
    case OP_NIL:
        return simpleInstruction("OP_NIL", offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    case OP_POP:
        return simpleInstruction("OP_POP", offset);
    case OP_POPN:
        return constantInstruction("OP_POPN", chunk, offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

static void printLineInfo(Chunk *chunk, int offset) {
    if (isSameLineAsPrevCode(chunk, offset)) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
}

static bool isSameLineAsPrevCode(Chunk *chunk, int offset) {
    return offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1];
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t slot = chunk->codes[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk,
                           int offset) {
    uint16_t jump = (uint16_t)(chunk->codes[offset + 1] << 8);
    jump |= chunk->codes[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int invokeInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->codes[offset + 1];
    uint8_t argCount = chunk->codes[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->codes[offset + 1];
    printf("%-16s %4d '", name, constant);
    printf("'\n");
    return offset + 2;
}

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}
