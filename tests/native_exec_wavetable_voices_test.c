#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_WAVETABLE_VOICES_NATIVE
#include "wavetable_voices_test.c"
int main(void)
{int result;native_memory_start();result=voices_fixture_main();native_memory_finish();return result;}
