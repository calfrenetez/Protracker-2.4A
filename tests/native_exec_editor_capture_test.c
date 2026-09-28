#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define EDITOR_CAPTURE_ENTRY editor_capture_fixture
#include "editor_capture_test.c"
#undef EDITOR_CAPTURE_ENTRY
int main(void)
{int result;native_memory_start();result=editor_capture_fixture();native_memory_finish();return result;}
