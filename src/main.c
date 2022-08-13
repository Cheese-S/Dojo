#include "error.h"
#include "hashmap.h"
#include "scanner.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>

static void repl();

static void runFile(const char *path);
static char *readFile(const char *path);
static FILE *openFile(const char *path);
static size_t getFileSize(FILE *file);
static char *readFileContent(FILE *file, size_t fileSize, const char *path);

int main(int argc, const char *argv[]) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else if (argc == 3) {
        hashmap_test();
    } else {
        fprintf(stderr, "Usage: dojo [path]\n");
        exit(64);
    }

    return 0;
}

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interpret(line);
        printf("\n");
        // Token Scan Code
        // initScanner(line);
        // int prevLine = -1;
        // for (;;) {
        //     Token *token = nextToken();
        //     if (token->line != prevLine) {
        //         printf("%4d ", token->line);
        //         prevLine = token->line;
        //     } else {
        //         printf("   | ");
        //     }
        //     printf("%2d '%.*s'\n", token->type, token->length, token->start);

        //     if (token->type == TOKEN_EOF)
        //         break;
        // }
    }
}

static void runFile(const char *path) {
    char *source = readFile(path);
    interpret(source);
    free(source);
}

static char *readFile(const char *path) {
    FILE *file = openFile(path);
    size_t fileSize = getFileSize(file);
    char *buffer = readFileContent(file, fileSize, path);
    fclose(file);
    return buffer;
}

static FILE *openFile(const char *path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    return file;
}

static size_t getFileSize(FILE *file) {
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    return fileSize;
}

static char *readFileContent(FILE *file, size_t fileSize, const char *path) {
    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\". \n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    return buffer;
}
