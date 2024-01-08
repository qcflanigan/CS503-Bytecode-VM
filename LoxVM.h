#ifndef lox_LoxVM_h
#define lox_LoxVM_h

#include "LoxObject.h"
#include "LoxTable.h"
#include "LoxValue.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  LoxObjClosure* closure;
  uint8_t* ip;
  LoxValue* slots;
} LoxCallFrame;

typedef struct {
  LoxCallFrame frames[FRAMES_MAX];
  int frameCount;
  
  LoxValue stack[STACK_MAX];
  LoxValue* stackTop;

  LoxTable globals;

  LoxTable strings;
  LoxObjString* initString;
  LoxObjUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGC;

  LoxObject* objects;

  int grayCount;
  int grayCapacity;
  LoxObject** grayStack;
} LoxVM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpreterResult;

extern LoxVM vm;

void initLoxVM();
void freeLoxVM();

InterpreterResult interpretCode(const char* source);
void push(LoxValue value);
LoxValue pop();

#endif