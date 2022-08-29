#ifndef dojo_node_h
#define dojo_node_h

#include "object.h"
#include "scanner.h"
#include "value.h"

typedef enum {
    ND_CLASS_DECL,
    ND_SUPER,
    ND_METHOD,
    ND_FN_DECL,
    ND_PARAM,
    ND_VAR_DECL,
    ND_FOR,
    ND_WHILE,
    ND_BREAK,
    ND_CONTINUE,
    ND_IF,
    ND_BLOCK,
    ND_EXPRESSION,
    ND_RETURN,

    ND_CALL,
    ND_ASSIGNMENT,
    ND_TERNARY,
    ND_BINARY,
    ND_AND,
    ND_OR,
    ND_UNARY,
    ND_PROPERTY,
    ND_THIS,
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
    int count;
    struct Node *next;
    struct Node *lhs;
    struct Node *rhs;
    struct Node *operand;
    struct Node *thenBranch;
    struct Node *elseBranch;
    struct Node *increment;
    struct Node *init;
} Node;

Node *newNode(NodeType type, Token *token);

#define NEW_CLASS_DECL(name, methods, heritage)                                \
    newClassDeclarationNode(name, methods, heritage)

static inline Node *newClassDeclarationNode(Token *name, Node *methods,
                                            Node *heritage) {
    Node *node = newNode(ND_CLASS_DECL, name);
    node->thenBranch = methods;
    node->operand = heritage;
    return node;
}

#define NEW_SUPER(token) newNode(ND_SUPER, token)

#define NEW_HERITAGE(token) newNode(ND_LITERAL, token)

#define NEW_METHOD(token, params, body) newMethodNode(token, params, body)

static inline Node *newMethodNode(Token *token, Node *params, Node *body) {
    Node *node = newNode(ND_METHOD, token);
    node->operand = params;
    node->thenBranch = body;
    return node;
}

#define NEW_FN_DECL(token, params, body)                                       \
    newFnDeclarationNode(token, params, body)

static inline Node *newFnDeclarationNode(Token *token, Node *params,
                                         Node *body) {
    Node *node = newNode(ND_FN_DECL, token);
    node->operand = params;
    node->thenBranch = body;
    return node;
}

#define NEW_PARAM(token) newParamNode(token)

static inline Node *newParamNode(Token *token) {
    Node *node = newNode(ND_PARAM, token);
    node->next = NULL;
    return node;
}

#define NEW_VAR_DECL(token, initializer)                                       \
    newVarDeclarationNode(token, initializer)

static inline Node *newVarDeclarationNode(Token *token, Node *initializer) {
    Node *node = newNode(ND_VAR_DECL, token);
    node->operand = initializer;
    return node;
}

#define NEW_FOR_STMT(token, init, condition, increment, body)                  \
    newForNode(token, init, condition, increment, body)

static inline Node *newForNode(Token *token, Node *init, Node *condition,
                               Node *increment, Node *body) {
    Node *node = newNode(ND_FOR, token);
    node->init = init;
    node->operand = condition;
    node->increment = increment;
    node->thenBranch = body;
    return node;
}

#define NEW_WHILE_STMT(token, condition, body)                                 \
    newWhileNode(token, condition, body)

static inline Node *newWhileNode(Token *token, Node *condition, Node *body) {
    Node *node = newNode(ND_WHILE, token);
    node->operand = condition;
    node->thenBranch = body;
    return node;
}

#define NEW_CONTINUE_STMT(token) newNode(ND_CONTINUE, token);

#define NEW_BREAK_STMT(token) newNode(ND_BREAK, token);

#define NEW_IF_STMT(token, condition, thenBranch, elseBranch)                  \
    newIfNode(token, condition, thenBranch, elseBranch)

static inline Node *newIfNode(Token *token, Node *condition, Node *thenBranch,
                              Node *elseBranch) {
    Node *node = newNode(ND_IF, token);
    node->operand = condition;
    node->thenBranch = thenBranch;
    node->elseBranch = elseBranch;
    return node;
}

#define NEW_BLOCK_STMT(token, stmts) newBlockNode(token, stmts)

static inline Node *newBlockNode(Token *token, Node *stmts) {
    Node *node = newNode(ND_BLOCK, token);
    node->operand = stmts;
    return node;
}

#define NEW_RETURN_STMT(token, returnVal) newReturnNode(token, returnVal)

static inline Node *newReturnNode(Token *token, Node *returnVal) {
    Node *node = newNode(ND_RETURN, token);
    node->operand = returnVal;
    return node;
}

#define NEW_EXPRESS_STMT(token, express) newExpressionNode(token, express)

static inline Node *newExpressionNode(Token *token, Node *expression) {
    Node *node = newNode(ND_EXPRESSION, token);
    node->operand = expression;
    return node;
}

#define NEW_CALL(token, lhs, args) newCallNode(token, lhs, args)

static inline Node *newCallNode(Token *token, Node *lhs, Node *args) {
    Node *node = newNode(ND_CALL, token);
    node->lhs = lhs;
    node->operand = args;
    return node;
}

#define NEW_ASSIGNMENT(token, lhs, rhs) newAssignmentNode(token, lhs, rhs)

static inline Node *newAssignmentNode(Token *token, Node *lhs, Node *rhs) {
    Node *node = newNode(ND_ASSIGNMENT, token);
    node->lhs = lhs;
    node->rhs = rhs;
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

#define NEW_AND(token, lhs, rhs) newAndNode(token, lhs, rhs)

static inline Node *newAndNode(Token *token, Node *lhs, Node *rhs) {
    Node *node = newNode(ND_AND, token);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

#define NEW_OR(token, lhs, rhs) newOrNode(token, lhs, rhs)

static inline Node *newOrNode(Token *token, Node *lhs, Node *rhs) {
    Node *node = newNode(ND_OR, token);
    node->lhs = lhs;
    node->rhs = rhs;
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
    node->operand = NULL;
    node->count = 0;
    return node;
}

#define NEW_PROPERTY(token, lhs) newPropertyNode(token, lhs)

static inline Node *newPropertyNode(Token *token, Node *lhs) {
    Node *node = newNode(ND_PROPERTY, token);
    node->lhs = lhs;
    return node;
}

#define NEW_THIS(token) newNode(ND_THIS, token)

#define NEW_VAR(token) newNode(ND_VAR, token)

#define NEW_TEMPLATE_SPAN(token, expression)                                   \
    newTemplateSpanNode(token, expression)

static inline Node *newTemplateSpanNode(Token *token, Node *expression) {
    Node *node = newNode(ND_TEMPLATE_SPAN, token);
    node->operand = expression;
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