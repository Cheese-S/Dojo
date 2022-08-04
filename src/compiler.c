#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "parser.h"
#include "vm.h"
#include <stdint.h>
static void initCompiler();
static Chunk *currentChunk();
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
    Node *AST = parse(source);
    emitConstant(AST->value);
    emitByte(OP_RETURN);
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