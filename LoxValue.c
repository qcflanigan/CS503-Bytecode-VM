#include <stdio.h>
#include "memory.h"
#include "LoxValue.h"
// #include "common.h"
#include "LoxObject.h"


//initialize the array needed to hold our lox values
void initLoxValueArray(LoxValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

//try to write the lox value to our array to store it
//if there is not enough room, expand the capacity of our array first, then store the value
void writeLoxValueArray(LoxValueArray* array, LoxValue value) {
    if (array->capacity < (array->count + 1)) {
        int oldCap = array->capacity;
        array->capacity = GrowCap(oldCap);
        array->values = GrowArr(LoxValue, array->values,
        oldCap, array->capacity);
}
    array->values[array->count] = value;
    array->count++;
}

//release the memory of array by initializing with capacity of 0 and no values
void freeLoxValueArray(LoxValueArray* array) {
    FreeArr(LoxValue, array->values, array->capacity);
    initLoxValueArray(array);
}

// void printLoxValue(LoxValue value){
//     switch (value.type) {
//         case VAL_BOOL:
//             printf(AS_BOOL(value) ? "true" : "false");
//             break;
//         case VAL_NIL: printf("nil"); break;
//         case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;

// }
// }

// bool valuesEqual(LoxValue left, LoxValue right) {
//     if (left.type != right.type) return false;
//     //handle each type of value we could be comparing for equality
//     //return nil as true
//     switch (left.type) {
//         case VAL_BOOL: return AS_BOOL(left) == AS_BOOL(right);
//         case VAL_NIL: return true;
//         case VAL_NUMBER: return AS_NUMBER(left) == AS_NUMBER(right);
//         default: 
//             return false;
//     }
// }

void printLoxValue(LoxValue value) {
    #ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } 
    else if (IS_NIL(value)) {
        printf("nil");
    } 
    else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } 
    else if (IS_OBJ(value)) {
        printObject(value);
    }
    #else
        switch (value.type) {
            case VAL_BOOL:
                printf(AS_BOOL(value) ? "true" : "false");
                break;
            case VAL_NIL: printf("nil"); break;
            case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
            case VAL_OBJ: printObject(value); break;
    }
    #endif
}

bool valuesEqual(LoxValue left, LoxValue right){
    #ifdef NAN_BOXING
    if (IS_NUMBER(left) && IS_NUMBER(right)){
    return AS_NUMBER(left) == AS_NUMBER(right);
    }
    return left == right;
    #else
    if (left.type != right.type) return false;
        switch (left.type){
            case VAL_BOOL: return AS_BOOL(left) == AS_BOOL(right);
            case VAL_NIL: return true;
            case VAL_NUMBER: return AS_NUMBER(left) == AS_NUMBER(right);
            case VAL_OBJ: return AS_OBJ(left) == AS_OBJ(right);
            default:         
                return false; 
            }
#endif
}
