#include <stdlib.h>

#include "LoxChunk.h"
#include "memory.h"
#include "LoxVM.h"

void initChunk(LoxChunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;

  initLoxValueArray(&chunk->constants);
}

void freeChunk(LoxChunk* chunk) {
  FreeArr(uint8_t, chunk->code, chunk->capacity);
  FreeArr(int, chunk->lines, chunk->capacity);

  freeLoxValueArray(&chunk->constants);
//< chunk-free-constants
  initChunk(chunk);
}

void writeChunk(LoxChunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCap = chunk->capacity;
    chunk->capacity = GrowCap(oldCap);
    chunk->code = GrowArr(uint8_t, chunk->code,
        oldCap, chunk->capacity);
    chunk->lines = GrowArr(int, chunk->lines,
        oldCap, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

int addConstant(LoxChunk* chunk, LoxValue value) {
  push(value);
  writeLoxValueArray(&chunk->constants, value);
  pop();
  return chunk->constants.count - 1;
}
