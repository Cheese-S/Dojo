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

static Chunk *currentChunk();

static void compileStmts(Node *node);
static void compileNode(Node *node);
static void freeStmts(Node *AST);
static void freeNode(Node *node);

static bool isGlobalScope();
static int currentScopeDepth();
static void beginScope();
static void endScope();

static void discardLocals();
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
static bool findIdentifierConstantIdx(ObjString *identifier, Value *receiver);
static uint8_t pushConstant(Value constant);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitBinaryOp(TokenType op);
static void emitByte(uint8_t byte);

static void compilerError(Token *token, const char *msg);

Compiler compiler;

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

bool compile(const char *source, Chunk *chunk) {
    bool parserError = false;
    initParser(source);
    initCompiler(chunk);
    compiler.script = parse(&parserError);
    if (parserError) {
        return true;
    }
    compileStmts(compiler.script);
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
        return;
    }
    case ND_BLOCK: {
        beginScope();
        compileStmts(node->operand);
        endScope();
        return;
    }
    case ND_PRINT: {
        compileNode(node->operand);
        emitByte(OP_PRINT);
        return;
    }
    case ND_EXPRESSION: {
        compileNode(node->operand);
        emitByte(OP_POP);
        return;
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

        return;
    }
    case ND_TERNARY: {
        return;
    }
    case ND_BINARY: {
        TokenType op = node->token->type;
        compileNode(node->lhs);
        compileNode(node->rhs);
        emitBinaryOp(op);
        return;
    }
    case ND_UNARY: {
        compileNode(node->operand);
        Opcode code = node->token->type == TOKEN_BANG ? OP_NOT : OP_NEGATE;
        emitByte(code);
        return;
    }
    case ND_VAR: {
        int vmStackPos = resolveLocal(&LOCAL_STATE(compiler), node->token);
        if (vmStackPos != -1) {
            emitBytes(OP_GET_LOCAL, vmStackPos);
        } else {
            uint8_t index = pushIdentifier(node->token);
            emitBytes(OP_GET_GLOBAL, index);
        }
        return;
    }
    case ND_NUMBER:
        emitConstant(NUMBER_VAL(strtod(node->token->start, NULL)));
        break;

    case ND_STRING:
        emitConstant(
            newObjStringInVal(node->token->start + 1, node->token->length - 2));
        return;
    case ND_TEMPLATE_HEAD:
        compileNode(node->span);
        emitConstant(
            newObjStringInVal(node->token->start + 1, node->token->length - 1));
        emitBytes(OP_TEMPLATE, (uint8_t)(node->numSpans));
        return;
    case ND_TEMPLATE_SPAN: {
        int len = node->token->type == TOKEN_AFTER_TEMPLATE
                      ? node->token->length - 1
                      : node->token->length;
        if (node->span != NULL) {
            compileNode(node->span);
        }
        emitConstant(newObjStringInVal(node->token->start, len));
        compileNode(node->operand);
        return;
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
        return;
    }

    case ND_EMPTY:
        return;
    }
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
    discardLocals();
}

static void discardLocals() {
    int discarded = 0;
    while (LOCAL_COUNT(compiler) > 0 &&
           LOCAL_ARR(compiler)[LOCAL_COUNT(compiler) - 1].depth >
               currentScopeDepth()) {
        discarded++;
        LOCAL_COUNT(compiler)--;
    }
    emitBytes(OP_POPN, discarded);
};

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
    case ND_VAR_DECL:
        return;
    case ND_BLOCK:
        freeStmts(node->operand);
        FREE(Node, node);
        return;
    case ND_TERNARY:
        freeNode(node->thenBranch);
        freeNode(node->elseBranch);
        freeNode(node->operand);
        FREE(Node, node);
        return;
    case ND_ASSIGNMENT:
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
    case ND_EXPRESSION:
    case ND_PRINT:
    case ND_UNARY:
        freeNode(node->operand);
        FREE(Node, node);
        return;
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