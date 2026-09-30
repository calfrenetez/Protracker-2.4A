#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_TEST_NATIVE_CHIP
#define PT_TEST_DISPATCH_EXEC
#define malloc native_allocate
#define free native_release
#include "paula_dispatch_test.c"
int main(void)
{int result;native_memory_start();result=paula_dispatch_fixture();native_memory_finish();return result;}
