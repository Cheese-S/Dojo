#ifndef dojo_node_h
#define dojo_node_h

#include "object.h"
#include "scanner.h"
#include "value.h"

typedef enum {
    ND_BINARY,
    ND_UNARY,
    ND_NUMBER,
    ND_STRING,
    ND_TEMPLATE_HEAD,
    ND_TEMPLATE_SPAN,
    ND_TRUE,
    ND_FALSE,
    ND_NIL
} NodeType;

typedef struct Node {
    int line;
    Value value;
    NodeType type;
    TokenType op;
    struct Node *lhs;
    struct Node *rhs;
    struct Node *operand;
    struct Node *span;
} Node;

Node *newNode(NodeType type, int line);

#define NEW_UNARY(op, line, operand) newUnaryNode(op, line, operand)

static inline Node *newUnaryNode(TokenType op, int line, Node *operand) {
    Node *node = newNode(ND_UNARY, line);
    node->op = op;
    node->operand = operand;
    return node;
}

#define NEW_BINARY(op, line, lhs, rhs) newBinaryNode(op, line, lhs, rhs)

static inline Node *newBinaryNode(TokenType op, int line, Node *lhs,
                                  Node *rhs) {
    Node *node = newNode(ND_BINARY, line);
    node->op = op;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

#define NEW_NUMBER(value, line) newNumberNode(value, line)

static inline Node *newNumberNode(Value value, int line) {
    Node *node = newNode(ND_NUMBER, line);
    node->value = value;
    return node;
}

#define NEW_STRING(str, line) newStringNode(str, line)

static inline Node *newStringNode(Value value, int line) {
    Node *node = newNode(ND_STRING, line);
    node->value = value;
    return node;
}

#define NEW_TEMPLATE_HEAD(literal, line) newTemplateHeadNode(literal, line)

static inline Node *newTemplateHeadNode(Value literal, int line) {
    Node *node = newNode(ND_TEMPLATE_HEAD, line);
    node->value = literal;
    node->line = line;
    node->span = NULL;
    return node;
}

#define NEW_TEMPLATE_SPAN(expression, literal, line)                           \
    newTemplateSpanNode(expression, literal, line)

static inline Node *newTemplateSpanNode(Node *expression, Value literal,
                                        int line) {
    Node *node = newNode(ND_TEMPLATE_SPAN, line);
    node->value = literal;
    node->line = line;
    node->operand = expression;
    node->span = NULL;
    return node;
}

#define NEW_LITERAL(type, line) newLiteralNode(type, line)

static inline Node *newLiteralNode(NodeType type, int line) {
    return newNode(type, line);
}

#endif