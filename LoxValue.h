// #ifndef clox_LoxValue_h
// #define clox_LoxValue_h

// #include "common.h"

//structs to represent the basic way lox values are represented within the C lang
//typedef enum {
// VAL_BOOL,
// VAL_NIL,
// VAL_NUMBER,
// } LoxValueType;

// typedef double Value;

// typedef struct {
//     LoxValueType type;
//     union {
//         bool boolean;
//         double number;
//     } as;
// } LoxValue;

// //adjusting value types to get rid of unicoded paradigm
// #define IS_BOOL(value)    ((Value).type == VAL_BOOL)
// #define IS_NIL(value)     ((Value).type == VAL_NIL)
// #define IS_NUMBER(value)  ((Value).type == VAL_NUMBER)
 
// #define AS_BOOL(value)    ((value).as.boolean)
// #define AS_NUMBER(value)  ((value).as.number)

// #define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
// #define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
// #define NUMBER_VAL(value) ((value){VAL_NUMBER, {.number = value}})

// //typedef double lvalue;

// //store the capacity of array for holding lox values and the current count of its space taken up
// //use a pointer to an array to hold the lox value data
// typedef struct {
//     int capacity;
//     int count;
//     LoxValue* values;
// } LoxValueArray;

// //same functions as declaring and using chunks of bytecode, just with actual lox values now 
// bool valuesEqual(LoxValue left, LoxValue right);
// void initLoxValueArray(LoxValueArray* array);
// void writeLoxValueArray(LoxValueArray* array, LoxValue value);
// void freeLoxValueArray(LoxValueArray* array);
// void printLoxValue(LoxValue value);

// #endif

#ifndef lox_LoxValue_h
#define lox_LoxValue_h

#include <string.h>

#include "common.h"

typedef struct LoxObject LoxObject;
typedef struct LoxObjString LoxObjString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NIL   1 
#define TAG_FALSE 2 
#define TAG_TRUE  3 

typedef uint64_t LoxValue;

#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJ(value) \
(((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value) \
((LoxObject*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((LoxValue)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((LoxValue)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((LoxValue)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj) \
(LoxValue)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double valueToNum(LoxValue value) {
    double num;
    memcpy(&num, &value, sizeof(LoxValue));
    return num;
}


static inline LoxValue numToValue(double num) {
    LoxValue value;
    memcpy(&value, &num, sizeof(double));
    return value;
}


#else

typedef enum {
    VAL_BOOL,
    VAL_NIL, 
    VAL_NUMBER,
    VAL_OBJ
} LoxValueType;

typedef struct {
    LoxValueType type;
    union {
        bool boolean;
        double number;
        LoxObject* obj;
    } as; 
} LoxValue;



#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif

typedef struct {
    int capacity;
    int count;
    LoxValue* values;
} LoxValueArray;

bool valuesEqual(LoxValue a, LoxValue b);
void initLoxValueArray(LoxValueArray* array);
void writeLoxValueArray(LoxValueArray* array, LoxValue value);
void freeLoxValueArray(LoxValueArray* array);
void printLoxValue(LoxValue value);

#endif