#ifndef dojo_memory_h
#define dojo_memory_h
#include "common.h"
#include "parser.h"
#include <stdlib.h>

typedef struct {
    Obj **grayStack;
    int count;
    int capacity;
    size_t allocated;
    size_t nextGC;
} GC;

#define GC_ALLOCATE(type, count)                                               \
    (type *)gcReallocate(NULL, 0, sizeof(type) * count);
#define ALLOCATE(type, count) (type *)reallocate(NULL, 0, sizeof(type) * count)

#define GC_FREE(type, pointer) gcReallocate(pointer, sizeof(type), 0)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define IS_EXCEEDING_CAPACITY(count, capacity) ((capacity) < (count + 1))

#define GROW_CAPACITY(capacity) (capacity < 8 ? 8 : capacity * 2)

#define GROW_ARRAY(type, arr, oldCount, newCount)                              \
    (type *)gcReallocate(arr, sizeof(type) * oldCount, sizeof(type) * newCount)

#define FREE_ARRAY(type, arr, oldCount)                                        \
    (type *)gcReallocate(arr, sizeof(type) * oldCount, 0)

void *gcReallocate(void *ptr, size_t oldSize, size_t newSize);
void *reallocate(void *ptr, size_t oldSize, size_t newSize);

void initGC();
void terminateGC();

void markValue(Value val);
void markObj(Obj *obj);
#endif