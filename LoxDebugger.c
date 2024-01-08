// #include <stdio.h>
// #include "LoxDebugger.h"
// #include "LoxValue.h"
// //iterate through chunk of bytecode and disassemble each instruction/byte to print out the value being passed
// void disassembleChunk(LoxChunk* chunk, const char* chunkName) {
//     printf("== %s ==\n", chunkName);
//     int index;
//     for (index = 0; index < chunk->count;) {
//         index = disassembleInstruction(chunk, index);
//     }
// }

// static int constInstruction(const char* name, LoxChunk* chunk, int index) {
//     uint8_t constVal = chunk->code[index + 1];
//     printf("%-16s %4d '", name, constVal);
//     printLoxValue(chunk->constants.values[constVal]);
//     printf("'\n");
//     //return +2 because our constants are two bytes, where as the OP_RETURN is only one byte
//     return (index+2);
// }


// // reads in single byte from bytecode as we switch on the instruction code in DI function
// static int simpleInstruction(const char* name, int index){
//     printf("%s\n", name);
//     return index+1;
// }

// //break down code from the struct after we have disassmebled the chunk
// //will disassemble each byte of the instruction as we iterate through the chunk
// int disassembleInstruction(LoxChunk* chunk, int index){

//     printf("%04d ", index);
//     if (index > 0 && chunk->lines[index] == chunk->lines[index - 1]) {
//         printf(" | ");
//     } 
//     else{
//         printf("%4d ", chunk->lines[index]);
//     }

//     uint8_t instruction = chunk->code[index];

//     switch (instruction) {
//         case OP_RETURN:
//             return simpleInstruction("OP_RETURN", index);
//         case OP_CONSTANT:
//             return constInstruction("OP_CONSTANT", chunk, index);
//         case OP_NIL:
//             return simpleInstruction("OP_NIL", index);
//         case OP_TRUE:
//             return simpleInstruction("OP_TRUE", index);
//         case OP_FALSE:
//             return simpleInstruction("OP_FALSE", index);
//         case OP_EQUAL:
//             return simpleInstruction("OP_EQUAL", index);
//         case OP_GREATER:
//             return simpleInstruction("OP_GREATER", index);
//         case OP_LESS:
//             return simpleInstruction("OP_LESS", index);
//         case OP_NEGATE:
//             return simpleInstruction("OP_NEGATE", index);
//         case OP_ADD:
//             return simpleInstruction("OP_ADD", index);
//         case OP_SUB:
//             return simpleInstruction("OP_SUB", index);
//         case OP_MULTIPLY:
//             return simpleInstruction("OP_MULTIPLY", index);
//         case OP_DIVIDE:
//             return simpleInstruction("OP_DIVIDE", index);
//         case OP_NOT:
//             return simpleInstruction("OP_NOT", index);
//         default:
//             printf("Unknown opcode %d\n", instruction);
//             return index + 1;
// }
// }

#include <stdio.h>
#include "LoxDebugger.h"
#include "LoxObject.h"
#include "LoxValue.h"

void disassembleChunk(LoxChunk* chunk, const char* name) {
  printf("== %s ==\n", name);
  
  for (int index = 0; index < chunk->count;) {
    index = disassembleInstruction(chunk, index);
  }
}

static int constantInstruction(const char* name, LoxChunk* chunk, int index) {
  uint8_t constant = chunk->code[index + 1];
  printf("%-16s %4d '", name, constant);
  printLoxValue(chunk->constants.values[constant]);
  printf("'\n");
  return index + 2;
}

static int invokeInstruction(const char* name, LoxChunk* chunk, int index) {
  uint8_t constant = chunk->code[index + 1];
  uint8_t argCount = chunk->code[index + 2];
  printf("%-16s (%d args) %4d '", name, argCount, constant);
  printLoxValue(chunk->constants.values[constant]);
  printf("'\n");
  return index + 3;
}

static int simpleInstruction(const char* name, int index) {
  printf("%s\n", name);
  return index + 1;
}

static int byteInstruction(const char* name, LoxChunk* chunk, int index) {
  uint8_t slot = chunk->code[index + 1];
  printf("%-16s %4d\n", name, slot);
  return index + 2; 
}
static int jumpInstruction(const char* name, int sign, LoxChunk* chunk, int index) {
  uint16_t jump = (uint16_t)(chunk->code[index + 1] << 8);
  jump |= chunk->code[index + 2];
  printf("%-16s %4d -> %d\n", name, index, index + 3 + sign * jump);
  return index + 3;
}

int disassembleInstruction(LoxChunk* chunk, int index) {
  printf("%04d ", index);
  if (index > 0 &&
      chunk->lines[index] == chunk->lines[index - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[index]);
  }
  
  uint8_t instruction = chunk->code[index];
  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, index);
    case OP_NIL:
      return simpleInstruction("OP_NIL", index);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", index);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", index);
    case OP_POP:
      return simpleInstruction("OP_POP", index);
    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, index);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, index);
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, index);
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, index);
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, index);
    case OP_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", chunk, index);
    case OP_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", chunk, index);
    case OP_GET_PROPERTY:
      return constantInstruction("OP_GET_PROPERTY", chunk, index);
    case OP_SET_PROPERTY:
      return constantInstruction("OP_SET_PROPERTY", chunk, index);
    case OP_GET_SUPER:
      return constantInstruction("OP_GET_SUPER", chunk, index);
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", index);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", index);
    case OP_LESS:
      return simpleInstruction("OP_LESS", index);
    case OP_ADD:
      return simpleInstruction("OP_ADD", index);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", index);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", index);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", index);
    case OP_NOT:
      return simpleInstruction("OP_NOT", index);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", index);
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", index);
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, index);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, index);
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, index);
    case OP_CALL:
      return byteInstruction("OP_CALL", chunk, index);
    case OP_INVOKE:
      return invokeInstruction("OP_INVOKE", chunk, index);
    case OP_SUPER_INVOKE:
      return invokeInstruction("OP_SUPER_INVOKE", chunk, index);
    case OP_CLOSURE: {
      index++;
      uint8_t constant = chunk->code[index++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printLoxValue(chunk->constants.values[constant]);
      printf("\n");

      LoxObjFunction* function = AS_FUNCTION(
          chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = chunk->code[index++];
        int index = chunk->code[index++];
        printf("%04d      |                     %s %d\n",
               index - 2, isLocal ? "local" : "upvalue", index);
      }
      return index;
    }
    case OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", index);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", index);
    case OP_CLASS:
      return constantInstruction("OP_CLASS", chunk, index);
    case OP_INHERIT:
      return simpleInstruction("OP_INHERIT", index);
    case OP_METHOD:
        return constantInstruction("OP_METHOD", chunk, index);
    //not a known instruction, probably input error
    default:
      printf("Unknown opcode %d\n", instruction);
      return index + 1;
  }
}
