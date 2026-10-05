/* Bounded host observation/fault model, not Exec, CIA, native layout or IRQ.
 * Never replaces the production entry/cleanup or any core state machine.
 */
#include "native_ram_entry_host_stubs.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
struct entry_model entry_model;
struct ExecBase *SysBase=&entry_model.exec;
static void note(unsigned e,unsigned i,const void *p,uint64_t v)
{
    struct entry_model *m=&entry_model;
    assert(!m->hold); assert(m->trace_count<4096);
    m->trace[m->trace_count++]=(struct entry_model_trace){e,i,(uintptr_t)p,v};
}
static void task(void)
{assert(!entry_model.irq_mode); assert(entry_model.task==&entry_model.process.pr_Task);}
static unsigned request_index(struct IORequest *p)
{unsigned i;for(i=0;i<2;++i)if(p==&entry_model.request[i].value.tr_node)return i;assert(!"foreign IO request");return 2;}
static unsigned port_index(struct MsgPort *p)
{unsigned i;for(i=0;i<2;++i)if(p==entry_model.port+i)return i;assert(!"foreign message port");return 2;}
static unsigned resource_index(struct Library *p)
{unsigned i;for(i=0;i<2;++i)if(p==&entry_model.resource[i].library)return i;assert(!"foreign CIA resource");return 2;}
static ULONG allocate_signal(void)
{unsigned i;for(i=0;i<32;++i)if(!(entry_model.process.pr_Task.tc_SigAlloc&(UINT32_C(1)<<i)))break;assert(i<32);entry_model.process.pr_Task.tc_SigAlloc|=UINT32_C(1)<<i;return i;}
static void exclusion(void)
{task();assert(entry_model.disable_depth>0);}
void entry_model_start(enum entry_model_mode mode,unsigned ordinal,const void *stack)
{
    struct entry_model *m=&entry_model;unsigned i,j;uintptr_t x=(uintptr_t)stack;
    memset(m,0,sizeof(*m));m->mode=mode;m->ordinal=ordinal;
    m->exec.LibNode.lib_Version=39;m->timer.dd_Library.lib_Version=mode==EM_OLD_DEVICE?35:39;
    m->process.pr_Task.tc_Node.ln_Type=NT_PROCESS;m->process.pr_Task.tc_Node.ln_Pri=3;
    /* Synthetic numeric Process bounds cover host sanitizer stack classes.
     * They are not an OS stack query, capacity measurement or native limit.
     * Production guards only compare this interval; no stack bytes are read.
     */
    assert(x>64U*1024U*1024U&&x<UINTPTR_MAX-64U*1024U*1024U);
    m->process.pr_Task.tc_SPLower=(void *)(x-64U*1024U*1024U);
    m->process.pr_Task.tc_SPUpper=(void *)(x+64U*1024U*1024U);
    m->original_signals=(UINT32_C(1)<<7)|SIGBREAKF_CTRL_C;
    m->process.pr_Task.tc_SigAlloc=m->original_signals;m->task=&m->process.pr_Task;
    m->received=UINT32_C(1)<<7;m->frequency=mode==EM_NTSC?715909:709379;m->ticks=1000;
    m->first=m->ticks+(UINT64_C(4096)*m->frequency+47999)/48000;
    m->last=m->ticks+(UINT64_C(4097)*m->frequency+47999)/48000;
    assert(m->first-1000==(mode==EM_NTSC?61091:60534));
    assert(m->last-1000==(mode==EM_NTSC?61106:60549));
    for(i=0;i<2;++i){
        m->resource[i].library.lib_Version=39;m->resource[i].mask=0x14;m->resource[i].pending=0x14;
        m->hardware[i].ciacra=0x40;m->hardware[i].ciacrb=0x80;
        for(j=0;j<2;++j)if(mode==EM_CIA_BUSY)m->resource[i].vector[j]=&m->foreign[i][j];
    }
}
struct Task *FindTask(void *name){assert(!name);return entry_model.task;}
ULONG AvailMem(ULONG flags)
{task();if(entry_model.mode==EM_NO_FAST&&(flags&MEMF_FAST))return 0;return 8U*1024U*1024U;}
APTR AllocMem(ULONG bytes,ULONG flags)
{
    struct entry_model *m=&entry_model;unsigned i;void *p;
    task();assert(!m->source_exposed);assert(bytes&&flags==(MEMF_FAST|MEMF_PUBLIC));
    ++m->alloc_calls;
    if(m->mode==EM_ALLOC_NULL&&m->alloc_calls==m->ordinal){note(E_ALLOC,m->alloc_calls,NULL,bytes);return NULL;}
    if(m->mode==EM_ALIAS_LIVE&&m->alloc_calls==2){assert(m->block[0].live);note(E_ALLOC,m->alloc_calls,m->block[0].pointer,bytes);return m->block[0].pointer;}
    for(i=0;i<32;++i)if(!m->block[i].live)break;
    assert(i<32);
    p=malloc(bytes);assert(p);memset(p,0xa7,bytes);
    m->block[i]=(struct entry_model_block){p,bytes,1};note(E_ALLOC,m->alloc_calls,p,bytes);return p;
}
void FreeMem(APTR p,ULONG bytes)
{
    struct entry_model *m=&entry_model;unsigned i;
    task();assert(p);
    if(m->source_exposed){unsigned chip,bit;
        for(chip=0;chip<2;++chip)for(bit=0;bit<2;++bit){
            assert(!m->resource[chip].vector[bit]||m->resource[chip].vector[bit]==&m->foreign[chip][bit]);
        }
        assert(!(m->resource[0].mask&3U)&&!(m->resource[0].pending&3U));
    }
    for(i=0;i<32;++i)if(m->block[i].live&&m->block[i].pointer==p)break;
    assert(i<32&&m->block[i].bytes==bytes);note(E_FREE,i,p,bytes);++m->free_calls;
    m->block[i].live=0;free(p);
}
ULONG TypeOfMem(APTR p)
{
    struct entry_model *m=&entry_model;unsigned i;uintptr_t x=(uintptr_t)p;
    task();for(i=0;i<32;++i)if(m->block[i].live){uintptr_t a=(uintptr_t)m->block[i].pointer;
        if(x>=a&&x-a<m->block[i].bytes)return m->mode==EM_BOOTSTRAP_TYPE&&i==0?MEMF_CHIP:MEMF_FAST;
    }
    return 0;
}
struct MsgPort *CreateMsgPort(void)
{
    struct entry_model *m=&entry_model;unsigned i=m->port_calls++;
    task();assert(i<2);note(E_PORT,i,NULL,0);
    if(m->mode==EM_PORT_NULL&&i+1==m->ordinal)return NULL;
    m->port[i].mp_SigBit=(UBYTE)allocate_signal();m->port[i].mp_SigTask=&m->process.pr_Task;m->port_live[i]=1;
    if(m->mode==EM_PORT_WRONG_TASK&&i==0)m->port[i].mp_SigTask=&m->other.pr_Task;
    if(m->mode==EM_PORT_RANGE&&i==0)m->port[i].mp_SigBit=32;
    if(m->mode==EM_PORT_ORIGINAL&&i==0)m->port[i].mp_SigBit=7;
    if(m->mode==EM_PORT_DUPLICATE&&i==1)m->port[i].mp_SigBit=m->port[0].mp_SigBit;
    if(m->mode==EM_TASK_PORT&&i==0)++m->process.pr_Task.tc_Node.ln_Pri;
    return m->port+i;
}
void DeleteMsgPort(struct MsgPort *p)
{
    struct entry_model *m=&entry_model;unsigned i=port_index(p);ULONG bit;
    task();assert(m->port_live[i]&&p->mp_SigTask==m->task&&p->mp_SigBit<32);
    assert(!m->request[i].live);bit=UINT32_C(1)<<p->mp_SigBit;
    assert((bit&m->original_signals)==0&&(m->task->tc_SigAlloc&bit));
    note(E_DELETE_PORT,i,p,bit);m->port_live[i]=0;
    if(m->mode!=EM_DELETE_SIGNAL_STICKY)m->task->tc_SigAlloc&=~bit;
}
APTR CreateIORequest(struct MsgPort *p,ULONG bytes)
{
    struct entry_model *m=&entry_model;unsigned i=port_index(p);
    task();assert(m->port_live[i]&&bytes==sizeof(struct timerequest));++m->request_calls;note(E_REQUEST,i,p,bytes);
    if(m->mode==EM_REQUEST_NULL&&i+1==m->ordinal)return NULL;
    assert(!m->request[i].live);m->request[i].live=1;return &m->request[i].value;
}
void DeleteIORequest(struct IORequest *p)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(m->request[i].live&&!m->request[i].opened&&!m->request[i].pending);
    note(E_DELETE_REQUEST,i,p,0);m->request[i].live=0;
}
LONG OpenDevice(const char *name,ULONG unit,struct IORequest *p,ULONG flags)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(!strcmp(name,TIMERNAME)&&!flags&&m->request[i].live&&!m->request[i].opened);
    assert(unit==(i?UNIT_WAITECLOCK:UNIT_ECLOCK));++m->open_calls;note(E_OPEN,i,p,unit);
    if(m->mode==EM_DEVICE_ERROR&&i+1==m->ordinal)return 5;
    m->request[i].opened=1;m->request[i].unit=unit;
    p->io_Device=m->mode==EM_DEVICE_NO_IDENTITY&&i==0?NULL:&m->timer;return 0;
}
void CloseDevice(struct IORequest *p)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(m->request[i].live&&m->request[i].opened&&!m->request[i].pending);
    note(E_CLOSE,i,p,0);m->request[i].opened=0;++m->close_calls;
}
void SendIO(struct IORequest *p)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(i==1&&m->request[i].opened&&!m->request[i].pending&&p->io_Command==TR_ADDREQUEST);
    note(E_SEND,i,p,0);m->request[i].pending=1;
}
struct IORequest *CheckIO(struct IORequest *p)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(i==1&&m->request[i].pending);++m->check_calls;note(E_CHECK,i,p,0);
    if(m->mode==EM_FOREIGN_CHECKIO)return &m->request[0].value.tr_node;
    if(m->mode==EM_ALREADY_COMPLETE)m->request[i].complete=1;
    return m->request[i].complete?p:NULL;
}
void AbortIO(struct IORequest *p)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(i==1&&m->request[i].pending&&!m->request[i].complete&&m->abort_calls==0);
    assert(m->trace_count&&m->trace[m->trace_count-1].event==E_CHECK);++m->abort_calls;note(E_ABORT,i,p,0);
    if(m->mode!=EM_ABORT_PENDING){m->request[i].complete=1;p->io_Error=IOERR_ABORTED;}
}
LONG WaitIO(struct IORequest *p)
{
    struct entry_model *m=&entry_model;unsigned i=request_index(p);
    task();assert(i==1&&m->request[i].pending&&m->request[i].complete&&!m->request[i].waited);
    assert(m->trace_count&&m->trace[m->trace_count-1].event==E_CHECK);++m->waitio_calls;note(E_WAITIO,i,p,0);
    m->request[i].pending=0;m->request[i].waited=1;
    if(m->mode==EM_TASK_WAITIO)++m->process.pr_Task.tc_Node.ln_Pri;
    return m->mode==EM_WAIT_ERROR?5:p->io_Error;
}
LONG AllocSignal(LONG requested)
{
    struct entry_model *m=&entry_model;ULONG bit;
    task();assert(requested==-1);note(E_ALLOC_SIGNAL,0,NULL,0);
    if(m->mode==EM_SIGNAL_NULL)return -1;
    if(m->mode==EM_SIGNAL_DUPLICATE)return m->port[0].mp_SigBit;
    if(m->mode==EM_SIGNAL_UNALLOCATED)return 2;
    bit=allocate_signal();return (LONG)bit;
}
void FreeSignal(LONG bit)
{
    struct entry_model *m=&entry_model;ULONG mask;
    task();assert(bit>=0&&bit<32);mask=UINT32_C(1)<<bit;
    assert((mask&m->original_signals)==0&&(m->task->tc_SigAlloc&mask));
    note(E_FREE_SIGNAL,(unsigned)bit,NULL,mask);m->task->tc_SigAlloc&=~mask;
}
ULONG SetSignal(ULONG value,ULONG mask)
{ULONG old=entry_model.received;task();assert((mask&entry_model.original_signals)==0);note(E_SET_SIGNAL,0,NULL,mask);entry_model.received=(old&~mask)|(value&mask);return old;}
void Signal(struct Task *t,ULONG mask)
{assert(t==&entry_model.process.pr_Task&&mask&&!(mask&entry_model.original_signals));note(E_SIGNAL,0,t,mask);++entry_model.signals;entry_model.received|=mask;}
ULONG Wait(ULONG mask)
{
    struct entry_model *m=&entry_model;unsigned i,j;ULONG answer;
    task();assert(!m->disable_depth&&!m->waits);++m->waits;note(E_WAIT,0,NULL,mask);
    if(m->mode==EM_TERMINATION_ONLY){m->request[1].complete=1;m->received|=UINT32_C(1)<<m->port[1].mp_SigBit;}
    else{
        for(i=0;i<2;++i)for(j=0;j<2;++j)if(m->resource[i].vector[j]&&m->resource[i].vector[j]!=&m->foreign[i][j]){
            assert(m->resource[i].vector[j]->is_Code==pt_private_native_ram_irq);m->irq_mode=1;
            m->ticks=m->mode==EM_WINDOW_SKIP?m->last:m->first-128;
            m->resource[i].vector[j]->is_Code();m->irq_mode=0;
        }
    }
    if(m->mode==EM_CTRL_C)m->received|=SIGBREAKF_CTRL_C;
    answer=m->received&mask;m->received&=~answer;return answer;
}
void Delay(ULONG ticks)
{
    struct entry_model *m=&entry_model;assert(!m->irq_mode&&!m->disable_depth);
    if(ticks==50){note(E_HOLD,0,NULL,0);m->hold_trace=m->trace_count;m->hold=1;longjmp(m->sink,1);}
    assert(ticks==1);assert(m->delay_one<50);++m->delay_one;note(E_DELAY,0,NULL,ticks);
}
BPTR Output(void){return 1;}
LONG Write(BPTR file,const void *bytes,LONG n)
{
    struct entry_model *m=&entry_model;assert(!m->irq_mode&&!m->disable_depth&&file==1&&n>=0);
    assert((size_t)n<sizeof(m->output)-m->output_bytes);memcpy(m->output+m->output_bytes,bytes,(size_t)n);
    m->output_bytes+=(size_t)n;m->output[m->output_bytes]=0;return n;
}
void Disable(void)
{assert(!entry_model.irq_mode);++entry_model.disable_depth;if(entry_model.maximum_depth<entry_model.disable_depth)entry_model.maximum_depth=entry_model.disable_depth;note(E_DISABLE,0,NULL,entry_model.disable_depth);}
void Enable(void)
{assert(!entry_model.irq_mode&&entry_model.disable_depth);note(E_ENABLE,0,NULL,entry_model.disable_depth);--entry_model.disable_depth;}
void *OpenResource(const char *name)
{unsigned i;exclusion();if(!strcmp(name,"ciab.resource"))i=0;else{assert(!strcmp(name,"ciaa.resource"));i=1;}note(E_RESOURCE,i,NULL,0);return &entry_model.resource[i].library;}
struct Interrupt *AddICRVector(struct Library *r,WORD bit,struct Interrupt *server)
{
    struct entry_model *m=&entry_model;unsigned i=resource_index(r);struct Interrupt *old;
    exclusion();assert(bit>=0&&bit<2&&server);note(E_ADD,i,server,(unsigned)bit);++m->adds;
    old=m->resource[i].vector[bit];if(old)return old;
    m->resource[i].vector[bit]=server;m->resource[i].mask|=1U<<bit;m->source_exposed=1;return NULL;
}
void RemICRVector(struct Library *r,WORD bit,struct Interrupt *server)
{
    struct entry_model *m=&entry_model;unsigned i=resource_index(r);
    exclusion();assert(bit>=0&&bit<2&&m->resource[i].vector[bit]==server);
    assert(!(m->resource[i].mask&(1U<<bit))&&!(m->resource[i].pending&(1U<<bit)));
    note(E_REMOVE,i,server,(unsigned)bit);++m->removes;m->resource[i].vector[bit]=NULL;
    if(m->mode==EM_CIA_CLOSE_UNCERTAIN)m->resource[i].pending|=1U<<bit;
}
WORD AbleICR(struct Library *r,WORD mask)
{
    struct entry_model *m=&entry_model;unsigned i=resource_index(r),v=(unsigned)(UWORD)mask,old=m->resource[i].mask;
    exclusion();note(E_MASK,i,NULL,v);
    if(v&0x80)m->resource[i].mask|=v&0x1f;else if(v)m->resource[i].mask&=~(v&0x1f);
    if(m->mode==EM_CIA_ACQUIRE_UNCERTAIN&&v==1)m->resource[i].mask|=1;
    return (WORD)old;
}
WORD SetICR(struct Library *r,WORD mask)
{
    struct entry_model *m=&entry_model;unsigned i=resource_index(r),v=(unsigned)(UWORD)mask,old=m->resource[i].pending;
    exclusion();note(E_PENDING,i,NULL,v);
    if(v&0x80)m->resource[i].pending|=v&0x1f;else if(v)m->resource[i].pending&=~(v&0x1f);
    return (WORD)old;
}
volatile struct CIA *entry_model_hardware(unsigned i){assert(i<2);return entry_model.hardware+i;}
void entry_model_control_write(volatile UBYTE *p,unsigned value)
{
    unsigned i,j;exclusion();for(i=0;i<2;++i)for(j=0;j<2;++j){volatile UBYTE *q=j?&entry_model.hardware[i].ciacrb:&entry_model.hardware[i].ciacra;
        if(p==q){note(E_CONTROL,i,(const void *)p,value);*p=(UBYTE)value;
            if(entry_model.mode==EM_CIA_ARM_UNCERTAIN&&(value&1U))*p&=(UBYTE)~1U;
            return;
        }
    }
    assert(!"unknown CIA control address");
}
ULONG entry_model_read_eclock(struct Device *timer,struct EClockVal *out)
{
    struct entry_model *m=&entry_model;uint64_t now=m->ticks;
    assert(timer==&m->timer&&out);note(E_CLOCK,0,timer,now);
    if(m->mode==EM_LATE_PREPARE&&m->request[1].pending&&!m->source_exposed)now=m->first-128;
    out->ev_hi=(ULONG)(now>>32);out->ev_lo=(ULONG)now;
    if(m->irq_mode)m->ticks+=4;
    return m->mode==EM_FREQUENCY?12345:m->frequency;
}
/* C simulation of the logical prefix/dispatch/signal sequence ONLY. The GAS
 * body, library-vector calling convention, offsets and interrupt stack do not
 * execute here. Registered actual is_Data supplies the genuine port/ledger.
 */
void pt_private_native_ram_irq(void)
{
    struct entry_model *m=&entry_model;struct pt_private_ram_irq *irq=NULL;unsigned i,j;struct EClockVal v;
    assert(m->irq_mode);for(i=0;i<2;++i)for(j=0;j<2;++j)if(m->resource[i].vector[j]&&m->resource[i].vector[j]!=&m->foreign[i][j]){
        assert(!irq);irq=m->resource[i].vector[j]->is_Data;
    }
    assert(irq&&irq->armed&&irq->task==&m->process.pr_Task);
    irq->before_frequency=entry_model_read_eclock(irq->timer,&v);irq->before.hi=v.ev_hi;irq->before.lo=v.ev_lo;
    irq->dispatch_result=pt_private_native_ram_dispatch(irq);
    irq->after_frequency=entry_model_read_eclock(irq->timer,&v);irq->after.hi=v.ev_hi;irq->after.lo=v.ev_lo;
    irq->armed=0;Signal(irq->task,irq->signal);
}
void entry_model_assert_result(int result)
{
    struct entry_model *m=&entry_model;unsigned i,j,live=0,ports=0,requests=0;int held;
    held=m->mode==EM_PORT_WRONG_TASK||m->mode==EM_PORT_RANGE||m->mode==EM_PORT_ORIGINAL||m->mode==EM_PORT_DUPLICATE||
        m->mode==EM_SIGNAL_DUPLICATE||m->mode==EM_SIGNAL_UNALLOCATED||m->mode==EM_DEVICE_NO_IDENTITY||m->mode==EM_TASK_PORT||
        m->mode==EM_TASK_WAITIO||m->mode==EM_ALIAS_LIVE||m->mode==EM_BOOTSTRAP_TYPE||m->mode==EM_FOREIGN_CHECKIO||
        m->mode==EM_ABORT_PENDING||m->mode==EM_WAIT_ERROR||m->mode==EM_DELETE_SIGNAL_STICKY||m->mode==EM_CIA_ACQUIRE_UNCERTAIN||
        m->mode==EM_CIA_ARM_UNCERTAIN||m->mode==EM_CIA_CLOSE_UNCERTAIN||m->mode==EM_WINDOW_SKIP||m->mode==EM_CTRL_C||m->mode==EM_TERMINATION_ONLY;
    assert(m->hold==(unsigned)held&&!m->disable_depth&&!m->irq_mode);
    assert(m->received&(UINT32_C(1)<<7)); /* unrelated notification is untouched */
    for(i=0;i<32;++i)live+=m->block[i].live;
    for(i=0;i<2;++i){ports+=m->port_live[i];requests+=m->request[i].live;
        assert((m->resource[i].mask&~3U)==0x14&&(m->resource[i].pending&~3U)==0x14);
        for(j=0;j<2;++j)if(m->mode==EM_CIA_BUSY)assert(m->resource[i].vector[j]==&m->foreign[i][j]);
    }
    if(!held){
        assert(!live&&!ports&&!requests&&m->task->tc_SigAlloc==m->original_signals);
        assert(m->free_calls==m->alloc_calls-(m->mode==EM_ALLOC_NULL?1U:0U));
        assert(result==((m->mode==EM_PAL||m->mode==EM_NTSC||m->mode==EM_ALREADY_COMPLETE)?0:20));
        if(!result){assert(m->removes==1&&m->signals==1&&m->waits==1);assert(strstr(m->output,"SOFTWARE OWNERSHIP PASS"));}
        else assert(!m->signals);
    }else{
        assert(m->hold_trace==m->trace_count&&strstr(m->output,"NATIVE RAM ENTRY HOLD"));
        assert(live||ports||requests);assert(!strstr(m->output,"SOFTWARE OWNERSHIP PASS"));
    }
    if(m->mode==EM_ABORT_PENDING)assert(m->abort_calls==1&&m->waitio_calls==0&&m->delay_one==50&&m->request[1].pending);
    if(m->mode==EM_FOREIGN_CHECKIO)assert(!m->abort_calls&&!m->waitio_calls);
    if(m->mode==EM_WAIT_ERROR||m->mode==EM_TASK_WAITIO)assert(m->abort_calls==1&&m->waitio_calls==1);
    if(m->mode==EM_ALREADY_COMPLETE)assert(!m->abort_calls&&m->waitio_calls==1);
    if(m->mode==EM_ALIAS_LIVE)assert(m->alloc_calls==2&&!m->free_calls);
    if(m->mode==EM_CIA_ACQUIRE_UNCERTAIN)assert(m->adds==1&&!m->removes);
    if(m->mode==EM_CIA_ARM_UNCERTAIN||m->mode==EM_CIA_CLOSE_UNCERTAIN)assert(m->adds==1&&m->removes==1);
    if(m->mode==EM_CIA_BUSY)assert(m->adds==4&&!m->removes);
    /* Arena disposal below is test harness disposal AFTER the retained-owner
     * snapshot, not an entry cleanup receipt or a protocol permission to free.
     * No genuine owner API is called after observing the permanent hold sink.
     */
    for(i=0;i<32;++i)if(m->block[i].live){free(m->block[i].pointer);m->block[i].live=0;}
}
