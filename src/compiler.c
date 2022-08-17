#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "error.h"
#include "memory.h"
#include "node.h"
#include "object.h"
#include "parser.h"
#include "scanner.h"
#include "vm.h"

static Chunk *currentChunk();

static void compileScript(Node *node);
static void compileNode(Node *node);
static void freeScript(Node *AST);
static void freeNode(Node *node);

static bool isAssignable(Node *lhs);

static void emitConstant(Value value);
static uint8_t pushIdentifier(Token *name);
static uint8_t pushConstant(Value constant);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitBinaryOp(TokenType op);
static void emitByte(uint8_t byte);

static void compilerError(Token *token, const char *msg);

Compiler compiler;

void initCompiler(Chunk *chunk) {
    compiler.compilingChunk = chunk;
    compiler.hadError = false;
}

void terminateCompiler() {
    freeScript(compiler.script);
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
    compileScript(compiler.script);
    emitByte(OP_RETURN);
    return compiler.hadError || parserError;
}

static void compileScript(Node *script) {
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
        if (node->operand) {
            compileNode(node->operand);
        } else {
            emitByte(OP_NIL);
        }
        uint8_t index = pushIdentifier(node->token);
        emitBytes(OP_DEFINE_GLOBAL, index);
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
        uint8_t index = pushIdentifier(node->lhs->token);
        emitBytes(OP_SET_GLOBAL, index);
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
        uint8_t index = pushIdentifier(node->token);
        emitBytes(OP_GET_GLOBAL, index);
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
        emitByte(OP_TEMPLATE);
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

static bool isAssignable(Node *lhs) {
    return lhs->type == ND_VAR;
}

static void emitConstant(Value constant) {
    emitBytes(OP_CONSTANT, pushConstant(constant));
}

// TODO: better names for the following two functions.

static uint8_t pushIdentifier(Token *name) {
    return (uint8_t)addConstantToChunk(
        currentChunk(), newObjStringInVal(name->start, name->length));
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

static void freeScript(Node *script) {
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