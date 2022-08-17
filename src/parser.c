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
static Node *varDeclaration();

static Node *stmt();
static Node *printStmt();
static Node *expressionStmt();

static void expectStmtEnd(const char *str);

/* ------------------------------- EXPRESSION ------------------------------- */

static Node *expression();
static Node *grouping();
static Node *assignment();
static Node *ternary();
static Node *binary();
static Node *unary();
static Node *variable();
static Node *stringTemplate();
static Node *string();
static Node *number();
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
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, assignment, PREC_ASSIGNMENT},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
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
    parser.panicMode = false;
    parser.stmts = &SENTINEL;
    parser.tail = parser.stmts;
}

void terminateParser() {
    terminateScanner();
}

static void resetSentinel() {
    SENTINEL.type = ND_EMPTY;
    SENTINEL.token = NULL;
    SENTINEL.nextStmt = NULL;
}

Node *parse(bool *hadError) {
    advance();

    while (!match(TOKEN_EOF)) {
        skipNewlines();
        Node *decl = declaration();
        if (decl) {
            appendToStmts(decl);
        }
    }
    *hadError = parser.hadError;
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
    Node *decl;
    if (match(TOKEN_VAR)) {
        decl = varDeclaration();
    } else {
        decl = stmt();
    }
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

static Node *varDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect an identifier after var.");
    Token *token = parser.previous;
    Node *initializer = NULL;
    if (match(TOKEN_EQUAL)) {
        initializer = expression();
    }
    consume(TOKEN_NEWLINE,
            "Expect a newline character after a variable declaration");
    return NEW_VAR_DECL(token, initializer);
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
    Token *token = parser.previous;
    Node *express = expression();
    expectStmtEnd("Expect a newline character after a print statement");
    return NEW_PRINT_STMT(token, express);
}

static Node *expressionStmt() {
    Token *token = parser.previous;
    Node *express = expression();
    expectStmtEnd("Expect a newline character after a expression statement");
    return NEW_EXPRESS_STMT(token, express);
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

static Node *assignment(Node *lhs) {
    Token *token = parser.previous;
    Node *rhs = expression();
    return NEW_ASSIGNMENT(token, lhs, rhs);
}

static Node *ternary(Node *condition) {
    Token *token = parser.previous;
    Node *thenBranch = expression();
    consume(TOKEN_COLON, "Expect ':' after expression");
    Node *elseBranch = parsePrecedence(getRule(TOKEN_QUESTION)->precedence - 1);
    return NEW_TERNARY(token, condition, thenBranch, elseBranch);
}

static Node *binary(Node *lhs) {
    Token *op = parser.previous;
    Node *rhs = parsePrecedence(getRule(op->type)->precedence + 1);
    return NEW_BINARY(op, lhs, rhs);
};

static Node *unary() {
    Token *op = parser.previous;
    Node *operand = parsePrecedence(PREC_UNARY);
    return NEW_UNARY(op, operand);
}

static Node *variable() {
    return NEW_VAR(parser.previous);
}

static Node *stringTemplate() {
    Node *head = NEW_TEMPLATE_HEAD(parser.previous);
    Node *prev = head;

    while (parser.previous->type != TOKEN_AFTER_TEMPLATE) {
        Node *express = expression();
        Node *span = NEW_TEMPLATE_SPAN(parser.current, express);
        prev->span = span;
        prev = span;
        advance();
    }

    return head;
}

static Node *string() {
    return NEW_STRING(parser.previous);
}

static Node *number() {
    return NEW_NUMBER(parser.previous);
}

static Node *literal() {
    return NEW_LITERAL(parser.previous);
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
