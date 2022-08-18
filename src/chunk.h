#ifndef dojo_chunk_h
#define dojo_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_PRINT,

    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    OP_CONSTANT,
    // Binary
    OP_ASSIGN,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_AND,
    OP_OR,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    // Unary
    OP_NOT,
    OP_NEGATE,
    // Literal
    OP_TEMPLATE,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_RETURN,
    // Stack Op
    OP_POP,
    OP_POPN,
    OP_PUSH
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