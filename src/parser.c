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
static void appendToNext(Node **tail, Node *stmt);

/* -------------------------------- STATEMENT ------------------------------- */

static Node *declaration();
static Node *fnDeclaration();
static Node *parameters();
static Node *varDeclaration();

static Node *stmt();
static Node *forStmt();
static Node *forInit();
static Node *forCondition();
static Node *forIncrement();
static Node *whileStmt();
static Node *continueStmt();
static Node *breakStmt();
static Node *ifStmt();
static bool isLastChainBlock(Node *thenBranch, Node *elseBranch);
static Node *branch();
static Node *chainBlockStmt();
static Node *blockStmt();
static Node *parseBlock();
static Node *printStmt();
static Node *returnStmt();
static Node *expressionStmt();

static void expectStmtEnd(const char *str);

/* ------------------------------- EXPRESSION ------------------------------- */

static Node *expression();
static Node *call();
static Node *arguments();
static Node *grouping();
static Node *assignment();
static Node *ternary();
static Node *and_();
static Node *or_();
static Node *binary();
static Node *unary();
static Node *variable();
static Node *stringTemplate();
static Node *string();
static Node *number();
static Node *literal();
static Node *parsePrecedence(Precedence precedence);
static ParseRule *getRule(TokenType type);

/* ----------------------------- PARSER FUNCIONS ---------------------------- */

static void advance();
static bool check(TokenType type);
static bool match(TokenType type);
static bool consume(TokenType type, const char *message);
static void errorAtCurrent(const char *msg);
static void errorAtPrevious(const char *msg);
static void parserError(Token *token, const char *msg);
static void synchronize();

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
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
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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
    SENTINEL.next = NULL;
}

Node *parse(bool *hadError) {
    advance();

    while (!match(TOKEN_EOF)) {
        skipNewlines();
        Node *decl = declaration();
        if (decl) {
            appendToNext(&parser.tail, decl);
        }
    }
    *hadError = parser.hadError;
    return parser.stmts->next;
}

static void skipNewlines() {
    while (match(TOKEN_NEWLINE)) {
        ;
    }
}

static void appendToNext(Node **tail, Node *stmt) {
    (*tail)->next = stmt;
    *tail = stmt;
    stmt->next = NULL;
}

static Node *declaration() {
    Node *decl;
    if (match(TOKEN_VAR)) {
        decl = varDeclaration();
    } else if (match(TOKEN_FN)) {
        decl = fnDeclaration();
    } else {
        decl = stmt();
    }
    if (parser.panicMode) {
        synchronize();
    }
    return decl;
}

static Node *fnDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect an identifier after 'fn'.");
    Token *fnName = parser.previous;
    Node *params = parameters();
    consume(TOKEN_LEFT_BRACE, "Expect a '{' after params list");
    Node *body = parseBlock();
    return NEW_FN_DECL(fnName, params, body);
}

static Node *parameters() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (match(TOKEN_RIGHT_PAREN)) {
        return NULL;
    }
    Node *head = NULL;
    Node *tail = head;
    do {
        consume(TOKEN_IDENTIFIER,
                "Expect identfier inside function paramlist.");
        Token *ident = parser.previous;
        Node *param = NEW_PARAM(ident);
        if (!tail) {
            head = tail = param;
        } else {
            appendToNext(&tail, param);
        }
    } while (match(TOKEN_COMMA));
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after params list.");
    return head;
}

static Node *varDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect an identifier after 'var'.");
    Token *token = parser.previous;
    Node *initializer = NULL;
    if (match(TOKEN_EQUAL)) {
        initializer = expression();
    }
    expectStmtEnd("Expect a newline character after a variable declaration");
    return NEW_VAR_DECL(token, initializer);
}

static Node *stmt() {
    if (match(TOKEN_FOR)) {
        return forStmt();
    } else if (match(TOKEN_WHILE)) {
        return whileStmt();
    } else if (match(TOKEN_CONTINUE)) {
        return continueStmt();
    } else if (match(TOKEN_BREAK)) {
        return breakStmt();
    } else if (match(TOKEN_IF)) {
        return ifStmt();
    } else if (match(TOKEN_LEFT_BRACE)) {
        return blockStmt();
    } else if (match(TOKEN_PRINT)) {
        return printStmt();
    } else if (match(TOKEN_RETURN)) {
        return returnStmt();
    }
    { return expressionStmt(); }
}

static Node *forStmt() {
    Token *token = parser.previous;
    consume(TOKEN_LEFT_PAREN, "Expect a '(' after 'for'.");
    Node *init = forInit();
    consume(TOKEN_SEMICOLON, "Expect a ';'.");
    Node *condition = forCondition();
    consume(TOKEN_SEMICOLON, "Expect a ';'.");
    Node *increment = forIncrement();
    consume(TOKEN_RIGHT_PAREN, "Expect a ')' after for clauses.");
    skipNewlines();
    Node *body = stmt();
    return NEW_FOR_STMT(token, init, condition, increment, body);
}

static Node *forInit() {
    if (check(TOKEN_SEMICOLON)) {
        return NULL;
    } else if (match(TOKEN_VAR)) {
        consume(TOKEN_IDENTIFIER, "Expect an identifier after 'var'.");
        Token *token = parser.previous;
        Node *initializer = NULL;
        if (match(TOKEN_EQUAL)) {
            initializer = expression();
        }
        return NEW_VAR_DECL(token, initializer);
    } else {
        Token *token = parser.previous;
        Node *express = expression();
        return NEW_EXPRESS_STMT(token, express);
    }
}

static Node *forCondition() {
    if (check(TOKEN_SEMICOLON)) {
        return NULL;
    } else {
        return expression();
    }
}

static Node *forIncrement() {
    if (check(TOKEN_RIGHT_PAREN)) {
        return NULL;
    } else {
        return expression();
    }
}

static Node *whileStmt() {
    Token *token = parser.previous;
    consume(TOKEN_LEFT_PAREN, "Expect a '(' after 'while'.");
    Node *condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect a ')' after the if condition.");
    skipNewlines();
    Node *body = stmt();
    return NEW_WHILE_STMT(token, condition, body);
}

static Node *breakStmt() {
    Token *token = parser.previous;
    expectStmtEnd("Expect a newline character after a break statement.");
    return NEW_BREAK_STMT(token);
}

static Node *continueStmt() {
    Token *token = parser.previous;
    expectStmtEnd("Expect a newline character after a continue statement.");
    return NEW_CONTINUE_STMT(token);
}

static Node *ifStmt() {
    Token *token = parser.previous;
    consume(TOKEN_LEFT_PAREN, "Expect a '(' after 'if'.");
    Node *condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect a ')' after the if condition.");
    Node *thenBranch = branch();
    Node *elseBranch = NULL;
    if (match(TOKEN_ELSE)) {
        elseBranch = branch();
    }

    if (isLastChainBlock(thenBranch, elseBranch)) {
        expectStmtEnd("Expect a newline character after an if-else statement");
    }

    return NEW_IF_STMT(token, condition, thenBranch, elseBranch);
}

static Node *branch() {
    skipNewlines();
    if (match(TOKEN_LEFT_BRACE)) {
        return chainBlockStmt();
    } else {
        return stmt();
    }
}

static Node *chainBlockStmt() {
    Node *block = parseBlock();
    return block;
}

static bool isLastChainBlock(Node *thenBranch, Node *elseBranch) {
    return ((thenBranch->type == ND_BLOCK && !elseBranch) ||
            (elseBranch && elseBranch->type == ND_BLOCK));
}

static Node *blockStmt() {
    Node *block = parseBlock();
    expectStmtEnd("Expect a new line character after a block statement");
    return block;
}

static Node *parseBlock() {
    Token *token = parser.previous;
    skipNewlines();
    Node *head = NULL;
    Node *tail = head;

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        skipNewlines();
        Node *decl = declaration();
        if (head) {
            appendToNext(&tail, decl);
        } else {
            head = decl;
            tail = head;
        }
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' at the end of a block statement");
    Node *block = NEW_BLOCK_STMT(token, head);
    return block;
}

static Node *printStmt() {
    // TODO: Replace print stmt with native function
    Token *token = parser.previous;
    Node *express = expression();
    expectStmtEnd("Expect a newline character after a print statement");
    return NEW_PRINT_STMT(token, express);
}

static Node *returnStmt() {
    Token *token = parser.previous;
    Node *returnVal = NULL;
    if (!check(TOKEN_NEWLINE)) {
        returnVal = expression();
    }
    expectStmtEnd("Expect a newline character after a return statement.");
    return NEW_RETURN_STMT(token, returnVal);
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

static Node *call(Node *lhs) {
    Token *token = parser.previous;
    Node *args = arguments();
    return NEW_CALL(token, lhs, args);
}

static Node *arguments() {
    Node *head = NULL;
    Node *tail = head;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            Node *arg = expression();
            if (!tail) {
                head = tail = arg;
            } else {
                appendToNext(&tail, arg);
            }
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect a ')' after function arguments");
    return head;
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

static Node *and_(Node *lhs) {
    Token *op = parser.previous;
    Node *rhs = parsePrecedence(PREC_AND);
    return NEW_AND(op, lhs, rhs);
}

static Node *or_(Node *lhs) {
    Token *op = parser.previous;
    Node *rhs = parsePrecedence(PREC_OR);
    return NEW_OR(op, lhs, rhs);
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
    Node *tail = head;

    while (parser.previous->type != TOKEN_AFTER_TEMPLATE) {
        Node *express = expression();
        Node *span = NEW_TEMPLATE_SPAN(parser.current, express);
        appendToNext(&tail, span);
        head->count++;
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
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
            return;
        default:;
        }
        advance();
    }
}
