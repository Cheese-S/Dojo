#ifndef dojo_object_h
#define dojo_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

typedef enum { OBJ_STRING, OBJ_NATIVE_FN, OBJ_FN } ObjType;

typedef struct Obj {
    ObjType type;
    struct Obj *next;
} Obj;

typedef struct {
    Obj obj;
    int length;
    uint32_t hash;
    bool isUsingHeap;
    const char *str;
} ObjString;

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString *name;
} ObjFn;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    int arity;
    NativeFn fn;
} ObjNativeFn;

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define AS_NATIVE_FN(value) ((ObjNativeFn *)AS_OBJ(value))
#define AS_FN(value) ((ObjFn *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->str)

#define IS_STRING(obj) isObjType(obj, OBJ_STRING)
#define IS_FN(obj) isObjType(obj, OBJ_FN)

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObjToFile(FILE *f, Value obj);
void freeObjs(Obj *head);

ObjNativeFn *newObjNativeFn(NativeFn fn);
ObjFn *newObjFn();
bool isObjStrEqual(ObjString *a, ObjString *b);
Value newObjStringInVal(const char *str, int len);
ObjString *newObjString(const char *str, int len);
void markUsingHeap(ObjString *str);
#endif