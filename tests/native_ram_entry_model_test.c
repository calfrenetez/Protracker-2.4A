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
#define ENTRY_MODEL_STACK_MODE 34U
#define ENTRY_MODEL_STACK_CASES 22U
static int entry_model_task_stack_case(unsigned which)
{
    struct entry_model *m=&entry_model;char marker=0;
    char name[]="entry-stack-model",session[]="abcdef0123456789";
    char *argv[]={name,session,NULL};uintptr_t base;unsigned admitted=0;int result;
    assert(which<ENTRY_MODEL_STACK_CASES);
    entry_model_start(EM_PAL,0,&marker);
    assert(m->live_sp>65536U&&m->live_sp<UINTPTR_MAX-131074U);
    base=m->live_sp-65536U;
    m->process.pr_Task.tc_SPLower=(void *)base;
    m->process.pr_Task.tc_SPUpper=(void *)(base+131074U); /* usable upper=base+131072 */
    switch(which){
    case 0:m->process.pr_Task.tc_SPLower=NULL;break;
    case 1:m->process.pr_Task.tc_SPUpper=NULL;break;
    case 2:m->process.pr_Task.tc_SPUpper=(void *)(base-2U);break;
    case 3:m->process.pr_Task.tc_SPUpper=(void *)base;break;
    case 4:m->process.pr_Task.tc_SPLower=(void *)(UINTPTR_MAX-1U);m->process.pr_Task.tc_SPUpper=(void *)2;break;
    case 5:m->process.pr_Task.tc_SPUpper=(void *)1;break;
    case 6:m->process.pr_Task.tc_SPLower=(void *)(base+1U);break;
    case 7:m->process.pr_Task.tc_SPUpper=(void *)(base+131073U);break;
    case 8:m->live_sp=0;break;
    case 9:m->live_sp=base+32769U;break;
    case 10:m->live_sp=base-2U;break;
    case 11:m->live_sp=base+131072U;break; /* rawUpper-2 is excluded */
    case 12:m->live_sp=base+32766U;break; /* nearest aligned value below policy */
    case 13:m->live_sp=base+32768U;admitted=1;break;
    case 14:m->process.pr_Task.tc_Node.ln_Type=NT_INTERRUPT;break;
    case 15:m->live_sp=base+131076U;break;
    case 16:m->process.pr_Task.tc_SPUpper=(void *)(base+2U);break;
    case 17:m->live_sp=base+32770U;admitted=1;break;
    case 18:m->process.pr_Task.tc_SPUpper=(void *)UINTPTR_MAX;break;
    case 19:m->live_sp=base;break;
    case 20:m->live_sp=base+131070U;admitted=1;break; /* last aligned usable SP */
    case 21:m->live_sp=base+32767U;break; /* policy-1 is also misaligned */
    default:assert(!"unknown stack case");return 1;
    }
    result=pt_entry_model_main(2,argv);
    if(admitted){assert(result==0&&m->stack_reads==1);entry_model_assert_result(result);}
    else{
        assert(result==20&&!m->hold&&!m->trace_count&&!m->alloc_calls&&!m->free_calls);
        assert(!m->port_calls&&!m->request_calls&&!m->open_calls&&!m->close_calls);
        assert(!m->abort_calls&&!m->waitio_calls&&!m->check_calls&&!m->delay_one);
        assert(!m->adds&&!m->removes&&!m->signals&&!m->waits&&!m->disable_depth);
        assert(m->process.pr_Task.tc_SigAlloc==m->original_signals&&m->received==(UINT32_C(1)<<7));
        assert(m->stack_reads==(which==14?0U:1U));
    }
    printf("NATIVE ENTRY TASK STACK CASE: case=%u admitted=%u result=%d resource_trace=%u\n",
        which,admitted,result,m->trace_count);
    puts("NATIVE ENTRY TASK STACK MODEL PASS: synthetic SP, pre-resource admission only; no native stack qualification");
    puts("NATIVE ENTRY HOST MODEL CASE PASS: software resource oracle only; no native ABI, placement, IRQ, timer or stack qualification");
    return 0;
}
int main(int argc,char **argv)
{
    char stack_marker=0;char name[]="entry-model";char session[]="abcdef0123456789";
    char *native_argv[]={name,session,NULL};unsigned mode,ordinal;int result=-1;
    assert(argc==3);mode=(unsigned)strtoul(argv[1],NULL,10);ordinal=(unsigned)strtoul(argv[2],NULL,10);
    if(mode==ENTRY_MODEL_STACK_MODE)return entry_model_task_stack_case(ordinal);
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
