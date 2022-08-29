#ifndef dojo_chunk_h
#define dojo_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_INHERIT,
    OP_CLASS,
    OP_METHOD,
    OP_CLOSURE,
    OP_SUPER_INVOKE,
    OP_INVOKE,
    OP_CALL,
    OP_RETURN,
    // Variable
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    // Jump
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
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
    OP_CONSTANT,
    OP_TEMPLATE,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
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