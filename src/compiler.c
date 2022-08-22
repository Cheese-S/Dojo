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

#define NOT_INITIALIZED -1
#define JUMP_PLACEHOLDER 0xff

static void initLoopState(LoopState *state);
static void initLocalState(LocalState *state);
static void claimFirstLocal(LocalState *state);

static Chunk *currentChunk();

static void compileStmts(Node *node);
static void compileNode(Node *node);
static void freeStmts(Node *AST);
static void freeNode(Node *node);
static void compileVarDeclValue(Node *operand);
static void compileFn(Node *fn);

static bool isGlobalScope();
static int currentScopeDepth();
static void beginScope();
static void endScope();

static bool isInLoop();
static void beginLoop();
static void endLoop();

static void discardHigherDepthLocals();
static void defineGlobal(Token *name);
static void declareLocal(Token *name);
static void defineLatestLocal();
static void pushNewLocal(Token *token);
static int resolveLocal(LocalState *state, Token *name);
static bool isIdentifiersEqual(const char *str1, int len1, const char *str2,
                               int len2);
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
static void emitBinaryOp(TokenType op);
static void emitImplicitReturn();
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitByte(uint8_t byte);

static void compilerError(Token *token, const char *msg);
static void compilerInternalError(const char *msg);
static LocalState *currentLocalState();
static LoopState *currentLoopState();

Compiler *current;
static bool compilerHadError;

void initCompiler(Compiler *compiler, FnType type) {
    compiler->enclosing = current;
    compiler->type = type;
    compiler->fn = NULL;
    compiler->stmts = NULL;
    initLoopState(&compiler->loopState);
    initLocalState(&compiler->localState);
    initMap(&compiler->stringConstants);
    compiler->fn = newObjFn();
    current = compiler;
    claimFirstLocal(currentLocalState());
}

static void initLoopState(LoopState *state) {
    state->innermostLoopStart = NOT_INITIALIZED;
    state->surrondingLoopStart = NOT_INITIALIZED;
    state->innermostLoopScopeDepth = NOT_INITIALIZED;
    state->surrondingLoopScopeDepth = NOT_INITIALIZED;
}

static void initLocalState(LocalState *state) {
    state->count = 0;
    state->scopeDepth = 0;
}

static void claimFirstLocal(LocalState *state) {

    Local *local = &state->locals[state->count++];
    local->depth = 0;
    local->name = "";
    local->length = 0;
}

ObjFn *terminateCompiler(Compiler *compiler) {
    emitImplicitReturn();
    current = compiler->enclosing;
    freeStmts(compiler->stmts);
    freeMap(&compiler->stringConstants);
    return compiler->fn;
}

// TODO: ALLOW EMPTY FILE

ObjFn *compile(const char *source) {
    Compiler compiler;
    bool parserError = false;
    initParser(source);
    initCompiler(&compiler, FN_SCRPIT);
    current->stmts = parse(&parserError);
    if (parserError) {
        terminateCompiler(&compiler);
        return NULL;
    }
    compileStmts(current->stmts);
    emitImplicitReturn();
    ObjFn *script = terminateCompiler(&compiler);
    return compilerHadError ? NULL : script;
}

static void compileStmts(Node *script) {
    Node *current = script;
    while (current) {
        Node *next = current->next;
        compileNode(current);
        current = next;
    }
}

static void compileNode(Node *node) {
    if (node == NULL)
        return;

    Node *prev = current->currentNode;
    current->currentNode = node;

    switch (node->type) {
    case ND_FN_DECL:
        if (isGlobalScope()) {
            compileFn(node);
            defineGlobal(node->token);
        } else {
            // Recursive function "uses" itself before it is fully defined.
            // So we have to allow this.
            declareLocal(node->token);
            defineLatestLocal();
            compileFn(node);
        }
        break;
    case ND_PARAM:
        declareLocal(node->token);
        defineLatestLocal();
        return;
    case ND_VAR_DECL: {
        if (isGlobalScope()) {
            compileVarDeclValue(node->operand);
            defineGlobal(node->token);
        } else {
            declareLocal(node->token);
            compileVarDeclValue(node->operand);
            defineLatestLocal();
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
        LoopState *state = currentLoopState();
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
            emitLoop(state->innermostLoopStart);
            state->innermostLoopStart = incrementStart;
            patchJump(jumpToBody);
        }
        compileNode(node->thenBranch);

        emitLoop(state->innermostLoopStart);
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
        discardHigherDepthLocals(currentLoopState()->innermostLoopScopeDepth);
        emitLoop(currentLoopState()->innermostLoopStart);
        break;
    }
    case ND_BREAK: {
        if (!isInLoop()) {
            compilerError(
                node->token,
                "Cannot use break statement outside a break statement. ");
        }
        discardHigherDepthLocals(currentLoopState()->innermostLoopScopeDepth);
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
    case ND_RETURN: {
        if (current->type == FN_SCRPIT) {
            compilerError(node->token, "Cannot return from top-level code.");
        }
        if (node->operand) {
            compileNode(node->operand);
        } else {
            emitByte(OP_NIL);
        }
        emitByte(OP_RETURN);
        break;
    }
    case ND_EXPRESSION: {
        compileNode(node->operand);
        emitByte(OP_POP);
        break;
    }
    case ND_CALL: {
        uint8_t argCount = 0;
        Node *args = node->operand;
        compileNode(node->lhs);
        while (args) {
            argCount++;
            Node *next = args->next;
            compileNode(args);
            if (argCount == 255) {
                compilerError(node->lhs->token,
                              "Can't have more than 255 arguments");
            }
            args = next;
        }
        emitBytes(OP_CALL, argCount);
        break;
    }
    case ND_ASSIGNMENT: {
        if (!isAssignable(node->lhs)) {
            compilerError(node->token, "Invalid assignment target.");
        }
        compileNode(node->rhs);

        int vmStackPos = resolveLocal(currentLocalState(), node->lhs->token);
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
        int vmStackPos = resolveLocal(currentLocalState(), node->token);
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
        compileNode(node->next);
        emitConstant(
            newObjStringInVal(node->token->start + 1, node->token->length - 1));
        emitBytes(OP_TEMPLATE, (uint8_t)(node->count));
        break;
    case ND_TEMPLATE_SPAN: {
        int len = node->token->type == TOKEN_AFTER_TEMPLATE
                      ? node->token->length - 1
                      : node->token->length;
        if (node->next != NULL) {
            compileNode(node->next);
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

    current->currentNode = prev;
}

static void compileFn(Node *fn) {
    Compiler fnCompiler;
    initCompiler(&fnCompiler, FN_FN);
    beginScope();
    current->fn->name = newObjString(fn->token->start, fn->token->length);
    Node *params = fn->operand;
    while (params) {
        current->fn->arity++;
        compileNode(params);
        params = params->next;
    }
    compileNode(fn->thenBranch);
    ObjFn *resFn = terminateCompiler(&fnCompiler);
    emitConstant(OBJ_VAL(resFn));
}

static void compileVarDeclValue(Node *operand) {
    if (operand) {
        compileNode(operand);
    } else {
        emitByte(OP_NIL);
    }
}

static void beginLoop() {
    LoopState *state = currentLoopState();
    state->surrondingLoopScopeDepth = state->innermostLoopScopeDepth;
    state->surrondingLoopStart = state->innermostLoopStart;
    state->innermostLoopStart = currentChunk()->count;
    state->innermostLoopScopeDepth = currentScopeDepth();
}

static void endLoop() {
    LoopState *state = currentLoopState();
    state->innermostLoopStart = state->surrondingLoopStart;
    state->innermostLoopScopeDepth = state->surrondingLoopScopeDepth;
}

static bool isGlobalScope() {
    return !currentScopeDepth();
}

static void beginScope() {
    currentLocalState()->scopeDepth++;
}

static void endScope() {
    currentLocalState()->scopeDepth--;
    discardHigherDepthLocals(currentScopeDepth());
}

static void discardHigherDepthLocals(int depth) {
    int discarded = 0;
    LocalState *local = currentLocalState();
    while (local->count > 0 && local->locals[local->count - 1].depth > depth) {
        discarded++;
        local->count--;
    }
    emitBytes(OP_POPN, discarded);
};

static bool isInLoop() {
    LoopState *state = currentLoopState();
    return state->innermostLoopStart != NOT_INITIALIZED;
}

static void defineGlobal(Token *name) {
    uint8_t index = pushIdentifier(name);
    emitBytes(OP_DEFINE_GLOBAL, index);
    return;
}

static void defineLatestLocal() {
    LocalState *state = currentLocalState();
    (&state->locals[state->count - 1])->depth = currentScopeDepth();
    return;
}

static void declareLocal(Token *ident) {
    if (current->localState.count == UINT8_COUNT) {
        compilerError(ident, "Too many local variables in scope.");
        return;
    }

    LocalState *state = currentLocalState();

    for (int i = state->count - 1; i >= 0; i--) {
        Local *local = &state->locals[i];
        if (local->depth != NOT_INITIALIZED &&
            local->depth < currentScopeDepth()) {
            break;
        }

        if (isIdentifiersEqual(local->name, local->length, ident->start,
                               ident->length)) {
            compilerError(
                ident,
                "Already a varaible with the same name exists in this scope");
        }
    }

    pushNewLocal(ident);
}

static void pushNewLocal(Token *ident) {
    LocalState *state = currentLocalState();
    Local *local = &state->locals[state->count++];
    local->name = ident->start;
    local->length = ident->length;
    local->depth = NOT_INITIALIZED;
}

static int resolveLocal(LocalState *state, Token *ident) {
    for (int i = state->count - 1; i >= 0; i--) {
        Local *local = &state->locals[i];
        if (isIdentifiersEqual(local->name, local->length, ident->start,
                               ident->length)) {
            if (local->depth == NOT_INITIALIZED) {
                compilerError(
                    ident,
                    "Cannot reference a local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static bool isIdentifiersEqual(const char *str1, int len1, const char *str2,
                               int len2) {
    if (len1 != len2) {
        return false;
    }
    return memcmp(str1, str2, len1) == 0;
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
    mapPut(&current->stringConstants, identifier, NUMBER_VAL((double)idx));
    return (uint8_t)idx;
}

static bool findIdentifierConstantIdx(ObjString *identifier, Value *receiver) {
    if (mapGet(&current->stringConstants, identifier, receiver)) {
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
        compilerError(current->currentNode->token,
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
    LoopState *state = currentLoopState();
    int loopEnd = chunk->count;
    for (int i = state->innermostLoopStart; i < loopEnd - 2; i++) {
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
        compilerError(current->currentNode->token, "Loop body too large.");
        return;
    }
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static void emitImplicitReturn() {
    addCodeToChunk(currentChunk(), OP_NIL, -1);
    addCodeToChunk(currentChunk(), OP_RETURN, -1);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitByte(uint8_t byte) {
    addCodeToChunk(currentChunk(), byte, current->currentNode->token->line);
}

static void freeStmts(Node *script) {
    Node *current = script;
    while (current) {
        Node *next = current->next;
        freeNode(current);
        current = next;
    }
}

static void freeNode(Node *node) {
    if (node == NULL)
        return;

    switch (node->type) {
    case ND_CALL: {
        freeNode(node->lhs);
        Node *current = node->operand;
        while (current) {
            Node *next = current->next;
            freeNode(current);
            current = next;
        }
        FREE(Node, node);
        return;
    }
    case ND_FOR:
        freeNode(node->init);
        freeNode(node->operand);
        freeNode(node->increment);
        freeNode(node->thenBranch);
        FREE(Node, node);
        return;
    case ND_FN_DECL:
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
    case ND_ASSIGNMENT:
    case ND_AND:
    case ND_OR:
    case ND_BINARY:
        freeNode(node->lhs);
        freeNode(node->rhs);
        FREE(Node, node);
        return;
    case ND_PARAM:
    case ND_TEMPLATE_HEAD:
        freeNode(node->next);
        FREE(Node, node);
        return;
    case ND_TEMPLATE_SPAN:
        freeNode(node->next);
        freeNode(node->operand);
        FREE(Node, node);
        return;
    case ND_RETURN:
    case ND_BLOCK:
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

static Chunk *currentChunk() {
    return &current->fn->chunk;
}

static LocalState *currentLocalState() {
    return &current->localState;
}

static LoopState *currentLoopState() {
    return &current->loopState;
}

static int currentScopeDepth() {
    return current->localState.scopeDepth;
}

static void compilerError(Token *token, const char *msg) {
    compilerHadError = true;
    errorAtToken(token, msg);
}

static void compilerInternalError(const char *msg) {
    compilerHadError = true;
    internalError(msg);
}