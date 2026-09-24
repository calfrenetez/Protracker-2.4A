#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define main import_fixture
#include "wav_import_test.c"
#undef main
int main(int argc,char **argv)
{int result;native_memory_start();result=import_fixture(argc,argv);native_memory_finish();return result;}
