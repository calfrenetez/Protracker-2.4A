#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_SAMPLER_WAVETABLE_NATIVE
#include "sampler_wavetable_test.c"
int main(void)
{int result;native_memory_start();result=sampler_fixture_main();native_memory_finish();return result;}
