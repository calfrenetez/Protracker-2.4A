/* A fresh executable process runs ONE mode. No reset of the native bootstrap,
 * no native code/layout/IRQ emulation, no old test-suite inclusion or core cast.
 */
#include "native_ram_entry_host_stubs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define ENTRY_MODEL_SYSTEM_MODE 35U
#define ENTRY_MODEL_SYSTEM_CASES 17U
#define ENTRY_MODEL_SAMPLE_MODE 36U
#define ENTRY_MODEL_SAMPLE_CASES 21U
static struct {
    unsigned char exec[sizeof(struct ExecBase)],system[4096];
} protected_before;
static void entry_model_no_resources(void)
{
    struct entry_model *m=&entry_model;
    assert(!m->hold&&!m->trace_count&&!m->alloc_calls&&!m->free_calls);
    assert(!m->port_calls&&!m->request_calls&&!m->open_calls&&!m->close_calls);
    assert(!m->abort_calls&&!m->waitio_calls&&!m->check_calls&&!m->delay_one);
    assert(!m->adds&&!m->removes&&!m->signals&&!m->waits&&!m->disable_depth);
    assert(m->process.pr_Task.tc_SigAlloc==m->original_signals&&m->received==(UINT32_C(1)<<7));
}
static void entry_model_retained(unsigned quiet,unsigned allocations,unsigned releases,
    unsigned retained,unsigned os_open,unsigned pending)
{
    struct entry_model *m=&entry_model;unsigned i,j,live=0;
    /* Exact source landmarks: bootstrap control; state/watch/port/IRQ/CIA/
     * scratch; five holders/four sources; ledger/queue =18 allocations.
     * Ended scratch is block6 and releases once before any source acquisition.
     * Only bootstrap block0 remains after positive domain/storage teardown. */
    assert(m->alloc_calls==allocations&&m->free_calls==releases);
    for(i=0;i<32;++i){
        live+=m->block[i].live;
        if(allocations==18U&&releases==1U)
            assert(m->block[i].live==(unsigned)(i<18U&&i!=6U));
        else assert(m->block[i].live==(unsigned)(retained==1U&&i==0U));
    }
    assert(live==retained);
    assert(m->port_calls==2U*os_open&&m->request_calls==2U*os_open);
    assert(m->open_calls==2U*os_open&&!m->close_calls);
    assert(!m->abort_calls&&!m->waitio_calls&&!m->check_calls&&!m->delay_one);
    for(i=0;i<2;++i){
        assert(m->port_live[i]==os_open&&m->request[i].live==os_open);
        assert(m->request[i].opened==os_open&&!m->request[i].complete&&!m->request[i].waited);
        assert(m->request[i].pending==(unsigned)(i==1U&&pending));
    }
    assert(m->process.pr_Task.tc_SigAlloc==(m->original_signals|(os_open?7U:0U)));
    assert(!(m->received&7U));
    assert(m->hold&&m->hold_trace==m->trace_count&&!m->disable_depth&&!m->irq_mode);
    assert(strstr(m->output,"NATIVE RAM ENTRY HOLD")&&!strstr(m->output,"SOFTWARE OWNERSHIP PASS"));
    assert(m->received&(UINT32_C(1)<<7));
    for(i=0;i<2;++i){
        assert((m->resource[i].mask&~3U)==0x14&&(m->resource[i].pending&~3U)==0x14);
        if(quiet)for(j=0;j<2;++j)assert(m->resource[i].vector[j]==NULL);
    }
    if(quiet){
        assert(m->adds==1&&m->removes==1&&m->signals==1&&m->waits==1);
        assert(!(m->resource[0].mask&3U)&&!(m->resource[0].pending&3U));
    }
    /* Disposal of the test arena follows the retained snapshot only. It is
     * not a native quiet/ownership receipt and calls no genuine owner API. */
    for(i=0;i<32;++i)if(m->block[i].live){free(m->block[i].pointer);m->block[i].live=0;}
}
static void entry_model_report_field(const char *line,const char *key,uintptr_t value)
{
    const char *begin=strstr(entry_model.output,line),*found,*end;char needle[96];int length;
    assert(begin);end=strchr(begin,'\n');assert(end);
    length=snprintf(needle,sizeof(needle),"%s%016llx",key,(unsigned long long)value);
    assert(length>0&&(size_t)length<sizeof(needle));
    found=strstr(begin,needle);assert(found&&found<end);
}
static int entry_model_system_case(unsigned which)
{
    struct entry_model *m=&entry_model;char marker=0;
    char name[]="entry-system-model",session[]="abcdef0123456789";
    char *argv[]={name,session,NULL};uintptr_t lower,upper;int result=-1;
    assert(which<ENTRY_MODEL_SYSTEM_CASES);
    entry_model_start(which==7U?EM_NO_FAST:EM_PAL,0,&marker);
    m->stack_group=ENTRY_MODEL_SYSTEM_MODE;m->stack_case=which;
    lower=(uintptr_t)m->exec.SysStkLower;upper=(uintptr_t)m->exec.SysStkUpper;
    assert(lower&&upper>lower+2U&&!(lower&1U)&&!(upper&1U));
    switch(which){
    case 0:m->exec.SysStkLower=NULL;break;
    case 1:m->exec.SysStkUpper=NULL;break;
    case 2:m->exec.SysStkUpper=(void *)(lower-2U);break;
    case 3:m->exec.SysStkUpper=(void *)lower;break;
    case 4:m->exec.SysStkLower=(void *)(UINTPTR_MAX-1U);m->exec.SysStkUpper=(void *)2;break;
    case 5:m->exec.SysStkLower=(void *)(lower+1U);break;
    case 6:m->exec.SysStkUpper=(void *)(upper-1U);break;
    case 7:m->exec.SysStkUpper=(void *)(lower+2U);break;
    default:break; /* 8 positive; 9..16 injected after snapshot in AllocMem. */
    }
    memcpy(protected_before.exec,&m->exec,sizeof(m->exec));
    memcpy(protected_before.system,m->system_storage.bytes,sizeof(protected_before.system));
    if(!setjmp(m->sink))result=pt_entry_model_main(2,argv);
    assert(m->stack_reads==1);
    if(which<7U){
        assert(result==20&&m->available_calls==0);entry_model_no_resources();
    }else if(which==7U){
        /* Tiny ordered span passes shape alone; NO_FAST stops before any
         * allocation or activation. It does not establish stack capacity. */
        assert(result==20&&m->available_calls>0);entry_model_no_resources();
    }else if(which==8U){
        assert(result==0);entry_model_assert_result(result);
    }else{
        assert(result==-1&&!m->free_calls&&!m->adds);
        assert(m->alloc_calls==(which==12U||which==13U?2U:1U));
        if(which==12U||which==13U){
            assert(m->port_calls==2&&m->request_calls==2&&m->open_calls==2&&!m->close_calls);
            assert(m->port_live[0]&&m->port_live[1]);
            assert(m->request[0].live&&m->request[1].live&&m->request[0].opened&&m->request[1].opened);
            assert(m->process.pr_Task.tc_SigAlloc==(m->original_signals|7U)&&!m->signals&&!m->waits);
        }else assert(!m->port_calls&&!m->request_calls&&!m->open_calls);
        if(which<=13U){
            assert(!memcmp(protected_before.exec,&m->exec,sizeof(m->exec)));
            assert(!memcmp(protected_before.system,m->system_storage.bytes,sizeof(protected_before.system)));
        }
        entry_model_retained(0,which==12U||which==13U?2U:1U,0,
            which<=11U?0U:1U,which==12U||which==13U?1U:0U,0);
    }
    printf("NATIVE ENTRY SYSTEM STACK CASE: case=%u result=%d hold=%u trace=%u allocations=%u releases=%u\n",
        which,result,m->hold,m->trace_count,m->alloc_calls,m->free_calls);
    puts("NATIVE ENTRY SYSTEM STACK MODEL PASS: synthetic bounds, protected spans and fixed identities only; no native system-stack qualification");
    puts("NATIVE ENTRY HOST MODEL CASE PASS: software resource oracle only; no native ABI, placement, IRQ, timer or stack qualification");
    return 0;
}
static int entry_model_sample_case(unsigned which)
{
    struct entry_model *m=&entry_model;char marker=0;
    char name[]="entry-sample-model",session[]="abcdef0123456789";
    char *argv[]={name,session,NULL};unsigned i,positive=which==0U||which==1U||which==20U;int result=-1;
    static const unsigned phases[7]={PT_PRIVATE_RAM_STACK_ENTRY,PT_PRIVATE_RAM_STACK_SAVED,
        PT_PRIVATE_RAM_STACK_DISPATCH_BEFORE,PT_PRIVATE_RAM_STACK_DISPATCH_AFTER,
        PT_PRIVATE_RAM_STACK_AFTER_CLOCK,PT_PRIVATE_RAM_STACK_AFTER_SIGNAL,PT_PRIVATE_RAM_STACK_EXIT};
    assert(which<ENTRY_MODEL_SAMPLE_CASES);
    entry_model_start(EM_PAL,0,&marker);m->stack_group=ENTRY_MODEL_SAMPLE_MODE;m->stack_case=which;
    if(!setjmp(m->sink))result=pt_entry_model_main(2,argv);
    assert(m->stack_reads==1&&m->sample_irq==NULL);
    if(positive){
        assert(result==0&&m->task_sample_reads==3&&m->irq_sample_reads==7);
        assert(m->task_sample_phases[0]==2U&&m->task_sample_phases[1]==4U&&m->task_sample_phases[2]==8U);
        for(i=0;i<7;++i)assert(m->irq_sample_phases[i]==phases[i]);
        assert(m->irq_record.version==1&&m->irq_record.status==PT_PRIVATE_RAM_STACK_PHASES&&m->irq_record.samples==7);
        assert(m->irq_record.entry==m->irq_record.exit&&m->irq_record.low<=m->irq_record.entry-48U);
        assert(m->irq_record.low>=m->irq_record.lower&&m->irq_record.entry<m->irq_record.upper);
        assert(m->irq_record.lower==(uintptr_t)m->exec.SysStkLower&&m->irq_record.upper==(uintptr_t)m->exec.SysStkUpper);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED IRQ STACK:"," low=",m->irq_record.low);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED IRQ STACK:"," status=",PT_PRIVATE_RAM_STACK_PHASES);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED IRQ STACK:"," samples=",7);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED TASK STACK:"," low=",m->task_sample_min);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED TASK STACK:"," status=",0);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED TASK STACK:"," phases=",15);
        entry_model_report_field("NATIVE RAM ENTRY SAMPLED TASK STACK:"," samples=",4);
        if(which==1U)assert(m->irq_record.low==m->irq_record.lower);
        if(which==20U)assert(m->task_sample_min==m->live_sp-32U);
        entry_model_assert_result(result);
    }else{
        assert(result==-1);
        if(which==16U){assert(!m->adds&&!m->signals&&!m->irq_sample_reads);entry_model_retained(0,18,1,17,1,1);}
        else{
            if(which>=2U&&which<=5U)assert(m->irq_record.status&PT_PRIVATE_RAM_STACK_BAD_SAMPLE);
            if(which==6U)assert(!m->irq_record.samples&&!m->irq_record.status&&!m->irq_record.entry);
            if(which==7U)assert(!m->irq_record.version&&(m->irq_record.status&PT_PRIVATE_RAM_STACK_BAD_VERSION));
            if(which==8U)assert(m->irq_record.samples==6&&!(m->irq_record.status&PT_PRIVATE_RAM_STACK_AFTER_SIGNAL));
            if(which==9U)assert(m->irq_record.status&PT_PRIVATE_RAM_STACK_BAD_PHASE);
            if(which==10U)assert(m->irq_record.samples==UINT32_MAX&&(m->irq_record.status&PT_PRIVATE_RAM_STACK_BAD_COUNT));
            if(which==11U)assert(m->irq_record.samples==6&&m->irq_record.status==PT_PRIVATE_RAM_STACK_PHASES);
            if(which==12U)assert(m->irq_record.status&PT_PRIVATE_RAM_STACK_BAD_SAMPLE);
            if(which==13U)assert(m->irq_record.lower==(uintptr_t)m->exec.SysStkLower+2U);
            if(which==14U)assert(m->irq_record.exit==m->irq_record.entry+2U);
            if(which==15U)assert(m->irq_record.low==m->irq_record.entry);
            if(which==17U)assert(m->task_sample_reads==2);
            if(which==18U)assert(m->task_sample_reads==3);
            if(which==19U)assert(m->irq_record.lower+2U==(uintptr_t)m->exec.SysStkLower);
            entry_model_retained(1,18,which==18U?17U:1U,which==18U?1U:17U,1,1);
        }
    }
    printf("NATIVE ENTRY SAMPLED STACK CASE: case=%u result=%d hold=%u task_reads=%u IRQ_reads=%u status=%u samples=%u\n",
        which,result,m->hold,m->task_sample_reads,m->irq_sample_reads,m->irq_record.status,m->irq_record.samples);
    puts("NATIVE ENTRY SAMPLED STACK MODEL PASS: seven logical GAS boundary stores and task observations only; no native IRQ, high-water or full stack qualification");
    puts("NATIVE ENTRY HOST MODEL CASE PASS: software resource oracle only; no native ABI, placement, IRQ, timer or stack qualification");
    return 0;
}
int main(int argc,char **argv)
{
    char stack_marker=0;char name[]="entry-model";char session[]="abcdef0123456789";
    char *native_argv[]={name,session,NULL};unsigned mode,ordinal;int result=-1;
    assert(argc==3);mode=(unsigned)strtoul(argv[1],NULL,10);ordinal=(unsigned)strtoul(argv[2],NULL,10);
    if(mode==ENTRY_MODEL_STACK_MODE)return entry_model_task_stack_case(ordinal);
    if(mode==ENTRY_MODEL_SYSTEM_MODE)return entry_model_system_case(ordinal);
    if(mode==ENTRY_MODEL_SAMPLE_MODE)return entry_model_sample_case(ordinal);
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
