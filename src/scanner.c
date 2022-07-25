#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

static Token EMPTY_TOKEN = {
    .type = TOKEN_EMPTY, .length = 0, .line = 0, .start = NULL};

static Token skipWhiteSpace();
static Token blockComment();

static Token identifier();
static TokenType identifierType();
static TokenType checkKeyword();
static Token number();
static Token multilineString();
static Token string();
static Token emptyToken();
static Token errorToken();
static Token makeToken(TokenType type);

static char advance();
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
}

Token scanToken() {
    Token error = skipWhiteSpace();
    scanner.start = scanner.current;

    if (error.type != TOKEN_EMPTY) {
        return error;
    }

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();
    if (isAlpha(c))
        return identifier();
    if (isDigit(c))
        return number();

    switch (c) {
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case '\n':
        return makeToken(TOKEN_NEWLINE);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(TOKEN_DOT);
    case '-':
        return makeToken(TOKEN_MINUS);
    case '+':
        return makeToken(TOKEN_PLUS);
    case '*':
        return makeToken(TOKEN_STAR);
    case '/':
        return makeToken(TOKEN_SLASH);
    case '!':
        return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '&':
        return makeToken(match('&') ? TOKEN_AND : TOKEN_BITAND);
    case '|':
        return makeToken(match('|') ? TOKEN_OR : TOKEN_BITOR);
    case '"':
        return string();
    case '`':
        return multilineString();
    }
    return errorToken("Unexpected character");
}

static Token skipWhiteSpace() {
    Token res = emptyToken();
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
                if (peek() == '\n')
                    advance();
            } else if (peekNext() == '*') {
                advance();
                advance();
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

static Token blockComment() {
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

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek()))
        advance();
    return makeToken(identifierType());
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

static Token multilineString() {
    while (peek() != '`' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
        }
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string");

    advance();
    return makeToken(TOKEN_STRING);
}

static Token string() {
    bool containNewLine = false;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
            containNewLine = true;
        }
        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string");
    if (containNewLine)
        return errorToken("Newline character '\\n' in string");

    advance();
    return makeToken(TOKEN_STRING);
}

static Token number() {
    while (isDigit(peek())) {
        advance();
    }

    if (peek() == '.' && isDigit(peekNext())) {
        advance();

        while (isDigit(peek())) {
            advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
}

static Token errorToken(const char *msg) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = msg;
    token.length = strlen(msg);
    token.line = scanner.line;
    return token;
}

static Token emptyToken() {
    return EMPTY_TOKEN;
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static char advance() {
    scanner.current++;
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