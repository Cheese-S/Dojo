#include "chunk.h"
#include "memory.h"
#include "vm.h"
#include <stdint.h>

static void growLinesAndCodes(Chunk *chunk);

void initChunk(Chunk *chunk) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->lines = NULL;
    chunk->codes = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    FREE_ARRAY(uint8_t, chunk->codes, chunk->capacity);
    freeValueArray(&chunk->constants);
}

void addCodeToChunk(Chunk *chunk, Opcode code, int line) {
    if (IS_EXCEEDING_CAPACITY(chunk->count, chunk->capacity)) {
        growLinesAndCodes(chunk);
    }
    chunk->lines[chunk->count] = line;
    chunk->codes[chunk->count] = code;
    chunk->count++;
}

static void growLinesAndCodes(Chunk *chunk) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    chunk->codes =
        GROW_ARRAY(uint8_t, chunk->codes, oldCapacity, chunk->capacity);
}

int addConstantToChunk(Chunk *chunk, Value value) {
    push(value);
    int index = writeValueArray(&chunk->constants, value);
    pop();
    return index;
}

Value getConstantAtIndex(Chunk *chunk, int index) {
    return chunk->constants.values[index];
}