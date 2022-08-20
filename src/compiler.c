#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "error.h"
#include "memory.h"
#include "node.h"
#include "object.h"
#include "parser.h"
#include "scanner.h"
#include "value.h"
#include "vm.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define LOCAL_STATE(compiler) (compiler.localState)
#define LOCAL_COUNT(compiler) (compiler.localState.count)
#define LOCAL_ARR(compiler) (compiler.localState.locals)
#define NOT_INITIALIZED -1
#define JUMP_PLACEHOLDER 0xff

static Chunk *currentChunk();

static void compileStmts(Node *node);
static void compileNode(Node *node);
static void freeStmts(Node *AST);
static void freeNode(Node *node);

static bool isGlobalScope();
static int currentScopeDepth();
static void beginScope();
static void endScope();

static bool isInLoop();
static void beginLoop();
static void endLoop();

static void discardHigherDepthLocals();
static void compileVarDeclValue(Node *operand);
static void defineGlobal(Token *name);
static void declareLocal(Token *name);
static void defineLatestLocal();
static int resolveLocal(LocalState *state, Token *name);
static bool isIdentifiersEqual(Token *ident1, Token *ident2);
static void pushNewLocal(Token *name);
static bool isAssignable(Node *lhs);

static void emitConstant(Value value);
static uint8_t pushIdentifier(Token *name);
static uint8_t pushConstant(Value constant);
static bool findIdentifierConstantIdx(ObjString *identifier, Value *receiver);
static void emitBranch(Node *branch);
static int emitJump(uint8_t jumpInstruction);
static void emitLoop(int loopStart);
static void patchJump(int offset);
static void patchBreaks();
static bool isUnpatchedBreak(uint8_t *code, int offset);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitBinaryOp(TokenType op);
static void emitByte(uint8_t byte);

static void compilerError(Token *token, const char *msg);
static void compilerInternalError(const char *msg);

Compiler compiler;

static LoopState loopState = {.innermostLoopStart = NOT_INITIALIZED,
                              .innermostLoopScopeDepth = NOT_INITIALIZED,
                              .surrondingLoopStart = NOT_INITIALIZED,
                              .surrondingLoopScopeDepth = NOT_INITIALIZED};

void initCompiler(Chunk *chunk) {
    compiler.compilingChunk = chunk;
    compiler.hadError = false;
    compiler.scope.depth = 0;
    compiler.localState.count = 0;
    initMap(&compiler.stringConstants);
}

void terminateCompiler() {
    freeStmts(compiler.script);
    freeMap(&compiler.stringConstants);
    terminateParser();
}

// TODO: ALLOW EMPTY FILE

bool compile(const char *source, Chunk *chunk) {
    bool parserError = false;
    initParser(source);
    initCompiler(chunk);
    compiler.script = parse(&parserError);
    if (parserError) {
        return true;
    }
    compileStmts(compiler.script);
    // TODO: remove this hack
    compiler.currentCompiling = compiler.script;
    emitByte(OP_RETURN);
    return compiler.hadError || parserError;
}

static void compileStmts(Node *script) {
    Node *current = script;
    while (current) {
        Node *next = current->nextStmt;
        compileNode(current);
        current = next;
    }
}

static void compileNode(Node *node) {
    if (node == NULL)
        return;

    Node *prev = compiler.currentCompiling;
    compiler.currentCompiling = node;

    switch (node->type) {
    case ND_VAR_DECL: {
        if (isGlobalScope()) {
            compileVarDeclValue(node->operand);
            defineGlobal(node->token);
        } else {
            declareLocal(node->token);
            compileVarDeclValue(node->operand);
            defineLatestLocal(node->token);
        }
        break;
    }
    // The bytecode is emitted in the following order
    // init -> condition -> if false jump to end -> goto end -> increment ->
    // goto condition -> body -> goto increment
    case ND_FOR: {
        beginScope();
        compileNode(node->init);
        beginLoop();
        int jumpToEnd = NOT_INITIALIZED;
        if (node->operand) {
            compileNode(node->operand);
            jumpToEnd = emitJump(OP_JUMP_IF_FALSE);
            emitByte(OP_POP);
        }
        if (node->increment) {
            int jumpToBody = emitJump(OP_JUMP);
            int incrementStart = currentChunk()->count;
            compileNode(node->increment);
            emitByte(OP_POP);
            emitLoop(loopState.innermostLoopStart);
            loopState.innermostLoopStart = incrementStart;
            patchJump(jumpToBody);
        }
        compileNode(node->thenBranch);

        emitLoop(loopState.innermostLoopStart);
        if (jumpToEnd != NOT_INITIALIZED) {
            patchJump(jumpToEnd);
            emitByte(OP_POP);
        }
        patchBreaks();
        endLoop();
        endScope();
        break;
    }
    case ND_WHILE: {
        beginLoop();
        int loopStart = currentChunk()->count;
        compileNode(node->operand);
        int jumpToEnd = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        compileNode(node->thenBranch);
        emitLoop(loopStart);
        patchJump(jumpToEnd);
        emitByte(OP_POP);
        patchBreaks();
        endLoop();
        break;
    }
    case ND_CONTINUE: {
        if (!isInLoop()) {
            compilerError(
                node->token,
                "Cannot use continue statement outside a loop statement. ");
        }
        discardHigherDepthLocals(loopState.innermostLoopScopeDepth);
        emitLoop(loopState.innermostLoopStart);
        break;
    }
    case ND_BREAK: {
        if (!isInLoop()) {
            compilerError(
                node->token,
                "Cannot use break statement outside a break statement. ");
        }
        discardHigherDepthLocals(loopState.innermostLoopScopeDepth);
        emitJump(OP_JUMP);
        break;
    }
    case ND_TERNARY:
    case ND_IF: {
        compileNode(node->operand);

        int jumpToElse = emitJump(OP_JUMP_IF_FALSE);

        emitBranch(node->thenBranch);

        int jumpToEnd = emitJump(OP_JUMP);
        patchJump(jumpToElse);

        emitBranch(node->elseBranch);

        patchJump(jumpToEnd);
        break;
    }
    case ND_BLOCK: {
        beginScope();
        compileStmts(node->operand);
        endScope();
        break;
    }
    case ND_PRINT: {
        compileNode(node->operand);
        emitByte(OP_PRINT);
        break;
    }
    case ND_EXPRESSION: {
        compileNode(node->operand);
        emitByte(OP_POP);
        break;
    }
    case ND_ASSIGNMENT: {
        if (!isAssignable(node->lhs)) {
            compilerError(node->token, "Invalid assignment target.");
        }
        compileNode(node->rhs);

        int vmStackPos = resolveLocal(&LOCAL_STATE(compiler), node->lhs->token);
        if (vmStackPos != -1) {
            emitBytes(OP_SET_LOCAL, vmStackPos);
        } else {
            uint8_t index = pushIdentifier(node->lhs->token);
            emitBytes(OP_SET_GLOBAL, index);
        }

        break;
    }
    case ND_AND: {
        compileNode(node->lhs);
        int jumpToEnd = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        compileNode(node->rhs);
        patchJump(jumpToEnd);
        break;
    }
    case ND_OR: {
        compileNode(node->lhs);
        int jumpToEnd = emitJump(OP_JUMP_IF_TRUE);
        emitByte(OP_POP);
        compileNode(node->rhs);
        patchJump(jumpToEnd);
        break;
    }
    case ND_BINARY: {
        TokenType op = node->token->type;
        compileNode(node->lhs);
        compileNode(node->rhs);
        emitBinaryOp(op);
        break;
    }
    case ND_UNARY: {
        compileNode(node->operand);
        Opcode code = node->token->type == TOKEN_BANG ? OP_NOT : OP_NEGATE;
        emitByte(code);
        break;
    }
    case ND_VAR: {
        int vmStackPos = resolveLocal(&LOCAL_STATE(compiler), node->token);
        if (vmStackPos != -1) {
            emitBytes(OP_GET_LOCAL, vmStackPos);
        } else {
            uint8_t index = pushIdentifier(node->token);
            emitBytes(OP_GET_GLOBAL, index);
        }
        break;
    }
    case ND_NUMBER:
        emitConstant(NUMBER_VAL(strtod(node->token->start, NULL)));
        break;

    case ND_STRING:
        emitConstant(
            newObjStringInVal(node->token->start + 1, node->token->length - 2));
        break;
    case ND_TEMPLATE_HEAD:
        compileNode(node->span);
        emitConstant(
            newObjStringInVal(node->token->start + 1, node->token->length - 1));
        emitBytes(OP_TEMPLATE, (uint8_t)(node->numSpans));
        break;
    case ND_TEMPLATE_SPAN: {
        int len = node->token->type == TOKEN_AFTER_TEMPLATE
                      ? node->token->length - 1
                      : node->token->length;
        if (node->span != NULL) {
            compileNode(node->span);
        }
        emitConstant(newObjStringInVal(node->token->start, len));
        compileNode(node->operand);
        break;
    }
    case ND_LITERAL: {
        TokenType op = node->token->type;
        if (op == TOKEN_FALSE) {
            emitByte(OP_FALSE);
        } else if (op == TOKEN_TRUE) {
            emitByte(OP_TRUE);
        } else {
            emitByte(OP_NIL);
        }
        break;
    }

    case ND_EMPTY:
        break;
    }

    compiler.currentCompiling = prev;
}

static bool isGlobalScope() {
    return !compiler.scope.depth;
}

static int currentScopeDepth() {
    return compiler.scope.depth;
}

static void beginScope() {
    compiler.scope.depth++;
}

static void endScope() {
    compiler.scope.depth--;
    discardHigherDepthLocals(currentScopeDepth());
}

static void discardHigherDepthLocals(int depth) {
    int discarded = 0;
    while (LOCAL_COUNT(compiler) > 0 &&
           LOCAL_ARR(compiler)[LOCAL_COUNT(compiler) - 1].depth > depth) {
        discarded++;
        LOCAL_COUNT(compiler)--;
    }
    emitBytes(OP_POPN, discarded);
};

static void beginLoop() {
    loopState.surrondingLoopScopeDepth = loopState.innermostLoopScopeDepth;
    loopState.surrondingLoopStart = loopState.innermostLoopStart;
    loopState.innermostLoopStart = currentChunk()->count;
    loopState.innermostLoopScopeDepth = currentScopeDepth();
}

static void endLoop() {
    loopState.innermostLoopStart = loopState.surrondingLoopStart;
    loopState.innermostLoopScopeDepth = loopState.surrondingLoopScopeDepth;
}

static bool isInLoop() {
    return loopState.innermostLoopStart != NOT_INITIALIZED;
}

static void defineGlobal(Token *name) {
    uint8_t index = pushIdentifier(name);
    emitBytes(OP_DEFINE_GLOBAL, index);
    return;
}

static void defineLatestLocal() {
    (&LOCAL_ARR(compiler)[LOCAL_COUNT(compiler) - 1])->depth =
        currentScopeDepth();
    return;
}

static void declareLocal(Token *name) {
    if (compiler.localState.count == UINT8_COUNT) {
        compilerError(name, "Too many local variables in scope.");
        return;
    }

    for (int i = LOCAL_COUNT(compiler) - 1; i >= 0; i--) {
        Local *local = &LOCAL_ARR(compiler)[i];
        if (local->depth != NOT_INITIALIZED &&
            local->depth < currentScopeDepth()) {
            break;
        }

        if (isIdentifiersEqual(local->name, name)) {
            compilerError(
                name,
                "Already a varaible with the same name exists in this scope");
        }
    }

    pushNewLocal(name);
}

static void pushNewLocal(Token *name) {
    Local *local = &LOCAL_ARR(compiler)[LOCAL_COUNT(compiler)++];
    local->name = name;
    local->depth = NOT_INITIALIZED;
}

static int resolveLocal(LocalState *state, Token *name) {
    for (int i = state->count - 1; i >= 0; i--) {
        Local *local = &state->locals[i];
        if (isIdentifiersEqual(name, local->name)) {
            if (local->depth == NOT_INITIALIZED) {
                compilerError(
                    name,
                    "Cannot reference a local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static bool isIdentifiersEqual(Token *ident1, Token *ident2) {
    if (ident1->length != ident2->length) {
        return false;
    }
    return memcmp(ident1->start, ident2->start, ident1->length) == 0;
}

static void compileVarDeclValue(Node *operand) {
    if (operand) {
        compileNode(operand);
    } else {
        emitByte(OP_NIL);
    }
}

static bool isAssignable(Node *lhs) {
    return lhs->type == ND_VAR;
}

static void emitConstant(Value constant) {
    emitBytes(OP_CONSTANT, pushConstant(constant));
}

// TODO: better names for the following two functions.

static uint8_t pushIdentifier(Token *name) {
    Value idx;
    ObjString *identifier = newObjString(name->start, name->length);
    if (findIdentifierConstantIdx(identifier, &idx)) {
        return (uint8_t)AS_NUMBER(idx);
    }
    idx = (uint8_t)addConstantToChunk(currentChunk(), OBJ_VAL(identifier));
    mapPut(&compiler.stringConstants, identifier, NUMBER_VAL((double)idx));
    return (uint8_t)idx;
}

static bool findIdentifierConstantIdx(ObjString *identifier, Value *receiver) {
    if (mapGet(&compiler.stringConstants, identifier, receiver)) {
        return true;
    }
    return false;
}

static uint8_t pushConstant(Value constant) {
    return (uint8_t)addConstantToChunk(currentChunk(), constant);
}

static void emitBinaryOp(TokenType type) {
    switch (type) {
    case TOKEN_BANG_EQUAL:
        emitByte(OP_NOT_EQUAL);
        return;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        return;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        return;
    case TOKEN_LESS_EQUAL:
        emitByte(OP_LESS_EQUAL);
        return;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        return;
    case TOKEN_GREATER_EQUAL:
        emitByte(OP_GREATER_EQUAL);
        return;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        return;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        return;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        return;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        return;
    case TOKEN_AND:
        emitByte(OP_AND);
        return;
    case TOKEN_OR:
        emitByte(OP_OR);
        return;
    default:
        // Unreachable
        ;
    }
}

static void emitBranch(Node *branch) {
    emitByte(OP_POP);
    compileNode(branch);
}

static int emitJump(uint8_t jumpInstruction) {
    emitByte(jumpInstruction);
    emitByte(JUMP_PLACEHOLDER);
    emitByte(JUMP_PLACEHOLDER);
    return currentChunk()->count - 2;
}

static void patchJump(int offset) {
    if (offset > UINT16_MAX) {
        compilerError(compiler.currentCompiling->token,
                      "Exceeded the maximum allowed jump distance");
        return;
    }
    Chunk *chunk = currentChunk();
    int jump = chunk->count - offset - 2;
    chunk->codes[offset] = (jump >> 8) & 0xff;
    chunk->codes[offset + 1] = jump & 0xff;
}

static void patchBreaks() {
    Chunk *chunk = currentChunk();
    int loopEnd = chunk->count;
    for (int i = loopState.innermostLoopStart; i < loopEnd - 2; i++) {
        if (isUnpatchedBreak(chunk->codes, i)) {
            patchJump(i + 1);
            i = i + 2;
        }
    }
}

static bool isUnpatchedBreak(uint8_t *code, int offset) {
    return code[offset] == OP_JUMP && code[offset + 1] == JUMP_PLACEHOLDER &&
           code[offset + 2] == JUMP_PLACEHOLDER;
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        compilerError(compiler.currentCompiling->token, "Loop body too large.");
        return;
    }
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitByte(uint8_t byte) {
    addCodeToChunk(currentChunk(), byte,
                   compiler.currentCompiling->token->line);
}

static Chunk *currentChunk() {
    return compiler.compilingChunk;
}

static void freeStmts(Node *script) {
    Node *current = script;
    while (current) {
        Node *next = current->nextStmt;
        freeNode(current);
        current = next;
    }
}

static void freeNode(Node *node) {
    if (node == NULL)
        return;

    switch (node->type) {
    case ND_FOR:
        freeNode(node->init);
        freeNode(node->operand);
        freeNode(node->increment);
        freeNode(node->thenBranch);
        FREE(Node, node);
        return;
    case ND_WHILE:
        freeNode(node->operand);
        freeNode(node->thenBranch);
        FREE(Node, node);
        return;
    case ND_TERNARY:
    case ND_IF:
        freeNode(node->elseBranch);
        freeNode(node->thenBranch);
        freeNode(node->operand);
        FREE(Node, node);
        return;
    case ND_BLOCK:
        freeStmts(node->operand);
        FREE(Node, node);
        return;
    case ND_ASSIGNMENT:
    case ND_AND:
    case ND_OR:
    case ND_BINARY:
        freeNode(node->lhs);
        freeNode(node->rhs);
        FREE(Node, node);
        return;
    case ND_TEMPLATE_HEAD:
        freeNode(node->span);
        FREE(Node, node);
        return;
    case ND_TEMPLATE_SPAN:
        freeNode(node->span);
        freeNode(node->operand);
        FREE(Node, node);
        return;
    case ND_VAR_DECL:
    case ND_EXPRESSION:
    case ND_PRINT:
    case ND_UNARY:
        freeNode(node->operand);
        FREE(Node, node);
        return;
    case ND_BREAK:
    case ND_CONTINUE:
    case ND_VAR:
    case ND_STRING:
    case ND_NUMBER:
    case ND_LITERAL:
        FREE(Node, node);
        return;
    case ND_EMPTY:
        return;
    }
}

static void compilerError(Token *token, const char *msg) {
    compiler.hadError = true;
    errorAtToken(token, msg);
}

static void compilerInternalError(const char *msg) {
    compiler.hadError = true;
    internalError(msg);
}