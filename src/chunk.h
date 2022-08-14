#ifndef dojo_chunk_h
#define dojo_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_CONSTANT,
    OP_TEMPLATE,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_RETURN,
} Opcode;

typedef struct {
    int capacity;
    int count;
    int *lines;
    uint8_t *codes;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void addCodeToChunk(Chunk *chunk, Opcode code, int line);
int addConstantToChunk(Chunk *chunk, Value value);
Value getConstantAtIndex(Chunk *chunk, int index);

#endif