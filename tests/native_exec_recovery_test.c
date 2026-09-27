#include "native_exec_memory.h"
#define malloc native_allocate
#define free native_release
#define PT_RECOVERY_NATIVE
#include "recovery_file_test.c"
int main(int argc,char **argv)
{int result;native_memory_start();result=recovery_fixture(argc,argv);native_memory_finish();return result;}
