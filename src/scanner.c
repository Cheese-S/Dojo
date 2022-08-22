#include <stdio.h>
#include <string.h>

#include "common.h"
#include "memory.h"
#include "scanner.h"

#define MAX_TEMPLATE_LEVELS 2

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define BEGIN_TEMPLATE_SCANNING (templateLevel++)
#define END_TEMPLATE_SCANNING (templateLevel--)

typedef struct {
    const char *start;
    const char *current;
    Token *sentinel;
    Token *tail;
    Token *tokenp;
    int line;
} Scanner;

Scanner scanner;

static int templateLevel = 0;
static bool hadTemplateScanningError = false;

static Token EMPTY_TOKEN = {
    .type = TOKEN_EMPTY, .length = 0, .line = 0, .start = NULL, .next = NULL};

static void freeTokens();

static void tokenize();
static void scanToken();
static Token *skipWhiteSpace();
static Token *blockComment();

static void identifier();
static TokenType identifierType();
static TokenType checkKeyword();
static void number();

static void stringTemplate();
static bool isScanningTemplateString(bool isTemplateSeen);
static void scanBeforeTemplate(bool isTempalteSeen);
static void scanTemplate();

static void string();
static Token *emptyToken();
static Token *errorToken();
static Token *makeToken(TokenType type);
static void appendNewToken(Token *token);

static inline void setScannerHeadToCurrent();
static char advance();
static char advanceN(int n);
static char peek();
static char peekNext();
static bool match(char expected);
static bool isAtEnd();
static int currentTokenLen();

static bool isAlpha(char c);
static bool isDigit(char c);

void initScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.sentinel = emptyToken();
    scanner.tail = emptyToken();
    tokenize();
    scanner.tokenp = scanner.sentinel->next;
}

void terminateScanner() {
    freeTokens();
}

Token *nextToken() {
    if (scanner.tokenp->next == NULL)
        return scanner.tokenp;
    Token *current = scanner.tokenp;
    scanner.tokenp = scanner.tokenp->next;
    return current;
}

static void freeTokens() {
    Token *current = scanner.sentinel->next;
    while (current != NULL) {
        Token *next = current->next;
        FREE(Token, current);
        current = next;
    }
}

static void freeNonErrorTokens(Token *oneBeforehead) {
    Token *prev = oneBeforehead;
    Token *current = oneBeforehead->next;
    while (current != NULL) {
        Token *next = current->next;
        if (current->type != TOKEN_ERROR) {
            prev->next = next;
            FREE(Token, current);
        } else {
            prev = current;
        }
        current = next;
    }
    scanner.tail = prev;
}

static void tokenize() {
    while (scanner.tail->type != TOKEN_EOF) {
        scanToken();
    }
}

void scanToken() {
    Token *error = skipWhiteSpace();
    scanner.start = scanner.current;

    if (error->type != TOKEN_EMPTY) {
        appendNewToken(error);
        return;
    }

    if (isAtEnd()) {
        appendNewToken(makeToken(TOKEN_EOF));
        return;
    }

    char c = advance();
    if (isAlpha(c)) {
        identifier();
        return;
    }
    if (isDigit(c)) {
        number();
        return;
    }

    switch (c) {
    case '(':
        appendNewToken(makeToken(TOKEN_LEFT_PAREN));
        return;
    case ')':
        appendNewToken(makeToken(TOKEN_RIGHT_PAREN));
        return;
    case '{':
        appendNewToken(makeToken(TOKEN_LEFT_BRACE));
        return;
    case '}':
        appendNewToken(makeToken(TOKEN_RIGHT_BRACE));
        return;
    case '\n':
        if (scanner.tail->type != TOKEN_NEWLINE) {
            appendNewToken(makeToken(TOKEN_NEWLINE));
        }
        scanner.line++;
        return;
    case ',':
        appendNewToken(makeToken(TOKEN_COMMA));
        return;
    case '.':
        appendNewToken(makeToken(TOKEN_DOT));
        return;
    case '-':
        appendNewToken(makeToken(TOKEN_MINUS));
        return;
    case '+':
        appendNewToken(makeToken(TOKEN_PLUS));
        return;
    case '*':
        appendNewToken(makeToken(TOKEN_STAR));
        return;
    case '/':
        appendNewToken(makeToken(TOKEN_SLASH));
        return;
    case '?':
        appendNewToken(makeToken(TOKEN_QUESTION));
        return;
    case ':':
        appendNewToken(makeToken(TOKEN_COLON));
        return;
    case ';':
        appendNewToken(makeToken(TOKEN_SEMICOLON));
        return;
    case '!':
        appendNewToken(makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG));
        return;
    case '=':
        appendNewToken(makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL));
        return;
    case '<':
        appendNewToken(makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS));
        return;
    case '>':
        appendNewToken(
            makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER));
        return;
    case '&':
        if (match('&')) {
            appendNewToken(makeToken(TOKEN_AND));
            return;
        }
        break;
    case '|':
        if (match('|')) {
            appendNewToken(makeToken(TOKEN_OR));
            return;
        }
        break;
    case '"':
        string();
        return;
    case '`':
        stringTemplate();
        return;
    }
    appendNewToken(errorToken("Unexpected character"));
    return;
}

static Token *skipWhiteSpace() {
    Token *res = emptyToken();
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '/':
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
                if (peek() == '\n') {
                    scanner.line++;
                    advance();
                    return makeToken(TOKEN_NEWLINE);
                }
            } else if (peekNext() == '*') {
                advanceN(2);
                res = blockComment();
            } else {
                return emptyToken();
            }
            break;
        default:
            return res;
        }
    }
}

static Token *blockComment() {
    int block = 1;
    while (!isAtEnd()) {
        char c = advance();
        if (c == '\n') {
            scanner.line++;
        } else if (c == '/' && peek() == '*') {
            block++;
        } else if (c == '*' && peek() == '/') {
            if (--block == 0) {
                advance();
                break;
            }
        }
    }
    if (block != 0) {
        return errorToken("Unending block comment");
    }
    return emptyToken();
}

static void identifier() {
    while (isAlpha(peek()) || isDigit(peek()))
        advance();
    appendNewToken(makeToken(identifierType()));
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
    case 'b':
        return checkKeyword(1, 4, "reak", TOKEN_BREAK);
    case 'c': {
        if (currentTokenLen() > 1) {
            switch (scanner.start[1]) {
            case 'l':
                return checkKeyword(2, 3, "ass", TOKEN_CLASS);
            case 'o':
                return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
            }
        }
        break;
    }
    case 'e':
        return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f': {
        int len = currentTokenLen();
        if (len > 1) {
            switch (scanner.start[1]) {
            case 'a':
                return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'n':
                return len == 2 ? TOKEN_FN : TOKEN_IDENTIFIER;
            }
        }
        break;
    }
    case 'i':
        return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'p':
        // TODO: REMOVE AFTER ADDING NATIVE FUNCTION "PRINT"
        return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (currentTokenLen() > 1) {
            switch (scanner.start[1]) {
            case 'h':
                return checkKeyword(2, 2, "is", TOKEN_THIS);
            case 'r':
                return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v':
        return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
        return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
    if (currentTokenLen() == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

// stringTemplate appends a series of Tokens with the given input `xxxxxxxxx`
// A string with no template will append only one token TOKEN_STRING
// A template string, `head ${false} mid ${true} end` is scanned as follows
// "`head" TOKEN_PRE_TEMPLATE
// false   TOKEN_FALSE
// " mid " TOKNE_MID_TEMPLATE
// true    TOKEN_TRUE
// " end`" TOKEN_AFTER_TEMPLATE

static void stringTemplate() {
    Token *beforeScan = scanner.tail;
    bool isTemplateSeen = false;
    while (peek() != '`' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
        }

        if (peek() == '$' && peekNext() == '{') {
            BEGIN_TEMPLATE_SCANNING;
            if (templateLevel > MAX_TEMPLATE_LEVELS) {
                appendNewToken(errorToken("Template string may only go " STR(
                    MAX_TEMPLATE_LEVELS) " levels deep"));
                hadTemplateScanningError = true;
            }
            scanBeforeTemplate(isTemplateSeen);
            scanTemplate();
            isTemplateSeen = true;
            END_TEMPLATE_SCANNING;
            continue;
        }
        advance();
    }

    if (isAtEnd()) {
        if (isScanningTemplateString(isTemplateSeen)) {
            hadTemplateScanningError = true;
        }
        appendNewToken(errorToken("Unterminated template string"));
    }

    if (templateLevel == 0 && hadTemplateScanningError) {
        freeNonErrorTokens(beforeScan);
        hadTemplateScanningError = false;
        match('`');
        return;
    }

    match('`');

    TokenType endTemplateType =
        isTemplateSeen ? TOKEN_AFTER_TEMPLATE : TOKEN_STRING;
    appendNewToken(makeToken(endTemplateType));
}

static void scanBeforeTemplate(bool isTemplateSeen) {
    TokenType nonTemplateType =
        isTemplateSeen ? TOKEN_TWEEN_TEMPLATE : TOKEN_PRE_TEMPLATE;
    appendNewToken(makeToken(nonTemplateType));
    advanceN(2);
    setScannerHeadToCurrent();
}

static void scanTemplate(bool *hadError) {
    while (peek() != '}' && !isAtEnd()) {
        scanToken();
        if (scanner.tail->type == TOKEN_ERROR) {
            hadTemplateScanningError = true;
        }
    }
    match('}');
    setScannerHeadToCurrent();
}

static bool isScanningTemplateString(bool isTemplateSeen) {
    return templateLevel > 0 || isTemplateSeen;
}

static void string() {
    bool containNewLine = false;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
            containNewLine = true;
        }
        advance();
    }

    if (isAtEnd()) {
        appendNewToken(errorToken("Unterminated string literal"));
        return;
    }
    if (containNewLine) {
        appendNewToken(errorToken("Newline character '\\n' in string"));
        return;
    }

    advance();
    appendNewToken(makeToken(TOKEN_STRING));
}

static void number() {
    while (isDigit(peek())) {
        advance();
    }

    if (peek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(peek())) {
            advance();
        }
    }

    appendNewToken(makeToken(TOKEN_NUMBER));
}

static Token *errorToken(const char *msg) {
    Token *token = ALLOCATE(Token, 1);
    token->type = TOKEN_ERROR;
    token->start = msg;
    token->length = strlen(msg);
    token->line = scanner.line;
    token->next = NULL;
    return token;
}

static Token *emptyToken() {
    return &EMPTY_TOKEN;
}

static void appendNewToken(Token *token) {
    scanner.tail->next = token;
    scanner.tail = token;
}

static Token *makeToken(TokenType type) {
    Token *token = ALLOCATE(Token, 1);
    token->type = type;
    token->start = scanner.start;
    token->length = (int)(scanner.current - scanner.start);
    token->line = scanner.line;
    return token;
}

static inline void setScannerHeadToCurrent() {
    scanner.start = scanner.current;
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char advanceN(int n) {
    scanner.current = scanner.current + n;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static int currentTokenLen() {
    return (int)(scanner.current - scanner.start);
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
};