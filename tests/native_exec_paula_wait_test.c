#include "native_exec_memory.h"
#include "../src/native/paula_pump.h"
#include "../src/native/task_priority.h"
#include <proto/dos.h>
#include <exec/tasks.h>
#include <string.h>
/* Ambient-priority, finite pre-musical-boundary Wait/termination diagnostics.
 * Zero24 master, no WRITE/output/frontend/listening/physical qualification.
 * Private termination timer/signal belongs to SAME task, never another Task. */
static void *fast_alloc(void *c,size_t n){(void)c;return native_allocate(n);}
static void fast_free(void *c,void *p){(void)c;native_release(p);}
/* An unresolved close must not let this Process exit with live IO targeting
 * its Task/stack. Park cooperatively, preserving EVERY context for explicit
 * recovery; no automatic cleanup retry, abort, restart or force-free. The host
 * timeout reports HOLD and cannot authorize terminating this task. */
static int hold(unsigned code,const char *reason) __attribute__((noreturn));
static int hold(unsigned code,const char *reason)
{printf("WAIT HOLD code=%u: %s; task and all storage retained for explicit recovery\n",code,reason);fflush(stdout);for(;;)Delay(50);__builtin_unreachable();}
static int fixture(unsigned mode)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *ed=native_allocate(sizeof(*ed));int32_t *pcm=native_allocate(32*sizeof(*pcm));
    struct pt_native_paula_transport t={0};struct pt_native_paula_pump pump={0};
    struct pt_native_alarm termination={0};struct pt_native_eclock diagnostic={0};struct pt_render_options o={0};
    struct Task *task=FindTask(NULL);BYTE priority=task?task->tc_Node.ln_Pri:0;
    struct pt_native_task_priority scope={0};
    ULONG original_signals=task?task->tc_SigAlloc:0;
    enum pt_paula_song_result r;enum pt_native_pump_result pr=PT_NATIVE_PUMP_INVALID;
    unsigned initialized=0,attached=0,i,starts=0,calls=0,work=0,wakes=0;int result=0;
    uint64_t ticks,frames=0,deadline;uint32_t frequency;ULONG abort_mask=0;
#define CHECK(c) do{if(!(c)){printf("PAULA WAIT FAIL case=%u line=%u\n",mode,(unsigned)__LINE__);result=20;goto done;}}while(0)
    memset(pcm,0,32*sizeof(*pcm));pt_document_init(&doc,&a);CHECK(task);
    CHECK(pt_document_new(&doc,4,SIZE_MAX)==PT_PROJECT_OK);
    doc.project.samples[0].pcm=(struct pt_pcm){pcm,32,32,8000,1,24};doc.project.samples[0].volume=64;
    doc.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};doc.project.events[12].effect=15;
    CHECK(pt_editor_init(ed,&doc.project));initialized=1;ed->sampler.allocator=a;
    CHECK(pt_native_paula_transport_attach(&t,ed));attached=1;
    CHECK(pt_native_alarm_open(&termination) && termination.port->mp_SigTask==task && termination.port->mp_SigBit<32);
    abort_mask=1UL<<termination.port->mp_SigBit;CHECK(!(abort_mask&SIGBREAKF_CTRL_C));
    CHECK(pt_native_eclock_open(&diagnostic) && diagnostic.port->mp_SigTask==task);
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    CHECK(pt_native_paula_transport_begin(&t,&o,32)==PT_PAULA_SONG_PREPARING);
    i=0;do{CHECK(!(SetSignal(0,0)&SIGBREAKF_CTRL_C));r=pt_native_editor_paula_advance(&t.native,NULL);CHECK(++i<100);if(r==PT_PAULA_SONG_PREPARING && !t.native.engine.ready)Delay(1);}while(r==PT_PAULA_SONG_PREPARING);
    CHECK(r==PT_PAULA_SONG_OK);
#ifdef PT_NATIVE_PAULA_WAIT_SCOPED_PRIORITY
    if(mode==2)CHECK(pt_native_task_priority_acquire(&scope,PT_NATIVE_PAULA_WAIT_SCOPED_PRIORITY) && scope.task==task && scope.saved==priority);
#endif
    i=0;do {
        CHECK(!(SetSignal(0,0)&SIGBREAKF_CTRL_C));r=pt_native_paula_transport_start(&t,48000);CHECK(++i<2000);
        if(r==PT_PAULA_SONG_PREPARING) {
            CHECK(!t.clock.port && !t.alarm.port && !t.service_alarm.port && !t.started && !(pt_native_paula_output_dma()&15));Delay(1);
        }
    }while(r==PT_PAULA_SONG_PREPARING);
    starts=i;CHECK(r==PT_PAULA_SONG_WAITING && pt_native_paula_pump_bind(&pump,&t));
    if(mode==0) {
        SetSignal(abort_mask,abort_mask);
        pr=pt_native_paula_pump_step(&pump,abort_mask|SIGBREAKF_CTRL_C);
        CHECK((pr==PT_NATIVE_PUMP_STOPPED || pr==PT_NATIVE_PUMP_HOLD) && !pump.wait_calls);
    }else if(mode==1) {
        /* Separate termination target between128/256 service notifications.
         * The actual arm still refuses lateness; no rebase or signal injection. */
        CHECK(pt_elapsed_clock_deadline(&t.service_clock,192,&deadline)==PT_ELAPSED_OK);
        CHECK(pt_native_alarm_arm(&termination,deadline)==PT_ALARM_WAITING);
        for(i=0;i<10000;++i) {
            pr=pt_native_paula_pump_step(&pump,abort_mask|SIGBREAKF_CTRL_C);++calls;
            if(pr==PT_NATIVE_PUMP_WORK){++work;continue;}
            if(pr==PT_NATIVE_PUMP_WAKE){++wakes;continue;}
            break;
        }
        CHECK(i<10000 && (pr==PT_NATIVE_PUMP_STOPPED || pr==PT_NATIVE_PUMP_HOLD) &&
            pump.wait_calls && (pump.last_wake&abort_mask));
    }else {
        struct pt_elapsed_clock observed=t.service_clock;
        /* This third private request independently terminates any ordinary
         * Wait at the finite target, even with no playback notification. Keep
         * independent reader alive through pump close to verify actual end. */
        CHECK(pt_elapsed_clock_deadline(&observed,24000,&deadline)==PT_ELAPSED_OK);
        CHECK(pt_native_alarm_arm(&termination,deadline)==PT_ALARM_WAITING);
        for(i=0;i<10000 && frames<24000;++i) {
            CHECK(pt_native_eclock_read(&diagnostic,&ticks,&frequency));
            CHECK(pt_elapsed_clock_advance(&observed,frequency,ticks,&frames)==PT_ELAPSED_OK);
            if(frames>=24000)break;
            pr=pt_native_paula_pump_step(&pump,abort_mask|SIGBREAKF_CTRL_C);++calls;
            if(pr==PT_NATIVE_PUMP_WORK){++work;continue;}
            if(pr==PT_NATIVE_PUMP_WAKE){++wakes;continue;}
            if((pr==PT_NATIVE_PUMP_STOPPED || pr==PT_NATIVE_PUMP_HOLD) && !t.failed &&
               (pump.last==PT_PAULA_SONG_OK || pump.last==PT_PAULA_SONG_WAITING) &&
               CheckIO((struct IORequest *)termination.request)) {
                CHECK(pt_native_eclock_read(&diagnostic,&ticks,&frequency));
                CHECK(pt_elapsed_clock_advance(&observed,frequency,ticks,&frames)==PT_ELAPSED_OK && frames>=24000);
                break;
            }
            CHECK(0);
        }
        CHECK(i<10000 && frames>=24000 && frames<48000 && wakes>=100 && pump.wait_calls>=100);
    }
    CHECK(!t.native.engine.output.held[0] && !(pt_native_paula_output_dma()&15));
 done:
    if(result)printf("WAIT REFUSED case=%u result=%d song=%u phase=%u frames=%lu last_frames=%lu mask=%u waits=%u wake=%lu termination_observed=%lu termination_target=%lu\n",
        mode,(int)pr,(unsigned)pump.last,(unsigned)t.failure_phase,(unsigned long)t.failure_frames,
        (unsigned long)t.failure_last_frames,t.observation_mask,pump.wait_calls,(unsigned long)pump.last_wake,
        (unsigned long)termination.observed_ticks,(unsigned long)termination.attempted_deadline);
    /* Every exit retains task/context/storage until BOTH owner and separate
     * termination IO close. Never force-free or retry a stuck foreign call. */
    if(!pt_native_task_priority_restore(&scope))return hold(25,"own priority restoration unresolved");
    if(scope.task)printf("WAIT PRIORITY requested=%d saved=%d restored=%d own_identity=%u active=%u\n",5,(int)scope.saved,(int)task->tc_Node.ln_Pri,FindTask(NULL)==scope.task,scope.active);
    if(FindTask(NULL)!=task || !task || task->tc_Node.ln_Pri!=priority)return hold(25,"task/priority identity unresolved");
    for(i=0;i<50 && !pt_native_paula_pump_close(&pump);++i)Delay(1);
    if(i==50)return hold(21,"pump cleanup unresolved");
    if(attached){for(i=0;i<50 && !pt_editor_paula_stop(&t.native.binding);++i)Delay(1);if(i==50)return hold(21,"transport cleanup unresolved");}
    for(i=0;i<50 && termination.pending;++i){pt_native_alarm_cancel(&termination);if(termination.pending)Delay(1);}
    if(termination.pending)return hold(26,"termination timer unresolved");
    if(abort_mask)SetSignal(0,abort_mask); /* Clear ONLY still-owned private bit. */
    if(!pt_native_alarm_close(&termination))return hold(26,"termination close unresolved");
    pt_native_eclock_close(&diagnostic);
    if(attached && !pt_editor_paula_detach(&t.native.binding))return hold(22,"editor detach unresolved");
    if(initialized && !pt_editor_dispose(ed))return hold(23,"editor disposal unresolved");
    if(diagnostic.port || termination.port || t.pump || pump.active || t.clock.port || t.alarm.port || t.service_alarm.port || t.native.active || t.native.engine.output.reservation.port || (pt_native_paula_output_dma()&15))return hold(24,"resource/reader closure unresolved");
    if(FindTask(NULL)!=task || task->tc_Node.ln_Pri!=priority)return hold(25,"final task/priority identity unresolved");
    if(task->tc_SigAlloc!=original_signals)return hold(27,"task signal allocation restoration unresolved");
    printf("WAIT TASK case=%u priority=%d unchanged=1 own_identity=1 owner_closed=1 termination_closed=1 diagnostic_closed=1 signal_alloc_restored=1 calls=%u work=%u wakes=%u waits=%u last_wake=%lu startup=%u frames=%lu\n",
        mode,(int)priority,calls,work,wakes,pump.wait_calls,(unsigned long)pump.last_wake,starts,(unsigned long)frames);
    pt_document_release(&doc);native_release(pcm);native_release(ed);
    if(result){native_memory_finish();return result;}
    printf("PAULA WAIT PASS: case=%u private task/timer/termination/editor/device cleanup; no WRITE/output/timing claim\n",mode);
#undef CHECK
    return 0;
}
int main(void){int r;native_memory_start();r=fixture(0);if(r)return r;r=fixture(1);if(r)return r;r=fixture(2);if(r)return r;native_memory_finish();return 0;}
