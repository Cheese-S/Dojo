#include "value.h"
#include "memory.h"
#include <stdbool.h>
#include <stdio.h>

static void growValueArray(ValueArray *arr);

void printValue(Value value) {

    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NUMBER(value)) {
        printf("%g", valueToNum(value));
    } else if (IS_NIL(value)) {
        printf("nil");
    }
}

void initValueArray(ValueArray *arr) {
    arr->capacity = 0;
    arr->count = 0;
    arr->values = NULL;
}

int writeValueArray(ValueArray *arr, Value value) {
    if (IS_EXCEEDING_CAPACITY(arr->count, arr->capacity)) {
        growValueArray(arr);
    }
    arr->values[arr->count] = value;
    return arr->count++;
}

static void growValueArray(ValueArray *arr) {
    int oldCapacity = arr->capacity;
    arr->capacity = GROW_CAPACITY(oldCapacity);
    arr->values = GROW_ARRAY(Value, arr->values, arr->capacity, arr->capacity);
}

void freeValueArray(ValueArray *arr) {
    FREE_ARRAY(Value, arr->values, arr->capacity);
}