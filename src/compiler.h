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
    const char *name;
} Local;

typedef struct {
    int count;
    int scopeDepth;
    Local locals[UINT8_COUNT];
} LocalState;

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
    LocalState localState;
    LoopState loopState;
    struct Compiler *enclosing;
    Hashmap stringConstants;
} Compiler;

void initCompiler(Compiler *compiler, FnType type);
ObjFn *terminateCompiler(Compiler *compiler);
ObjFn *compile(const char *source);

#endif