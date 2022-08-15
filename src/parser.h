#ifndef dojo_parser_h
#define dojo_parser_h

#include "common.h"
#include "node.h"

typedef struct {
    Token *previous;
    Token *current;
    Node *stmts;
    Node *tail;
    bool hadError;
    bool panicMode;
} Parser;

typedef struct {
    Node *stmts;
    bool hadError;
} ParserResult;

void initParser(const char *source);
void terminateParser();
Node *parse();

#endif