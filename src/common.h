#ifndef dojo_common_h
#define dojo_common_h

#include "value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UINT8_COUNT (UINT8_MAX + 1)

typedef struct Node {
    int line;
    Value value;
} Node;

#endif