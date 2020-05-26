#include "memory.h"

void* reallocate(Collector* collector, void* pointer, size_t oldsize, size_t newsize) {
    if (newsize == 0) {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, newsize);
}

void initCollector(Collector* collector) {
    collector->allocated = 0;
    collector->objects = NULL;
    initMap(&collector->interned);
}

void freeCollector(Collector* collector) {
    // todo: be sure that map is garbage collected
    freeMap(NULL, &collector->interned);
    while (collector->objects != NULL) {
        Obj* next = collector->objects->next;
        freeObject(NULL, collector->objects);
        collector->objects = next;
    }
}
