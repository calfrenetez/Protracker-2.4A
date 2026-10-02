#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main fixture_main
#include "native_amigus_ram_bus_test.c"
#undef main
int main(void)
{int result;native_memory_start();result=fixture_main();native_memory_finish();return result;}
