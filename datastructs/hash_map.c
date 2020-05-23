#include <stdlib.h>

#include "hash_map.h"
#include "../services/memory.h"

#define LOAD_FACTOR 0.65

static Entry* findEntry(Entry** entries, int capacity, Value key) {
    uint32_t hash = get_value_hash(key);
    Entry* head = entries[hash % capacity];
    while (head != NULL) {
        if (valuesEqual(key, head->key)) {
            return head;
        }
    }
    return head;
}

static int entriesPut(Entry** entries, int capacity, Value key, Value value) {
    uint32_t hash = get_value_hash(key);
    int index = hash % capacity;
    Entry* head = findEntry(entries, capacity, key);
    if (head == NULL) {
        Entry* newhead = allocate_pointer(Entry, sizeof(Entry));
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

static void growMap(HashMap* map) {
    int newcap = compute_capacity(map->capacity);
    Entry** newentries = allocate_block(Entry*, newcap);
    for (int i = 0; i < newcap; i++) {
        newentries[i] = NULL;
    }
    for (int i = 0; i < map->capacity; i++) {
        entriesPut(newentries, newcap, map->entries[i]->key, map->entries[i]->value);
    }
    free_array(Entry*, map->entries, map->capacity);
    map->entries = newentries;
    map->capacity = newcap;
}

void initMap(HashMap* map) {
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

int mapPut(HashMap* map, Value key, Value value) {
    if (map->count + 1 > map->capacity * LOAD_FACTOR) {
        growMap(map);
    }
    return entriesPut(map->entries, map->capacity, key, value);
}

int mapGet(HashMap* map, Value key, Value* result) {
    Entry* entry = findEntry(map->entries, map->capacity, key);
    if (entry == NULL) {
        return 0;
    }
    *result = entry->value;
    return 1;
}

int mapRemove(HashMap* map, Value key) {
    uint32_t hash = get_value_hash(key);
    int index = hash % map->capacity;
    Entry* dummy = (Entry*) malloc(sizeof(Entry));
    Entry* previous = dummy;
    Entry* current = map->entries[index];
    previous->next = current;

    while (current != NULL) {
        if (valuesEqual(current->key, key)) {
            previous->next = current->next;
            if (current != dummy)
                free(dummy);
            free_pointer(current, sizeof(Entry));
            return 1;
        }
        previous = current;
        current = current->next;
    }

    free(dummy);
    return 0;
}

void freeMap(HashMap* map) {
    for (int i = 0; i < map->capacity; i++) {
        if (map->entries[i] != NULL) {
            free_pointer(map->entries[i], sizeof(Entry));
        }
    }
    free_array(Entry*, map->entries, map->capacity);
    initMap(map);
}
