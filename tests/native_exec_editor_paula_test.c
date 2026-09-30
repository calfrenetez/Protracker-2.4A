#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_TEST_NATIVE_CHIP
#define PT_TEST_EDITOR_PAULA_EXEC
#define malloc native_allocate
#define free native_release
#include "editor_paula_test.c"
int main(void)
{int result;native_memory_start();result=editor_paula_fixture();native_memory_finish();return result;}
