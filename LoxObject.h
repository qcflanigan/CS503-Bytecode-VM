#ifndef lox_LoxObject_h
#define lox_LoxObject_h

#include "common.h"
#include "LoxChunk.h"
#include "LoxTable.h"
#include "LoxValue.h"


#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)

#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)

#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define AS_BOUND_METHOD(value) ((LoxObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)        ((LoxObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((LoxObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((LoxObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((LoxObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value) \
(((LoxObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)       ((LoxObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((LoxObjString*)AS_OBJ(value))->chars)


typedef enum {
OBJ_BOUND_METHOD,
OBJ_CLASS,
OBJ_CLOSURE,
OBJ_FUNCTION,
OBJ_INSTANCE,
OBJ_NATIVE,
OBJ_STRING,
OBJ_UPVALUE
} ObjType;

struct LoxObject{
    ObjType type;
    bool isMarked;
    struct LoxObject* next;
};

typedef struct {
    LoxObject obj;
    int arity;
    int upvalueCount;
    LoxChunk chunk;
    LoxObjString* name;
} LoxObjFunction;


typedef LoxValue (*LoxNativeFunc)(int argCount, LoxValue* args);

typedef struct {
    LoxObject obj;
    LoxNativeFunc function;
} LoxObjNative;


struct LoxObjString {
    LoxObject obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct LoxObjUpvalue {
    LoxObject obj;
    LoxValue* location;
    LoxValue closed;
    struct LoxObjUpvalue* next;
} LoxObjUpvalue;

typedef struct {
    LoxObject obj;
    LoxObjFunction* function;
    LoxObjUpvalue** upvalues;
    int upvalueCount;
} LoxObjClosure;


typedef struct {
    LoxObject obj;
    LoxObjString* name;
    LoxTable methods;
} LoxObjClass;

typedef struct {
    LoxObject obj;
    LoxObjClass* klass;
    LoxTable fields; 
} LoxObjInstance;

typedef struct {
    LoxObject obj;
    LoxValue receiver;
    LoxObjClosure* method;
} LoxObjBoundMethod;

LoxObjBoundMethod* newBoundMethod(LoxValue receiver, LoxObjClosure* method);
LoxObjClass* newClass(LoxObjString* name);
LoxObjClosure* newClosure(LoxObjFunction* function);
LoxObjFunction* newFunction();
LoxObjInstance* newInstance(LoxObjClass* klass);
LoxObjNative* newNative(LoxNativeFunc function);
LoxObjString* takeString(char* chars, int length);
LoxObjString* copyString(const char* chars, int length);
LoxObjUpvalue* newUpvalue(LoxValue* slot);
void printObject(LoxValue value);
static inline bool isObjType(LoxValue value, ObjType type) {
return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif