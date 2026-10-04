#include "native_exec_memory.h"
#define PT_WORKFLOW_NATIVE
#define malloc native_allocate
#define free native_release
#define main editor_workflow_fixture
#include "editor_workflow_test.c"
#undef main
int main(void)
{
    int result;native_memory_start();result=editor_workflow_fixture();native_memory_finish();return result;
}
