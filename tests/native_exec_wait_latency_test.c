#include "native_exec_memory.h"
#include "../src/native/eclock_alarm.h"
#include "../src/native/task_priority.h"
#include "../src/core/elapsed_clock.h"
#include <exec/tasks.h>
#include <proto/dos.h>
/* Isolate private WAITECLOCK/Exec Wait with no song, cache or audio device.
 * Immutable 128-frame grid, actual 48kHz observations, unchanged 256-frame
 * gap refusal. Independent half-second termination in EVERY Wait. Diagnostic
 * snapshots allocate bounded Fast storage and print only after timer closure. */
struct observation {uint64_t target,armed,ticks,frames,gap;ULONG wake;};
static int hold(const char *why) __attribute__((noreturn));
static int hold(const char *why)
{printf("LATENCY HOLD: %s; live Task and all storage retained for explicit recovery\n",why);fflush(stdout);for(;;)Delay(50);__builtin_unreachable();}
int main(void)
{
    struct Task *task=FindTask(NULL);BYTE priority=task?task->tc_Node.ln_Pri:0;
    struct pt_native_task_priority scope={0};BYTE expected=priority;
    ULONG original_signals=task?task->tc_SigAlloc:0,termination_mask=0,periodic_mask=0,wake=0;
    struct pt_native_eclock reader={0};struct pt_native_alarm periodic={0},termination={0};
    struct pt_elapsed_clock clock;struct observation *trace;
    uint64_t ticks=0,frames=0,last=0,target=0,deadline=0,max_gap=0,max_lag=0;
    uint32_t frequency=0;unsigned calls=0,i,initialized=0;int result=0;
#define CHECK(c) do{if(!(c)){printf("WAIT LATENCY FAIL line=%u frames=%lu calls=%u\n",(unsigned)__LINE__,(unsigned long)frames,calls);result=20;goto done;}}while(0)
    native_memory_start();trace=native_allocate(256*sizeof(*trace));CHECK(task);
    CHECK(pt_native_eclock_open(&reader) && reader.port->mp_SigTask==task);
    CHECK(pt_native_alarm_open(&periodic) && periodic.port->mp_SigTask==task && periodic.port->mp_SigBit<32);
    CHECK(pt_native_alarm_open(&termination) && termination.port->mp_SigTask==task && termination.port->mp_SigBit<32);
    periodic_mask=1UL<<periodic.port->mp_SigBit;termination_mask=1UL<<termination.port->mp_SigBit;
    CHECK(!(periodic_mask&termination_mask) && !((periodic_mask|termination_mask)&SIGBREAKF_CTRL_C));
#ifdef PT_NATIVE_WAIT_SCOPED_PRIORITY
    expected=PT_NATIVE_WAIT_SCOPED_PRIORITY;
    CHECK(pt_native_task_priority_acquire(&scope,expected) && scope.task==task && scope.saved==priority);
#endif
    CHECK(pt_native_eclock_read(&reader,&ticks,&frequency));
    CHECK(pt_elapsed_clock_init(&clock,frequency,48000,ticks,0)==PT_ELAPSED_OK);initialized=1;
    CHECK(pt_elapsed_clock_deadline(&clock,24000,&deadline)==PT_ELAPSED_OK);
    CHECK(pt_native_alarm_arm(&termination,deadline)==PT_ALARM_WAITING);
    while(calls<256 && frames<24000) {
        CHECK(!(SetSignal(0,0)&SIGBREAKF_CTRL_C));
        target=(frames/128+1)*128;
        CHECK(pt_elapsed_clock_deadline(&clock,target,&deadline)==PT_ELAPSED_OK);
        CHECK(pt_native_alarm_arm(&periodic,deadline)==PT_ALARM_WAITING);
        wake=Wait(periodic_mask|termination_mask|SIGBREAKF_CTRL_C);
        CHECK(FindTask(NULL)==task && task->tc_Node.ln_Pri==expected);
        CHECK(pt_native_eclock_read(&reader,&ticks,&frequency));
        CHECK(pt_elapsed_clock_advance(&clock,frequency,ticks,&frames)==PT_ELAPSED_OK);
        trace[calls++]=(struct observation){deadline,periodic.observed_ticks,ticks,frames,frames-last,wake};
        if(frames-last>max_gap)max_gap=frames-last;
        if(ticks>=deadline && ticks-deadline>max_lag)max_lag=ticks-deadline;
        CHECK(!(wake&SIGBREAKF_CTRL_C));
        if(wake&termination_mask){CHECK(frames>=24000 && CheckIO((struct IORequest *)termination.request));break;}
        CHECK((wake&periodic_mask) && pt_native_alarm_poll(&periodic)==PT_ALARM_READY);
        CHECK(frames-last<=256);last=frames;
    }
    CHECK(calls<256 && calls>=100 && frames>=24000 && frames<48000 && max_gap<=256);
 done:
    /* Restore the exact saved priority BEFORE all timer/storage cleanup, even
     * on timing/IO failure. Failed restoration parks the live owner. */
    if(!pt_native_task_priority_restore(&scope))return hold("own priority restoration unresolved");
    if(scope.task)printf("LATENCY PRIORITY requested=%d saved=%d restored=%d own_identity=%u active=%u\n",(int)expected,(int)scope.saved,(int)task->tc_Node.ln_Pri,FindTask(NULL)==scope.task,scope.active);
    if(FindTask(NULL)!=task || !task || task->tc_Node.ln_Pri!=priority)return hold("task/priority identity unresolved");
    for(i=0;i<50 && !pt_native_alarm_close(&periodic);++i)Delay(1);
    if(i==50)return hold("periodic timer closure unresolved");
    for(i=0;i<50 && termination.pending;++i){pt_native_alarm_cancel(&termination);if(termination.pending)Delay(1);}
    if(termination.pending)return hold("termination timer closure unresolved");
    if(termination_mask)SetSignal(0,termination_mask); /* Still allocated, completed IO only. */
    if(!pt_native_alarm_close(&termination))return hold("termination port closure unresolved");
    pt_native_eclock_close(&reader);
    if(reader.port || reader.request || periodic.port || periodic.request || termination.port || termination.request)return hold("resource closure unresolved");
    if(FindTask(NULL)!=task || task->tc_Node.ln_Pri!=priority || task->tc_SigAlloc!=original_signals)return hold("task/priority/signal restoration unresolved");
    printf("LATENCY TASK priority=%d unchanged=1 own_identity=1 timers_closed=1 reader_closed=1 signal_alloc_restored=1 calls=%u frames=%lu max_gap_frames=%lu max_lag_ticks=%lu frequency=%lu initialized=%u termination_target=%lu termination_arm=%lu\n",
        (int)priority,calls,(unsigned long)frames,(unsigned long)max_gap,(unsigned long)max_lag,(unsigned long)frequency,initialized,(unsigned long)termination.attempted_deadline,(unsigned long)termination.observed_ticks);
    for(i=0;i<calls;++i)printf("LATENCY OBS index=%u target=%lu arm=%lu actual=%lu frames=%lu gap=%lu wake=%lu\n",i,
        (unsigned long)trace[i].target,(unsigned long)trace[i].armed,(unsigned long)trace[i].ticks,
        (unsigned long)trace[i].frames,(unsigned long)trace[i].gap,(unsigned long)trace[i].wake);
    native_release(trace);native_memory_finish();
    if(!result)puts("WAIT LATENCY PASS: isolated actual-clock private Wait grid, no playback/output acceptance");
    return result;
#undef CHECK
}
