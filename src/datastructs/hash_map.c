#include <stdlib.h>

#include "hash_map.h"
#include "value_operations.h"
#include "../memory.h"
#include "../util.h"
#include "../debug/debug_switches.h"

#define LOAD_FACTOR 0.65 
#define get_index(hash, capacity) ((hash) & ((capacity) - 1))

static Entry* findEntry(Entry** entries, int capacity, Value key) {
    if (entries == NULL)
        return NULL;
    uint32_t hash = get_value_hash(key);
    Entry* head = entries[get_index(hash, capacity)];
    while (head != NULL) {
        if (valuesEqual(key, head->key)) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

static int entriesPut(Collector* collector, Entry** entries, int capacity, Value key, Value value) {
    Entry* head = findEntry(entries, capacity, key);
    if (head == NULL) {
        uint32_t hash = get_value_hash(key);
        int index = get_index(hash, capacity);
        Entry* newhead = allocate_pointer(collector, Entry, sizeof(Entry));
        newhead->key = key;
        newhead->value = value;
        newhead->next = entries[index];
        entries[index] = newhead;
        return 0;
    } else {
        head->value = value;
        return 1;
    }
}

static void growMap(Collector* collector, struct sHashMap* map) {
    int newcap = compute_capacity(map->capacity);
    Entry** newentries = allocate_block(collector, Entry*, newcap);
    for (int i = 0; i < newcap; i++) {
        newentries[i] = NULL;
    }
    for (int i = 0; i < map->capacity; i++) {
        Entry* head = map->entries[i];
        while (head != NULL) {
            entriesPut(collector, newentries, newcap, head->key, head->value);
            head = head->next;
        }
    }
    free_array(collector, Entry*, map->entries, map->capacity);
    map->entries = newentries;
    map->capacity = newcap;
}

void initMap(struct sHashMap* map) {
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

int mapPut(Collector* collector, struct sHashMap* map, Value key, Value value) {
    if (map->count + 1 > map->capacity * LOAD_FACTOR) {
        growMap(collector, map);
    }
    int result = entriesPut(collector, map->entries, map->capacity, key, value);
    if (!result)
        map->count++;
    return result;
}

int mapGet(struct sHashMap* map, Value key, Value* result) {
    Entry* entry = findEntry(map->entries, map->capacity, key);
    if (entry == NULL) {
        return 0;
    }
    *result = entry->value;
    return 1;
}

int mapRemove(Collector* collector, struct sHashMap* map, Value key) {
    if (map->count == 0)
        return 0;
    uint32_t hash = get_value_hash(key);
    int index = get_index(hash, map->capacity);
    Entry* dummy = (Entry*) malloc(sizeof(Entry));
    Entry* previous = dummy;
    Entry* current = map->entries[index];
    previous->next = current;

    while (current != NULL) {
        if (valuesEqual(current->key, key)) {
            previous->next = current->next;
            map->entries[index] = dummy->next;
            free(dummy);
            free_pointer(collector, current, sizeof(Entry));
            return 1;
        }
        previous = current;
        current = current->next;
    }

    free(dummy);
    return 0;
}

void freeMap(Collector* collector, struct sHashMap* map) {
    for (int i = 0; i < map->capacity; i++) {
        Entry* entry = map->entries[i];
        while (entry != NULL) {
            Entry* toFree = entry;
            entry = entry->next;
            free_pointer(collector, toFree, sizeof(Entry));
        }
    }
    free_array(collector, Entry*, map->entries, map->capacity);
    initMap(map);
}

ObjString* containsStringDeepEqual(struct sHashMap* map, char* chars, int length) {
    if (map->count == 0)
        return NULL;
    uint32_t hash = hash_string(chars, length);
    int index = get_index(hash, map->capacity);
    Entry* head = map->entries[index];
    while (head != NULL) {
        Value key = head->key;
        if (!is_string(key)) 
            continue;
        ObjString* objString = as_string(head->key);
        if (objString->length == length && memcmp(chars, objString->chars, length) == 0)
            return objString;
        head = head->next;
    }
    return NULL;
}

void markMap(Collector* collector, struct sHashMap* map) {
    for (int i = 0; i < map->capacity; i++) {
        Entry* entry = map->entries[i];
        if (entry != NULL) {
            // mark inner linked list
            while (entry != NULL) {
                // mark each entry in linked list
                markValue(collector, entry->key);
                markValue(collector, entry->value);
                entry = entry->next;
            }
        }
    }
}

void removeUnmarkedKeys(Collector* collector, struct sHashMap* map) {
    Entry* dummy = (Entry*) malloc(sizeof(Entry));
    for (int i = 0; i < map->capacity; i++) {
        dummy->next = map->entries[i];
        Entry* previous = dummy;
        Entry* current = map->entries[i];
        while (current != NULL) {
            if (is_obj(current->key) && !as_obj(current->key)->marked) {
                previous->next = current->next;
                free_pointer(collector, current, sizeof(Entry));
                current = previous->next;
            } else { 
                previous = previous->next;
                if (current != NULL)
                    current = current->next;
            }
        }
        map->entries[i] = dummy->next;
    }
    free(dummy);
}
