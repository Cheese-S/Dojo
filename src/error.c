#include "error.h"
#include "scanner.h"
#include "vm.h"
#include <stdarg.h>
#include <stdio.h>

#define START_PRINT_RED fprintf(stderr, RED)
#define END_PRINT_RED fprintf(stderr, RESET)
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

static void printStackTrace();

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

void internalError(const char *message) {
    START_PRINT_RED;
    fprintf(stderr, "[Internal Error]: %s", message);
    END_PRINT_RED;
}

void runtimeError(const char *format, ...) {
    START_PRINT_RED;
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    printStackTrace();

    END_PRINT_RED;
}

static void printStackTrace() {
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFn *fn = frame->closure->fn;
        size_t instruction = frame->ip - fn->chunk.codes - 1;
        fprintf(stderr, "[Line %d] in ", fn->chunk.lines[instruction]);
        if (fn->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%.*s\n", fn->name->length, fn->name->str);
        }
    }
}