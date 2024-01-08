#ifndef lox_LoxCompiler_h
#define lox_LoxCompiler_h

#include "LoxObject.h"
#include "LoxVM.h"

LoxObjFunction* compileCode(const char* source);

void markCompilerRoots();

#endif