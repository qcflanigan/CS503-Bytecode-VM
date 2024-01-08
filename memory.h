#ifndef lox_memory_h
#define lox_memory_h

#include "common.h"
#include "LoxObject.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GrowCap(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GrowArr(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FreeArr(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void markObject(LoxObject* object);
void markValue(LoxValue value);
void collectGarbage();
void freeObjects();

#endif