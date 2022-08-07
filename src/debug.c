#include "debug.h"
#include "chunk.h"
#include <stdint.h>
#include <stdio.h>

static void printLineInfo(Chunk *chunk, int offset);
static bool isSameLineAsPrevCode(Chunk *chunk, int offset);

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
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
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
