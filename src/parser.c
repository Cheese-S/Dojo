#include "parser.h"
#include "error.h"
#include "memory.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdlib.h>

Parser parser;

static void expression();
static void number();
static void unary();

void initParser(const char *source) {
    initScanner(source);
    parser.hadError = false;
    parser.AST = newNode();
}

Node *parse(const char *source) {
    advance();

    while (!match(TOKEN_EOF)) {
        expression();
    }

    return parser.AST;
}

static void expression() {
    if (check(TOKEN_NUMBER)) {
        number();
    }
    advance();
}

static void number() {
    parser.AST->value = numToValue(strtod(parser.current.start, NULL));
}

void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;
        errorAtCurrent(parser.current.start);
    }
}

bool check(TokenType type) {
    return parser.current.type == type;
}

bool match(TokenType type) {
    if (!check(type))
        return false;
    advance();
    return true;
}

void consume(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

void error(const char *message) {
    errorAt(&parser.previous, message);
}

void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}