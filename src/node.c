#include "node.h"
#include "memory.h"

static void initNode(Node *node, NodeType type, Token *token);

Node *newNode(NodeType type, Token *token) {
    Node *node = ALLOCATE(Node, 1);
    initNode(node, type, token);
    return node;
}

static void initNode(Node *node, NodeType type, Token *token) {
    node->type = type;
    node->token = token;
}