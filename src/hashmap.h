#ifndef dojo_hashmap_h
#define dojo_hashmap_h

#include "common.h"
#include "value.h"

typedef struct ObjString ObjString;

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct Hashmap {
    int count;
    int capacity;
    Entry *entries;
} Hashmap;

void initMap(Hashmap *map);
void freeMap(Hashmap *map);
void markMap(Hashmap *map);

bool mapPut(Hashmap *map, ObjString *key, Value value);
void mapPutAll(Hashmap *src, Hashmap *dest);
bool mapGet(Hashmap *map, ObjString *key, Value *receiver);
bool mapDelete(Hashmap *map, ObjString *key);
ObjString *mapFindString(Hashmap *map, const char *str, int len, uint32_t hash);
void mapRemoveWhite(Hashmap *map);

uint32_t hash(const char *s, int len);

void hashmap_test(void);

#endif