#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_EDITOR_WAVETABLE_NATIVE
#include "editor_wavetable_test.c"
int main(void){int result;native_memory_start();result=editor_wavetable_fixture();native_memory_finish();return result;}
