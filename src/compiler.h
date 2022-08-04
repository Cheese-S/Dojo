#ifndef dojo_compiler_h
#define dojo_compiler_h

#include "chunk.h"

typedef struct {
    Chunk *compilingChunk;
} Compiler;

void compile(const char *source, Chunk *chunk);

#endif