#ifndef dojo_hashmap_h
#define dojo_hashmap_h

#include "common.h"
#include "object.h"

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Hashmap;

void initMap(Hashmap *map);
void freeMap(Hashmap *map);
bool mapPut(Hashmap *map, ObjString *key, Value value);
bool mapGet(Hashmap *map, ObjString *key, Value *receiver);
bool mapDelete(Hashmap *map, ObjString *key);
ObjString *mapFindString(Hashmap *map, const char *str, int len, uint32_t hash);

uint32_t hash(const char *s, int len);

void hashmap_test(void);

#endif