#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "event_resource.h"
#ifdef PT_EVENT_RESOURCE_PROFILE
#include <time.h>
#endif

struct fixture {
    struct pt_project p;
    struct pt_event events[5*64*16];
    uint16_t orders[256];
    struct pt_sample samples[3];
    int32_t pcm[3][64];
};
static struct pt_event *event(struct fixture *f,unsigned pattern,unsigned row,unsigned track)
{return f->events+((size_t)pattern*64+row)*f->p.channels.count+track;}
static void init(struct fixture *f,unsigned tracks,unsigned patterns,unsigned orders)
{
    unsigned i;
    memset(f,0,sizeof(*f));pt_channels_init(&f->p.channels);
    assert(pt_channels_resize(&f->p.channels,tracks)==PT_CHANNEL_OK);
    for(i=4;i<tracks;++i)f->p.channels.track[i].route=(uint8_t)(i&1?PT_MIDI:PT_AMIGUS);
    f->p.events=f->events;f->p.orders=f->orders;f->p.samples=f->samples;
    f->p.pattern_count=(uint16_t)patterns;f->p.order_count=(uint16_t)orders;
    f->p.sample_count=3;f->p.speed=1;f->p.bpm=125;
    for(i=0;i<3;++i) {
        f->samples[i].pcm=(struct pt_pcm){f->pcm[i],64,8,8287,1,8};
        f->samples[i].volume=64;f->pcm[i][1]=(int32_t)i+1;f->pcm[i][63]=INT32_MAX;
    }
    f->samples[2].pcm.data=NULL;f->samples[2].pcm.capacity=f->samples[2].pcm.frames=0;
}
static struct pt_event_resource_origin origin(unsigned pattern,unsigned row,unsigned track,unsigned order)
{return (struct pt_event_resource_origin){pattern,row,track,order,1,0,PT_FLOW_EXTENDED256};}
static struct pt_event_resource_result resolve(struct pt_event_resource_job *j,struct fixture *f,
    struct pt_event_resource_origin o,uint32_t limit)
{
    struct pt_event_resource_result out;unsigned ready=0,calls=0;uint32_t before;
    assert(pt_event_resource_begin(j,&f->p,&o,11,22,limit)==PT_EVENT_RESOURCE_OK);
    while(!ready && calls++<limit+1) {
        before=j->flow.ticks;
        assert(pt_event_resource_step(j,11,22,17,&ready)==PT_EVENT_RESOURCE_OK);
        assert(j->flow.ticks-before<=17);
    }
    assert(ready);assert(pt_event_resource_get(j,11,22,&out)==PT_EVENT_RESOURCE_OK);return out;
}
static void explicit_empty_and_routes(void)
{
    struct fixture *f=calloc(1,sizeof(*f)),*before=malloc(sizeof(*before));
    struct pt_event_resource_job j={0};struct pt_event_resource_result r;
    struct pt_event_resource_origin o;unsigned ch,kind;
    assert(f && before);init(f,16,1,1);
    for(ch=0;ch<16;++ch)for(kind=0;kind<3;++kind) {
        struct pt_event *e=event(f,0,3,ch);
        memset(e,0,sizeof(*e));e->instrument=3;
        if(kind) {e->kind=PT_NOTE_PERIOD;e->pitch=428;e->effect=(uint8_t)(kind==1?3:5);}
        o=origin(0,3,ch,0);memcpy(before,f,sizeof(*f));
        r=resolve(&j,f,o,1);
        assert(r.state==PT_EVENT_RESOURCE_EXPLICIT && r.instrument==3 && r.ticks==0 && !r.visits);
        assert(r.origin.track==ch && r.source.track==ch && r.source_unique);
        assert(r.destination==(f->p.channels.track[ch].route==PT_MIDI?PT_EVENT_RESOURCE_MIDI_ROUTING:PT_EVENT_RESOURCE_AUDIO_MASTER));
        assert(r.midi_channel==f->p.channels.track[ch].midi_channel);
        assert(!memcmp(before,f,sizeof(*f)));
    }
    memset(f->events,0,sizeof(f->events));o=origin(0,3,0,0);
    r=resolve(&j,f,o,20);assert(r.state==PT_EVENT_RESOURCE_NO_RESOURCE && r.destination==PT_EVENT_RESOURCE_NONE);
    event(f,0,3,0)->kind=PT_NOTE_PERIOD;event(f,0,3,0)->pitch=428;
    o.order_known=0;r=resolve(&j,f,o,20);
    assert(r.state==PT_EVENT_RESOURCE_AMBIGUOUS && r.reason==PT_EVENT_RESOURCE_DETACHED_PATTERN && !r.instrument);
    event(f,0,3,0)->instrument=1;r=resolve(&j,f,o,1);assert(r.state==PT_EVENT_RESOURCE_EXPLICIT && r.instrument==1);
    free(before);free(f);
}
static void deterministic_orders_and_repeats(void)
{
    struct fixture *f=calloc(1,sizeof(*f)),*before=malloc(sizeof(*before));
    struct pt_event_resource_job j={0};struct pt_event_resource_result r;
    unsigned ch;
    assert(f && before);init(f,16,4,5);
    f->orders[0]=0;f->orders[1]=1;f->orders[2]=2;f->orders[3]=1;f->orders[4]=3;
    for(ch=0;ch<16;++ch) {
        event(f,0,0,ch)->instrument=1;event(f,2,0,ch)->instrument=2;
        event(f,1,0,ch)->kind=(uint8_t)(f->p.channels.track[ch].route==PT_MIDI?PT_NOTE_MIDI:PT_NOTE_PERIOD);
        event(f,1,0,ch)->pitch=(uint16_t)(f->p.channels.track[ch].route==PT_MIDI?60:428);
    }
    event(f,3,0,15)->effect=15;memcpy(before,f,sizeof(*f));
    for(ch=0;ch<16;++ch) {
        r=resolve(&j,f,origin(1,0,ch,1),1000);
        assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==1 && r.visits==1 && r.source_unique);
        assert(r.source.order==0 && r.source.pattern==0 && r.source.row==0 && r.source.track==ch);
        r=resolve(&j,f,origin(1,0,ch,3),1000);
        assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==2 && r.visits==1);
        assert(r.source.order==2 && r.source.pattern==2);
        assert(r.destination==(f->p.channels.track[ch].route==PT_MIDI?PT_EVENT_RESOURCE_MIDI_ROUTING:PT_EVENT_RESOURCE_AUDIO_MASTER));
    }
    assert(!memcmp(before,f,sizeof(*f)));free(before);free(f);
}
static void jumps_delays_and_loops(void)
{
    struct fixture *f=calloc(1,sizeof(*f));struct pt_event_resource_job j={0};struct pt_event_resource_result r;
    assert(f);init(f,4,3,3);f->orders[1]=1;f->orders[2]=2;
    event(f,0,0,0)->instrument=1;event(f,0,0,0)->effect=11;event(f,0,0,0)->parameter=2;
    event(f,1,0,0)->instrument=2;event(f,2,0,0)->kind=PT_NOTE_PERIOD;event(f,2,0,0)->pitch=428;
    event(f,2,1,0)->effect=15;
    r=resolve(&j,f,origin(2,0,0,2),100);assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==1 && r.source.order==0);
    r=resolve(&j,f,origin(1,1,0,1),100);assert(r.state==PT_EVENT_RESOURCE_NO_RESOURCE);
    event(f,1,1,0)->kind=PT_NOTE_PERIOD;event(f,1,1,0)->pitch=428;
    r=resolve(&j,f,origin(1,1,0,1),100);assert(r.state==PT_EVENT_RESOURCE_UNRESOLVED && r.reason==PT_EVENT_RESOURCE_UNREACHABLE);
    memset(&j,0,sizeof(j));init(f,16,2,2);f->orders[1]=1;
    event(f,0,0,0)->instrument=2;
    event(f,0,0,15)->effect=13;event(f,0,0,15)->parameter=0x10;
    event(f,1,10,0)->kind=PT_NOTE_PERIOD;event(f,1,10,0)->pitch=428;event(f,1,10,0)->effect=3;
    event(f,1,10,15)->effect=14;event(f,1,10,15)->parameter=0xe2;
    event(f,1,11,15)->effect=15;
    r=resolve(&j,f,origin(1,10,0,1),100);
    assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==2 && r.visits==1);
    memset(&j,0,sizeof(j));init(f,4,1,1);
    event(f,0,0,0)->instrument=1;event(f,0,0,0)->effect=14;event(f,0,0,0)->parameter=0x60;
    event(f,0,1,0)->kind=PT_NOTE_PERIOD;event(f,0,1,0)->pitch=428;event(f,0,1,0)->effect=5;
    event(f,0,2,0)->effect=14;event(f,0,2,0)->parameter=0x62;event(f,0,3,0)->effect=15;
    r=resolve(&j,f,origin(0,1,0,0),100);
    assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==1 && r.visits==3);
    memset(&j,0,sizeof(j));event(f,0,0,0)->instrument=0;event(f,0,0,0)->kind=PT_NOTE_PERIOD;event(f,0,0,0)->pitch=428;
    event(f,0,1,0)->instrument=2;
    r=resolve(&j,f,origin(0,0,0,0),100);
    assert(r.state==PT_EVENT_RESOURCE_AMBIGUOUS && r.reason==PT_EVENT_RESOURCE_MULTIPLE_SELECTIONS && r.visits==2 && !r.instrument);
    free(f);
}
static void cycles_and_budget(void)
{
    struct fixture *f=calloc(1,sizeof(*f));struct pt_event_resource_job j={0};struct pt_event_resource_result r;
    assert(f);init(f,4,1,1);event(f,0,0,0)->instrument=1;
    event(f,0,1,0)->kind=PT_NOTE_PERIOD;event(f,0,1,0)->pitch=428;
    r=resolve(&j,f,origin(0,1,0,0),300);
    assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==1 && r.visits>1 && r.ticks<=300);
    r=resolve(&j,f,origin(0,1,0,0),1);
    assert(r.state==PT_EVENT_RESOURCE_UNRESOLVED && r.reason==PT_EVENT_RESOURCE_TICK_BUDGET && !r.instrument);
    memset(&j,0,sizeof(j));memset(f->events,0,sizeof(f->events));
    event(f,0,0,0)->kind=PT_NOTE_PERIOD;event(f,0,0,0)->pitch=428;event(f,0,1,0)->effect=15;
    r=resolve(&j,f,origin(0,0,0,0),100);assert(r.state==PT_EVENT_RESOURCE_UNRESOLVED && r.reason==PT_EVENT_RESOURCE_NO_CARRY);
    memset(&j,0,sizeof(j));init(f,4,1,1);
    event(f,0,0,0)->kind=PT_NOTE_PERIOD;event(f,0,0,0)->pitch=428;event(f,0,63,0)->instrument=1;
    r=resolve(&j,f,origin(0,0,0,0),300);
    assert(r.state==PT_EVENT_RESOURCE_AMBIGUOUS && r.visits==2);
    free(f);
}
static void engine_modes(void)
{
    struct fixture *f=calloc(1,sizeof(*f));struct pt_event_resource_job j={0};
    struct pt_event_resource_result r;struct pt_event_resource_origin o;unsigned ready=91;
    assert(f);init(f,4,2,128);f->p.speed=6;f->orders[127]=1;
    event(f,0,0,0)->instrument=1;event(f,0,0,0)->effect=11;event(f,0,0,0)->parameter=255;
    event(f,1,0,0)->kind=PT_NOTE_PERIOD;event(f,1,0,0)->pitch=428;event(f,1,1,0)->effect=15;
    o=origin(1,0,0,127);o.flow_mode=PT_FLOW_CLASSIC128;
    r=resolve(&j,f,o,100);assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==1);
    o.flow_mode=PT_FLOW_EXTENDED256;
    r=resolve(&j,f,o,100);assert(r.state==PT_EVENT_RESOURCE_UNRESOLVED && r.reason==PT_EVENT_RESOURCE_UNREACHABLE);
    /* B80 reaches the full-byte position128 only in the 256-position engine. */
    memset(&j,0,sizeof(j));f->p.order_count=256;f->orders[128]=1;event(f,0,0,0)->parameter=128;
    o=origin(1,0,0,128);r=resolve(&j,f,o,100);
    assert(r.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED && r.instrument==1 && r.source.order==0);
    o.flow_mode=PT_FLOW_CLASSIC128;
    assert(pt_event_resource_begin(&j,&f->p,&o,11,22,100)==PT_EVENT_RESOURCE_OK);
    assert(pt_event_resource_step(&j,11,22,1,&ready)==PT_EVENT_RESOURCE_INVALID && ready==91);
    free(f);
}
static void stale_alias_and_protocol(void)
{
    struct fixture *f=calloc(1,sizeof(*f)),*before=malloc(sizeof(*before));
    struct pt_event_resource_job j={0},save;struct pt_event_resource_result r,sentinel;
    struct pt_event_resource_origin o=origin(0,1,0,0);unsigned ready=77;
    assert(f && before);init(f,4,1,1);event(f,0,0,0)->instrument=1;
    event(f,0,1,0)->kind=PT_NOTE_PERIOD;event(f,0,1,0)->pitch=428;event(f,0,2,0)->effect=15;
    assert(pt_event_resource_begin(&j,&f->p,&o,11,22,100)==PT_EVENT_RESOURCE_OK);save=j;
    assert(pt_event_resource_step(&j,11,22,0,&ready)==PT_EVENT_RESOURCE_INVALID && ready==77 && !memcmp(&save,&j,sizeof(j)));
    assert(pt_event_resource_step(&j,11,22,257,&ready)==PT_EVENT_RESOURCE_INVALID && ready==77);
    memset(&sentinel,0xa5,sizeof(sentinel));r=sentinel;
    assert(pt_event_resource_get(&j,11,22,&r)==PT_EVENT_RESOURCE_INVALID && !memcmp(&r,&sentinel,sizeof(r)));
    memcpy(before,f,sizeof(*f));
    assert(pt_event_resource_step(&j,11,22,1,(unsigned *)&f->pcm[0][63])==PT_EVENT_RESOURCE_INVALID);
    assert(pt_event_resource_step(&j,11,22,1,&f->p.midi_flags)==PT_EVENT_RESOURCE_INVALID);
    assert(pt_event_resource_step(&j,11,22,1,(unsigned *)&j.power)==PT_EVENT_RESOURCE_INVALID);
    assert(pt_event_resource_step(&j,11,22,1,(unsigned *)(UINTPTR_MAX-1))==PT_EVENT_RESOURCE_INVALID);
    assert(!memcmp(before,f,sizeof(*f)) && !memcmp(&save,&j,sizeof(j)));
    assert(pt_event_resource_begin((struct pt_event_resource_job *)f->events,&f->p,&o,11,22,100)==PT_EVENT_RESOURCE_INVALID);
    assert(!memcmp(before,f,sizeof(*f)));
    assert(pt_event_resource_step(&j,12,22,1,&ready)==PT_EVENT_RESOURCE_STALE && ready==77);
    assert(pt_event_resource_step(&j,11,23,1,&ready)==PT_EVENT_RESOURCE_STALE && ready==77);
    event(f,0,0,0)->instrument=2;
    assert(pt_event_resource_step(&j,12,22,1,&ready)==PT_EVENT_RESOURCE_STALE && ready==77);
    event(f,0,0,0)->instrument=1;
    f->p.samples=NULL;
    assert(pt_event_resource_step(&j,11,22,1,&ready)==PT_EVENT_RESOURCE_STALE && ready==77);
    assert(pt_event_resource_get(&j,11,22,&r)==PT_EVENT_RESOURCE_STALE && !memcmp(&r,&sentinel,sizeof(r)));
    f->p.samples=f->samples;f->p.channels.selected=1;
    assert(pt_event_resource_step(&j,11,22,1,&ready)==PT_EVENT_RESOURCE_OK && !ready);
    while(!ready)assert(pt_event_resource_step(&j,11,22,10,&ready)==PT_EVENT_RESOURCE_OK);
    assert(pt_event_resource_get(&j,11,22,(struct pt_event_resource_result *)f->pcm[0])==PT_EVENT_RESOURCE_INVALID);
    assert(pt_event_resource_get(&j,11,22,(struct pt_event_resource_result *)(UINTPTR_MAX-1))==PT_EVENT_RESOURCE_INVALID);
    assert(pt_event_resource_get(&j,11,22,&r)==PT_EVENT_RESOURCE_OK && r.origin.track==0 && r.instrument==1);
    /* Same-version audited initialization is reused; new revisions lose it. */
    assert(pt_event_resource_begin(&j,&f->p,&o,11,22,100)==PT_EVENT_RESOURCE_OK && j.validated);
    assert(pt_event_resource_begin(&j,&f->p,&o,12,22,100)==PT_EVENT_RESOURCE_OK && !j.validated);
    f->samples[0].pcm.capacity=SIZE_MAX;
    assert(pt_event_resource_begin(&j,&f->p,&o,12,22,100)==PT_EVENT_RESOURCE_INVALID);
    f->samples[0].pcm.capacity=64;f->samples[0].pcm.data=(int32_t *)(UINTPTR_MAX-3);
    assert(pt_event_resource_begin(&j,&f->p,&o,12,22,100)==PT_EVENT_RESOURCE_INVALID);
    f->samples[0].pcm.data=f->pcm[0];o.order=1;
    assert(pt_event_resource_begin(&j,&f->p,&o,12,22,100)==PT_EVENT_RESOURCE_INVALID);
    free(before);free(f);
}
#ifdef PT_EVENT_RESOURCE_PROFILE
static void validation_profile(void)
{
    struct fixture *f=calloc(1,sizeof(*f));struct pt_event_resource_job j={0};
    struct pt_event_resource_origin o=origin(0,1,0,0);unsigned ready=0;clock_t start,first,cached;
    const size_t values=8UL*1024*1024;int32_t *master=calloc(values,sizeof(*master));
    assert(f && master);init(f,4,1,1);
    f->samples[0].pcm=(struct pt_pcm){master,values,(uint32_t)values,8287,1,8};
    event(f,0,0,0)->instrument=1;event(f,0,1,0)->kind=PT_NOTE_PERIOD;event(f,0,1,0)->pitch=428;
    event(f,0,2,0)->effect=15;
    assert(pt_event_resource_begin(&j,&f->p,&o,11,22,100)==PT_EVENT_RESOURCE_OK);
    start=clock();assert(pt_event_resource_step(&j,11,22,1,&ready)==PT_EVENT_RESOURCE_OK);first=clock()-start;
    assert(pt_event_resource_begin(&j,&f->p,&o,11,22,100)==PT_EVENT_RESOURCE_OK && j.validated);
    start=clock();assert(pt_event_resource_step(&j,11,22,1,&ready)==PT_EVENT_RESOURCE_OK);cached=clock()-start;
    printf("EVENT RESOURCE HOST PROFILE: master_values=%lu first_validation_cpu_s=%.6f cached_step_cpu_s=%.6f; host observation only, no030 latency claim\n",
        (unsigned long)values,(double)first/CLOCKS_PER_SEC,(double)cached/CLOCKS_PER_SEC);
    free(master);free(f);
}
#endif
int main(void)
{
    explicit_empty_and_routes();deterministic_orders_and_repeats();jumps_delays_and_loops();
    cycles_and_budget();engine_modes();stale_alias_and_protocol();
#ifdef PT_EVENT_RESOURCE_PROFILE
    validation_profile();
#endif
    puts("EVENT RESOURCE PASS: explicit and bounded audited inheritance, repeated-order/control-flow ambiguity,16 routes, stale and full-capacity refusal; no playback or MIDI sends");
    return 0;
}
