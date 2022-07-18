#ifndef dojo_chunk_h
#define dojo_chunk_h

#include "common.h"

typedef enum { OP_CONSTANT, OP_RETURN } Opcode;

typedef struct {
    int line;
    uint8_t *code;
} Chunk;

#endif