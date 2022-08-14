#ifndef dojo_parser_h
#define dojo_parser_h

#include "common.h"
#include "node.h"

typedef struct {
    Token *previous;
    Token *current;
    Node *AST;
    bool hadError;
} Parser;

void initParser(const char *source);
void terminateParser();
Node *parse();

void error(const char *message);
void errorAtCurrent(const char *message);

#endif