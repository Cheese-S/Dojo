#ifndef dojo_object_h
#define dojo_object_h

#include "chunk.h"
#include "common.h"
#include "hashmap.h"
#include "value.h"

typedef enum {
    OBJ_BOUND_METHOD,
    OBJ_INSTANCE,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FN,
    OBJ_NATIVE_FN,
    OBJ_UPVALUE,
    OBJ_STRING,
} ObjType;

typedef struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj *next;
} Obj;

typedef struct ObjString {
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

typedef struct {
    Obj obj;
    ObjString *name;
    Hashmap methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *djClass;
    Hashmap fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure *method;
} ObjBoundMethod;

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_FN(value) ((ObjFn *)AS_OBJ(value))
#define AS_NATIVE_FN(value) ((ObjNativeFn *)AS_OBJ(value))
#define AS_UPVALUE(value) ((ObjUpvalue *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->str)

#define IS_BOUND_METHOD(obj) isObjType(obj, OBJ_BOUND_METHOD)
#define IS_INSTANCE(obj) isObjType(obj, OBJ_INSTANCE)
#define IS_CLASS(obj) isObjType(obj, OBJ_CLASS)
#define IS_FN(obj) isObjType(obj, OBJ_FN)
#define IS_CLOUSRE(obj) isObjType(obj, OBJ_CLOSURE)
#define IS_UPVALUE(obj) isObjType(obj, OBJ_UPVALUE)
#define IS_STRING(obj) isObjType(obj, OBJ_STRING)

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void printObjToFile(FILE *f, Value obj);
void freeObjs(Obj *head);
void freeObj(Obj *obj);

ObjBoundMethod *newObjBoundMethod(Value receiver, ObjClosure *closure);
ObjInstance *newObjInstance(ObjClass *djClass);
ObjClass *newObjClass(ObjString *name);
ObjClosure *newObjClosure(ObjFn *fn);
ObjFn *newObjFn();
ObjNativeFn *newObjNativeFn(NativeFn fn, int arity);
ObjUpvalue *newObjUpvalue(Value *slot);

ObjString *newObjString(const char *str, int len);
Value newObjStringInVal(const char *str, int len);
bool isObjStrEqual(ObjString *a, ObjString *b);
void markUsingHeap(ObjString *str);
#endif
