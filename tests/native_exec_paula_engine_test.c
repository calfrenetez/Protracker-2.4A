#include "native_exec_memory.h"
#include <proto/dos.h>
#define PT_ENGINE_NATIVE
#define malloc native_allocate
#define free native_release
#include "paula_engine_test.c"
