#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_SONG_FIRST_BLOCK 256
#define main fixture_main
#include "render_invert_session_test.c"
#undef main
int main(void)
{
    int result;native_memory_start();result=fixture_main();native_memory_finish();return result;
}
