#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main mixed_preflight_fixture
#include "mixed_preflight_test.c"
#undef main
int main(void){int result;native_memory_start();result=mixed_preflight_fixture();native_memory_finish();return result;}
