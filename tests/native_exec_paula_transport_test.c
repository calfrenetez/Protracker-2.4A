#include "native_exec_memory.h"
#include "../src/native/editor_paula_transport.h"
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
    enum pt_paula_song_result r;unsigned i,initialized=0,attached=0;int result=0;
    uint64_t ticks;uint32_t frequency;
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
    r=pt_native_paula_transport_start(&t,48000);
    if(late!=2)printf("TRANSPORT start=%u clock=%u alarm=%u pending=%u\n",(unsigned)r,t.clock.opened,t.alarm.opened,t.alarm.pending);
    CHECK(r==PT_PAULA_SONG_WAITING && t.clock.opened && t.alarm.opened && t.alarm.pending && t.service_alarm.opened && t.service_alarm.pending);
    CHECK(pt_native_paula_transport_signal(&t));
    if(late==2) {
        struct pt_elapsed_clock observed=t.service_clock;
        uint64_t frames=0,previous=0,before,after,max_gap=0,max_cost=0;
        unsigned calls=0,completions=0,primed=0;
        for(i=0;i<1000000 && frames<24000;++i) {
            CHECK(pt_native_eclock_read(&t.clock,&ticks,&frequency));
            CHECK(pt_elapsed_clock_advance(&observed,frequency,ticks,&frames)==PT_ELAPSED_OK);
            if(frames>=24000)break;
            if(primed && !CheckIO((struct IORequest *)t.service_alarm.request))continue;
            if(CheckIO((struct IORequest *)t.service_alarm.request))++completions;
            before=ticks;
            if(calls && before-previous>max_gap)max_gap=before-previous;
            previous=before;++calls;
            r=pt_native_paula_transport_service(&t);
            if(r!=PT_PAULA_SONG_WAITING && r!=PT_PAULA_SONG_OK) {
                printf("CADENCE REFUSED calls=%u completions=%u frames=%lu result=%u max_gap_ticks=%lu frequency=%lu\n",
                    calls,completions,(unsigned long)frames,(unsigned)r,(unsigned long)max_gap,(unsigned long)frequency);
                CHECK(0);
            }
            if(r==PT_PAULA_SONG_OK)primed=1;
            CHECK(pt_native_eclock_read(&t.clock,&after,&frequency) && after>=before);
            if(after-before>max_cost)max_cost=after-before;
        }
        printf("CADENCE observed_frames=%lu calls=%u completions=%u loops=%u max_gap_ticks=%lu max_cost_ticks=%lu frequency=%lu\n",
            (unsigned long)frames,calls,completions,i,(unsigned long)max_gap,(unsigned long)max_cost,(unsigned long)frequency);
        CHECK(i<1000000 && frames>=24000 && frames<48000 && calls>=100 && completions>=100 && primed);
    }
    if(late==1) {
        Delay(60);
        CHECK(pt_native_eclock_read(&t.clock,&ticks,&frequency) && ticks>t.alarm_deadline);
        r=pt_native_paula_transport_service(&t);
        printf("TRANSPORT late=%u\n",(unsigned)r);
        CHECK(r==PT_PAULA_SONG_DEADLINE && pt_native_paula_transport_service(&t)==PT_PAULA_SONG_INVALID);
    }
    CHECK(!t.native.engine.output.held[0] && !t.native.engine.cache.cache.bytes && !(pt_native_paula_output_dma()&15));
 done:
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
