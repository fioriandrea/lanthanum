#include "memory.h"

void* reallocate(Collector* collector, void* pointer, size_t oldsize, size_t newsize) {
    if (newsize == 0) {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, newsize);
}

void initCollector(Collector* collector, HashMap* map) {
    collector->allocated = 0;
    collector->objects = NULL;
    collector->interned = map;
}

void freeCollector(Collector* collector) {
    while (collector->objects != NULL) {
        Obj* next = collector->objects->next;
        freeObject(NULL, collector->objects);
        collector->objects = next;
    }
    freeMap(NULL, collector->interned);
}
