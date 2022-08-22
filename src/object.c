#include "object.h"
#include "hashmap.h"
#include "memory.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>

#define ALLOCATE_OBJ(type, objectType)                                         \
    (type *)allocateObj(sizeof(type), objectType)

static Obj *allocateObj(size_t size, ObjType type);
static void freeObj(Obj *obj);
static char *allocateNewCStr(const char *str, int len);
static ObjString *getInternedString(const char *str, int len);
static ObjString *internString(const char *str, int len);
static void initObjString(ObjString *objstr, const char *str, int len);

static Obj *allocateObj(size_t size, ObjType type) {
    Obj *object = gcReallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objs;
    vm.objs = object;
    return object;
}

void freeObjs(Obj *head) {
    Obj *current = head;
    while (current) {
        Obj *next = current->next;
        freeObj(current);
        current = next;
    }
}

static void freeObj(Obj *obj) {
    switch (obj->type) {
    case OBJ_CLOSURE: {
        ObjClosure *closure = (ObjClosure *)obj;
        GC_FREE(ObjClosure, obj);
        FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
        break;
    }
    case OBJ_FN: {
        ObjFn *fn = (ObjFn *)obj;
        freeChunk(&fn->chunk);
        GC_FREE(ObjFn, fn);
        break;
    }
    case OBJ_NATIVE_FN: {
        GC_FREE(ObjNativeFn, obj);
        break;
    }
    case OBJ_STRING: {
        ObjString *str = (ObjString *)obj;
        if (str->isUsingHeap) {
            FREE_ARRAY(char, (char *)str->str, str->length);
        }
        GC_FREE(ObjString, obj);
        break;
    }
    case OBJ_UPVALUE: {
        GC_FREE(ObjUpvalue, obj);
        break;
    }
    }
}

void printObjToFile(FILE *f, Value obj) {
    switch (OBJ_TYPE(obj)) {
    case OBJ_CLOSURE: {
        ObjFn *fn = AS_CLOSURE(obj)->fn;
        fprintf(f, "<fn %.*s>", fn->name->length, fn->name->str);
        return;
    }
    case OBJ_FN: {
        ObjFn *fn = AS_FN(obj);
        fprintf(f, "<fn %.*s>", fn->name->length, fn->name->str);
        return;
    }
    case OBJ_NATIVE_FN: {
        fprintf(f, "<native_fn>");
        return;
    }
    case OBJ_STRING: {
        ObjString *str = AS_STRING(obj);
        fprintf(f, "%.*s", str->length, str->str);
        return;
    }
    case OBJ_UPVALUE: {
        fprintf(f, "upvalue");
        return;
    }
    }
}

ObjClosure *newObjClosure(ObjFn *fn) {
    ObjUpvalue **values = GC_ALLOCATE(ObjUpvalue *, fn->upvalueCount);
    for (int i = 0; i < fn->upvalueCount; i++) {
        values[i] = NULL;
    }
    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->upvalues = values;
    closure->upvalueCount = fn->upvalueCount;
    closure->fn = fn;
    return closure;
}

ObjFn *newObjFn() {
    ObjFn *fn = ALLOCATE_OBJ(ObjFn, OBJ_FN);
    fn->arity = 0;
    fn->upvalueCount = 0;
    fn->name = NULL;
    initChunk(&fn->chunk);
    return fn;
}

ObjNativeFn *newObjNativeFn(NativeFn fn) {
    ObjNativeFn *native = ALLOCATE_OBJ(ObjNativeFn, OBJ_NATIVE_FN);
    native->fn = fn;
    return native;
}

ObjUpvalue *newObjUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

Value newObjStringInVal(const char *str, int len) {
    if (vm.isREPL) {
        str = allocateNewCStr(str, len);
    }
    return OBJ_VAL(newObjString(str, len));
}

static char *allocateNewCStr(const char *str, int len) {
    char *dest = GC_ALLOCATE(char, len);
    memcpy(dest, str, sizeof(char) * len);
    return dest;
}

ObjString *newObjString(const char *str, int len) {
    ObjString *interned = getInternedString(str, len);
    if (!interned) {
        interned = internString(str, len);
    }
    return interned;
}

static ObjString *getInternedString(const char *str, int len) {
    uint32_t strHash = hash(str, len);
    return mapFindString(&vm.stringLiterals, str, len, strHash);
}

static ObjString *internString(const char *str, int len) {
    ObjString *objstr = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    initObjString(objstr, str, len);
    mapPut(&vm.stringLiterals, objstr, NIL_VAL);
    return objstr;
}

static void initObjString(ObjString *objstr, const char *str, int len) {
    objstr->hash = hash(str, len);
    objstr->length = len;
    objstr->str = str;
    objstr->isUsingHeap = false;
}

void markUsingHeap(ObjString *str) {
    str->isUsingHeap = true;
}

bool isObjStrEqual(ObjString *a, ObjString *b) {
    return a->length == b->length && a->hash == b->hash &&
           memcmp(a->str, b->str, sizeof(char) * a->length);
}