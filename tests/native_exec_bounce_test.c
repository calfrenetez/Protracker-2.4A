#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main bounce_fixture
#include "bounce_test.c"
#undef main
int main(void)
{
    int result;native_memory_start();result=bounce_fixture();native_memory_finish();return result;
}
