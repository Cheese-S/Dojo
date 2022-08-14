#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "memory.h"
#include "node.h"
#include "parser.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
static Chunk *currentChunk();

static void compileAST(Node *node);
static void compileNode(Node *node);
static void freeAST(Node *AST);
static void freeNode(Node *node);

static void emitConstant(Value value);
static uint8_t getConstantChunkIndex(Value constant);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitByte(uint8_t byte);

Compiler compiler;

void initCompiler(Chunk *chunk) {
    compiler.compilingChunk = chunk;
}

void terminateCompiler() {
    freeAST(compiler.AST);
    terminateParser();
}

void compile(const char *source, Chunk *chunk) {
    initParser(source);
    initCompiler(chunk);
    compiler.AST = parse(source);
    compileAST(compiler.AST);
    emitByte(OP_RETURN);
}

static void freeAST(Node *AST) {
    freeNode(AST);
}

static void freeNode(Node *node) {
    if (node == NULL)
        return;

    switch (node->type) {
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
    case ND_UNARY:
        freeNode(node->operand);
        FREE(Node, node);
        return;
    case ND_STRING:
    case ND_NUMBER:
    case ND_TRUE:
    case ND_FALSE:
    case ND_NIL:
        FREE(Node, node);
        return;
    }
}

static void compileAST(Node *AST) {
    compileNode(AST);
}

static void compileNode(Node *node) {
    if (node == NULL)
        return;

    compiler.currentCompiling = node;

    switch (node->type) {
    case ND_BINARY: {
        TokenType op = node->op;
        compileNode(node->lhs);
        compileNode(node->rhs);
        if (op == TOKEN_PLUS) {
            emitByte(OP_ADD);
        } else if (op == TOKEN_MINUS) {
            emitByte(OP_SUBTRACT);
        } else if (op == TOKEN_SLASH) {
            emitByte(OP_DIVIDE);
        } else if (op == TOKEN_STAR) {
            emitByte(OP_MULTIPLY);
        }
        return;
    }
    case ND_UNARY: {
        compileNode(node->operand);
        Opcode code = node->op == TOKEN_BANG ? OP_NOT : OP_NEGATE;
        emitByte(code);
        return;
    }
    case ND_NUMBER:
    case ND_STRING:
        emitConstant(node->value);
        return;
    case ND_TEMPLATE_HEAD: {
        compileNode(node->span);
        emitConstant(node->value);
        emitByte(OP_TEMPLATE);
        return;
    }

    case ND_TEMPLATE_SPAN:
        if (node->span != NULL) {
            compileNode(node->span);
        }
        emitConstant(node->value);
        compileNode(node->operand);
        return;
    case ND_TRUE:
        emitByte(OP_TRUE);
        return;
    case ND_FALSE:
        emitByte(OP_FALSE);
        return;
    case ND_NIL:
        emitByte(OP_NIL);
        return;
    }
}

static void emitConstant(Value constant) {
    emitBytes(OP_CONSTANT, getConstantChunkIndex(constant));
}

static uint8_t getConstantChunkIndex(Value constant) {
    return (uint8_t)addConstantToChunk(currentChunk(), constant);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitByte(uint8_t byte) {
    addCodeToChunk(currentChunk(), byte, compiler.currentCompiling->line);
}

static Chunk *currentChunk() {
    return compiler.compilingChunk;
}