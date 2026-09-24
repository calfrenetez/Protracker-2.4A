#include "native_exec_memory.h"
#include <string.h>
static void *native_editor_calloc(size_t count,size_t size)
{
    void *p;assert(!size || count<=SIZE_MAX/size);
    p=native_allocate(count*size);memset(p,0,count*size);return p;
}
#define malloc native_allocate
#define calloc native_editor_calloc
#define free native_release
#define main fixture_main
#include "editor_studio_test.c"
#undef main
int main(void)
{
    int result;native_memory_start();result=fixture_main();native_memory_finish();return result;
}
