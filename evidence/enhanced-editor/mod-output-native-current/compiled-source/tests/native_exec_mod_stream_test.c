#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_MOD_STREAM_NATIVE
#include "mod_stream_test.c"
int main(int argc,char **argv)
{int result;native_memory_start();result=mod_stream_fixture(argc,argv);native_memory_finish();return result;}
