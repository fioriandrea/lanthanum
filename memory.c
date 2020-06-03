#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "./debug/debug_switches.h"

static void collectGarbage(struct sCollector* collector) {
#ifdef TRACE_GC
    printf("START GC\n");
#endif

    // mark stack
    for (Value* stackValue = collector->vm->stack; stackValue < collector->vm->sp; stackValue++) {
        markValue(*stackValue);
    }

    // mark globals

    markMap(&collector->vm->globals);

    // todo: mark frames (mark main script function)
    
    // mark open upvalues

    for (ObjUpvalue* upvalue = collector->vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*) upvalue);
    }

#ifdef TRACE_GC
    printf("END GC\n");
#endif 
}

void* reallocate(Collector* collector, void* pointer, size_t oldsize, size_t newsize) {
    if (collector != NULL && collector->vm != NULL && oldsize < newsize) {
#ifdef STRESS_GC
        collectGarbage(collector);
#endif
    }
    if (newsize == 0) {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, newsize);
}

void initCollector(Collector* collector) {
    collector->allocated = 0;
    collector->objects = NULL;
    collector->vm = NULL;
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
