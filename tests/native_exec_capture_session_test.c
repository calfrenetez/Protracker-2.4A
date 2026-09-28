#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main capture_session_fixture
#include "capture_session_test.c"
#undef main
int main(void)
{int result;native_memory_start();result=capture_session_fixture();native_memory_finish();return result;}
