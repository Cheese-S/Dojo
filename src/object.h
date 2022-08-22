#ifndef dojo_object_h
#define dojo_object_h

#include "chunk.h"
#include "common.h"
#include "value.h"

typedef enum {
    OBJ_STRING,
    OBJ_NATIVE_FN,
    OBJ_FN,
    OBJ_CLOSURE,
    OBJ_UPVALUE
} ObjType;

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
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
} ObjFn;

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFn *fn;
    ObjUpvalue **upvalues;
    int upvalueCount; // Needed for GC since fn can be freed first.
} ObjClosure;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    int arity;
    NativeFn fn;
} ObjNativeFn;

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_FN(value) ((ObjFn *)AS_OBJ(value))
#define AS_NATIVE_FN(value) ((ObjNativeFn *)AS_OBJ(value))
#define AS_UPVALUE(value) ((ObjUpvalue *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->str)

#define IS_FN(obj) isObjType(obj, OBJ_FN)
#define IS_CLOUSRE(obj) isObjType(obj, OBJ_CLOSURE)
#define IS_UPVALUE(obj) isObjType(obj, OBJ_UPVALUE)
#define IS_STRING(obj) isObjType(obj, OBJ_STRING)

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObjToFile(FILE *f, Value obj);
void freeObjs(Obj *head);

ObjClosure *newObjClosure(ObjFn *fn);
ObjFn *newObjFn();
ObjNativeFn *newObjNativeFn(NativeFn fn);
ObjUpvalue *newObjUpvalue(Value *slot);
bool isObjStrEqual(ObjString *a, ObjString *b);
Value newObjStringInVal(const char *str, int len);
ObjString *newObjString(const char *str, int len);
void markUsingHeap(ObjString *str);
#endif