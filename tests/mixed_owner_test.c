#define main paula_legacy_main
#include "paula_voices_test.c"
#undef main
#define PT_WAVETABLE_NATIVE
#include "amigus_wavetable_cache_test.c"
#include "../src/editor/mixed_owner_internal.h"
#include "../src/editor/paula_internal.h"
#ifdef PT_TEST_MIXED_NATIVE_COST
#include "../src/core/render_lookahead.h"
struct mixed_cost_clock {struct pt_native_eclock clock;uint64_t frame,sample_at,first,last;uint32_t frequency;unsigned callbacks;};
static struct mixed_cost_clock *mixed_cost_active;
static uint64_t mixed_cost_tick(struct mixed_cost_clock *c)
{uint64_t t;uint32_t f;assert(pt_native_eclock_read(&c->clock,&t,&f));if(c->frequency)assert(f==c->frequency);else c->frequency=f;return t;}
static int mixed_cost_read(void *p,uint64_t *t,uint32_t *f)
{struct mixed_cost_clock *c=p;c->sample_at=mixed_cost_tick(c);*t=c->frame;*f=48000;return 1;}
static void mixed_cost_callback(void)
{struct mixed_cost_clock *c=mixed_cost_active;uint64_t t;if(!c)return;t=mixed_cost_tick(c);if(!c->callbacks)c->first=t;c->last=t;++c->callbacks;}
#else
static void mixed_cost_callback(void){}
#endif
static unsigned output_order[32],output_count,fast_calls;
static void *counted_fast(void *c,size_t bytes){++fast_calls;return fast_alloc(c,bytes);}
static int mixed_start(void *c,unsigned slot,const struct pt_paula_voice_plan *p)
{mixed_cost_callback();assert(output_count<32);output_order[output_count++]=1;return start(c,slot,p);}
static int mixed_control(void *c,unsigned slot,uint16_t period,uint8_t volume)
{assert(output_count<32);output_order[output_count++]=3;return control(c,slot,period,volume);}
struct wave_driver {unsigned starts,stops,barriers;int stop_result,barrier_result,start_result;};
static int wave_start(void *c,unsigned id,const struct pt_amigus_voice_plan *p)
{struct wave_driver *d=c;(void)id;(void)p;mixed_cost_callback();assert(output_count<32);output_order[output_count++]=2;++d->starts;return d->start_result;}
static int wave_stop(void *c,unsigned id){struct wave_driver *d=c;(void)id;++d->stops;return d->stop_result;}
static int wave_control(void *c,unsigned id,uint32_t rate,uint16_t l,uint16_t r)
{(void)c;(void)id;(void)rate;(void)l;(void)r;assert(output_count<32);output_order[output_count++]=4;return 1;}
static int wave_quiesce(void *c){struct wave_driver *d=c;++d->barriers;return d->barrier_result;}
static void *refuse_alloc(void *c,size_t n){(void)c;(void)n;return NULL;}
struct mixed_counter {uint64_t ticks;uint32_t frequency;unsigned reads;int result;};
static int mixed_read(void *c,uint64_t *ticks,uint32_t *frequency)
{struct mixed_counter *m=c;++m->reads;*ticks=m->ticks;*frequency=m->frequency;return m->result;}
static void owner_fixture(unsigned bits,unsigned mode)
{
    struct pt_allocator a={NULL,counted_fast,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula pb={0};struct pt_sampler_wavetable ab={0};
    struct pt_paula_voices pv={0};struct pt_wavetable_voices av={0};
    struct driver d={0};struct wave_driver wd={0};struct fixture *f=malloc(sizeof(*f));
    struct pt_paula_voice_api pa={&d,mixed_start,stop,mixed_control};struct pt_wavetable_voice_api aa={&wd,wave_start,wave_stop,wave_control,NULL};
    struct pt_render_options o={0};struct pt_paula_render_caps caps={3546895,124,65535};struct pt_playback_format format={8,0,0,0};
    struct pt_mixed_owner *owner=NULL,*other=(void *)(uintptr_t)1;struct pt_mixed_report report;
    int32_t pcm[2048];unsigned i,n=0,oldbarriers;enum pt_mixed_owner_result r;
    struct pt_paula_voice_request request={0,16,428,64};struct pt_amigus_voice_request wr={8000,1,0,64,128};
    uint8_t staging[256];struct pt_render_plan *plan=malloc(sizeof(*plan));struct pt_paula_batch *batch=malloc(sizeof(*batch));
    assert(f && plan && batch);for(i=0;i<2048;++i)pcm[i]=(int32_t)(i%120)+1;
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0,4096,4096,f,bus_owned,bus_write));
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=PT_PAULA;doc.project.channels.track[4].pan=0;
    for(i=0;i<3;++i){doc.project.samples[i].pcm=(struct pt_pcm){pcm,2048,2048,8000,1,(uint8_t)bits};doc.project.samples[i].volume=64;}
    doc.project.speed=1;doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[7]=doc.project.events[4]; /* Shared source: one union pin. */
    doc.project.events[16+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    doc.project.events[32+15].effect=15;
    if(mode>=23)doc.project.events[16+15]=(struct pt_event){0,0,0,0,15,150,0,0};
    o.rate=mode>=23 && bits==16?44100:48000;o.bits=24;o.tracks=(1U<<4)|(1U<<7);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
#if defined(PT_TEST_MIXED_NATIVE_COST) || !defined(PT_TEST_MIXED_EXEC)
    if(mode==59 || mode==66){o.tracks=65535;
        for(i=0;i<16;++i){doc.project.channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
            if(i<4)doc.project.channels.track[i].pan=(i==0 || i==3)?0:255;
            doc.project.events[i]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};}}
    if(mode>=58)o.rate=48000; /* Injected clock below uses this explicit rate. */
#endif
    pt_sampler_init(&sampler,&a,1024*1024);
    assert(pt_sampler_paula_bind(&pb,&sampler,&doc.project,&d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&ab,&sampler,&doc.project,&f->cache));
    assert(pt_paula_voices_bind(&pv,&pb,&pa));assert(pt_wavetable_voices_bind(&av,&ab,&aa));
    assert(pt_paula_voices_bind_quiesce(&pv,quiesce,&d));assert(pt_wavetable_voices_bind_quiesce(&av,wave_quiesce,&wd));
    output_count=0;d.start_result=d.control_result=d.quiesce_result=wd.start_result=wd.stop_result=wd.barrier_result=1;for(i=0;i<4;++i)d.stop_result[i]=1;
    {struct pt_allocator bad={NULL,refuse_alloc,fast_free};
    assert(pt_mixed_owner_begin(&pv,&av,&o,&caps,&format,&bad,&other)==PT_MIXED_OWNER_MEMORY);
    assert(other==(void *)(uintptr_t)1 && !pv.song_owner && !av.song_owner);}
    if(mode==4){doc.project.events[16+4]=doc.project.events[16+7];doc.project.samples[1].loop=PT_LOOP_FORWARD;doc.project.samples[1].loop_end=2048;}
    assert(pt_mixed_owner_begin(&pv,&av,&o,&caps,&format,&a,&owner)==PT_MIXED_OWNER_PREPARING);
    assert(pv.song_owner==owner && av.song_owner==owner && !sampler.bytes && !d.live && !f->writes);
    assert(pt_mixed_owner_begin(&pv,&av,&o,&caps,&format,&a,&other)==PT_MIXED_OWNER_INVALID && other==(void *)(uintptr_t)1);
    assert(!pt_paula_voices_close(&pv) && !pt_wavetable_voices_close(&av));
    assert(pt_paula_voices_stop(&pv,4)==-1 && pt_wavetable_voices_stop(&av,7)==-1);
    assert(pt_paula_voices_trigger(&pv,4,0,0,&request)==PT_PAULA_VOICE_REFUSED);
    assert(pt_wavetable_voices_trigger(&av,7,0,&format,&wr,staging,sizeof(staging))==PT_VOICE_REFUSED);
    plan->count=0;assert(!pt_wavetable_dispatch(&av,ab.version,48000,plan,&format,staging,sizeof(staging)));
    if(mode==1)goto close;
    if(mode==3)sampler.budget=0;
    do {r=pt_mixed_owner_prepare(owner,&report);assert(++n<100 && !d.starts && !wd.starts && !d.live && !f->writes);
        if(mode==2 && sampler.current[0]){doc.project.bpm=150;break;}
    }while(r==PT_MIXED_OWNER_PREPARING);
    if(mode==2)assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_STALE);
    else if(mode==3)assert(r==PT_MIXED_OWNER_MEMORY && !sampler.bytes);
    else if(mode==4)assert(r==PT_MIXED_OWNER_CAPABILITY && !sampler.bytes && !sampler.current[0]);
    else {
        assert(r==PT_MIXED_OWNER_OK && sampler.current[0] && sampler.current[1] && !sampler.current[2]);
        assert(report.samples[0][0] && report.samples[1][0] && report.samples[1][1]);
        doc.project.channels.selected=15;assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_OK);
        if(mode==67) {
            struct pt_voice voice;struct pt_render_action action[5];unsigned iteration;
            assert(pt_voice_init(&voice,&doc.project.samples[0].pcm,0,2048,PT_VOICE_ONCE,0,0,((uint64_t)8000<<32)/48000,0)==PT_PCM_OK);
            action[0]=(struct pt_render_action){PT_RENDER_TRIGGER,7,voice,{32768,32768}};action[0].voice.pcm=&doc.project.samples[1].pcm;
            action[1]=(struct pt_render_action){PT_RENDER_TRIGGER,4,voice,{65536,0}};
            action[2]=action[0];action[3]=action[1];action[3].kind=PT_RENDER_CONTROL;
            action[4]=action[0];action[4].kind=PT_RENDER_CONTROL;
            for(iteration=0;iteration<12;++iteration) {
                struct pt_paula_voice saved_pv[PT_PAULA_VOICES];struct pt_wavetable_voice saved_av[PT_WAVETABLE_VOICES];
                unsigned width=iteration%6,steps=0,pins_before=0,pins_after=0,starts=d.starts,wstarts=wd.starts;
                memcpy(saved_pv,pv.voice,sizeof(saved_pv));memcpy(saved_av,av.voice,sizeof(saved_av));output_count=0;
                for(i=0;i<PT_CACHE_SLOTS;++i)pins_before+=pb.cache.entry[i].pins+f->cache.cache.entry[i].pins;
                if(width==5) {
                    plan->count=2;plan->action[0]=action[1];plan->action[1]=action[0];plan->action[1].channel=16;
                    assert(pt_mixed_stage_begin(owner,plan)==PT_MIXED_OWNER_CAPABILITY && !output_count);
                }
                plan->count=width==0 || width==5?5:width==4?0:width==2?3:1;
                if(width==1)plan->action[0]=action[0];
                else if(width==3)plan->action[0]=action[1];
                else for(i=0;i<plan->count;++i)plan->action[i]=action[i];
                assert(pt_mixed_stage_begin(owner,plan)==PT_MIXED_OWNER_PREPARING);
                do {
                    r=pt_mixed_stage_step(owner);assert(++steps<1000 && !output_count && d.starts==starts && wd.starts==wstarts);
                    if(width==2){assert(r==PT_MIXED_OWNER_PREPARING);break;}
                }while(r==PT_MIXED_OWNER_PREPARING);
                if(width==0 || width==2) {
                    pt_mixed_stage_cancel(owner);pt_mixed_stage_cancel(owner);
                    assert(pt_mixed_stage_commit(owner)==PT_MIXED_OWNER_INVALID && !output_count);
                    assert(!memcmp(saved_pv,pv.voice,sizeof(saved_pv)) && !memcmp(saved_av,av.voice,sizeof(saved_av)));
                    for(i=0;i<PT_CACHE_SLOTS;++i)pins_after+=pb.cache.entry[i].pins+f->cache.cache.entry[i].pins;
                    assert(pins_after==pins_before);
                }else {
                    unsigned writes=f->writes,allocs=fast_calls;size_t calls=d.calls;
                    assert(r==PT_MIXED_OWNER_OK && pt_mixed_stage_commit(owner)==PT_MIXED_OWNER_OK);
                    assert(f->writes==writes && fast_calls==allocs && d.calls==calls && output_count==plan->count);
                    if(width==1)assert(output_order[0]==2);
                    else if(width==3)assert(output_order[0]==1);
                    else if(width==5)assert(output_order[0]==2 && output_order[1]==1 && output_order[2]==2 && output_order[3]==3 && output_order[4]==4);
                    assert(pt_mixed_stage_commit(owner)==PT_MIXED_OWNER_INVALID);
                }
                assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_OK);
            }
            goto close;
        }
        if(mode>=65) {
            struct mixed_counter counter={0,48000,0,1};uint64_t deadline=777,now=1000,prior;
            unsigned polls,boundaries=0,starts,wstarts,allocs,writes,outputs,voices=mode==66?16:2;size_t calls;
#ifdef PT_TEST_MIXED_NATIVE_COST
            struct mixed_cost_clock clock={0};uint64_t before,after;unsigned service=0;
            (void)counter;
            assert(pt_native_eclock_open(&clock.clock));mixed_cost_active=&clock;
            assert(pt_mixed_owner_clocked_begin(owner,1000,mixed_cost_read,&clock)==PT_MIXED_OWNER_OK);
#else
            (void)voices;
            assert(pt_mixed_owner_clocked_begin(owner,1000,mixed_read,&counter)==PT_MIXED_OWNER_OK);
#endif
            for(polls=0;;++polls){assert(polls<1000);r=pt_mixed_owner_clocked_service(owner,&deadline);
                assert(!d.starts && !wd.starts);if(r!=PT_MIXED_OWNER_WAITING)break;}
            assert(r==PT_MIXED_OWNER_OK && deadline==1000);
#ifdef PT_TEST_MIXED_NATIVE_COST
            clock.frame=now;
#else
            counter.ticks=now;
#endif
            assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);
            assert(d.starts==(mode==66?4:1) && wd.starts==(mode==66?12:1));
            if(mode==66)for(i=0;i<16;++i)assert(output_order[i]==(i<4?1:2));
            do {
                starts=d.starts;wstarts=wd.starts;outputs=output_count;
                /* Prep at the current logical frame; one bounded job per call.
                 * Finish before advancing. This is not actual wakeup evidence. */
                for(polls=0;polls<64;++polls) {
                    calls=d.calls;allocs=fast_calls;writes=f->writes;
#ifdef PT_TEST_MIXED_NATIVE_COST
                    before=mixed_cost_tick(&clock);
#endif
                    assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);
                    assert(fast_calls==allocs && d.calls-calls<=1 && f->writes-writes<=128);
                    assert(!(d.calls!=calls && f->writes!=writes));
#ifdef PT_TEST_MIXED_NATIVE_COST
                    after=mixed_cost_tick(&clock);
                    printf("NATIVE MIXED RUN prepare voices=%u total=%lu frequency=%lu bits=%u interval=%u case=%u writes=%u chip_alloc=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,boundaries,polls,f->writes-writes,(unsigned)(d.calls-calls));
#endif
                    assert(d.starts==starts && wd.starts==wstarts && output_count==outputs);
                }
                while(deadline-now>128) {
                    now+=128;counter.ticks=now;
#ifdef PT_TEST_MIXED_NATIVE_COST
                    clock.frame=now;before=mixed_cost_tick(&clock);
#endif
                    calls=d.calls;allocs=fast_calls;writes=f->writes;
                    assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);
                    assert(d.calls==calls && fast_calls==allocs && f->writes==writes && output_count==outputs);
#ifdef PT_TEST_MIXED_NATIVE_COST
                    after=mixed_cost_tick(&clock);
                    printf("NATIVE MIXED RUN service voices=%u total=%lu frequency=%lu bits=%u interval=%u case=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,boundaries,service++);
#endif
                }
                now=deadline;counter.ticks=now;prior=deadline;calls=d.calls;allocs=fast_calls;writes=f->writes;
                output_count=0;
#ifdef PT_TEST_MIXED_NATIVE_COST
                clock.frame=now;before=mixed_cost_tick(&clock);
#endif
                r=pt_mixed_owner_clocked_service(owner,&deadline);
                assert(d.calls==calls && fast_calls==allocs && f->writes==writes);
                ++boundaries;
                if(r==PT_MIXED_OWNER_WAITING) {
                    assert(deadline>prior && d.starts==starts && wd.starts==wstarts+1);
                    assert(output_count==voices+1 && output_order[0]==2);
                    for(i=1;i<output_count;++i)assert(output_order[i]==(i<=(mode==66?4:1)?3:4));
                }else assert(r==PT_MIXED_OWNER_DONE && deadline==prior && now==1000+report.frames && !output_count);
#ifdef PT_TEST_MIXED_NATIVE_COST
                after=mixed_cost_tick(&clock);
                printf("NATIVE MIXED RUN boundary voices=%u total=%lu frequency=%lu bits=%u interval=%u result=%u outputs=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,boundaries,(unsigned)r,output_count);
#endif
            }while(r==PT_MIXED_OWNER_WAITING);
            assert(boundaries==2);
            assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_DONE);
#ifdef PT_TEST_MIXED_NATIVE_COST
            assert(pt_mixed_owner_close(&owner));mixed_cost_active=NULL;pt_native_eclock_close(&clock.clock);
            assert(!clock.clock.port && !clock.clock.request);goto detached;
#else
            goto close;
#endif
        }
        if(mode>=60) {
            struct mixed_counter counter={700,2*o.rate,0,1};uint64_t deadline=777;
            struct pt_sample_version *saved=sampler.current[0];void *context=av.api.context;
            unsigned reads,starts,wstarts,stops,wstops;
            if(mode==60) {
                ++doc.project.bpm;
                assert(pt_mixed_owner_clocked_begin(owner,1000,mixed_read,&counter)==PT_MIXED_OWNER_STALE);
                assert(!counter.reads && !d.starts && !wd.starts);goto close;
            }
            assert(pt_mixed_owner_clocked_begin(owner,1000,mixed_read,&counter)==PT_MIXED_OWNER_OK);
            do {r=pt_mixed_owner_clocked_service(owner,&deadline);assert(!d.starts && !wd.starts);}while(r==PT_MIXED_OWNER_WAITING);
            assert(r==PT_MIXED_OWNER_OK && deadline==1000);
            if(mode>=63) {
                counter.ticks=2700;
                assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);
                assert(d.starts==1 && wd.starts==1);
                d.stop_result[0]=wd.stop_result=0;
            }
            reads=counter.reads;starts=d.starts;wstarts=wd.starts;stops=d.stops;wstops=wd.stops;
            if(mode==61 || mode==64)sampler.current[0]=sampler.current[1];
            else if(mode==62)av.api.context=NULL;
            else ++doc.project.bpm;
            counter.ticks=2700;
            {uint64_t prior=deadline;
                assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_STALE && deadline==prior);
                assert(counter.reads==reads && d.starts==starts && wd.starts==wstarts);
                assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_STALE && deadline==prior && counter.reads==reads);
            }
            if(mode>=63) {
                assert(d.stops==stops+1 && wd.stops==wstops+1 && pv.voice[0].held && av.voice[7].held);
                assert(!pt_mixed_owner_close(&owner));d.stop_result[0]=wd.stop_result=1;
            }else assert(d.stops==stops && wd.stops==wstops);
            if(mode==62)assert(!pt_mixed_owner_close(&owner));
            sampler.current[0]=saved;av.api.context=context;goto close;
        }
#ifndef PT_TEST_MIXED_EXEC
        if(mode>=58){uint64_t deadline=777;unsigned polls=0,allocs,writes;size_t calls;
            assert(pt_mixed_owner_schedule_begin(owner,1000)==PT_MIXED_OWNER_OK);
            do {r=pt_mixed_owner_schedule_step(owner,0,&deadline);assert(++polls<1000 && !d.starts && !wd.starts);}while(r==PT_MIXED_OWNER_WAITING);
            assert(r==PT_MIXED_OWNER_OK);calls=d.calls;allocs=fast_calls;writes=f->writes;
            assert(pt_mixed_owner_schedule_step(owner,1000,&deadline)==PT_MIXED_OWNER_WAITING);
            assert(d.calls==calls && fast_calls==allocs && f->writes==writes && d.starts==(mode==59?4:1) && wd.starts==(mode==59?12:1));
            if(mode==59)for(i=0;i<16;++i)assert(output_order[i]==(i<4?1:2));
            goto close;
        }
#endif
#ifdef PT_TEST_MIXED_NATIVE_COMPONENTS
        if(mode>=58) {
            struct mixed_cost_clock clock={0};struct pt_render_interval interval;
            uint64_t before,after;unsigned j,polls=0,voices=mode==59?16:2,allocs,writes;size_t calls;
            assert(pt_native_eclock_open(&clock.clock));mixed_cost_active=&clock;
            for(j=0;j<4;++j) {
                calls=d.calls;allocs=fast_calls;writes=f->writes;
                before=mixed_cost_tick(&clock);assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_OK);after=mixed_cost_tick(&clock);
                assert(d.calls==calls && fast_calls==allocs && f->writes==writes && !clock.callbacks);
                printf("NATIVE MIXED COMPONENT current voices=%u total=%lu frequency=%lu bits=%u case=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,j);
            }
            before=mixed_cost_tick(&clock);assert(pt_mixed_owner_next(owner,&interval)==PT_MIXED_OWNER_OK);after=mixed_cost_tick(&clock);
            assert(interval.emit && !interval.frames && !interval.end && !clock.callbacks);
            printf("NATIVE MIXED COMPONENT next_start voices=%u total=%lu frequency=%lu bits=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits);
            do {
                calls=d.calls;allocs=fast_calls;writes=f->writes;
                before=mixed_cost_tick(&clock);r=pt_mixed_owner_prefetch(owner);after=mixed_cost_tick(&clock);
                assert(++polls<1000 && !clock.callbacks && !d.starts && !wd.starts && fast_calls==allocs);
                assert((d.calls==calls && f->writes-writes<=128) || (d.calls==calls+1 && f->writes==writes));
                assert(r==PT_MIXED_OWNER_PREPARING || r==PT_MIXED_OWNER_OK);
                printf("NATIVE MIXED COMPONENT prefetch voices=%u total=%lu frequency=%lu bits=%u case=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,polls);
            }while(r==PT_MIXED_OWNER_PREPARING);
            calls=d.calls;allocs=fast_calls;writes=f->writes;
            before=mixed_cost_tick(&clock);assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_OK);after=mixed_cost_tick(&clock);
            assert(d.calls==calls && fast_calls==allocs && f->writes==writes && clock.callbacks==voices);
            assert(d.starts==(mode==59?4:1) && wd.starts==(mode==59?12:1));
            assert(before<=clock.first && clock.first<=clock.last && clock.last<=after);
            if(mode==59)for(j=0;j<16;++j)assert(output_order[j]==(j<4?1:2));
            printf("NATIVE MIXED COMPONENT complete voices=%u before_first=%lu first_to_last=%lu last_to_return=%lu total=%lu frequency=%lu bits=%u\n",voices,(unsigned long)(clock.first-before),(unsigned long)(clock.last-clock.first),(unsigned long)(after-clock.last),(unsigned long)(after-before),(unsigned long)clock.frequency,bits);
#ifdef PT_TEST_MIXED_NATIVE_CLEAR_COST
            {
                struct pt_render_lookahead *workspace=malloc(sizeof(*workspace));
                struct pt_paula_voice saved_pv[PT_PAULA_VOICES];struct pt_wavetable_voice saved_av[PT_WAVETABLE_VOICES];
                unsigned iteration,callbacks=clock.callbacks;size_t byte;
                assert(workspace);memset(workspace,0,sizeof(*workspace));
                memcpy(saved_pv,pv.voice,sizeof(saved_pv));memcpy(saved_av,av.voice,sizeof(saved_av));
                for(iteration=0;iteration<4;++iteration) {
                    calls=d.calls;allocs=fast_calls;writes=f->writes;
                    before=mixed_cost_tick(&clock);pt_mixed_stage_cancel(owner);after=mixed_cost_tick(&clock);
                    assert(d.calls==calls && fast_calls==allocs && f->writes==writes && clock.callbacks==callbacks);
                    assert(!memcmp(saved_pv,pv.voice,sizeof(saved_pv)) && !memcmp(saved_av,av.voice,sizeof(saved_av)));
                    assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_OK);
                    printf("NATIVE MIXED COMPONENT cancel_idle voices=%u total=%lu frequency=%lu bits=%u case=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,iteration);
                    before=mixed_cost_tick(&clock);pt_render_lookahead_cancel(workspace);after=mixed_cost_tick(&clock);
                    for(byte=0;byte<sizeof(*workspace);++byte)assert(!((unsigned char *)workspace)[byte]);
                    assert(d.calls==calls && fast_calls==allocs && f->writes==writes && clock.callbacks==callbacks);
                    printf("NATIVE MIXED COMPONENT lookahead_cancel voices=%u total=%lu frequency=%lu bits=%u case=%u bytes=%lu\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits,iteration,(unsigned long)sizeof(*workspace));
                }
                free(workspace);
            }
#endif
            calls=d.calls;allocs=fast_calls;writes=f->writes;j=clock.callbacks;
            before=mixed_cost_tick(&clock);assert(pt_mixed_owner_next(owner,&interval)==PT_MIXED_OWNER_OK);after=mixed_cost_tick(&clock);
            assert(interval.emit && interval.frames && !interval.end && clock.callbacks==j && d.calls==calls && fast_calls==allocs && f->writes==writes);
            printf("NATIVE MIXED COMPONENT next_live voices=%u total=%lu frequency=%lu bits=%u\n",voices,(unsigned long)(after-before),(unsigned long)clock.frequency,bits);
            assert(pt_mixed_owner_close(&owner));mixed_cost_active=NULL;pt_native_eclock_close(&clock.clock);
            assert(!clock.clock.port && !clock.clock.request);goto detached;
        }
#elif defined(PT_TEST_MIXED_NATIVE_COST)
        if(mode>=58) {
            struct mixed_cost_clock clock={0};uint64_t before,after,deadline;unsigned polls=0,j,allocs,writes,voices=mode==59?16:2;size_t calls;
            assert(pt_native_eclock_open(&clock.clock));mixed_cost_active=&clock;
            assert(pt_mixed_owner_clocked_begin(owner,1000,mixed_cost_read,&clock)==PT_MIXED_OWNER_OK);
            do {r=pt_mixed_owner_clocked_service(owner,&deadline);assert(++polls<1000 && !d.starts && !wd.starts);}while(r==PT_MIXED_OWNER_WAITING);
            assert(r==PT_MIXED_OWNER_OK);
            for(j=0;j<4;++j){before=mixed_cost_tick(&clock);assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_OK);after=mixed_cost_tick(&clock);
                assert(before<=clock.sample_at && clock.sample_at<=after && !clock.callbacks);
                printf("NATIVE MIXED COST ready voices=%u before_sample=%lu after_sample=%lu total=%lu frequency=%lu bits=%u case=%u\n",
                    voices,(unsigned long)(clock.sample_at-before),(unsigned long)(after-clock.sample_at),(unsigned long)(after-before),(unsigned long)clock.frequency,bits,j);}
            clock.frame=1000;calls=d.calls;allocs=fast_calls;writes=f->writes;before=mixed_cost_tick(&clock);
            assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);after=mixed_cost_tick(&clock);
            assert(d.calls==calls && fast_calls==allocs && f->writes==writes && clock.callbacks==voices && d.starts==(mode==59?4:1) && wd.starts==(mode==59?12:1));
            assert(before<=clock.sample_at && clock.sample_at<=clock.first && clock.first<=clock.last && clock.last<=after);
            if(mode==59)for(j=0;j<16;++j)assert(output_order[j]==(j<4?1:2));
            printf("NATIVE MIXED COST start voices=%u before_sample=%lu sample_to_first=%lu first_to_last=%lu last_to_return=%lu total=%lu frequency=%lu bits=%u\n",
                voices,(unsigned long)(clock.sample_at-before),(unsigned long)(clock.first-clock.sample_at),(unsigned long)(clock.last-clock.first),(unsigned long)(after-clock.last),(unsigned long)(after-before),(unsigned long)clock.frequency,bits);
            assert(pt_mixed_owner_close(&owner));mixed_cost_active=NULL;pt_native_eclock_close(&clock.clock);
            assert(!clock.clock.port && !clock.clock.request);goto detached;
        }
#endif
#ifdef PT_TEST_MIXED_NATIVE_GATE
        if(mode>=56) {
            struct pt_native_eclock clock={0};struct pt_native_alarm alarm={0},watchdog={0};
            uint64_t deadline=777,tick,now,origin,frame;uint32_t frequency,initial;unsigned guard=0;
            enum pt_alarm_result ar;
            assert(pt_native_eclock_open(&clock) && pt_native_alarm_open(&alarm) && pt_native_alarm_open(&watchdog));
            assert(pt_native_eclock_read(&clock,&origin,&initial) && initial);
            /* The initial observation inside clocked_begin defines the epoch.
             * Retain an enclosing observation for diagnostic bounds only. */
            assert(pt_mixed_owner_clocked_begin(owner,o.rate,pt_native_eclock_read,&clock)==PT_MIXED_OWNER_OK);
            do {r=pt_mixed_owner_clocked_service(owner,&deadline);assert(++guard<1000 && !d.starts && !wd.starts);}while(r==PT_MIXED_OWNER_WAITING);
            assert(r==PT_MIXED_OWNER_OK && sampler.current[0] && sampler.current[1]);
            assert(pt_mixed_owner_clocked_deadline(owner,&tick)==PT_MIXED_OWNER_OK);
            assert(pt_native_alarm_arm(&watchdog,tick+initial*2)==PT_ALARM_WAITING);
            assert(pt_native_alarm_arm(&alarm,tick)==PT_ALARM_WAITING);guard=0;
            for(;;) {
                assert(pt_native_alarm_poll(&watchdog)==PT_ALARM_WAITING);
                ar=pt_native_alarm_poll(&alarm);if(ar!=PT_ALARM_WAITING)break;
                assert(++guard<=64);Wait(pt_native_alarm_signal(&alarm)|pt_native_alarm_signal(&watchdog));
            }
            assert(ar==PT_ALARM_READY);
            if(mode==57)Delay(1); /* Explicitly delayed service; never timing proof. */
            deadline=777;r=pt_mixed_owner_clocked_service(owner,&deadline);
            assert(pt_native_eclock_read(&clock,&now,&frequency) && frequency==initial && now>=tick);
            frame=(now-origin)*o.rate/initial; /* Later observation, not sampled dispatch time. */
            if(mode==57)assert(r==PT_MIXED_OWNER_DEADLINE);
            assert(r==PT_MIXED_OWNER_DEADLINE || r==PT_MIXED_OWNER_WAITING);
            if(r==PT_MIXED_OWNER_DEADLINE)assert(deadline==777 && !d.starts && !wd.starts);
            else assert(d.starts==1 && wd.starts==1 && deadline>o.rate);
            printf("NATIVE MIXED GATE observed later_frame=%lu start=%lu result=%u paula_starts=%u amigus_starts=%u injected_delay=%u frequency=%lu bits=%u\n",
                (unsigned long)frame,(unsigned long)o.rate,(unsigned)r,d.starts,wd.starts,mode==57,(unsigned long)initial,bits);
            /* Keep counter and alarm owners until combined sample ownership ends. */
            d.quiesce_result=wd.barrier_result=0;
            assert(!pt_mixed_owner_close(&owner) && owner && pv.song_owner && av.song_owner && sampler.current[0] && sampler.current[1]);
            d.quiesce_result=wd.barrier_result=1;assert(pt_mixed_owner_close(&owner));
            guard=0;while(!pt_native_alarm_close(&watchdog)){assert(++guard<=8);Delay(1);}
            assert(pt_native_alarm_close(&alarm));pt_native_eclock_close(&clock);
            assert(!alarm.port && !alarm.request && !watchdog.port && !watchdog.request && !clock.port && !clock.request);
            goto detached;
        }
#endif
        if(mode>=49) {
            struct mixed_counter counter={700,2*o.rate,0,1};uint64_t deadline=777,ticks=888,now=0,prior;unsigned polls,reads,starts,wstarts,allocs,writes;size_t calls;
            assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_INVALID && deadline==777);
            assert(pt_mixed_owner_clocked_deadline(owner,&ticks)==PT_MIXED_OWNER_INVALID && ticks==888);
            assert(pt_mixed_owner_clocked_begin(owner,1000,NULL,&counter)==PT_MIXED_OWNER_INVALID && !counter.reads);
            if(mode==50)counter.result=0;
            if(mode==54)counter.ticks=UINT64_MAX;
            if(mode==55)counter.frequency=o.rate/2;
            r=pt_mixed_owner_clocked_begin(owner,mode==55?1001:1000,mixed_read,&counter);
            if(mode==50){assert(r==PT_MIXED_OWNER_CLOCK && counter.reads==1 && !d.starts && !wd.starts);goto close;}
            assert(r==PT_MIXED_OWNER_OK && counter.reads==1);
            assert(pt_mixed_owner_clocked_begin(owner,1000,mixed_read,&counter)==PT_MIXED_OWNER_INVALID && counter.reads==1);
            assert(pt_mixed_owner_schedule_begin(owner,1000)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_schedule_step(owner,0,&deadline)==PT_MIXED_OWNER_INVALID && deadline==777);
            assert(pt_mixed_owner_clocked_service(owner,NULL)==PT_MIXED_OWNER_INVALID && counter.reads==1);
            assert(pt_mixed_owner_clocked_deadline(owner,NULL)==PT_MIXED_OWNER_INVALID && counter.reads==1);
            if(mode==54){assert(pt_mixed_owner_clocked_deadline(owner,&ticks)==PT_MIXED_OWNER_CLOCK && ticks==888);goto close;}
            if(mode==51 || mode==52){if(mode==51)++counter.frequency;else --counter.ticks;
                assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_CLOCK && deadline==777 && !d.starts && !wd.starts);goto close;}
            do {r=pt_mixed_owner_clocked_service(owner,&deadline);assert(!d.starts && !wd.starts);}while(r==PT_MIXED_OWNER_WAITING);
            assert(r==PT_MIXED_OWNER_OK);
            reads=counter.reads;assert(pt_mixed_owner_clocked_deadline(owner,&ticks)==PT_MIXED_OWNER_OK && counter.reads==reads);
            if(mode==55){assert(ticks==1201);counter.ticks=ticks;
                assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_DEADLINE && deadline==1001 && !d.starts && !wd.starts);goto close;}
            assert(ticks==2700);
            /* Half-frame observation retains fractional carry without early output. */
            counter.ticks=701;assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_OK);
            assert(pt_mixed_owner_clocked_deadline(owner,&ticks)==PT_MIXED_OWNER_OK && ticks==2700);
            counter.ticks=ticks;assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING && d.starts==1 && wd.starts==1);
            if(mode==53){unsigned stops=d.stops,wstops=wd.stops;counter.result=0;d.stop_result[0]=wd.stop_result=0;prior=deadline;
                assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_CLOCK && deadline==prior);
                assert(d.stops==stops+1 && wd.stops==wstops+1 && pv.voice[0].held && av.voice[7].held);
                reads=counter.reads;assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_CLOCK && counter.reads==reads && d.stops==stops+1);
                assert(!pt_mixed_owner_close(&owner));d.stop_result[0]=wd.stop_result=1;goto close;}
            now=1000;
            do {
                starts=d.starts;wstarts=wd.starts;
                for(polls=0;polls<64;++polls)assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);
                assert(d.starts==starts && wd.starts==wstarts);
                while(deadline-now>128){now+=128;counter.ticks=700+2*now;assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_WAITING);}
                assert(pt_mixed_owner_clocked_deadline(owner,&ticks)==PT_MIXED_OWNER_OK && ticks==700+2*deadline);
                now=deadline;counter.ticks=ticks;prior=deadline;calls=d.calls;allocs=fast_calls;writes=f->writes;
                r=pt_mixed_owner_clocked_service(owner,&deadline);assert(d.calls==calls && fast_calls==allocs && f->writes==writes);
                if(r==PT_MIXED_OWNER_WAITING)assert(deadline>prior);else assert(r==PT_MIXED_OWNER_DONE && now==1000+report.frames && deadline==prior);
            }while(r==PT_MIXED_OWNER_WAITING);
            reads=counter.reads;counter.result=0;assert(pt_mixed_owner_clocked_service(owner,&deadline)==PT_MIXED_OWNER_DONE && deadline==prior && counter.reads==reads);
            ticks=888;assert(pt_mixed_owner_clocked_deadline(owner,&ticks)==PT_MIXED_OWNER_DONE && ticks==888);
            d.quiesce_result=wd.barrier_result=0;assert(!pt_mixed_owner_close(&owner));d.quiesce_result=wd.barrier_result=1;goto close;
        }
        if(mode>=38) {
            uint64_t start_time=1000,now=0,deadline=777,prior,total_end;unsigned polls=0,starts,wstarts,allocs,writes;size_t calls;
            assert(pt_mixed_owner_schedule_step(owner,0,&deadline)==PT_MIXED_OWNER_INVALID && deadline==777);
            if(mode==39){assert(pt_mixed_owner_schedule_begin(owner,UINT64_MAX)==PT_MIXED_OWNER_CLOCK && !d.starts && !wd.starts);goto close;}
            assert(pt_mixed_owner_schedule_begin(owner,start_time)==PT_MIXED_OWNER_OK);
            assert(pt_mixed_owner_schedule_begin(owner,start_time)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_next(owner,NULL)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_consume(owner,1)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_clock_arm(owner,0)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_clock_service(owner,0)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_schedule_step(owner,0,NULL)==PT_MIXED_OWNER_INVALID && !d.starts && !wd.starts);
            if(mode==40){assert(pt_mixed_owner_schedule_step(owner,start_time,&deadline)==PT_MIXED_OWNER_DEADLINE && deadline==777 && !d.starts && !wd.starts);goto close;}
            do {
                r=pt_mixed_owner_schedule_step(owner,100,&deadline);assert(++polls<1000 && !d.starts && !wd.starts && deadline==start_time);
                if(mode==46 && d.live && !f->writes){d.quiesce_result=wd.barrier_result=0;
                    assert(!pt_mixed_owner_close(&owner) && owner && sampler.current[0] && sampler.current[1]);
                    d.quiesce_result=wd.barrier_result=1;goto close;}
            }while(r==PT_MIXED_OWNER_WAITING);
            assert(r==PT_MIXED_OWNER_OK);
            assert(pt_mixed_owner_schedule_step(owner,101,&deadline)==PT_MIXED_OWNER_OK && !d.starts && !wd.starts);
            if(mode==42){assert(pt_mixed_owner_schedule_step(owner,100,&deadline)==PT_MIXED_OWNER_CLOCK && deadline==start_time);goto close;}
            if(mode==41){assert(pt_mixed_owner_schedule_step(owner,start_time+1,&deadline)==PT_MIXED_OWNER_DEADLINE && deadline==start_time);goto close;}
            if(mode==44){d.quiesce_result=wd.barrier_result=0;assert(!pt_mixed_owner_close(&owner) && owner && !d.starts && !wd.starts);
                d.quiesce_result=wd.barrier_result=1;goto close;}
            if(mode==47){wd.start_result=0;d.stop_result[0]=wd.stop_result=0;}
            calls=d.calls;allocs=fast_calls;writes=f->writes;
            r=pt_mixed_owner_schedule_step(owner,start_time,&deadline);
            assert(d.calls==calls && fast_calls==allocs && f->writes==writes);
            if(mode==47){assert(r==PT_MIXED_OWNER_DEVICE && deadline==start_time && pv.voice[0].held && av.voice[7].held);
                assert(!pt_mixed_owner_close(&owner));d.stop_result[0]=wd.stop_result=1;goto close;}
            assert(r==PT_MIXED_OWNER_WAITING && deadline>start_time && d.starts==1 && wd.starts==1);
            now=start_time;total_end=start_time+report.frames;
            if(mode==43){d.stop_result[0]=wd.stop_result=0;prior=deadline;
                assert(pt_mixed_owner_schedule_step(owner,deadline+1,&deadline)==PT_MIXED_OWNER_DEADLINE && deadline==prior);
                assert(pv.voice[0].held && av.voice[7].held && !pt_mixed_owner_close(&owner));d.stop_result[0]=wd.stop_result=1;goto close;}
            if(mode==45){prior=deadline;assert(pt_mixed_owner_schedule_step(owner,deadline,&deadline)==PT_MIXED_OWNER_DEADLINE && deadline==prior);goto close;}
            do {
                starts=d.starts;wstarts=wd.starts;
                /* Prime the upcoming shared plan without changing live time. */
                for(polls=0;polls<64;++polls){unsigned stepwrites=f->writes;size_t stepcalls=d.calls;
                    assert(pt_mixed_owner_schedule_step(owner,now,&deadline)==PT_MIXED_OWNER_WAITING);
                    assert(d.starts==starts && wd.starts==wstarts && f->writes-stepwrites<=128 && d.calls-stepcalls<=1);}
                while(deadline-now>128){now+=128;assert(pt_mixed_owner_schedule_step(owner,now,&deadline)==PT_MIXED_OWNER_WAITING);
                    assert(d.starts==starts && wd.starts==wstarts);}
                if(mode==48)assert(!pt_cache_clear(&f->cache.cache));
                prior=deadline;now=deadline;calls=d.calls;allocs=fast_calls;writes=f->writes;
                r=pt_mixed_owner_schedule_step(owner,now,&deadline);
                assert(d.calls==calls && fast_calls==allocs && f->writes==writes);
                if(mode==48){assert(r==PT_MIXED_OWNER_CAPABILITY && deadline==prior);goto close;}
                if(r==PT_MIXED_OWNER_WAITING)assert(deadline>prior);
                else assert(r==PT_MIXED_OWNER_DONE && deadline==prior && now==total_end);
            }while(r==PT_MIXED_OWNER_WAITING);
            assert(mode==38 && d.starts==1 && wd.starts==2);
            assert(pt_mixed_owner_schedule_step(owner,now,&deadline)==PT_MIXED_OWNER_DONE && deadline==prior);
            d.quiesce_result=wd.barrier_result=0;assert(!pt_mixed_owner_close(&owner) && owner);
            d.quiesce_result=wd.barrier_result=1;goto close;
        }
        if(mode>=23) {
            struct pt_render_interval span,sentinel={123,456,789};uint64_t frames=0;
            unsigned intervals=0,polls,starts,wstarts,stops,wstops,allocs,writes;size_t calls;
            assert(pt_mixed_owner_next(owner,NULL)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_clock_service(owner,0)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_INVALID);
            assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_INVALID);
            do {
                assert(pt_mixed_owner_next(owner,&span)==PT_MIXED_OWNER_OK && ++intervals<20);
                frames+=span.frames;
                if(!span.frames)assert(pt_mixed_owner_clock_arm(owner,1000)==PT_MIXED_OWNER_INVALID);
                assert(pt_mixed_owner_next(owner,&sentinel)==PT_MIXED_OWNER_INVALID);
                assert(sentinel.frames==123 && sentinel.emit==456 && sentinel.end==789);
                assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_INVALID);
                assert(pt_mixed_owner_consume(owner,0)==PT_MIXED_OWNER_INVALID);
                assert(pt_mixed_owner_consume(owner,257)==PT_MIXED_OWNER_INVALID);
                assert(pt_mixed_owner_complete(owner)==(span.frames?PT_MIXED_OWNER_INVALID:PT_MIXED_OWNER_PREPARING));
                if(mode>=29 && span.frames && span.emit) {
                    uint64_t now=1000,end=now+span.frames;unsigned before=d.stops,wbefore=wd.stops;
                    enum pt_mixed_owner_result expected=PT_MIXED_OWNER_DEADLINE;
                    if(mode==31) {
                        assert(pt_mixed_owner_clock_arm(owner,UINT64_MAX)==PT_MIXED_OWNER_CLOCK);goto close;
                    }
                    assert(pt_mixed_owner_clock_arm(owner,now)==PT_MIXED_OWNER_OK);
                    assert(pt_mixed_owner_clock_arm(owner,now)==PT_MIXED_OWNER_INVALID);
                    assert(pt_mixed_owner_next(owner,&sentinel)==PT_MIXED_OWNER_INVALID && sentinel.frames==123);
                    assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_INVALID);
                    assert(pt_mixed_owner_consume(owner,1)==PT_MIXED_OWNER_INVALID);
                    assert(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_INVALID);
                    assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_INVALID);
                    starts=d.starts;wstarts=wd.starts;
                    if(mode==30){expected=PT_MIXED_OWNER_CLOCK;r=pt_mixed_owner_clock_service(owner,now-1);}
                    else if(mode==32 || mode==35){
                        if(mode==35)d.stop_result[0]=wd.stop_result=0;
                        r=pt_mixed_owner_clock_service(owner,end+1);
                    }else if(mode==33)r=pt_mixed_owner_clock_service(owner,end);
                    else {
                        /* Constant pre-boundary timestamps allow bounded preparation
                         * without consuming any live phase or emitting voices. */
                        for(polls=0;polls<64;++polls) {
                            unsigned stepwrites=f->writes;size_t stepcalls=d.calls;
                            assert(pt_mixed_owner_clock_service(owner,now)==PT_MIXED_OWNER_WAITING);
                            assert(f->writes-stepwrites<=128 && d.calls-stepcalls<=1 && d.starts==starts && wd.starts==wstarts);
                            if(mode==36 && polls==1) {
                                d.stop_result[0]=wd.stop_result=0;assert(!pt_mixed_owner_close(&owner));
                                d.stop_result[0]=wd.stop_result=1;goto close;
                            }
                        }
                        if(mode==34)r=pt_mixed_owner_clock_service(owner,end); /* Ready but excess live debt. */
                        else {
                            while(end-now>128) {now+=128;
                                assert(pt_mixed_owner_clock_service(owner,now)==PT_MIXED_OWNER_WAITING);
                                assert(d.starts==starts && wd.starts==wstarts);
                            }
                            if(mode==37){assert(!pt_cache_clear(&f->cache.cache));expected=PT_MIXED_OWNER_CAPABILITY;}
                            calls=d.calls;allocs=fast_calls;writes=f->writes;
                            r=pt_mixed_owner_clock_service(owner,end);
                            assert(d.calls==calls && fast_calls==allocs && f->writes==writes);
                            if(mode==29){assert(r==PT_MIXED_OWNER_OK || r==PT_MIXED_OWNER_DONE);
                                assert(pt_mixed_owner_clock_service(owner,end)==PT_MIXED_OWNER_INVALID);continue;}
                        }
                    }
                    assert(r==expected && pv.closing && av.closing);
                    assert(d.starts==starts && wd.starts==wstarts && d.stops==before+1 && wd.stops==wbefore+1);
                    assert(pt_mixed_owner_clock_service(owner,end)==expected && d.stops==before+1 && wd.stops==wbefore+1);
                    if(mode==35){assert(pv.voice[0].held && av.voice[7].held && !pt_mixed_owner_close(&owner));
                        d.stop_result[0]=wd.stop_result=1;}
                    goto close;
                }
                if(span.frames>256){assert(pt_mixed_owner_consume(owner,256)==PT_MIXED_OWNER_OK);span.frames-=256;}
                starts=d.starts;wstarts=wd.starts;stops=d.stops;wstops=wd.stops;polls=0;
                if(mode==25 && intervals==2) {
                    d.stop_result[0]=wd.stop_result=0;doc.project.bpm=151;
                    assert(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_STALE);
                    assert(d.stops==stops+1 && wd.stops==wstops+1 && pv.voice[0].held && av.voice[7].held);
                    assert(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_STALE && d.stops==stops+1 && wd.stops==wstops+1);
                    assert(!pt_mixed_owner_close(&owner));d.stop_result[0]=wd.stop_result=1;goto close;
                }
                if(mode==26 && intervals==2){f->cache.cache.budget=0;assert(!pt_cache_trim(&f->cache.cache,4096));}
                do {
                    unsigned stepwrites=f->writes;size_t stepcalls=d.calls;
                    r=pt_mixed_owner_prefetch(owner);assert(++polls<2000 && f->writes-stepwrites<=128 && d.calls-stepcalls<=1);
                    assert(d.starts==starts && wd.starts==wstarts);
                    if(mode==24 && d.live && !f->writes)goto close;
                    if(polls==1 && span.frames>17){assert(pt_mixed_owner_consume(owner,17)==PT_MIXED_OWNER_OK);span.frames-=17;}
                    if(r==PT_MIXED_OWNER_PREPARING)assert(pt_mixed_owner_complete(owner)==(span.frames?PT_MIXED_OWNER_INVALID:PT_MIXED_OWNER_PREPARING));
                }while(r==PT_MIXED_OWNER_PREPARING);
                if(mode==26 && intervals==2){assert(r==PT_MIXED_OWNER_CAPABILITY && pv.closing && av.closing);goto close;}
                assert(r==PT_MIXED_OWNER_OK && pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_OK);
                assert(d.stops==stops && wd.stops==wstops);
                if(mode==28 && intervals==2){d.quiesce_result=wd.barrier_result=0;
                    assert(!pt_mixed_owner_close(&owner) && owner && sampler.current[0] && sampler.current[1]);
                    d.quiesce_result=wd.barrier_result=1;goto close;}
                if(span.frames)assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_INVALID);
                while(span.frames){unsigned chunk=span.frames>256?256:span.frames;
                    assert(pt_mixed_owner_consume(owner,chunk)==PT_MIXED_OWNER_OK);span.frames-=chunk;}
                assert(pt_mixed_owner_consume(owner,1)==PT_MIXED_OWNER_INVALID);
                calls=d.calls;allocs=fast_calls;writes=f->writes;
                if(mode==27 && intervals==1){wd.start_result=0;wd.stop_result=0;d.stop_result[0]=0;}
                r=pt_mixed_owner_complete(owner);assert(d.calls==calls && fast_calls==allocs && f->writes==writes);
                if(mode==27){assert(r==PT_MIXED_OWNER_DEVICE && pv.closing && av.closing && av.voice[7].held);
                    assert(!pt_mixed_owner_close(&owner));d.stop_result[0]=wd.stop_result=1;goto close;}
                assert(r==PT_MIXED_OWNER_OK || r==PT_MIXED_OWNER_DONE);
                assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_INVALID);
            }while(r!=PT_MIXED_OWNER_DONE);
            assert((mode==23 || mode==29) && frames==report.frames && intervals==report.intervals && d.starts==1 && wd.starts==2);
            assert(pt_mixed_owner_next(owner,&sentinel)==PT_MIXED_OWNER_DONE && sentinel.frames==123);
            assert(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_INVALID);
            assert(pv.voice[0].held && av.voice[7].held);
            d.quiesce_result=wd.barrier_result=0;
            assert(!pt_mixed_owner_close(&owner) && owner && sampler.current[0] && sampler.current[1]);
            d.quiesce_result=wd.barrier_result=1;
            goto close;
        }
        if(mode>=7){struct pt_voice voice;unsigned polls=0,starts=d.starts,wstarts=wd.starts;
            assert(pt_voice_init(&voice,&doc.project.samples[0].pcm,0,2048,PT_VOICE_ONCE,0,0,((uint64_t)8000<<32)/48000,0)==PT_PCM_OK);
            plan->count=3;plan->action[0]=(struct pt_render_action){PT_RENDER_TRIGGER,4,voice,{65536,0}};
            plan->action[1]=(struct pt_render_action){PT_RENDER_TRIGGER,7,voice,{32768,32768}};
            plan->action[1].voice.pcm=&doc.project.samples[1].pcm;plan->action[2]=plan->action[1];
            if(mode>=15) {
                /* AmiGUS first, Paula second, duplicate AmiGUS last: emitting
                 * either whole backend at once violates original order. */
                struct pt_render_action swap=plan->action[0];plan->action[0]=plan->action[1];plan->action[1]=swap;
                plan->count=5;plan->action[3]=plan->action[1];plan->action[3].kind=PT_RENDER_CONTROL;
                plan->action[4]=plan->action[0];plan->action[4].kind=PT_RENDER_CONTROL;
                if(mode==22)plan->action[3].kind=PT_RENDER_STOP;
            }
            if(mode==11)plan->action[2].voice.pcm=(void *)(uintptr_t)1;
            if(mode==12)plan->action[2].voice.pcm=&doc.project.samples[2].pcm;
            if(mode==11 || mode==12){assert(pt_mixed_stage_begin(owner,plan)==PT_MIXED_OWNER_CAPABILITY && !d.live && !f->writes);goto close;}
            assert(pt_mixed_stage_begin(owner,plan)==PT_MIXED_OWNER_PREPARING && !d.live && !f->writes);
            assert(pt_mixed_stage_begin(owner,plan)==PT_MIXED_OWNER_INVALID);
            if(mode==9)f->cache.cache.budget=0;
            if(mode==10)f->fail=f->writes+1;
            do{unsigned writes=f->writes;size_t calls=d.calls;
                r=pt_mixed_stage_step(owner);assert(++polls<1000 && f->writes-writes<=128 && d.calls-calls<=1);
                assert(d.starts==starts && wd.starts==wstarts);
                if((mode==8 || mode==14) && d.live && !f->writes){
                    if(mode==14){doc.project.bpm=150;assert(pt_mixed_stage_step(owner)==PT_MIXED_OWNER_STALE);}
                    else pt_mixed_stage_cancel(owner);
                    assert(!d.live && !f->writes);goto close;
                }
                if(mode==13 && d.live && !f->writes){av.voice[7].uncertain=1;
                    assert(pt_mixed_stage_step(owner)==PT_MIXED_OWNER_CAPABILITY);av.voice[7].uncertain=0;goto close;}
            }while(r==PT_MIXED_OWNER_PREPARING);
            if(mode==9 || mode==10)assert(r==PT_MIXED_OWNER_CAPABILITY);
            else {
                unsigned pins=0;assert(r==PT_MIXED_OWNER_OK && pt_mixed_stage_step(owner)==PT_MIXED_OWNER_OK);
                for(i=0;i<PT_CACHE_SLOTS;++i)pins+=f->cache.cache.entry[i].pins;
                assert(pins==2);pins=0;for(i=0;i<PT_CACHE_SLOTS;++i)pins+=pb.cache.entry[i].pins;assert(pins==1);
                if(mode>=15) {
                    unsigned writes=f->writes,allocs=fast_calls;size_t calls=d.calls;
                    if(mode==16){wd.start_result=0;wd.stop_result=0;}
                    if(mode==17){d.start_result=-1;d.stop_result[0]=0;wd.stop_result=-1;}
                    if(mode==18)assert(!pt_cache_clear(&f->cache.cache));
                    if(mode==19)assert(!pt_cache_clear(&pb.cache));
                    if(mode==20)av.voice[7].uncertain=1;
                    if(mode==21)wd.stop_result=0;
                    if(mode==22)d.stop_result[0]=0;
                    r=pt_mixed_stage_commit(owner);
                    assert(f->writes==writes && d.calls==calls && fast_calls==allocs);
                    if(mode>=18 && mode<=20) {
                        assert(r==PT_MIXED_OWNER_CAPABILITY && !output_count && !d.stops && !wd.stops);
                        av.voice[7].uncertain=0;
                    }else if(mode==16 || mode==17 || mode==21 || mode==22) {
                        assert(r==PT_MIXED_OWNER_DEVICE && pv.closing && av.closing);
                        assert(pt_mixed_stage_commit(owner)==PT_MIXED_OWNER_DEVICE);
                        assert(sampler.current[0] && sampler.current[1]);
                        if(mode==22)assert(!av.voice[7].held);else assert(av.voice[7].held);
                        if(mode!=22)assert(av.voice[7].uncertain);
                        if(mode==16)assert(output_count==1 && output_order[0]==2 && !pv.voice[0].held);
                        else if(mode==17)assert(output_count==2 && output_order[0]==2 && output_order[1]==1 && pv.voice[0].held);
                        else if(mode==21)assert(!pv.voice[0].held && wd.stops==1 && output_count==2);
                        else assert(pv.voice[0].held && d.stops==1 && output_count==3);
                        wd.barrier_result=0;assert(!pt_mixed_owner_close(&owner) && owner);
                        d.stop_result[0]=wd.stop_result=1;wd.barrier_result=0;
                        assert(!pt_mixed_owner_close(&owner) && owner);
                        wd.barrier_result=1;
                    }else {
                        assert(r==PT_MIXED_OWNER_OK && output_count==5);
                        assert(output_order[0]==2 && output_order[1]==1 && output_order[2]==2 && output_order[3]==3 && output_order[4]==4);
                        assert(pv.voice[0].held && av.voice[7].held && !pv.voice[0].uncertain && !av.voice[7].uncertain);
                        pins=0;for(i=0;i<PT_CACHE_SLOTS;++i)pins+=f->cache.cache.entry[i].pins;assert(pins==1);
                        assert(pt_mixed_stage_commit(owner)==PT_MIXED_OWNER_INVALID);
                    }
                    goto close;
                }
                pt_mixed_stage_cancel(owner);assert(pt_mixed_stage_step(owner)==PT_MIXED_OWNER_INVALID);
            }
            for(i=0;i<PT_CACHE_SLOTS;++i)assert(!pb.cache.entry[i].pins && !f->cache.cache.entry[i].pins);
        }
        if(mode==5){struct pt_voice voice;struct pt_cache_lease lease;
            assert(pt_voice_init(&voice,&doc.project.samples[0].pcm,0,2048,PT_VOICE_ONCE,0,0,((uint64_t)8000<<32)/48000,0)==PT_PCM_OK);
            plan->count=1;plan->action[0]=(struct pt_render_action){PT_RENDER_TRIGGER,4,voice,{65536,0}};
            assert(pt_paula_dispatch_owned(&pv,pb.version,48000,plan,&caps,batch,owner)==1);
            /* Controlled injected retained reader for the future combined batch;
             * this fixture does not claim a mixed live dispatcher exists. */
            assert(pt_sampler_wavetable_acquire(&ab,1,&format,staging,sizeof(staging),&lease)==PT_CACHE_LOAD);
            av.voice[7].lease=lease;av.voice[7].held=av.voice[7].uncertain=1;wd.starts=1;
            d.stop_result[0]=0;wd.barrier_result=0;
            assert(!pt_mixed_owner_close(&owner) && owner && pv.song_owner==owner && av.song_owner==owner);
            assert(pv.voice[0].held && !av.voice[7].held && sampler.current[0] && sampler.current[1]);
            d.stop_result[0]=1;d.quiesce_result=0;assert(!pt_mixed_owner_close(&owner));
            d.quiesce_result=1;wd.barrier_result=-1;assert(!pt_mixed_owner_close(&owner));oldbarriers=d.barriers;
            wd.barrier_result=1;assert(pt_mixed_owner_close(&owner) && !owner && d.barriers==oldbarriers);
            goto detached;
        }
        if(mode==6){void *saved=av.api.context;av.api.context=NULL;
            assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_STALE && !pt_mixed_owner_close(&owner));av.api.context=saved;}
    }
close:assert(pt_mixed_owner_close(&owner) && !owner);
detached:
    assert(!pv.song_owner && !av.song_owner && pv.closing && av.closing);
    assert(pt_paula_voices_close(&pv) && pt_wavetable_voices_close(&av) && !d.live);
    assert(pt_amigus_wavetable_cache_detach(&f->cache));assert(pt_amigus_reservation_close(&f->reservation));
    assert(doc.project.samples[0].pcm.bits==bits && doc.project.samples[0].pcm.data[0]==1);
    pt_sampler_release(&sampler);assert(!sampler.bytes);pt_document_release(&doc);free(plan);free(batch);free(f);
}
static int mixed_owner_fixture(void){unsigned bits,mode;(void)fixture;(void)wavetable_fixture_main;
#ifdef PT_TEST_MIXED_NATIVE_COMPONENTS
    for(bits=8;bits<=24;bits+=8)for(mode=58;mode<60;++mode)owner_fixture(bits,mode);
    for(bits=8;bits<=24;bits+=8)for(mode=65;mode<67;++mode)owner_fixture(bits,mode);
    for(bits=8;bits<=24;bits+=8)owner_fixture(bits,67);
    puts("MIXED OWNER PASS:15 native retirement8/16/24 scenarios,6component/6running/3cancel-reuse, manual source/current/next/prefetch/complete public calls, bounded preparation, allocation/upload-free commit/next, global action order, resource cleanup; injected voices, no timed playback acceptance");
#elif defined(PT_TEST_MIXED_NATIVE_STARTUP)
    for(bits=8;bits<=24;bits+=8)for(mode=58;mode<60;++mode)owner_fixture(bits,mode);
    puts("MIXED OWNER PASS:6 native startup-cost8/16/24 scenarios, fully prepared2/16voice startup, allocation/upload-free exact commit, global action order, resource cleanup; injected logical time/voices, no audio/timing acceptance");
#elif defined(PT_TEST_MIXED_NATIVE_COST)
    for(bits=8;bits<=24;bits+=8)for(mode=65;mode<67;++mode)owner_fixture(bits,mode);
    puts("MIXED OWNER PASS:6 native running-cost8/16/24 scenarios, bounded preparation, allocation/upload-free running and tempo/trigger/end boundaries, global action order, full frame duration and resource cleanup; injected logical time/voices, no audio/timing acceptance");
#elif defined(PT_TEST_MIXED_NATIVE_GATE)
    for(bits=8;bits<=24;bits+=8)for(mode=56;mode<58;++mode)owner_fixture(bits,mode);
    puts("MIXED OWNER PASS:6 native real-clock/alarm8/16/24 scenarios, primed mixed startup, strict observed or delayed deadline refusal, uncertain barriers retain both tokens/masters, alarm/watchdog/counter cleanup; injected voices only, no audio/timing acceptance");
#elif defined(PT_TEST_MIXED_SCHEDULE_ONLY)
    /* Keep the already-qualified legacy cases in full host regressions; this
     * separate native window qualifies the new scheduling scenarios only. */
    for(bits=8;bits<=24;bits+=8)for(mode=49;mode<56;++mode)owner_fixture(bits,mode);
    puts("MIXED OWNER PASS:21 native clock-binding-only8/16/24 scenarios, fractional counter conversion, absolute tempo boundaries, reader/frequency/regression/overflow/skipped-frame refusal and retained-reader cleanup; injected counters only");
#else
    for(bits=8;bits<=24;bits+=8)for(mode=0;mode<56;++mode)owner_fixture(bits,mode);
    for(bits=8;bits<=24;bits+=8)for(mode=58;mode<68;++mode)owner_fixture(bits,mode);
    puts("MIXED OWNER PASS: full198 host scenarios, master/cache/staging/commit/sequence/deadline/schedule ownership; no native clock");
#endif
    return 0;}

#ifndef PT_TEST_MIXED_EXEC
int main(void){return mixed_owner_fixture();}
#endif
