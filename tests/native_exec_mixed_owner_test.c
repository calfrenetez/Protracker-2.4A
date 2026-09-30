#include "native_exec_memory.h"
#include "../src/native/paula_memory.h"
#define PT_TEST_NATIVE_CHIP
#define malloc native_allocate
#define free native_release
#define PT_TEST_MIXED_SCHEDULE_ONLY
#define PT_TEST_MIXED_EXEC
#include "mixed_owner_test.c"
int main(void){int result;native_memory_start();result=mixed_owner_fixture();native_memory_finish();return result;}
