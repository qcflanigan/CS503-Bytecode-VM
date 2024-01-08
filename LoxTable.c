#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "LoxObject.h"
#include "LoxTable.h"
#include "LoxValue.h"

#define TABLE_MAX_LOAD 0.75

void initTable(LoxTable* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(LoxTable* table) {
    FreeArr(LoxEntry, table->entries, table->capacity);
    initTable(table);
}

static LoxEntry* findEntry(LoxEntry* entries, int capacity, LoxObjString* key){
    uint32_t index = key->hash & (capacity - 1);
    LoxEntry* tombstone = NULL;

    for (;;) {
        LoxEntry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)){
                return tombstone != NULL ? tombstone : entry;
            } 
            else{
            if (tombstone == NULL){
                tombstone = entry;
            }
            }
        } 
        else if (entry->key == key){
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool tableGet(LoxTable* table, LoxObjString* key, LoxValue* value) {
    if (table->count == 0) return false;

    LoxEntry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

static void adjustCapacity(LoxTable* table, int capacity) {
    LoxEntry* entries = ALLOCATE(LoxEntry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        LoxEntry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        LoxEntry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FreeArr(LoxEntry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(LoxTable* table, LoxObjString* key, LoxValue value){
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GrowCap(table->capacity);
        adjustCapacity(table, capacity);
    }

    LoxEntry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;

    if (isNewKey && IS_NIL(entry->value)) table->count++;
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(LoxTable* table, LoxObjString* key){
    if (table->count == 0) return false;

    LoxEntry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

void tableAddAll(LoxTable* src, LoxTable* dest){
    for (int i = 0; i < src->capacity; i++) {
        LoxEntry* entry = &src->entries[i];
        if (entry->key != NULL) {
            tableSet(dest, entry->key, entry->value);
        }
    }
}

LoxObjString* tableFindString(LoxTable* table, const char* chars, int length, uint32_t hash){
    if (table->count == 0) return NULL;

    uint32_t index = hash & (table->capacity - 1);
    for (;;) {
        LoxEntry* entry = &table->entries[index];
        if (entry->key == NULL) {
            if(IS_NIL(entry->value)) return NULL;
        } 
        else if (entry->key->length == length &&
            entry->key->hash == hash &&
            memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

void tableRemoveWhite(LoxTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        LoxEntry* entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(LoxTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        LoxEntry* entry = &table->entries[i];
        markObject((LoxObject*)entry->key);
        markValue(entry->value);
    }
}
