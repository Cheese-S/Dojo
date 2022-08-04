#ifndef dojo_parser_h
#define dojo_parser_h

#include "common.h"
#include "scanner.h"

typedef struct {
    Token previous;
    Token current;
    Node *AST;
    bool hadError;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParserFn)(bool canAssign);

typedef struct {
    ParserFn prefix;
    ParserFn infix;
    Precedence precedence;
} ParserRule;

void initParser(const char *source);
Node *parse();

void parsePrecedence(Precedence precedence);

void advance();
bool check(TokenType type);
bool match(TokenType type);
void consume(TokenType type, const char *message);

void error(const char *message);
void errorAtCurrent(const char *message);

#endif