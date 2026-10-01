#include "native_exec_memory.h"
#include "../src/native/task_priority.h"
#include "../src/native/paula_pump.h"
#include <proto/dos.h>
#include <string.h>
/* Private timer lifetime/refusal and bounded pre-startup periodic polling.
 * No musical boundary/output or Wait-loop qualification; zero24 source. */
static void *fast_alloc(void *c,size_t n){(void)c;return native_allocate(n);}
static void fast_free(void *c,void *p){(void)c;native_release(p);}
static int fixture(unsigned late)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *ed=native_allocate(sizeof(*ed));struct pt_native_paula_transport t={0};
    int32_t *pcm=native_allocate(32*sizeof(*pcm));struct pt_render_options o={0};
    enum pt_paula_song_result r;unsigned i,startup_calls=0,initialized=0,attached=0;int result=0;
    uint64_t ticks;uint32_t frequency;struct pt_native_task_priority priority={0};
#define CHECK(c) do{if(!(c)){printf("PAULA TRANSPORT FAIL case=%u line=%u\n",late,(unsigned)__LINE__);result=20;goto done;}}while(0)
    memset(pcm,0,32*sizeof(*pcm));pt_document_init(&doc,&a);
    CHECK(pt_document_new(&doc,4,SIZE_MAX)==PT_PROJECT_OK);
    doc.project.samples[0].pcm=(struct pt_pcm){pcm,32,32,8000,1,24};doc.project.samples[0].volume=64;
    doc.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};doc.project.events[12].effect=15;
    CHECK(pt_editor_init(ed,&doc.project));initialized=1;ed->sampler.allocator=a;
    CHECK(pt_native_paula_transport_attach(&t,ed));attached=1;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    CHECK(pt_native_paula_transport_begin(&t,&o,32)==PT_PAULA_SONG_PREPARING);
    i=0;do{r=pt_native_editor_paula_advance(&t.native,NULL);CHECK(++i<100);if(r==PT_PAULA_SONG_PREPARING && !t.native.engine.ready)Delay(1);}while(r==PT_PAULA_SONG_PREPARING);
    CHECK(r==PT_PAULA_SONG_OK);
    if(late==2)CHECK(pt_native_task_priority_acquire(&priority,5));
    i=0;do {
        CHECK(!(SetSignal(0,0)&SIGBREAKF_CTRL_C));
        r=pt_native_paula_transport_start(&t,48000);CHECK(++i<2000);
        if(r==PT_PAULA_SONG_PREPARING) {
            CHECK(!t.clock.port && !t.alarm.port && !t.service_alarm.port && !t.started && !(pt_native_paula_output_dma()&15));
            Delay(1);
        }
    }while(r==PT_PAULA_SONG_PREPARING);
    startup_calls=i;
    if(late!=2)printf("TRANSPORT start=%u clock=%u alarm=%u pending=%u\n",(unsigned)r,t.clock.opened,t.alarm.opened,t.alarm.pending);
    CHECK(r==PT_PAULA_SONG_WAITING && t.clock.opened && t.alarm.opened && t.alarm.pending && t.service_alarm.opened && t.service_alarm.pending);
    CHECK(pt_native_paula_transport_signal(&t));
    if(late==2) {
        struct pt_elapsed_clock observed=t.service_clock;
        uint64_t frames=0,previous=0,before,after,max_gap=0,max_cost=0,max_prime_cost=0,max_ready_cost=0;
        unsigned calls=0,completions=0,primed=1,was_primed,prime_calls=0,ready_calls=0;
        uint64_t to_core=0,to_post=0,notify_lag=0,max_to_core=0,max_to_post=0,max_notify_lag=0;
        for(i=0;i<1000000 && frames<24000;++i) {
            CHECK(!(SetSignal(0,0)&SIGBREAKF_CTRL_C));
            CHECK(pt_native_eclock_read(&t.clock,&ticks,&frequency));
            CHECK(pt_elapsed_clock_advance(&observed,frequency,ticks,&frames)==PT_ELAPSED_OK);
            if(frames>=24000)break;
            if(primed && !CheckIO((struct IORequest *)t.service_alarm.request))continue;
            if(CheckIO((struct IORequest *)t.service_alarm.request))++completions;
            before=ticks;
            if(calls && before-previous>max_gap)max_gap=before-previous;
            previous=before;++calls;
            was_primed=primed;if(was_primed)++ready_calls;else ++prime_calls;
            r=pt_native_paula_transport_service(&t);
            to_core=(t.observation_mask&3)==3 && t.core_ticks>=t.entry_ticks?t.core_ticks-t.entry_ticks:0;
            to_post=(t.observation_mask&6)==6 && t.post_ticks>=t.core_ticks?t.post_ticks-t.core_ticks:0;
            notify_lag=(t.observation_mask&1) && t.entry_ticks>=t.notification_deadline?t.entry_ticks-t.notification_deadline:0;
            if(to_core>max_to_core)max_to_core=to_core;
            if(to_post>max_to_post)max_to_post=to_post;
            if(notify_lag>max_notify_lag)max_notify_lag=notify_lag;
            if(r!=PT_PAULA_SONG_WAITING && r!=PT_PAULA_SONG_OK) {
                printf("UNBOUND STARTUP calls=%u no timer epoch or WRITE before readiness\n",startup_calls);
                printf("CADENCE REFUSED calls=%u completions=%u frames=%lu result=%u max_gap_ticks=%lu frequency=%lu\n",
                    calls,completions,(unsigned long)frames,(unsigned)r,(unsigned long)max_gap,(unsigned long)frequency);
                printf("CADENCE STAGE=%u service_frames=%lu last_frames=%lu observed_cost_ticks=%lu max_prior_cost_ticks=%lu periodic_observed_ticks=%lu periodic_target_ticks=%lu\n",
                    (unsigned)t.failure_phase,(unsigned long)t.failure_frames,(unsigned long)t.failure_last_frames,
                    (unsigned long)(t.service_clock.ticks>=before?t.service_clock.ticks-before:0),
                    (unsigned long)max_cost,(unsigned long)t.service_alarm.observed_ticks,
                    (unsigned long)t.service_alarm.attempted_deadline);
                printf("CADENCE readiness=%u prime_calls=%u ready_calls=%u max_prime_cost_ticks=%lu max_ready_cost_ticks=%lu\n",
                    was_primed,prime_calls,ready_calls,(unsigned long)max_prime_cost,(unsigned long)max_ready_cost);
                printf("CADENCE SPLIT mask=%u entry_to_core_ticks=%lu core_to_post_ticks=%lu notification_lag_ticks=%lu max_entry_to_core_ticks=%lu max_core_to_post_ticks=%lu max_notification_lag_ticks=%lu\n",
                    t.observation_mask,(unsigned long)to_core,(unsigned long)to_post,(unsigned long)notify_lag,
                    (unsigned long)max_to_core,(unsigned long)max_to_post,(unsigned long)max_notify_lag);
                CHECK(0);
            }
            if(r==PT_PAULA_SONG_OK)primed=1;
            CHECK(pt_native_eclock_read(&t.clock,&after,&frequency) && after>=before);
            if(after-before>max_cost)max_cost=after-before;
            if(was_primed){if(after-before>max_ready_cost)max_ready_cost=after-before;}
            else if(after-before>max_prime_cost)max_prime_cost=after-before;
        }
        printf("UNBOUND STARTUP calls=%u no timer epoch or WRITE before readiness\n",startup_calls);
        printf("CADENCE observed_frames=%lu calls=%u completions=%u loops=%u max_gap_ticks=%lu max_cost_ticks=%lu frequency=%lu\n",
            (unsigned long)frames,calls,completions,i,(unsigned long)max_gap,(unsigned long)max_cost,(unsigned long)frequency);
        printf("CADENCE SPLIT max_entry_to_core_ticks=%lu max_core_to_post_ticks=%lu max_notification_lag_ticks=%lu\n",
            (unsigned long)max_to_core,(unsigned long)max_to_post,(unsigned long)max_notify_lag);
        CHECK(i<1000000 && frames>=24000 && frames<48000 && calls>=100 && completions>=100 && primed);
    }
    if(late==1) {
        Delay(60);
        CHECK(pt_native_eclock_read(&t.clock,&ticks,&frequency) && ticks>t.alarm_deadline);
        r=pt_native_paula_transport_service(&t);
        printf("TRANSPORT late=%u\n",(unsigned)r);
        CHECK(r==PT_PAULA_SONG_DEADLINE && pt_native_paula_transport_service(&t)==PT_PAULA_SONG_INVALID);
    }
    CHECK(!t.native.engine.output.held[0] && !(pt_native_paula_output_dma()&15));
 done:
    /* Every exit, including an unresolved device/timer cleanup, first returns
     * the calling task to its exact saved priority. No other task is touched. */
    if(!pt_native_task_priority_restore(&priority)){puts("TRANSPORT HOLD: priority restoration unresolved, all storage retained");return 25;}
    if(late==2 && priority.task) {
        if(FindTask(NULL)!=priority.task || priority.task->tc_Node.ln_Pri!=priority.saved)return 25;
        printf("TASK PRIORITY requested=5 saved=%d restored=%d own_identity=1 active=%u\n",
            (int)priority.saved,(int)priority.task->tc_Node.ln_Pri,priority.active);
    }
    if(attached){for(i=0;i<50 && !pt_editor_paula_stop(&t.native.binding);++i)Delay(1);if(i==50){puts("TRANSPORT HOLD: pending cleanup, all storage retained");return 21;}}
    if(attached && !pt_editor_paula_detach(&t.native.binding))return 22;
    if(initialized && !pt_editor_dispose(ed))return 23;
    if(t.clock.port || t.alarm.port || t.service_alarm.port || t.native.active || t.native.engine.output.reservation.port || (pt_native_paula_output_dma()&15))return 24;
    pt_document_release(&doc);native_release(pcm);native_release(ed);
    if(result){native_memory_finish();return result;}
    printf("PAULA TRANSPORT PASS: case=%u private clock/alarm and editor/device cleanup; no WRITE/cadence/listening claim\n",late);
#undef CHECK
    return 0;
}
int main(void){int r;native_memory_start();r=fixture(0);if(r)return r;r=fixture(1);if(r)return r;r=fixture(2);if(r)return r;native_memory_finish();return 0;}
