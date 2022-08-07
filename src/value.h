#include <stdint.h>
#include <string.h>

#ifndef dojo_value_h
#define dojo_value_h

typedef uint64_t Value;

#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1
#define TAG_TRUE 2
#define TAG_FALSE 3

#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define NUMBER_VAL(num) numToValue(num)

static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_BOOL(value) (((value) | 1) == FALSE_VAL)
#define IS_NUMBER(value) (((value)&QNAN) != QNAN)

#define AS_BOOL(value) ((value) == TRUE_VAL)

#define AS_NUMBER(value) valueToNum(value)

static inline double valueToNum(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *arr);
int writeValueArray(ValueArray *arr, Value value);
void freeValueArray(ValueArray *arr);
void printValue(Value val);

#endif