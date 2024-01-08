#ifndef lox_common_h
#define lox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NAN_BOXING
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTIONc

#define DEBUG_STRESS_GCgc
#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXECUTION
#undef DEBUG_STRESS_GC
#undef DEBUG_LOG_GC