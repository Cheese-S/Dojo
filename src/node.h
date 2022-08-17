#ifndef dojo_node_h
#define dojo_node_h

#include "object.h"
#include "scanner.h"
#include "value.h"

typedef enum {
    ND_VAR_DECL,
    ND_PRINT,
    ND_EXPRESSION,

    ND_ASSIGNMENT,
    ND_TERNARY,
    ND_BINARY,
    ND_UNARY,
    ND_VAR,
    ND_TEMPLATE_HEAD,
    ND_TEMPLATE_SPAN,
    ND_STRING,
    ND_NUMBER,
    ND_LITERAL,

    ND_EMPTY
} NodeType;

// TODO: use union rather than a bunch of pointers
typedef struct Node {
    NodeType type;
    Token *token;
    struct Node *nextStmt;
    struct Node *lhs;
    struct Node *rhs;
    struct Node *operand;
    struct Node *span;
    struct Node *thenBranch;
    struct Node *elseBranch;
} Node;

Node *newNode(NodeType type, Token *token);

#define NEW_ASSIGNMENT(token, lhs, rhs) newAssignmentNode(token, lhs, rhs)

static inline Node *newAssignmentNode(Token *token, Node *lhs, Node *rhs) {
    Node *node = newNode(ND_ASSIGNMENT, token);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

#define NEW_VAR_DECL(token, initializer)                                       \
    newVarDeclarationNode(token, initializer)

static inline Node *newVarDeclarationNode(Token *token, Node *initializer) {
    Node *node = newNode(ND_VAR_DECL, token);
    node->operand = initializer;
    return node;
}

#define NEW_PRINT_STMT(token, expression) newPrintNode(token, expression)

static inline Node *newPrintNode(Token *token, Node *express) {
    Node *node = newNode(ND_PRINT, token);
    node->operand = express;
    return node;
}

#define NEW_EXPRESS_STMT(expression, line) newExpressionNode(expression, line)

static inline Node *newExpressionNode(Token *token, Node *expression) {
    Node *node = newNode(ND_EXPRESSION, token);
    node->operand = expression;
    return node;
}

#define NEW_TERNARY(token, condition, thenBranch, elseBranch)                  \
    newTernaryNode(token, condition, thenBranch, elseBranch)

static inline Node *newTernaryNode(Token *token, Node *condition,
                                   Node *thenBranch, Node *elseBranch) {
    Node *node = newNode(ND_TERNARY, token);
    node->operand = condition;
    node->thenBranch = thenBranch;
    node->elseBranch = elseBranch;
    return node;
}

#define NEW_BINARY(token, lhs, rhs) newBinaryNode(token, lhs, rhs)

static inline Node *newBinaryNode(Token *token, Node *lhs, Node *rhs) {
    Node *node = newNode(ND_BINARY, token);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

#define NEW_UNARY(token, operand) newUnaryNode(token, operand)

static inline Node *newUnaryNode(Token *token, Node *operand) {
    Node *node = newNode(ND_UNARY, token);
    node->operand = operand;
    return node;
}

#define NEW_TEMPLATE_HEAD(token) newTemplateHeadNode(token)

static inline Node *newTemplateHeadNode(Token *token) {
    Node *node = newNode(ND_TEMPLATE_HEAD, token);
    node->span = NULL;
    return node;
}

#define NEW_VAR(token) newVarNode(token)

static inline Node *newVarNode(Token *token) {
    Node *node = newNode(ND_VAR, token);
    return node;
}

#define NEW_TEMPLATE_SPAN(token, expression)                                   \
    newTemplateSpanNode(token, expression)

static inline Node *newTemplateSpanNode(Token *token, Node *expression) {
    Node *node = newNode(ND_TEMPLATE_SPAN, token);
    node->operand = expression;
    node->span = NULL;
    return node;
}

#define NEW_STRING(token) newStringNode(token)

static inline Node *newStringNode(Token *token) {
    return newNode(ND_STRING, token);
}

#define NEW_NUMBER(token) newNumberNode(token)

static inline Node *newNumberNode(Token *token) {
    return newNode(ND_NUMBER, token);
}

#define NEW_LITERAL(token) newLiteralNode(token)

static inline Node *newLiteralNode(Token *token) {
    return newNode(ND_LITERAL, token);
}

#endif