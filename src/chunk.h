#ifndef dojo_chunk_h
#define dojo_chunk_h

#include "common.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE
} Opcode;

typedef struct {
    int count;
    int *lines;
    uint8_t *code;
    // ValueArray constants;
} Chunk;

#endif