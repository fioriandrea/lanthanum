#ifndef debug_switches
#define debug_switches

//#define DEBUG_ON

#ifdef DEBUG_ON

#include "token_printer.h"
#include "asm_printer.h"
#include "asm_printer.h"
#include "map_printer.h"
#include "value_dump.h"

#define STRESS_GC
#define TRACE_GC
#define TRACE_OBJECT_LIST
#define TRACE_TOKENS 
#define PRINT_CODE
#define TRACE_EXEC
#define TRACE_OPEN_UPVALUES
#define TRACE_INTERNED
#define TRACE_GLOBALS

#endif


#endif
