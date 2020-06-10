#include "line_array.h"
#include "../memory.h"

void initLineArray(LineArray* linearr) {
    linearr->count = 0;
    linearr->capacity = 0;
    linearr->lines = NULL;
}

int writeLineArray(Collector* collector, LineArray* linearr, int line) {
    if (linearr->count + 1 >= linearr->capacity) {
        int newcap = compute_capacity(linearr->capacity);
        linearr->lines = grow_array(collector, LineData, linearr->lines, linearr->capacity, newcap);
        linearr->capacity = newcap; 
    }
    if (linearr->count == 0 || linearr->lines[linearr->count - 1].line != line) {
        linearr->lines[linearr->count].line = line;
        linearr->lines[linearr->count].count = 1;
        linearr->count++;
    } else {
        linearr->lines[linearr->count - 1].count++; 
    }
    return linearr->count - 1;
}

void freeLineArray(Collector* collector, LineArray* linearr) {
    free_array(collector, int, linearr->lines, linearr->capacity);
    initLineArray(linearr);
}

int lineArrayGet(LineArray* linearr, int index) {
    int accumulator = 0;
    for (int i = 0; i < linearr->count; i++) {
        LineData datum = linearr->lines[i];
        accumulator += datum.count;
        if (accumulator > index)
            return datum.line;
    }
}
