#ifndef line_array_h
#define line_array_h

typedef struct {
    int count;
    int line;
} LineData;

typedef struct {
    int count;
    int capacity;
    LineData* lines;
} LineArray;

void initLineArray(LineArray* linearr);
int writeLineArray(LineArray* linearr, int line);
void freeLineArray(LineArray* linearr);
int lineArrayGet(LineArray* linearr, int index);

#endif
