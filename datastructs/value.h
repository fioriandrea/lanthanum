#ifndef value_h
#define value_h

typedef double Value;

typedef struct {
    int count;
    int capacity;
    Value* values;
} ValueArray;


void initValueArray(ValueArray* valarray);
int writeValueArray(ValueArray* valarray, Value value);
void freeValueArray(ValueArray* valarray);

#endif
