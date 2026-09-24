#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main sample_raw_fixture
#include "sample_raw_file_test.c"
#undef main
int main(int argc,char **argv)
{int result;native_memory_start();result=sample_raw_fixture(argc,argv);native_memory_finish();return result;}
