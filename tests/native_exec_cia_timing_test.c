#include "native_exec_memory.h"
#include "../src/native/eclock_alarm.h"
#include "../src/core/elapsed_clock.h"
#include "native_cia_owner.h"
#include <proto/dos.h>
#include <stddef.h>
#include <string.h>

/* Fixed finite observation run. Handler timestamps are actual ReadEClock,
 * not target substitution. No Paula channels/audio.device/PCM or frontend.
 * Immutable 1536-frame grid, 16 one-shot observations, independent termination
 * in EVERY Wait. Continue collecting predefined observations after late ones;
 * this is not application replay/retry and never emits sound. */
struct irq_state {
    struct Device *timer;volatile struct EClockVal ticks;
    volatile ULONG frequency,calls;struct Task *task;ULONG signal;
    volatile ULONG armed;
};
#define OFFSET(field,n) typedef char offset_##field[(offsetof(struct irq_state,field)==n)?1:-1]
OFFSET(timer,0);OFFSET(ticks,4);OFFSET(frequency,12);OFFSET(calls,16);
OFFSET(task,20);OFFSET(signal,24);OFFSET(armed,28);
extern void pt_diagnostic_cia_irq(void);
struct trace {uint64_t target,arm_before,arm_after,actual,frame;ULONG wake,calls;unsigned count;};
static int hold(const char *reason) __attribute__((noreturn));
static int hold(const char *reason)
{printf("CIA HOLD: %s; live Task/IRQ storage retained for explicit recovery\n",reason);fflush(stdout);for(;;)Delay(50);__builtin_unreachable();}
int main(void)
{
    struct Task *task=FindTask(NULL);BYTE priority=task?task->tc_Node.ln_Pri:0;
    ULONG original_signals=task?task->tc_SigAlloc:0,termination_mask=0,wake=0;
    BYTE signal=-1;struct pt_native_eclock reader={0};struct pt_native_alarm termination={0};
    struct pt_diagnostic_cia owner={0};struct irq_state *irq=NULL;struct trace *trace=NULL;
    struct pt_elapsed_clock epoch,observed;
    uint64_t ticks=0,deadline=0,now=0,frames=0;uint32_t frequency=0;
    unsigned i,completed=0,late=0,early=0,chip=0,bit=0,acquired=0;int result=0;
#define CHECK(c) do{if(!(c)){printf("CIA TIMING FAIL line=%u completed=%u\n",(unsigned)__LINE__,completed);result=20;goto done;}}while(0)
    CHECK(task);native_memory_start();irq=native_allocate(sizeof(*irq));trace=native_allocate(16*sizeof(*trace));
    memset(irq,0,sizeof(*irq));memset(trace,0,16*sizeof(*trace));
    CHECK(pt_native_eclock_open(&reader) && reader.port->mp_SigTask==task);
    CHECK(pt_native_alarm_open(&termination) && termination.port->mp_SigTask==task && termination.port->mp_SigBit<32);
    signal=AllocSignal(-1);CHECK(signal>=0 && signal<32);
    irq->signal=1UL<<signal;termination_mask=1UL<<termination.port->mp_SigBit;
    CHECK(!(irq->signal&termination_mask) && !((irq->signal|termination_mask)&SIGBREAKF_CTRL_C));
    irq->timer=reader.request->tr_node.io_Device;irq->task=task;
    CHECK(pt_diagnostic_cia_acquire(&owner,pt_diagnostic_cia_irq,irq));
    acquired=1;chip=owner.chip;bit=owner.bit;
    CHECK(pt_native_eclock_read(&reader,&ticks,&frequency) && (frequency==709379 || frequency==715909));
    CHECK(pt_elapsed_clock_init(&epoch,frequency,48000,ticks,0)==PT_ELAPSED_OK);
    CHECK(pt_elapsed_clock_deadline(&epoch,60000,&deadline)==PT_ELAPSED_OK);
    CHECK(pt_native_alarm_arm(&termination,deadline)==PT_ALARM_WAITING);
    for(i=0;i<16;++i) {
        unsigned count;int read_ok;struct trace *entry=&trace[i];struct EClockVal stamp;ULONG actual_frequency,calls,armed;
        CHECK(!(SetSignal(0,0)&(SIGBREAKF_CTRL_C|termination_mask)));
        CHECK(pt_elapsed_clock_deadline(&epoch,(i+1)*1536UL,&entry->target)==PT_ELAPSED_OK);
        /* Atomic publication/arm; no bulk work or output in exclusion. The
         * diagnostic reports pre/post arm so programming bias is not mislabeled
         * pure interrupt latency. No calibrated subtraction or fake timestamp. */
        Disable();SetSignal(0,irq->signal);irq->armed=1;
        if(!pt_native_eclock_read(&reader,&now,&actual_frequency) || actual_frequency!=frequency ||
           now>=entry->target || entry->target-now>65535) {
            irq->armed=0;Enable();CHECK(0);
        }
        entry->arm_before=now;count=(unsigned)(entry->target-now);entry->count=count;
        if(!pt_diagnostic_cia_arm(&owner,count)){irq->armed=0;Enable();CHECK(0);}
        read_ok=pt_native_eclock_read(&reader,&ticks,&actual_frequency);entry->arm_after=ticks;
        Enable(); /* nested arm exclusion is balanced before Wait */
        CHECK(read_ok && actual_frequency==frequency);
        wake=Wait(irq->signal|termination_mask|SIGBREAKF_CTRL_C);
        CHECK(FindTask(NULL)==task && task->tc_Node.ln_Pri==priority);
        CHECK(!(wake&(termination_mask|SIGBREAKF_CTRL_C)) && (wake&irq->signal));
        Disable();stamp.ev_hi=irq->ticks.ev_hi;stamp.ev_lo=irq->ticks.ev_lo;
        actual_frequency=irq->frequency;calls=irq->calls;armed=irq->armed;Enable();
        CHECK(actual_frequency==frequency && calls==i+1 && !armed);
        entry->actual=((uint64_t)stamp.ev_hi<<32)|stamp.ev_lo;entry->wake=wake;entry->calls=calls;
        observed=epoch;CHECK(pt_elapsed_clock_advance(&observed,actual_frequency,entry->actual,&frames)==PT_ELAPSED_OK);
        entry->frame=frames;++completed;
        if(frames<(i+1)*1536UL)++early;
        if(frames>(i+1)*1536UL)++late;
    }
    CHECK(completed==16);
    if(late || early)result=20; /* Numeric strict admission only, never relaxed. */
done:
    /* Close the acquired vector while all handler/Task/reader contexts remain
     * live. Failed close retains everything. Never free an IRQ target on error. */
    if(!pt_diagnostic_cia_close(&owner))return hold("owned timer/vector close unresolved");
    if(irq)irq->armed=0;
    for(i=0;i<50 && termination.pending;++i){pt_native_alarm_cancel(&termination);if(termination.pending)Delay(1);}
    if(termination.pending)return hold("independent termination IO unresolved");
    if(termination_mask)SetSignal(0,termination_mask);
    if(!pt_native_alarm_close(&termination))return hold("termination port closure unresolved");
    pt_native_eclock_close(&reader);
    if(signal>=0){SetSignal(0,1UL<<signal);FreeSignal(signal);}
    if(!task || FindTask(NULL)!=task || task->tc_Node.ln_Pri!=priority || task->tc_SigAlloc!=original_signals ||
       reader.port || reader.request || termination.port || termination.request || owner.held || owner.resource)
        return hold("task/signals/resource closure unresolved");
    printf("CIA CLOSED timer_acquired=%u chip=%u bit=%u vector_removed=%u owned_timer_stopped=%u task_identity=1 priority=%d unchanged=1 signal_alloc_restored=1 timers_closed=1 reader_closed=1 completed=%u early=%u late=%u frequency=%lu\n",
        acquired,chip,bit,acquired,acquired,(int)priority,completed,early,late,(unsigned long)frequency);
    for(i=0;i<completed;++i)printf("CIA OBS index=%u target=%lu arm_before=%lu arm_after=%lu actual=%lu frame=%lu count=%u wake=%lu calls=%lu\n",
        i,(unsigned long)trace[i].target,(unsigned long)trace[i].arm_before,(unsigned long)trace[i].arm_after,
        (unsigned long)trace[i].actual,(unsigned long)trace[i].frame,trace[i].count,(unsigned long)trace[i].wake,(unsigned long)trace[i].calls);
    native_release(trace);native_release(irq);if(native_started)native_memory_finish();
    if(!result)puts("CIA TIMING PASS: fixed IRQ observations admitted by logical frame; no playback/output acceptance");
    return result;
#undef CHECK
}
