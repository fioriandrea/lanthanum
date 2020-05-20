#ifndef memory_h
#define memory_h

#include "../standardtypes.h"

#define compute_capacity(oldcap) \
    ((oldcap) < 8 ? 8 : (oldcap) * 2)

#define grow_array(type, array, oldcap, newcap) \
    ((type*) reallocate(array, (oldcap) * sizeof(type), (newcap) * sizeof(type)))

#define free_array(type, array, oldcap) \
    reallocate(array, (oldcap) * sizeof(type), 0)

void* reallocate(void* pointer, size_t oldsize, size_t newsize);

#endif
