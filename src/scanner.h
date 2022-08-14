#ifndef dojo_scanner_h
#define dojo_scanner_h

typedef enum {
    // Single Char
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_NEWLINE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SLASH,
    TOKEN_STAR,
    // One or two character token
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_PRE_TEMPLATE,
    TOKEN_TWEEN_TEMPLATE,
    TOKEN_AFTER_TEMPLATE,
    // Keywords
    TOKEN_VAR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_PRINT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_THIS,
    TOKEN_CLASS,
    TOKEN_SUPER,
    TOKEN_FN,
    TOKEN_RETURN,
    // Special Tokens
    TOKEN_EMPTY,
    TOKEN_ERROR,
    TOKEN_EOF,
} TokenType;

typedef struct Token {
    TokenType type;
    const char *start;
    int length;
    int line;
    struct Token *next;
} Token;

void initScanner(const char *source);
void terminateScanner();
Token *nextToken();

#endif