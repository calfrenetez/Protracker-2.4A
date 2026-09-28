#include "native_exec_memory.h"
/* A failed assertion must not strand direct Exec allocations in the shared
 * guest. This fixture ledger owns only blocks returned through its allocator. */
static void *recovery_blocks[1024];
static void recovery_failure(const char *condition,const char *file,unsigned line)
{
    unsigned i;
    printf("RECOVERY ASSERTION FAILED: %s at %s:%u\n",condition,file,line);
    for(i=0;i<1024;++i)if(recovery_blocks[i]) {
        native_release(recovery_blocks[i]);recovery_blocks[i]=NULL;
    }
    if(!native_pool.used)puts("EXEC MEMORY FAILURE CLEANUP: zero owned bytes");
    else printf("EXEC MEMORY FAILURE RETAINED: %lu bytes\n",(unsigned long)native_pool.used);
    fflush(stdout);exit(20);
}
static void *recovery_allocate(size_t bytes)
{
    unsigned i;void *p;
    for(i=0;i<1024 && recovery_blocks[i];++i) {}
    if(i==1024)recovery_failure("fixture ledger full",__FILE__,__LINE__);
    p=native_allocate(bytes);recovery_blocks[i]=p;return p;
}
static void recovery_release(void *p)
{
    unsigned i;if(!p)return;
    for(i=0;i<1024 && recovery_blocks[i]!=p;++i) {}
    if(i==1024)recovery_failure("fixture release is not owned",__FILE__,__LINE__);
    native_release(p);recovery_blocks[i]=NULL;
}
#define malloc recovery_allocate
#define free recovery_release
#define PT_RECOVERY_NATIVE
#include "recovery_file_test.c"
int main(int argc,char **argv)
{
    int result;native_memory_start();
    if(argc==2 && !strcmp(argv[1],"--failure-cleanup")) {
        (void)recovery_allocate(257);(void)recovery_allocate(31);
        recovery_failure("intentional cleanup probe",__FILE__,__LINE__);
    }
    if(argc==5 && !strcmp(argv[1],"--seed"))result=native_recovery_seed(argv[2],argv[3],argv[4]);
    else result=recovery_fixture(argc,argv);
    native_memory_finish();return result;
}
