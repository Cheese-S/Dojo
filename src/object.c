#include "object.h"
#include "hashmap.h"
#include "memory.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType)                                         \
    (type *)allocateObj(sizeof(type), objectType)

static Obj *allocateObj(size_t size, ObjType type);
static ObjString *getInternedString(const char *str, int len);
static ObjString *internString(const char *str, int len);
static void initObjString(ObjString *objstr, const char *str, int len);

static Obj *allocateObj(size_t size, ObjType type) {
    Obj *object = gcReallocate(NULL, 0, size);
    object->type = type;
    return object;
}

void markUsingHeap(ObjString *str) {
    str->isUsingHeap = true;
}

Value newObjStringInVal(const char *str, int len) {
    return OBJ_VAL(newObjString(str, len));
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
