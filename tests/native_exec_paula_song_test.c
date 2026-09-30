#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_TEST_NATIVE_CHIP
#define PT_TEST_SONG_EXEC
#define malloc native_allocate
#define free native_release
#include "paula_song_test.c"
int main(void)
{int result;native_memory_start();result=paula_song_fixture();native_memory_finish();return result;}
