#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define INVERT_FIRST_PULL 17
#define main fixture_main
#include "editor_invert_reference_test.c"
#undef main
int main(int argc,char **argv)
{int result;native_memory_start();result=fixture_main(argc,argv);native_memory_finish();return result;}
