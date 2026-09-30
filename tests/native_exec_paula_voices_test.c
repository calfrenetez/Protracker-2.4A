#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_TEST_NATIVE_CHIP
#define malloc native_allocate
#define free native_release
#define main paula_voices_fixture
#include "paula_voices_test.c"
#undef main
int main(void)
{int result;native_memory_start();result=paula_voices_fixture();native_memory_finish();return result;}
