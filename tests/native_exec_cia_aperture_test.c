#include "native_exec_memory.h"
#include "../src/native/eclock_alarm.h"
#include "native_cia_owner.h"
#include "cia_aperture_model.h"
#include <proto/dos.h>
#include <stddef.h>
#include <string.h>
#define PT_APERTURE_EMBEDDED
#include "cia_aperture_test.c"
struct native_reader {struct Device *timer;};
struct native_irq {
    struct native_reader reader;struct Task *task;ULONG signal;
    volatile ULONG armed,calls;struct pt_aperture_state *state;
    struct pt_aperture_control *control;
    volatile struct EClockVal dispatch_before;volatile ULONG dispatch_before_frequency;
    volatile struct EClockVal dispatch_after;volatile ULONG dispatch_after_frequency;
};
#define OFFSET(field,n) typedef char irq_offset_##field[(offsetof(struct native_irq,field)==n)?1:-1]
OFFSET(reader,0);OFFSET(task,4);OFFSET(signal,8);OFFSET(armed,12);
OFFSET(calls,16);OFFSET(state,20);OFFSET(control,24);
OFFSET(dispatch_before,28);OFFSET(dispatch_before_frequency,36);
OFFSET(dispatch_after,40);OFFSET(dispatch_after_frequency,48);
extern void pt_diagnostic_cia_aperture_irq(void);
static int native_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct native_reader *r=context;struct Device *TimerBase=r->timer;struct EClockVal value={0};ULONG rate;
    rate=ReadEClock(&value);*ticks=((uint64_t)value.ev_hi<<32)|value.ev_lo;*frequency=rate;
    return rate?1:0;
}
void pt_diagnostic_cia_aperture_dispatch(struct native_irq *irq)
{
    ++irq->calls;
    pt_aperture_run(irq->state,irq->control,native_clock,&irq->reader,sizeof(irq->reader));
}
struct native_trace {
    struct pt_aperture_state state;struct pt_aperture_control control;
    uint64_t arm_target,arm_observed,arm_after,dispatch_before,dispatch_after;
    unsigned arm_count,mode,phase,prepared,arm_valid,arm_after_valid,delivered;
    ULONG wake,calls,before_frequency,after_frequency;
    uint32_t arm_frequency,arm_after_frequency;unsigned arm_read_valid,arm_reads;
};
struct task_arm_reader {struct pt_native_eclock *clock;uint64_t actual;uint32_t frequency;unsigned valid,reads;};
static int task_arm_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct task_arm_reader *r=context;int ok;
    *ticks=0;*frequency=0;++r->reads;ok=pt_native_eclock_read(r->clock,ticks,frequency);
    r->actual=*ticks;r->frequency=*frequency;r->valid=ok?1U:0U;return ok;
}
static int aperture_hold(const char *reason) __attribute__((noreturn));
static int aperture_hold(const char *reason)
{
    printf("CIA APERTURE HOLD: %s; live task/timer/IRQ storage retained for explicit recovery\n",reason);
    fflush(stdout);for(;;)Delay(50);__builtin_unreachable();
}
#define HI(value) ((unsigned long)((value)>>32))
#define LO(value) ((unsigned long)(value))
/* Caller excludes the owned handler while copying. An unresolved-close record
 * is only this snapshot, never proof that the handler cannot run afterward. */
static void trace_snapshot(struct native_trace *entry,const struct native_irq *irq)
{
    if(irq->state!=&entry->state)return;
    entry->calls=irq->calls;
    entry->dispatch_before=((uint64_t)irq->dispatch_before.ev_hi<<32)|irq->dispatch_before.ev_lo;
    entry->dispatch_after=((uint64_t)irq->dispatch_after.ev_hi<<32)|irq->dispatch_after.ev_lo;
    entry->before_frequency=irq->dispatch_before_frequency;entry->after_frequency=irq->dispatch_after_frequency;
}
static void trace_report(const struct native_trace *trace,unsigned attempted,unsigned closed)
{
    unsigned i;
    for(i=0;i<attempted;++i) {
        const struct native_trace *e=trace+i;const struct pt_aperture_state *s=&e->state;
        unsigned dispatch_valid=e->before_frequency==s->frequency && e->after_frequency==s->frequency && s->frequency && e->dispatch_after>=e->dispatch_before;
        unsigned aperture_valid=s->entry_valid && s->last_read_valid && s->entry_frequency==s->frequency && s->last_read_frequency==s->frequency && s->last_read>=s->entry;
        printf("CIA APERTURE OBS index=%u mode=%u phase=%u prepared=%u arm_valid=%u arm_after_valid=%u delivered=%u closed=%u result=%u first=%lu:%lu last=%lu:%lu arm_target=%lu:%lu arm_observed=%lu:%lu arm_after=%lu:%lu arm_frequency=%lu/%lu arm_read_valid=%u arm_reads=%u count=%u entry=%lu:%lu before=%lu:%lu after=%lu:%lu last_observed=%lu:%lu last_read=%lu:%lu last_read_frequency=%lu last_read_valid=%u valid=%u/%u/%u reads=%u commits=%u shadow=%lu aperture_span_valid=%u aperture_span=%lu:%lu dispatch_span_valid=%u dispatch_before=%lu:%lu dispatch_after=%lu:%lu dispatch_span=%lu:%lu dispatch_frequency=%lu/%lu wake=%lu calls=%lu\n",
            i,e->mode,e->phase,e->prepared,e->arm_valid,e->arm_after_valid,e->delivered,closed,(unsigned)s->result,
            HI(s->first),LO(s->first),HI(s->last),LO(s->last),HI(e->arm_target),LO(e->arm_target),HI(e->arm_observed),LO(e->arm_observed),HI(e->arm_after),LO(e->arm_after),(unsigned long)e->arm_frequency,(unsigned long)e->arm_after_frequency,e->arm_read_valid,e->arm_reads,e->arm_count,
            HI(s->entry),LO(s->entry),HI(s->before),LO(s->before),HI(s->after),LO(s->after),HI(s->last_observed),LO(s->last_observed),HI(s->last_read),LO(s->last_read),
            (unsigned long)s->last_read_frequency,s->last_read_valid,s->entry_valid,s->before_valid,s->after_valid,s->reads,s->commits,(unsigned long)s->shadow,
            aperture_valid,HI(aperture_valid?s->last_read-s->entry:0),LO(aperture_valid?s->last_read-s->entry:0),
            dispatch_valid,HI(e->dispatch_before),LO(e->dispatch_before),HI(e->dispatch_after),LO(e->dispatch_after),
            HI(dispatch_valid?e->dispatch_after-e->dispatch_before:0),LO(dispatch_valid?e->dispatch_after-e->dispatch_before:0),
            (unsigned long)e->before_frequency,(unsigned long)e->after_frequency,(unsigned long)e->wake,(unsigned long)e->calls);
    }
    fflush(stdout);
}
int main(void)
{
    struct Task *task=FindTask(NULL);BYTE priority=task?task->tc_Node.ln_Pri:0;
    ULONG signals=task?task->tc_SigAlloc:0,wake=0,termination_mask=0;
    struct pt_native_eclock clock={0};struct pt_native_alarm termination={0};
    struct pt_diagnostic_cia owner={0};struct native_irq *irq=NULL;struct native_trace *trace=NULL;
    struct pt_elapsed_clock epoch;struct pt_aperture_policy policy={128,256,64};
    uint64_t now=0,deadline=0;uint32_t frequency=0,actual_frequency=0;
    BYTE signal=-1;unsigned i,attempted=0,completed=0,positive=0,failures=0,negative=0,acquired=0,chip=0,bit=0;int result=0;
#define CHECK(condition) do{if(!(condition)){printf("CIA APERTURE FAIL line=%u completed=%u\n",(unsigned)__LINE__,completed);result=20;goto done;}}while(0)
    CHECK(task && cia_aperture_fixture()==0);native_memory_start();
    irq=native_allocate(sizeof(*irq));trace=native_allocate(16*sizeof(*trace));
    memset(irq,0,sizeof(*irq));memset(trace,0,16*sizeof(*trace));
    CHECK(pt_native_eclock_open(&clock) && clock.port->mp_SigTask==task);
    CHECK(pt_native_alarm_open(&termination) && termination.port->mp_SigTask==task && termination.port->mp_SigBit<32);
    signal=AllocSignal(-1);CHECK(signal>=0 && signal<32);
    irq->signal=1UL<<signal;termination_mask=1UL<<termination.port->mp_SigBit;
    CHECK(!(irq->signal&termination_mask) && !((irq->signal|termination_mask)&SIGBREAKF_CTRL_C));
    irq->reader.timer=clock.request->tr_node.io_Device;irq->task=task;
    CHECK(pt_diagnostic_cia_acquire(&owner,pt_diagnostic_cia_aperture_irq,irq));
    acquired=1;chip=owner.chip;bit=owner.bit;
    CHECK(pt_native_eclock_read(&clock,&now,&frequency) && (frequency==709379 || frequency==715909));
    CHECK(pt_elapsed_clock_init(&epoch,frequency,48000,now,0)==PT_ELAPSED_OK);
    CHECK(pt_elapsed_clock_deadline(&epoch,60000,&deadline)==PT_ELAPSED_OK);
    CHECK(pt_native_alarm_arm(&termination,deadline)==PT_ALARM_WAITING);
    for(i=0;i<16;++i) {
        struct native_trace *entry=trace+i;unsigned count=0;uint64_t observed=0;int armed,read_ok;
        struct task_arm_reader arm_reader={&clock,0,0,0,0};
        CHECK(!(SetSignal(0,0)&(SIGBREAKF_CTRL_C|termination_mask)));
        entry->control=aperture_control();entry->control.key.trigger=i+1;entry->control.key.serial=i+1;
        entry->control.publication=i+1;entry->mode=i<12?0:i-11;entry->phase=1;attempted=i+1;
        CHECK(pt_aperture_prepare(&epoch,(i+1)*1536UL,&policy,&entry->control,&entry->state)==PT_APERTURE_READY);
        entry->prepared=1;entry->arm_target=entry->state.arm_at;
        /* Four explicit diagnostic negative cases. No change to first/last or
         * epoch; late wake is deliberate and never claimed an activation. */
        if(i==12)++entry->control.key.serial;
        if(i==13)entry->control.cancelled=1;
        if(i==14)entry->arm_target=entry->state.last+128;
        if(i==15)entry->control.armed=0;
        Disable();SetSignal(0,irq->signal);irq->state=&entry->state;irq->control=&entry->control;irq->armed=1;
        memset((void *)&irq->dispatch_before,0,sizeof(irq->dispatch_before));
        memset((void *)&irq->dispatch_after,0,sizeof(irq->dispatch_after));
        irq->dispatch_before_frequency=irq->dispatch_after_frequency=0;entry->phase=2;
        armed=pt_diagnostic_cia_arm_at(&owner,task_arm_clock,&arm_reader,entry->arm_target,frequency,&observed,&count);
        entry->arm_observed=arm_reader.actual;entry->arm_frequency=arm_reader.frequency;
        entry->arm_read_valid=arm_reader.valid;entry->arm_reads=arm_reader.reads;
        if(!armed) {irq->armed=0;Enable();CHECK(0);}
        entry->arm_count=count;entry->arm_valid=1;entry->phase=3;
        now=0;actual_frequency=0;read_ok=pt_native_eclock_read(&clock,&now,&actual_frequency);
        entry->arm_after=now;entry->arm_after_frequency=actual_frequency;entry->arm_after_valid=read_ok?1U:0U;
        if(!read_ok) {irq->armed=0;Enable();CHECK(0);}
        entry->phase=4;Enable();CHECK(actual_frequency==frequency && entry->arm_read_valid && entry->arm_reads==1 && entry->arm_frequency==frequency && observed==entry->arm_observed && count==entry->arm_target-entry->arm_observed);
        entry->phase=5;wake=Wait(irq->signal|termination_mask|SIGBREAKF_CTRL_C);entry->wake=wake;
        CHECK(FindTask(NULL)==task && task->tc_Node.ln_Pri==priority);
        CHECK(!(wake&(termination_mask|SIGBREAKF_CTRL_C)) && (wake&irq->signal));
        Disable();entry->wake=wake;entry->calls=irq->calls;
        if(irq->armed || irq->calls!=i+1 || entry->state.busy) {Enable();CHECK(0);}
        trace_snapshot(entry,irq);entry->delivered=1;entry->phase=6;
        Enable();++completed;
        CHECK(entry->before_frequency==frequency && entry->after_frequency==frequency &&
              entry->dispatch_after>=entry->dispatch_before && entry->dispatch_after-entry->dispatch_before<=256);
        CHECK(entry->state.entry_valid && entry->state.entry_frequency==frequency && entry->state.reads && entry->state.reads<=64);
        if(i<12) {
            if(entry->state.result==PT_APERTURE_COMMITTED && entry->state.commits==1 && entry->state.shadow==PT_APERTURE_SHADOW &&
               entry->state.before_valid && entry->state.after_valid && entry->state.before>=entry->state.first &&
               entry->state.before<entry->state.last && entry->state.after>=entry->state.before && entry->state.after<entry->state.last)++positive;
            else ++failures;
        } else {
            enum pt_aperture_result expected=i<14?PT_APERTURE_STALE:i==14?PT_APERTURE_EXPIRED:PT_APERTURE_STALE;
            if(!entry->state.commits && !entry->state.shadow && entry->state.result==expected)++negative;
            else ++failures;
        }
    }
    if(positive!=12 || negative!=4 || failures)result=20;
done:
    /* IRQ/task/reader storage stays live until positive owned-vector close.
     * No release from IRQ, force close, timer reset or test retry. */
    if(!pt_diagnostic_cia_close(&owner)) {
        if(irq && attempted){Disable();trace_snapshot(trace+attempted-1,irq);Enable();}
        trace_report(trace,attempted,0);return aperture_hold("owned timer/vector close unresolved");
    }
    if(irq && attempted){Disable();trace_snapshot(trace+attempted-1,irq);Enable();}
    trace_report(trace,attempted,1);
    if(irq)irq->armed=0;
    for(i=0;i<50 && termination.pending;++i) {
        pt_native_alarm_cancel(&termination);if(termination.pending)Delay(1);
    }
    if(termination.pending)return aperture_hold("independent termination IO unresolved");
    if(termination_mask)SetSignal(0,termination_mask);
    if(!pt_native_alarm_close(&termination))return aperture_hold("termination port closure unresolved");
    pt_native_eclock_close(&clock);
    if(signal>=0) {SetSignal(0,1UL<<signal);FreeSignal(signal);}
    if(!task || FindTask(NULL)!=task || task->tc_Node.ln_Pri!=priority || task->tc_SigAlloc!=signals ||
       clock.port || clock.request || termination.port || termination.request || owner.held || owner.resource)
        return aperture_hold("task/signals/resource closure unresolved");
    printf("CIA APERTURE CLOSED acquired=%u chip=%u bit=%u vector_removed=%u timer_stopped=%u task_identity=1 priority=%d unchanged=1 signals_restored=1 attempted=%u completed=%u positive=%u negative=%u failures=%u frequency=%lu early_ticks=128 residency_ticks=256 reads_cap=64\n",
        acquired,chip,bit,acquired,acquired,(int)priority,attempted,completed,positive,negative,failures,(unsigned long)frequency);
    native_release(trace);native_release(irq);if(native_started)native_memory_finish();
    if(!result)puts("CIA APERTURE PASS: original-frame actual RAM commit brackets and zero-effect negative gates; timer/software diagnostic only, no DMA or audio proof");
    return result;
#undef CHECK
}
