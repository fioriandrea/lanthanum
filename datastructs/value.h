#ifndef value_h
#define value_h

typedef enum {
    VALUE_NIHL,
    VALUE_BOOL,
    VALUE_NUMBER,
} ValueType;

typedef struct {
    ValueType type;
    union {
        int boolean;
        double number;
    } as; 
} Value;

#define is_nihl(value) (value.type == VALUE_NIHL)
#define is_bool(value) (value.type == VALUE_BOOL)
#define is_number(value) (value.type == VALUE_NUMBER)

#define as_cbool(value) (value.as.boolean)
#define as_cnumber(value) (value.as.number)

#define to_vbool(cbool) ((Value) {VALUE_BOOL, {.boolean = cbool}})
#define to_vnihl() ((Value) {VALUE_NIHL, {.number = 0}}) 
#define to_vnumber(cnumber) ((Value) {VALUE_NUMBER, {.number = cnumber}})

typedef struct {
    int count;
    int capacity;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* valarray);
int writeValueArray(ValueArray* valarray, Value value);
void freeValueArray(ValueArray* valarray);

#endif
