#ifndef lox_LoxDebugger_h
#define lox_LoxDebugger_h

#include "LoxChunk.h"

void disassembleChunk(LoxChunk* chunk, const char* chunkName);
int disassembleInstruction(LoxChunk* chunk, int index);


#endif
