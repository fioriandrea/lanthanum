#ifndef hash_map_h
#define hash_map_h

#include "value.h"
#include "../commontypes.h"

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
int mapPut(Collector* collector, HashMap* map, Value key, Value value);
int mapGet(HashMap* map, Value key, Value* result);
int mapRemove(Collector* collector, HashMap* map, Value key);
int containsDeepEqual(HashMap* map, Value key);
void freeMap(Collector* collector, HashMap* map);

#endif
