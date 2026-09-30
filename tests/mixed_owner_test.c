#define main paula_legacy_main
#include "paula_voices_test.c"
#undef main
#define PT_WAVETABLE_NATIVE
#include "amigus_wavetable_cache_test.c"
#include "../src/editor/mixed_owner_internal.h"
#include "../src/editor/paula_internal.h"
struct wave_driver {unsigned starts,stops,barriers;int stop_result,barrier_result;};
static int wave_start(void *c,unsigned id,const struct pt_amigus_voice_plan *p)
{struct wave_driver *d=c;(void)id;(void)p;++d->starts;return 1;}
static int wave_stop(void *c,unsigned id){struct wave_driver *d=c;(void)id;++d->stops;return d->stop_result;}
static int wave_control(void *c,unsigned id,uint32_t rate,uint16_t l,uint16_t r)
{(void)c;(void)id;(void)rate;(void)l;(void)r;return 1;}
static int wave_quiesce(void *c){struct wave_driver *d=c;++d->barriers;return d->barrier_result;}
static void *refuse_alloc(void *c,size_t n){(void)c;(void)n;return NULL;}
static void owner_fixture(unsigned bits,unsigned mode)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula pb={0};struct pt_sampler_wavetable ab={0};
    struct pt_paula_voices pv={0};struct pt_wavetable_voices av={0};
    struct driver d={0};struct wave_driver wd={0};struct fixture *f=malloc(sizeof(*f));
    struct pt_paula_voice_api pa={&d,start,stop,control};struct pt_wavetable_voice_api aa={&wd,wave_start,wave_stop,wave_control,NULL};
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
    o.rate=48000;o.bits=24;o.tracks=(1U<<4)|(1U<<7);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    pt_sampler_init(&sampler,&a,1024*1024);
    assert(pt_sampler_paula_bind(&pb,&sampler,&doc.project,&d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&ab,&sampler,&doc.project,&f->cache));
    assert(pt_paula_voices_bind(&pv,&pb,&pa));assert(pt_wavetable_voices_bind(&av,&ab,&aa));
    assert(pt_paula_voices_bind_quiesce(&pv,quiesce,&d));assert(pt_wavetable_voices_bind_quiesce(&av,wave_quiesce,&wd));
    d.start_result=d.quiesce_result=wd.stop_result=wd.barrier_result=1;for(i=0;i<4;++i)d.stop_result[i]=1;
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
        if(mode>=7){struct pt_voice voice;unsigned polls=0,starts=d.starts,wstarts=wd.starts;
            assert(pt_voice_init(&voice,&doc.project.samples[0].pcm,0,2048,PT_VOICE_ONCE,0,0,((uint64_t)8000<<32)/48000,0)==PT_PCM_OK);
            plan->count=3;plan->action[0]=(struct pt_render_action){PT_RENDER_TRIGGER,4,voice,{65536,0}};
            plan->action[1]=(struct pt_render_action){PT_RENDER_TRIGGER,7,voice,{32768,32768}};
            plan->action[1].voice.pcm=&doc.project.samples[1].pcm;plan->action[2]=plan->action[1];
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
    for(bits=8;bits<=24;bits+=8)for(mode=0;mode<15;++mode)owner_fixture(bits,mode);
    puts("MIXED OWNER PASS: exclusive engines, union masters, bounded promotion, stale/cancel/refusal and both-reader retention, coordinated bounded cache staging and cancellation; no scheduler");return 0;}

#ifndef PT_TEST_MIXED_EXEC
int main(void){return mixed_owner_fixture();}
#endif
