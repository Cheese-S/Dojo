#include "object.h"
#include "hashmap.h"
#include "memory.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>

#define ALLOCATE_OBJ(type, objectType)                                         \
    (type *)allocateObj(sizeof(type), objectType)

static Obj *allocateObj(size_t size, ObjType type);
static char *allocateNewCStr(const char *str, int len);
static ObjString *getInternedString(const char *str, int len);
static ObjString *internString(const char *str, int len);
static void initObjString(ObjString *objstr, const char *str, int len);

static Obj *allocateObj(size_t size, ObjType type) {
    Obj *object = gcReallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm.objs;
    vm.objs = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif
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

void freeObj(Obj *obj) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void *)obj, obj->type);
#endif
    switch (obj->type) {
    case OBJ_BOUND_METHOD: {
        GC_FREE(ObjBoundMethod, obj);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *instance = (ObjInstance *)obj;
        freeMap(&instance->fields);
        GC_FREE(ObjInstance, obj);
        break;
    }
    case OBJ_CLASS: {
        ObjClass *djClass = (ObjClass *)obj;
        freeMap(&djClass->methods);
        GC_FREE(ObjClass, obj);
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure *closure = (ObjClosure *)obj;
        FREE_ARRAY(ObjUpvalue *, closure->upvalues, closure->upvalueCount);
        GC_FREE(ObjClosure, obj);
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
    case OBJ_BOUND_METHOD: {
        ObjBoundMethod *bMethod = AS_BOUND_METHOD(obj);
        fprintf(f, "<bound method %.*s>", bMethod->method->fn->name->length,
                bMethod->method->fn->name->str);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *instance = AS_INSTANCE(obj);
        fprintf(f, "<%.*s instance>", instance->djClass->name->length,
                instance->djClass->name->str);
        break;
    }
    case OBJ_CLASS: {
        ObjClass *djClass = AS_CLASS(obj);
        fprintf(f, "<class %.*s>", djClass->name->length, djClass->name->str);
        break;
    }
    case OBJ_CLOSURE: {
        ObjFn *fn = AS_CLOSURE(obj)->fn;
        if (fn->name) {
            fprintf(f, "<fn %.*s>", fn->name->length, fn->name->str);
        } else {
            fprintf(f, "<script>");
        }
        return;
    }
    case OBJ_FN: {
        ObjFn *fn = AS_FN(obj);
        if (fn->name) {
            fprintf(f, "<fn %.*s>", fn->name->length, fn->name->str);
        } else {
            fprintf(f, "<script>");
        }
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

ObjBoundMethod *newObjBoundMethod(Value receiver, ObjClosure *method) {
    ObjBoundMethod *bMethod = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bMethod->receiver = receiver;
    bMethod->method = method;
    return bMethod;
}

ObjInstance *newObjInstance(ObjClass *djClass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->djClass = djClass;
    initMap(&instance->fields);
    return instance;
}

ObjClass *newObjClass(ObjString *name) {
    ObjClass *djClass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    djClass->name = name;
    initMap(&djClass->methods);
    return djClass;
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

ObjNativeFn *newObjNativeFn(NativeFn fn, int arity) {
    ObjNativeFn *native = ALLOCATE_OBJ(ObjNativeFn, OBJ_NATIVE_FN);
    native->fn = fn;
    native->arity = arity;
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
    push(OBJ_VAL(objstr));
    mapPut(&vm.stringLiterals, objstr, NIL_VAL);
    pop();
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
