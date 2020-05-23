#ifndef hash_map_h
#define hash_map_h

#include "value.h"

struct sEntry {
    Value key;
    Value value;
    struct sEntry* next;
};

typedef struct sEntry Entry;

typedef struct {
    Entry** entries;
    int capacity;
    int count;
} HashMap;

void initMap(HashMap* map);
int mapPut(HashMap* map, Value key, Value value);
int mapGet(HashMap* map, Value key, Value* result);
int mapRemove(HashMap* map, Value key);
void freeMap(HashMap* map);

#endif
