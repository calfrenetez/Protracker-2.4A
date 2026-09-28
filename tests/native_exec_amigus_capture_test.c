#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define AMIGUS_CAPTURE_ENTRY amigus_capture_fixture
#include "amigus_capture_test.c"
#undef AMIGUS_CAPTURE_ENTRY
int main(void)
{int result;native_memory_start();result=amigus_capture_fixture();native_memory_finish();return result;}
