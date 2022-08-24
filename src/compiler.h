#ifndef dojo_compiler_h
#define dojo_compiler_h

#include "chunk.h"
#include "common.h"
#include "hashmap.h"
#include "node.h"
#include "object.h"

typedef struct {
    int depth;
    int length;
    bool isCaptured;
    const char *name;
} Local;

typedef struct {
    int count;
    int scopeDepth;
    Local locals[UINT8_COUNT];
} LocalState;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef struct {
    Upvalue upvalues[UINT8_COUNT];
} UpvalueState;

typedef struct {
    int innermostLoopStart;
    int innermostLoopScopeDepth;
    int surrondingLoopStart;
    int surrondingLoopScopeDepth;
} LoopState;

typedef enum { FN_SCRPIT, FN_FN } FnType;

typedef struct Compiler {
    FnType type;
    ObjFn *fn;
    Node *stmts;
    Node *currentNode;
    UpvalueState upvalueState;
    LocalState localState;
    LoopState loopState;
    struct Compiler *enclosing;
    Hashmap stringConstants;
} Compiler;

void initCompiler(Compiler *compiler, FnType type);
ObjFn *terminateCompiler(Compiler *compiler);
void markCompilerRoots();
ObjFn *compile(const char *source);

#endif