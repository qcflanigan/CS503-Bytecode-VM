#ifndef lox_LoxTable_h
#define lox_LoxTable_h

#include "common.h"
#include "LoxValue.h"
#include "LoxObject.h"

typedef struct {
    LoxObjString* key;
    LoxValue value;
} LoxEntry;

typedef struct {
    int count;
    int capacity;
    LoxEntry* entries;
} LoxTable;

void initTable(LoxTable* table);
void freeTable(LoxTable* table);
bool tableGet(LoxTable* table, LoxObjString* key, LoxValue* value);
bool tableSet(LoxTable* table, LoxObjString* key, LoxValue value);
bool tableDelete(LoxTable* table, LoxObjString* key);
void tableAddAll(LoxTable* from, LoxTable* to);
LoxObjString* tableFindString(LoxTable* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(LoxTable* table);
void markTable(LoxTable* table);

#endif