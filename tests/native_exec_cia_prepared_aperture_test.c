#include "native_exec_memory.h"
#include "../src/native/eclock_alarm.h"
#include "native_cia_owner.h"
#include "cia_prepared_aperture.h"
#include <proto/dos.h>
#include <stddef.h>
#include <string.h>
#define PT_PREPARED_APERTURE_EMBEDDED
#include "cia_prepared_aperture_test.c"
/* Private one-shot experiment payload, NOT an editor/provider or queue ABI.
 * Only this task finalizes descriptor/key under Disable, before arm. It does
 * not modify descriptor/result/reads while the owned handler may execute. */
struct prepared_irq {
    struct Device *timer;struct Task *task;ULONG signal;
    volatile ULONG armed,calls,busy;
    struct pt_prepared_clock descriptor;
    struct pt_prepared_result result;
    struct pt_prepared_read samples[64];
};
#define OFFSET(type,field,n) typedef char offset_##type##_##field[(offsetof(struct type,field)==n)?1:-1]
OFFSET(prepared_irq,timer,0);OFFSET(prepared_irq,task,4);OFFSET(prepared_irq,signal,8);
OFFSET(prepared_irq,armed,12);OFFSET(prepared_irq,calls,16);OFFSET(prepared_irq,busy,20);
OFFSET(prepared_irq,descriptor,24);OFFSET(prepared_irq,result,52);OFFSET(prepared_irq,samples,80);
OFFSET(pt_prepared_clock,arm_hi,0);OFFSET(pt_prepared_clock,arm_lo,4);
OFFSET(pt_prepared_clock,first_offset,8);OFFSET(pt_prepared_clock,last_offset,12);
OFFSET(pt_prepared_clock,frequency,16);OFFSET(pt_prepared_clock,read_cap,20);OFFSET(pt_prepared_clock,gate,24);
OFFSET(pt_prepared_result,result,0);OFFSET(pt_prepared_result,reads,4);OFFSET(pt_prepared_result,commits,8);
OFFSET(pt_prepared_result,shadow,12);OFFSET(pt_prepared_result,before_index,16);
OFFSET(pt_prepared_result,after_index,20);OFFSET(pt_prepared_result,reentry,24);
OFFSET(pt_prepared_read,hi,0);OFFSET(pt_prepared_read,lo,4);OFFSET(pt_prepared_read,frequency,8);
typedef char read_stride[(sizeof(struct pt_prepared_read)==12)?1:-1];
typedef char result_stride[(sizeof(struct pt_prepared_result)==28)?1:-1];
typedef char descriptor_stride[(sizeof(struct pt_prepared_clock)==28)?1:-1];
extern void pt_diagnostic_cia_prepared_aperture_irq(void);
struct prepared_trace {
    struct pt_aperture_state original;struct pt_aperture_control control;
    struct pt_prepared_clock descriptor;struct pt_prepared_result result;
    struct pt_prepared_read samples[64];
    uint64_t arm_target,arm_observed,arm_after;
    uint32_t arm_frequency,arm_after_frequency;
    unsigned mode,phase,admitted,arm_count,arm_valid,arm_after_valid,delivered,replay_ok;
    ULONG wake,calls,busy;
};
struct prepared_arm_reader {struct pt_native_eclock *clock;uint64_t actual;uint32_t frequency;unsigned valid,reads;};
static int prepared_arm_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct prepared_arm_reader *r=context;int ok;
    *ticks=0;*frequency=0;++r->reads;ok=pt_native_eclock_read(r->clock,ticks,frequency);
    r->actual=*ticks;r->frequency=*frequency;r->valid=ok?1U:0U;return ok;
}
static int prepared_hold(const char *reason) __attribute__((noreturn));
static int prepared_hold(const char *reason)
{
    printf("CIA PREPARED HOLD: %s; live task/timer/IRQ storage retained for explicit recovery\n",reason);
    fflush(stdout);for(;;)Delay(50);__builtin_unreachable();
}
#define HI(value) ((unsigned long)((value)>>32))
#define LO(value) ((unsigned long)(value))
/* Caller excludes its handler. Copying a trace alone never proves closure. */
static void prepared_snapshot(struct prepared_trace *e,const struct prepared_irq *irq)
{
    if(e->phase<2)return;
    e->descriptor=irq->descriptor;e->result=irq->result;e->calls=irq->calls;e->busy=irq->busy;
    memcpy(e->samples,irq->samples,sizeof(e->samples));
}
static void prepared_report(const struct prepared_trace *trace,unsigned attempted,unsigned closed)
{
    unsigned i,j;
    for(i=0;i<attempted;++i) {
        const struct prepared_trace *e=trace+i;const struct pt_prepared_result *r=&e->result;
        printf("CIA PREPARED OBS index=%u mode=%u phase=%u admitted=%u arm_valid=%u arm_after_valid=%u delivered=%u replay=%u closed=%u first=%lu:%lu last=%lu:%lu arm_target=%lu:%lu arm_observed=%lu:%lu arm_after=%lu:%lu arm_frequency=%lu/%lu count=%u gate=%lu result=%lu reads=%lu commits=%lu shadow=%lu before_index=%lu after_index=%lu reentry=%lu wake=%lu calls=%lu busy=%lu samples=",
            i,e->mode,e->phase,e->admitted,e->arm_valid,e->arm_after_valid,e->delivered,e->replay_ok,closed,
            HI(e->original.first),LO(e->original.first),HI(e->original.last),LO(e->original.last),
            HI(e->arm_target),LO(e->arm_target),HI(e->arm_observed),LO(e->arm_observed),HI(e->arm_after),LO(e->arm_after),
            (unsigned long)e->arm_frequency,(unsigned long)e->arm_after_frequency,e->arm_count,
            (unsigned long)e->descriptor.gate,(unsigned long)r->result,(unsigned long)r->reads,
            (unsigned long)r->commits,(unsigned long)r->shadow,(unsigned long)r->before_index,(unsigned long)r->after_index,
            (unsigned long)r->reentry,(unsigned long)e->wake,(unsigned long)e->calls,(unsigned long)e->busy);
        for(j=0;j<r->reads && j<64;++j)printf("%s%lu:%lu:%lu",j?",":"",(unsigned long)e->samples[j].hi,(unsigned long)e->samples[j].lo,(unsigned long)e->samples[j].frequency);
        puts("");
    }
    fflush(stdout);
}
int main(void)
{
    struct Task *task=FindTask(NULL);BYTE priority=task?task->tc_Node.ln_Pri:0;
    ULONG signals=task?task->tc_SigAlloc:0,termination_mask=0;
    struct pt_native_eclock clock={0};struct pt_native_alarm termination={0};
    struct pt_diagnostic_cia owner={0};struct prepared_irq *irq=NULL;struct prepared_trace *trace=NULL;
    struct pt_elapsed_clock epoch;struct pt_aperture_policy policy={128,256,64};
    uint64_t now=0,deadline=0;uint32_t frequency=0,actual_frequency=0;
    BYTE signal=-1;unsigned i,attempted=0,completed=0,positive=0,negative=0,failures=0,acquired=0,chip=0,bit=0;int result=0;
#define CHECK(condition) do{if(!(condition)){printf("CIA PREPARED FAIL line=%u completed=%u\n",(unsigned)__LINE__,completed);result=20;goto done;}}while(0)
    CHECK(task && cia_prepared_aperture_fixture()==0);native_memory_start();
    irq=native_allocate(sizeof(*irq));trace=native_allocate(16*sizeof(*trace));
    memset(irq,0,sizeof(*irq));memset(trace,0,16*sizeof(*trace));
    CHECK(pt_native_eclock_open(&clock) && clock.port->mp_SigTask==task);
    CHECK(pt_native_alarm_open(&termination) && termination.port->mp_SigTask==task && termination.port->mp_SigBit<32);
    signal=AllocSignal(-1);CHECK(signal>=0 && signal<32);
    irq->signal=1UL<<signal;termination_mask=1UL<<termination.port->mp_SigBit;
    CHECK(!(irq->signal&termination_mask) && !((irq->signal|termination_mask)&SIGBREAKF_CTRL_C));
    irq->timer=clock.request->tr_node.io_Device;irq->task=task;
    CHECK(pt_diagnostic_cia_acquire(&owner,pt_diagnostic_cia_prepared_aperture_irq,irq));
    acquired=1;chip=owner.chip;bit=owner.bit;
    CHECK(pt_native_eclock_read(&clock,&now,&frequency) && (frequency==709379 || frequency==715909));
    CHECK(pt_elapsed_clock_init(&epoch,frequency,48000,now,0)==PT_ELAPSED_OK);
    CHECK(pt_elapsed_clock_deadline(&epoch,60000,&deadline)==PT_ELAPSED_OK);
    CHECK(pt_native_alarm_arm(&termination,deadline)==PT_ALARM_WAITING);
    for(i=0;i<16;++i) {
        struct prepared_trace *e=trace+i;struct pt_prepared_result replay;
        struct prepared_arm_reader arm_reader={&clock,0,0,0,0};
        uint64_t observed=0;unsigned count=0;int armed,read_ok;ULONG wake;
        CHECK(!(SetSignal(0,0)&(SIGBREAKF_CTRL_C|termination_mask)));
        e->control=prepared_control();e->control.key.trigger=i+1;e->control.key.serial=i+1;e->control.publication=i+1;
        e->mode=i<12?0:i-11;e->phase=1;attempted=i+1;
        CHECK(pt_aperture_prepare(&epoch,(i+1)*1536UL,&policy,&e->control,&e->original)==PT_APERTURE_READY);
        e->arm_target=e->original.arm_at;
        if(i==12)++e->control.key.serial;
        if(i==13)e->control.cancelled=1;
        if(i==14)e->arm_target=e->original.last+128;
        if(i==15)e->control.armed=0;
        /* Explicit private ownership/exclusion covers bind, publication, arm,
         * and complete handler lifetime. No external key reader can mutate it. */
        Disable();SetSignal(0,irq->signal);
        e->admitted=(unsigned)pt_prepared_clock_bind(&e->original,&e->control,&e->descriptor);
        if(e->admitted!=(i==12 || i==13 || i==15?PT_APERTURE_STALE:PT_APERTURE_READY) || e->descriptor.read_cap!=64)
            {Enable();CHECK(0);}
        irq->descriptor=e->descriptor;memset(&irq->result,0,sizeof(irq->result));memset(irq->samples,0,sizeof(irq->samples));
        irq->armed=1;e->phase=2;
        armed=pt_diagnostic_cia_arm_at(&owner,prepared_arm_clock,&arm_reader,e->arm_target,frequency,&observed,&count);
        e->arm_observed=arm_reader.actual;e->arm_frequency=arm_reader.frequency;
        if(!armed){irq->armed=0;Enable();CHECK(0);}
        e->arm_count=count;e->arm_valid=1;e->phase=3;
        now=0;actual_frequency=0;read_ok=pt_native_eclock_read(&clock,&now,&actual_frequency);
        e->arm_after=now;e->arm_after_frequency=actual_frequency;e->arm_after_valid=read_ok?1U:0U;
        if(!read_ok){irq->armed=0;Enable();CHECK(0);}
        e->phase=4;Enable();
        CHECK(actual_frequency==frequency && arm_reader.valid && arm_reader.reads==1 && arm_reader.frequency==frequency && observed==e->arm_observed && count==e->arm_target-e->arm_observed);
        e->phase=5;wake=Wait(irq->signal|termination_mask|SIGBREAKF_CTRL_C);e->wake=wake;
        CHECK(FindTask(NULL)==task && task->tc_Node.ln_Pri==priority);
        CHECK(!(wake&(termination_mask|SIGBREAKF_CTRL_C)) && (wake&irq->signal));
        Disable();prepared_snapshot(e,irq);e->delivered=1;e->phase=6;Enable();
        CHECK(!irq->armed && !e->busy && e->calls==i+1 && !e->result.reentry && e->result.reads<=64);
        e->replay_ok=pt_prepared_replay(&e->descriptor,e->samples,e->result.reads,0,&replay) && !memcmp(&replay,&e->result,sizeof(replay));
        CHECK(e->replay_ok);++completed;
        if(i<12) {if(e->result.result==PT_APERTURE_COMMITTED && e->result.commits==1 && e->result.shadow==PT_APERTURE_SHADOW)++positive;else ++failures;}
        else {if(!e->result.commits && !e->result.shadow && e->result.result==(i==14?PT_APERTURE_EXPIRED:PT_APERTURE_STALE))++negative;else ++failures;}
    }
    if(positive!=12 || negative!=4 || failures)result=20;
done:
    if(!pt_diagnostic_cia_close(&owner)) {
        if(irq && attempted){Disable();prepared_snapshot(trace+attempted-1,irq);Enable();}
        prepared_report(trace,attempted,0);return prepared_hold("owned timer/vector close unresolved");
    }
    if(irq && attempted){
        struct prepared_trace *e=trace+attempted-1;
        Disable();
        if(e->delivered && (e->calls!=irq->calls || e->busy!=irq->busy ||
           memcmp(&e->result,&irq->result,sizeof(e->result)) || memcmp(e->samples,irq->samples,sizeof(e->samples)))) {
            result=20;e->replay_ok=0;
        }
        prepared_snapshot(e,irq);Enable();
    }
    prepared_report(trace,attempted,1);
    if(irq)irq->armed=0;
    for(i=0;i<50 && termination.pending;++i){pt_native_alarm_cancel(&termination);if(termination.pending)Delay(1);}
    if(termination.pending)return prepared_hold("independent termination IO unresolved");
    if(termination_mask)SetSignal(0,termination_mask);
    if(!pt_native_alarm_close(&termination))return prepared_hold("termination port closure unresolved");
    pt_native_eclock_close(&clock);
    if(signal>=0){SetSignal(0,1UL<<signal);FreeSignal(signal);}
    if(!task || FindTask(NULL)!=task || task->tc_Node.ln_Pri!=priority || task->tc_SigAlloc!=signals ||
       clock.port || clock.request || termination.port || termination.request || owner.held || owner.resource)
        return prepared_hold("task/signals/resource closure unresolved");
    printf("CIA PREPARED CLOSED acquired=%u chip=%u bit=%u vector_removed=%u timer_stopped=%u task_identity=1 priority=%d unchanged=1 signals_restored=1 attempted=%u completed=%u positive=%u negative=%u failures=%u frequency=%lu early_ticks=128 residency_ticks=256 reads_cap=64\n",
        acquired,chip,bit,acquired,acquired,(int)priority,attempted,completed,positive,negative,failures,(unsigned long)frequency);
    native_release(trace);native_release(irq);if(native_started)native_memory_finish();
    if(!result)puts("CIA PREPARED PASS: original-frame direct-clock RAM brackets and serialized zero-effect negative gates; diagnostic only, no DMA or audio proof");
    return result;
#undef CHECK
}
