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

struct sHashMap {
    Entry** entries;
    int capacity;
    int count;
};

void initMap(struct sHashMap* map);
int mapPut(Collector* collector, struct sHashMap* map, Value key, Value value);
int mapGet(struct sHashMap* map, Value key, Value* result);
int mapRemove(Collector* collector, struct sHashMap* map, Value key);
ObjString* containsStringDeepEqual(struct sHashMap* map, char* chars, int length);
void freeMap(Collector* collector, struct sHashMap* map);
void markMap(Collector* collector, struct sHashMap* map);
void removeUnmarkedKeys(Collector* collector, struct sHashMap* map);

#endif
