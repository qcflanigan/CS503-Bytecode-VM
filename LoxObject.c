#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "LoxObject.h"
#include "LoxTable.h"
#include "LoxValue.h"
#include "LoxVM.h"

#define ALLOCATE_OBJ(type, objectType) \
(type*)allocateObject(sizeof(type), objectType)

static LoxObject* allocateObject(size_t size, ObjType type) {
    LoxObject* obj = (LoxObject*)reallocate(NULL, 0, size);
    obj->type = type;
    obj->isMarked = false;

    obj->next = vm.objects;
    vm.objects = obj;

    #ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
    #endif

    return obj;
}

LoxObjBoundMethod* newBoundMethod(LoxValue receiver, LoxObjClosure* method) {
    LoxObjBoundMethod* bound = ALLOCATE_OBJ(LoxObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

LoxObjClass* newClass(LoxObjString* name) {
    LoxObjClass* klass = ALLOCATE_OBJ(LoxObjClass, OBJ_CLASS);
    klass->name = name; 
    initTable(&klass->methods);
    return klass;
}

LoxObjClosure* newClosure(LoxObjFunction* function) {
    LoxObjUpvalue** upvalues = ALLOCATE(LoxObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    LoxObjClosure* closure = ALLOCATE_OBJ(LoxObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

LoxObjFunction* newFunction() {
    LoxObjFunction* function = ALLOCATE_OBJ(LoxObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

LoxObjInstance* newInstance(LoxObjClass* klass) {
    LoxObjInstance* instance = ALLOCATE_OBJ(LoxObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

LoxObjNative* newNative(LoxNativeFunc function) {
    LoxObjNative* native = ALLOCATE_OBJ(LoxObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

static LoxObjString* allocateString(char* chars, int length, uint32_t hash) {
    LoxObjString* str = ALLOCATE_OBJ(LoxObjString, OBJ_STRING);
    str->length = length;
    str->chars = chars;
    str->hash = hash;

    push(OBJ_VAL(str));
    tableSet(&vm.strings, str, NIL_VAL);
    pop();

    return str;
}

static uint32_t hashString(const char* key, int length){
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

LoxObjString* takeString(char* chars, int length){
    uint32_t hash = hashString(chars, length);
    LoxObjString* objStr = tableFindString(&vm.strings, chars, length, hash);
    if (objStr != NULL) {
        FreeArr(char, chars, length + 1);
        return objStr;
    }

    return allocateString(chars, length, hash);
}

LoxObjString* copyString(const char* chars, int length) {
    uint32_t hashVal = hashString(chars, length);
    LoxObjString* objStr = tableFindString(&vm.strings, chars, length, hashVal);

    if (objStr != NULL) return objStr;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hashVal);
}

LoxObjUpvalue* newUpvalue(LoxValue* slot) {
    LoxObjUpvalue* upvalue = ALLOCATE_OBJ(LoxObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void printFunction(LoxObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void printObject(LoxValue value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_INSTANCE:
            printf("%s instance",
                    AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
        }
}
