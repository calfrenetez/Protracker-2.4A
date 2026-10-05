/* A fresh executable process runs ONE mode. No reset of the native bootstrap,
 * no native code/layout/IRQ emulation, no old test-suite inclusion or core cast.
 */
#include "native_ram_entry_host_stubs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef main
#undef main
#endif
int pt_entry_model_main(int,char **);
int main(int argc,char **argv)
{
    char stack_marker=0;char name[]="entry-model";char session[]="abcdef0123456789";
    char *native_argv[]={name,session,NULL};unsigned mode,ordinal;int result=-1;
    assert(argc==3);mode=(unsigned)strtoul(argv[1],NULL,10);ordinal=(unsigned)strtoul(argv[2],NULL,10);
    assert(mode<=EM_TERMINATION_ONLY);
    entry_model_start((enum entry_model_mode)mode,ordinal,&stack_marker);
    if(!setjmp(entry_model.sink))result=pt_entry_model_main(2,native_argv);
    entry_model_assert_result(result);
    printf("NATIVE ENTRY HOST CASE: mode=%u ordinal=%u result=%d hold=%u trace=%u allocations=%u releases=%u abort=%u waitio=%u delay=%u\n",
        mode,ordinal,result,entry_model.hold,entry_model.trace_count,entry_model.alloc_calls,entry_model.free_calls,
        entry_model.abort_calls,entry_model.waitio_calls,entry_model.delay_one);
    puts("NATIVE ENTRY HOST MODEL CASE PASS: software resource oracle only; no native ABI, placement, IRQ, timer or stack qualification");
    return 0;
}
