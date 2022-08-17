#include "error.h"
#include "scanner.h"
#include <stdarg.h>
#include <stdio.h>

#define START_PRINT_RED fprintf(stderr, RED)
#define END_PRINT_RED fprintf(stderr, RESET)
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

void errorAtToken(Token *token, const char *message) {
    START_PRINT_RED;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_NEWLINE) {
        fprintf(stderr, " at newline character");
    } else if (token->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    END_PRINT_RED;
}

void runtimeError(int line, const char *format, ...) {
    START_PRINT_RED;
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    fprintf(stderr, "[line %d] in script\n", line);
    END_PRINT_RED;
}