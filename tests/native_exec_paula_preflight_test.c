#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main paula_preflight_fixture
#include "paula_preflight_test.c"
#undef main
int main(void)
{int result;native_memory_start();result=paula_preflight_fixture();native_memory_finish();return result;}
