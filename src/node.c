#include "node.h"
#include "memory.h"

static void initNode(Node *node, NodeType type, int line);

Node *newNode(NodeType type, int line) {
    Node *node = ALLOCATE(Node, 1);
    initNode(node, type, line);
    return node;
}

static void initNode(Node *node, NodeType type, int line) {
    node->line = line;
    node->type = type;
    node->nextStmt = NULL;
}