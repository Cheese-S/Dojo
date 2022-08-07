#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "node.h"
#include "parser.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
static void initCompiler();
static Chunk *currentChunk();

static void visitAST(Node *node);
static void visitNode(Node *node);

static void emitConstant(Value value);
static uint8_t getConstantChunkIndex(Value constant);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitByte(uint8_t byte);

Compiler compiler;

static void initCompiler(Chunk *chunk) {
    compiler.compilingChunk = chunk;
}

static Chunk *currentChunk() {
    return compiler.compilingChunk;
}

void compile(const char *source, Chunk *chunk) {
    initParser(source);
    initCompiler(chunk);
    Node *root = parse(source);
    visitAST(root);
    emitByte(OP_RETURN);
}

static void visitAST(Node *node) {
    visitNode(node);
}

static void visitNode(Node *node) {
    if (node == NULL)
        return;

    switch (node->type) {
    case ND_BINARY: {
        TokenType op = node->op;
        visitNode(node->lhs);
        visitNode(node->rhs);
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
        visitNode(node->operand);
        Opcode code = node->op == TOKEN_BANG ? OP_NOT : OP_NEGATE;
        emitByte(code);
        return;
    }
    case ND_NUMBER:
        emitConstant(node->value);
        return;
    case ND_STRING:
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
    addCodeToChunk(currentChunk(), byte, 0);
}