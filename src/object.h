#ifndef dojo_object_h
#define dojo_object_h

#include "common.h"
#include "value.h"

typedef enum { OBJ_STRING } ObjType;

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

#define AS_STRING(obj) ((ObjString *)AS_OBJ(obj))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->str)

#define IS_STRING(obj) isObjType(obj, OBJ_STRING)

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void freeObjs(Obj *head);
bool isObjStrEqual(ObjString *a, ObjString *b);
Value newObjStringInVal(const char *str, int len);
ObjString *newObjString(const char *str, int len);
void markUsingHeap(ObjString *str);
#endif