#ifndef dojo_compiler_h
#define dojo_compiler_h

#include "chunk.h"
#include "common.h"
#include "hashmap.h"
#include "node.h"

typedef struct {
    Token *name;
    int depth;
} Local;

typedef struct {
    Local locals[UINT8_COUNT];
    int count;
} LocalState;

typedef struct {
    int innermostLoopStart;
    int innermostLoopScopeDepth;
    int surrondingLoopStart;
    int surrondingLoopScopeDepth;
} LoopState;

typedef struct {
    int depth;
} Scope;

typedef struct {
    Chunk *compilingChunk;
    Node *script;
    Node *currentCompiling;
    Scope scope;
    bool hadError;
    LocalState localState;
    Hashmap stringConstants;
} Compiler;

void initCompiler(Chunk *chunk);
void terminateCompiler();
bool compile(const char *source, Chunk *chunk);

#endif