#include "parser.h"
#include "error.h"
#include "memory.h"
#include "node.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <stdbool.h>
#include <stdlib.h>

Parser parser;

Node SENTINEL;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_TERNARY,    // ?:
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

static void resetSentinel();
static void skipNewlines();
static void appendToStmts(Node *stmt);

/* -------------------------------- STATEMENT ------------------------------- */

static Node *declaration();
static void synchronize();

static Node *stmt();
static Node *printStmt();
static Node *expressionStmt();

static void expectStmtEnd(const char *str);

/* ------------------------------- EXPRESSION ------------------------------- */

static Node *expression();
static Node *grouping();
static Node *ternary();
static Node *binary();
static Node *unary();
static Node *number();
static Node *stringTemplate();
static Node *string();
static Node *literal();
static Node *parsePrecedence(Precedence precedence);
static ParseRule *getRule(TokenType type);

static void advance();
static bool check(TokenType type);
static bool match(TokenType type);
static bool consume(TokenType type, const char *message);
static void errorAtCurrent(const char *msg);
static void errorAtPrevious(const char *msg);
static void parserError(Token *token, const char *msg);

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
    [TOKEN_QUESTION] = {NULL, ternary, PREC_TERNARY},
    [TOKEN_BANG] = {unary, NULL, PREC_UNARY},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_PRE_TEMPLATE] = {stringTemplate, NULL, PREC_NONE},
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
    resetSentinel();
    parser.hadError = false;
    parser.stmts = &SENTINEL;
    parser.tail = parser.stmts;
}

void terminateParser() {
    terminateScanner();
}

static void resetSentinel() {
    SENTINEL.line = -1;
    SENTINEL.value = NIL_VAL;
    SENTINEL.type = ND_EMPTY;
    SENTINEL.op = TOKEN_EMPTY;
    SENTINEL.nextStmt = NULL;
}

Node *parse(const char *source) {
    advance();

    while (!match(TOKEN_EOF)) {
        skipNewlines();
        Node *decl = declaration();
        if (decl) {
            appendToStmts(decl);
        }
    }

    return parser.stmts->nextStmt;
}

static void skipNewlines() {
    while (match(TOKEN_NEWLINE)) {
        ;
    }
}

static void appendToStmts(Node *stmt) {
    parser.tail->nextStmt = stmt;
    parser.tail = stmt;
}

static Node *declaration() {

    Node *decl = stmt();
    if (parser.panicMode) {
        synchronize();
    }
    return decl;
}

static void synchronize() {
    parser.panicMode = false;
    while (parser.current->type != TOKEN_EOF) {
        if (parser.previous->type == TOKEN_NEWLINE)
            return;
        switch (parser.current->type) {
        case TOKEN_CLASS:
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;
        default:;
        }
        advance();
    }
}

static Node *stmt() {
    if (match(TOKEN_PRINT)) {
        return printStmt();
    } else {
        return expressionStmt();
    }
}

static Node *printStmt() {
    // TODO: Replace print stmt with native function
    int line = parser.previous->line;
    Node *express = expression();
    expectStmtEnd("Expect a new line character after a print statement");
    return NEW_PRINT(express, line);
}

static Node *expressionStmt() {
    Node *express = expression();
    expectStmtEnd("Expect a newline character after a expression statement");
    return express;
}

static void expectStmtEnd(const char *msg) {
    if (check(TOKEN_EOF))
        return;
    consume(TOKEN_NEWLINE, msg);
}

static Node *expression() {
    return parsePrecedence(PREC_ASSIGNMENT);
}

static Node *grouping() {
    Node *node = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
    return node;
};

static Node *ternary(Node *condition) {
    int line = parser.previous->line;
    Node *thenBranch = expression();
    consume(TOKEN_COLON, "Expect ':' after expression");
    Node *elseBranch = parsePrecedence(getRule(TOKEN_QUESTION)->precedence - 1);
    return NEW_TERNARY(condition, thenBranch, elseBranch, line);
}

static Node *binary(Node *lhs) {
    TokenType op = parser.previous->type;
    int line = parser.previous->line;
    Node *rhs = parsePrecedence(getRule(op)->precedence + 1);
    return NEW_BINARY(op, line, lhs, rhs);
};

static Node *unary() {
    TokenType op = parser.previous->type;
    int line = parser.previous->line;
    Node *operand = parsePrecedence(PREC_UNARY);
    return NEW_UNARY(op, line, operand);
}

static Node *stringTemplate() {
    Node *head =
        NEW_TEMPLATE_HEAD(newObjStringInVal(parser.previous->start + 1,
                                            parser.previous->length - 1),
                          parser.previous->line);
    Node *prev = head;

    while (parser.previous->type != TOKEN_AFTER_TEMPLATE) {
        Node *express = expression();
        int length = parser.current->type == TOKEN_AFTER_TEMPLATE
                         ? parser.current->length - 1
                         : parser.current->length;
        Node *span = NEW_TEMPLATE_SPAN(
            express, newObjStringInVal(parser.current->start, length),
            parser.current->line);
        prev->span = span;
        prev = span;
        advance();
    }

    return head;
}

static Node *string() {
    return NEW_STRING(OBJ_VAL(newObjString(parser.previous->start + 1,
                                           parser.previous->length - 2)),
                      parser.previous->line);
}

static Node *number() {
    return NEW_NUMBER(NUMBER_VAL(strtod(parser.previous->start, NULL)),
                      parser.previous->line);
}

static Node *literal() {
    TokenType type = parser.previous->type;
    int line = parser.previous->line;
    if (type == TOKEN_TRUE) {
        return NEW_LITERAL(ND_TRUE, line);
    } else if (type == TOKEN_FALSE) {
        return NEW_LITERAL(ND_FALSE, line);
    } else {
        return NEW_LITERAL(ND_NIL, line);
    }
}

static Node *parsePrecedence(Precedence precedence) {
    advance();

    PrefixFn prefix = getRule(parser.previous->type)->prefix;

    if (prefix == NULL) {
        errorAtPrevious("Expected expression");
        return NULL;
    }

    Node *left = prefix();

    while (precedence <= getRule(parser.current->type)->precedence) {
        advance();
        InfixFn infix = getRule(parser.previous->type)->infix;
        left = infix(left);
    }

    return left;
}

static ParseRule *getRule(TokenType type) {
    return &rules[type];
}

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = nextToken();
        if (parser.current->type != TOKEN_ERROR)
            break;
        errorAtCurrent(parser.current->start);
    }
}

static bool check(TokenType type) {
    return parser.current->type == type;
}

static bool match(TokenType type) {
    if (!check(type))
        return false;
    advance();
    return true;
}

static bool consume(TokenType type, const char *message) {
    if (parser.current->type == type) {
        advance();
        return true;
    }
    errorAtCurrent(message);
    return false;
}

static void errorAtCurrent(const char *msg) {
    parserError(parser.current, msg);
}

static void errorAtPrevious(const char *msg) {
    parserError(parser.previous, msg);
}

static void parserError(Token *token, const char *msg) {
    if (parser.panicMode)
        return;

    parser.panicMode = true;
    parser.hadError = true;
    errorAtToken(token, msg);
}
