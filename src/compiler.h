#ifndef dojo_compiler_h
#define dojo_compiler_h

#include "chunk.h"
#include "node.h"

typedef struct {
    Chunk *compilingChunk;
    Node *AST;
    Node *currentCompiling;
} Compiler;

void initCompiler(Chunk *chunk);
void terminateCompiler();
void compile(const char *source, Chunk *chunk);

#endif