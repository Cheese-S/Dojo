#include "hashmap.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_LOAD 0.7
#define TOMBSTONE ((void *)-1)
#define IS_EMPTY_ENTRY(entry) (entry->key == NULL)
#define IS_TOMBSTONE(entry) (entry->key == TOMBSTONE)

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static void rehash(Hashmap *map, int newCapacity);
static void cleanEntries(Entry *entries, int capacity);
static void refillEntries(Hashmap *map, Entry *newEntries, int newCapacity);
static Entry *findEntry(Entry *entries, ObjString *key, int capacity);

static inline bool isInvalidEntry(Entry *entry);
static uint32_t calcEntryIndex(int capacity, uint32_t hash);

void initMap(Hashmap *map) {
    map->count = 0;
    map->capacity = 0;
    map->entries = NULL;
}

void freeMap(Hashmap *map) {
    FREE_ARRAY(Entry, map->entries, map->capacity);
}

bool mapGet(Hashmap *map, ObjString *key, Value *receiver) {
    if (map->count == 0)
        return false;

    Entry *entry = findEntry(map->entries, key, map->capacity);
    if (isInvalidEntry(entry)) {
        return false;
    }

    *receiver = entry->value;
    return true;
}

bool mapPut(Hashmap *map, ObjString *key, Value value) {
    if (IS_EXCEEDING_CAPACITY(map->count, map->capacity * MAX_LOAD)) {
        int capacity = GROW_CAPACITY(map->capacity);
        rehash(map, capacity);
    }

    Entry *entry = findEntry(map->entries, key, map->capacity);
    bool isNewKey = IS_EMPTY_ENTRY(entry);
    if (isNewKey) {
        map->count++;
    }
    entry->key = key;
    entry->value = value;

    return isNewKey;
}

ObjString *mapFindString(Hashmap *map, const char *str, int len,
                         uint32_t hash) {
    if (map->count == 0) {
        return NULL;
    }
    uint32_t index = calcEntryIndex(map->capacity, hash);
    for (;;) {
        Entry *entry = &map->entries[index];

        if (IS_EMPTY_ENTRY(entry)) {
            return NULL;
        }

        if (!IS_TOMBSTONE(entry)) {
            if (entry->key->length == len &&
                memcmp(entry->key->str, str, len) == 0 &&
                hash == entry->key->hash) {
                return entry->key;
            }
        }

        index = calcEntryIndex(map->capacity, index + 1);
    }
}

bool mapDelete(Hashmap *map, ObjString *key) {
    if (map->count == 0)
        return false;

    Entry *entry = findEntry(map->entries, key, map->capacity);
    if (IS_EMPTY_ENTRY(entry))
        return false;

    entry->key = TOMBSTONE;
    return true;
}

static void rehash(Hashmap *map, int newCapacity) {
    Entry *entries = GC_ALLOCATE(Entry, newCapacity);
    cleanEntries(entries, newCapacity);
    refillEntries(map, entries, newCapacity);
    FREE_ARRAY(Entry, map->entries, map->capacity);
    map->entries = entries;
    map->capacity = newCapacity;
}

static void cleanEntries(Entry *entries, int capacity) {
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
    }
}

static void refillEntries(Hashmap *map, Entry *newEntries, int newCapacity) {
    map->count = 0;
    for (int i = 0; i < map->capacity; i++) {
        Entry *entry = &map->entries[i];
        if (isInvalidEntry(entry))
            continue;

        Entry *dest = findEntry(newEntries, entry->key, newCapacity);
        dest->key = entry->key;
        dest->value = entry->value;
        map->count++;
    }
}

static Entry *findEntry(Entry *entries, ObjString *key, int capacity) {
    uint32_t index = calcEntryIndex(capacity, key->hash);
    Entry *foundTombstone = NULL;
    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == key) {
            return entry;
        }
        if (IS_EMPTY_ENTRY(entry)) {
            return foundTombstone == NULL ? entry : foundTombstone;
        }
        if (IS_TOMBSTONE(entry) && foundTombstone == NULL) {
            foundTombstone = entry;
        }
        index = calcEntryIndex(capacity, index + 1);
    }
}

static inline bool isInvalidEntry(Entry *entry) {
    return IS_EMPTY_ENTRY(entry) || IS_TOMBSTONE(entry);
}

static uint32_t calcEntryIndex(int capacity, uint32_t hash) {
    return hash & (capacity - 1);
}

uint32_t hash(const char *s, int len) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < len; i++) {
        hash ^= (uint8_t)s[i];
        hash *= 16777619;
    }
    return hash;
}

void hashmap_test(void) {
    Hashmap *testmap = (Hashmap *)malloc(sizeof(Hashmap));
    ObjString strs[1000];
    bool deleted[1000];
    int deletedNum = 0;
    int actualDeletedNum = 0;
    initMap(testmap);
    for (uint64_t i = 0; i < 1000; i++) {
        ObjString *str = &strs[i];
        str->hash = hash("test " STR(i), 5 + i / 10);
        mapPut(testmap, str, i);
    }
    printf("count: %d\n", testmap->count);

    for (uint64_t i = 0; i < 1000; i++) {
        Value res = -1;
        mapGet(testmap, &strs[i], &res);
        if (res != i) {
            printf("Expected: %ld, Recieved: %ld\n", i, res);
        }
    }

    for (int i = 0; i < 1000; i++) {
        deleted[i] = false;
    }

    for (uint64_t i = 0; i < 500; i++) {
        int index = rand() % (1001);
        mapDelete(testmap, &strs[index]);
        if (!deleted[index]) {
            deletedNum++;
            deleted[index] = true;
        }
    }

    for (uint64_t i = 0; i < 1000; i++) {
        Value res = -1;
        if (!mapGet(testmap, &strs[i], &res)) {
            actualDeletedNum++;
        };
    }

    if (deletedNum != actualDeletedNum) {
        printf("Expected to delete %d entries, actually deleted %d\n",
               deletedNum, actualDeletedNum);
    }

    for (int i = 0; i < 1000; i++) {
        if (deleted[i]) {
            mapPut(testmap, &strs[i], i);
        }
    }

    for (uint64_t i = 0; i < 1000; i++) {
        Value res = -1;
        mapGet(testmap, &strs[i], &res);
        if (res != i) {
            printf("Expected: %ld, Recieved: %ld\n", i, res);
        }
    }

    printf("count: %d\n", testmap->count);
}