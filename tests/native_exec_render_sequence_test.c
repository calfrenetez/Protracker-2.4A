#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_RENDER_SEQUENCE_NATIVE
#include "render_sequence_test.c"
int main(void)
{int result;native_memory_start();result=sequence_fixture_main();native_memory_finish();return result;}
