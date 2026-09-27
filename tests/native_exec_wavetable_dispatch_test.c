#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_WAVETABLE_DISPATCH_NATIVE
#include "wavetable_dispatch_test.c"
int main(void)
{int result;native_memory_start();result=dispatch_fixture_main();native_memory_finish();return result;}
