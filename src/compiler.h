#ifndef dojo_compiler_h
#define dojo_compiler_h

#include "chunk.h"
#include "node.h"

typedef struct {
    Chunk *compilingChunk;
    Node *script;
    Node *currentCompiling;
    bool hadError;
} Compiler;

void initCompiler(Chunk *chunk);
void terminateCompiler();
bool compile(const char *source, Chunk *chunk);

#endif