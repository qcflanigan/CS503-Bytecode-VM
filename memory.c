#include <stdlib.h>
#include "LoxCompiler.h"
#include "memory.h"
#include "LoxVM.h"
#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "LoxDebugger.h"
#endif
#define GC_HEAP_GROW_FACTOR 2


void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;

    if (newSize > oldSize) {
        #ifdef DEBUG_STRESS_GC
        collectGarbage();
        #endif

        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
}

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);
    return result;
}

void markObject(LoxObject* object){
    if (object == NULL) return;
    if (object->isMarked) return;

    #ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
    #endif

    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GrowCap(vm.grayCapacity);
        vm.grayStack = (LoxObject**)realloc(vm.grayStack, sizeof(LoxObject*) * vm.grayCapacity);

        if (vm.grayStack == NULL) exit(1);
    }

    vm.grayStack[vm.grayCount++] = object;
}

void markValue(LoxValue value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(LoxValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

static void blackenObject(LoxObject* object){
    #ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
    #endif

    switch (object->type){
        case OBJ_BOUND_METHOD:{
            LoxObjBoundMethod* bound = (LoxObjBoundMethod*)object;
            markValue(bound->receiver);
            markObject((LoxObject*)bound->method);
            break;
        }
        case OBJ_CLASS:{
            LoxObjClass* klass = (LoxObjClass*)object;
            markObject((LoxObject*)klass->name);
            markTable(&klass->methods);
            break;
        }
        case OBJ_CLOSURE:{
            LoxObjClosure* closure = (LoxObjClosure*)object;
            markObject((LoxObject*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
            markObject((LoxObject*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION:{
            LoxObjFunction* function = (LoxObjFunction*)object;
            markObject((LoxObject*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_INSTANCE:{
            LoxObjInstance* instance = (LoxObjInstance*)object;
            markObject((LoxObject*)instance->klass);
            markTable(&instance->fields);
            break;
        }
        case OBJ_UPVALUE:
            markValue(((LoxObjUpvalue*)object)->closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
        }
}

static void freeObject(LoxObject* object) {
    #ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
    #endif

    switch (object->type){
        case OBJ_BOUND_METHOD:
            FREE(LoxObjBoundMethod, object);
            break;
        case OBJ_CLASS:{
            LoxObjClass* klass = (LoxObjClass*)object;
            freeTable(&klass->methods);
            FREE(LoxObjClass, object);
            break;
        } 
        case OBJ_CLOSURE:{
            LoxObjClosure* closure = (LoxObjClosure*)object;
            FreeArr(LoxObjUpvalue*, closure->upvalues, closure->upvalueCount);
            FREE(LoxObjClosure, object);
            break;
        }
        case OBJ_FUNCTION:{
            LoxObjFunction* function = (LoxObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(LoxObjFunction, object);
            break;
        }
        case OBJ_INSTANCE:{
            LoxObjInstance* instance = (LoxObjInstance*)object;
            freeTable(&instance->fields);
            FREE(LoxObjInstance, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(LoxObjNative, object);
            break;
        case OBJ_STRING:{
            LoxObjString* string = (LoxObjString*)object;
            FreeArr(char, string->chars, string->length + 1);
            FREE(LoxObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(LoxObjUpvalue, object);
            break;
    }
}

static void markRoots() {
    for (LoxValue* slot = vm.stack; slot < vm.stackTop; slot++){
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++){
        markObject((LoxObject*)vm.frames[i].closure);
    }

    for (LoxObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next){
        markObject((LoxObject*)upvalue);
    }

    markTable(&vm.globals);
    markCompilerRoots();
    markObject((LoxObject*)vm.initString);
}

static void traceReferences() {
    while (vm.grayCount > 0) {
        LoxObject* obj = vm.grayStack[--vm.grayCount];
        blackenObject(obj);
    }
}

static void sweep() {
    LoxObject* prev = NULL;
    LoxObject* obj = vm.objects;

    while (obj != NULL) {
        if (obj->isMarked) {
            obj->isMarked = false;
            prev = obj;
            obj = obj->next;
        } 
        else {
            LoxObject* unreached = obj;
            obj = obj->next;
            if (prev != NULL) {
                prev->next = obj;
            } 
            else {
                vm.objects = obj;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage() {
    #ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytesAllocated;
    #endif


    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();


    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;


    #ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
    #endif
}

void freeObjects() {
    LoxObject* obj = vm.objects;
    while (obj != NULL) {
        LoxObject* nextObj = obj->next;
        freeObject(obj);
        obj = nextObj;
}

    free(vm.grayStack);
}
