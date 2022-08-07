#include "parser.h"
#include "error.h"
#include "memory.h"
#include "node.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdlib.h>

Parser parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // ||
    PREC_AND,        // &&
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_SHIFT,      // << >>
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef Node *(*ParserFn)();

typedef Node *(*InfixFn)(Node *);
typedef Node *(*PrefixFn)();

typedef struct {
    PrefixFn prefix;
    InfixFn infix;
    Precedence precedence;
} ParseRule;

static Node *expression();
static Node *grouping();
static Node *binary();
static Node *unary();
static Node *number();
static Node *literal();
static Node *parsePrecedence(Precedence precedence);
static ParseRule *getRule(TokenType type);

static void advance();
static bool check(TokenType type);
static bool match(TokenType type);
static void consume(TokenType type, const char *message);

static void initNode(Node *node, NodeType type, int line);
static void makeNodeHead(Node *node);

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_NEWLINE] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_UNARY},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

void initParser(const char *source) {
    initScanner(source);
    parser.hadError = false;
    parser.AST = NULL;
}

Node *parse(const char *source) {
    advance();

    while (!match(TOKEN_EOF)) {
        parser.AST = expression();
    }

    return parser.AST;
}

static Node *expression() {
    return parsePrecedence(PREC_ASSIGNMENT);
}

static Node *parsePrecedence(Precedence precedence) {
    advance();

    if (parser.previous.type == TOKEN_NEWLINE) {
        return parser.AST;
    }

    PrefixFn prefix = getRule(parser.previous.type)->prefix;

    if (prefix == NULL) {
        error("Expected expression");
        return NULL;
    }

    Node *left = prefix();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        InfixFn infix = getRule(parser.previous.type)->infix;
        left = infix(left);
    }

    return left;
}

static Node *grouping() {
    Node *node = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
    return node;
};

static Node *binary(Node *lhs) {
    TokenType op = parser.previous.type;
    int line = parser.previous.line;
    Node *rhs = parsePrecedence(getRule(op)->precedence + 1);
    return NEW_BINARY(op, line, lhs, rhs);
};

static Node *unary() {
    TokenType op = parser.previous.type;
    int line = parser.previous.line;
    Node *operand = parsePrecedence(PREC_UNARY);
    return NEW_UNARY(op, line, operand);
}

static Node *number() {
    return NEW_NUMBER(NUMBER_VAL(strtod(parser.previous.start, NULL)),
                      parser.previous.line);
}

static Node *literal() {
    TokenType type = parser.previous.type;
    int line = parser.previous.line;
    if (type == TOKEN_TRUE) {
        return NEW_LITERAL(ND_TRUE, line);
    } else if (type == TOKEN_FALSE) {
        return NEW_LITERAL(ND_FALSE, line);
    } else {
        return NEW_LITERAL(ND_NIL, line);
    }
}

static ParseRule *getRule(TokenType type) {
    return &rules[type];
}

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;
        errorAtCurrent(parser.current.start);
    }
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type))
        return false;
    advance();
    return true;
}

static void consume(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void makeNodeHead(Node *node) {
    parser.AST = node;
}

void error(const char *message) {
    errorAt(&parser.previous, message);
}

void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}
