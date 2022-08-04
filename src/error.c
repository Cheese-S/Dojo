#include "error.h"
#include "scanner.h"
#include <stdio.h>

#define RED_TEXT(text) RED text RESET
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

void errorAt(Token *token, const char *message) {
    fprintf(stderr, RED_TEXT("[line %d] Error"), token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, RED_TEXT("at '%.*s'"), token->length, token->start);
    }

    fprintf(stderr, RED_TEXT(": %s\n"), message);
}
