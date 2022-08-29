#include "memory.h"
#include "compiler.h"
#include "hashmap.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include <stdlib.h>

#ifdef DEBUG_LOG_GC
#include "debug.h"
#include <stdio.h>
#endif

#define GC_HEAP_GROW_FACTOR 2

static void appendToGrayStack(Obj *obj);
static void collectGarbage();
static void markRoots();
static void traceRefs();
static void sweep();
static void blackenObj();
static void markStack();
static void markFrameClosures();
static void markArray(ValueArray *array);

GC gc;

void *gcReallocate(void *ptr, size_t oldSize, size_t newSize) {
    gc.allocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
        if (gc.allocated > gc.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0) {
        free(ptr);
        return NULL;
    }

    void *result = realloc(ptr, newSize);
    if (result == NULL) {
        exit(1);
    }
    return result;
}

void *reallocate(void *ptr, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }

    void *result = realloc(ptr, newSize);
    if (result == NULL) {
        exit(1);
    }
    return result;
}

void initGC() {
    gc.grayStack = NULL;
    gc.capacity = 0;
    gc.count = 0;
    gc.allocated = 0;
    gc.nextGC = 1024 * 1 - 24;
}

void terminateGC() {
    free(gc.grayStack);
}

static void appendToGrayStack(Obj *obj) {
    if (IS_EXCEEDING_CAPACITY(gc.count, gc.capacity)) {
        int newCapacity = GROW_CAPACITY(gc.capacity);
        gc.grayStack = reallocate(gc.grayStack, sizeof(Obj *) * gc.capacity,
                                  sizeof(Obj *) * newCapacity);
        gc.capacity = newCapacity;
    }
    gc.grayStack[gc.count++] = obj;
}

static void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = gc.allocated;
#endif
    markRoots();
    traceRefs();
    mapRemoveWhite(&vm.stringLiterals);
    sweep();

    gc.nextGC = gc.allocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
           before - gc.allocated, before, gc.allocated, gc.nextGC);
    printf("-- gc end\n");
#endif
}

static void markRoots() {
    markStack();
    markFrameClosures();
    markMap(&vm.globals);
    markCompilerRoots();
    markObj((Obj *)vm.initString);
}

static void traceRefs() {
    while (gc.count) {
        Obj *obj = gc.grayStack[--gc.count];
        blackenObj(obj);
    }
}

static void sweep() {
    Obj *prev = NULL;
    Obj *obj = vm.objs;
    while (obj) {
        if (obj->isMarked) {
            obj->isMarked = false;
            prev = obj;
            obj = obj->next;
        } else {
            Obj *unreached = obj;
            obj = obj->next;
            if (prev) {
                prev->next = obj;
            } else {
                vm.objs = obj;
            }
            freeObj(unreached);
        }
    }
}

static void blackenObj(Obj *obj) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)obj);
    printValue(OBJ_VAL(obj));
    printf("\n");
#endif
    switch (obj->type) {
    case OBJ_BOUND_METHOD: {
        ObjBoundMethod *bMethod = ((ObjBoundMethod *)obj);
        markValue(bMethod->receiver);
        markObj((Obj *)bMethod->method);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *instance = ((ObjInstance *)obj);
        markObj((Obj *)instance->djClass);
        markMap(&instance->fields);
        break;
    }
    case OBJ_CLASS: {
        ObjClass *djClass = ((ObjClass *)obj);
        markObj((Obj *)djClass->name);
        markMap(&djClass->methods);
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure *closure = ((ObjClosure *)obj);
        markObj((Obj *)closure->fn);
        for (int i = 0; i < closure->upvalueCount; i++) {
            markObj((Obj *)closure->upvalues[i]);
        }
        break;
    }
    case OBJ_FN: {
        ObjFn *fn = ((ObjFn *)obj);
        markObj((Obj *)fn->name);
        markArray(&fn->chunk.constants);
        break;
    }

    case OBJ_UPVALUE:
        markValue(((ObjUpvalue *)obj)->closed);
        break;
    case OBJ_NATIVE_FN:
    case OBJ_STRING:
        break;
    }
}

static void markStack() {
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }
}

static void markFrameClosures() {
    for (int i = 0; i < vm.frameCount; i++) {
        markObj((Obj *)vm.frames[i].closure);
    }
}

static void markUpvalues() {
    for (ObjUpvalue *upval = vm.openUpvalues; upval; upval = upval->next) {
        markObj((Obj *)upval);
    }
}

static void markArray(ValueArray *array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

void markValue(Value val) {
    if (IS_OBJ(val)) {
        markObj(AS_OBJ(val));
    }
}

void markObj(Obj *obj) {
    if (obj && !obj->isMarked) {
#ifdef DEBUG_LOG_GC
        printf("%p mark ", (void *)obj);
        printValue(OBJ_VAL(obj));
        printf("\n");
#endif
        obj->isMarked = true;
        appendToGrayStack(obj);
    }
}