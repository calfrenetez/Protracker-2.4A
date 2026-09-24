#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main dispatch_fixture
#include "sample_dispatch_test.c"
#undef main
int main(int argc,char **argv)
{int result;native_memory_start();result=dispatch_fixture(argc,argv);native_memory_finish();return result;}
