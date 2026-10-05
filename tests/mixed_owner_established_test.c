#define PT_TEST_MIXED_OWNER_INCLUDED
#include "mixed_owner_test.c"
#include "../src/editor/mixed_owner_established.h"
#include "../src/editor/mixed_owner_state_internal.h"
#include "../src/editor/sampler_paula_internal.h"
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define POISON(p,n) __asan_poison_memory_region(p,n)
#define UNPOISON(p,n) __asan_unpoison_memory_region(p,n)
#endif
#endif
#ifndef POISON
#define POISON(p,n) ((void)(p),(void)(n))
#define UNPOISON(p,n) ((void)(p),(void)(n))
#endif
struct checked_allocator {
    unsigned calls,live,releases,fail,mode,alias_releases;
    void *alias;
    struct pt_mixed_owner **owner;
    struct pt_mixed_established *control;
    struct pt_project *project;
};
static void *checked_allocate(void *context,size_t n)
{
    struct checked_allocator *a=context;void *p;++a->calls;
    if(a->mode==1)++a->project->bpm;
    if(a->mode==2&&*a->owner)assert(!pt_mixed_owner_close(a->owner));
    if(a->mode==3&&*a->owner)assert(pt_mixed_owner_prepare(*a->owner,NULL)==PT_MIXED_OWNER_STALE);
    if(a->mode==4)assert(!pt_mixed_owner_established_finish(a->control));
    if(a->alias)return a->alias;
    if(a->calls==a->fail)return NULL;
    p=malloc(n);if(p)++a->live;return p;
}
static void checked_release(void *context,void *p)
{
    struct checked_allocator *a=context;
    if(p==a->alias){++a->alias_releases;return;}
    assert(p&&a->live);--a->live;++a->releases;free(p);
}
static void checked_fixture(unsigned bits,unsigned mode)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;struct pt_sampler sampler;
    struct pt_sampler_paula pb={0};struct pt_sampler_wavetable ab={0};
    struct pt_paula_voices pv={0};struct pt_wavetable_voices av={0};
    struct driver d={0};struct wave_driver wd={0};struct fixture *f=malloc(sizeof(*f));
    struct pt_paula_voice_api pa={&d,mixed_start,stop,mixed_control};
    struct pt_wavetable_voice_api aa={&wd,wave_start,wave_stop,wave_control,NULL};
    struct pt_render_options o={0};struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_playback_format format={8,0,0,0};struct pt_mixed_established c={0};
    struct pt_mixed_owner *owner=NULL;struct pt_mixed_report report;
    struct checked_allocator ca={0};struct pt_allocator checked={&ca,checked_allocate,checked_release};
    struct pt_sampler_storage_span extra[3]={{&ca,sizeof(ca)},{&d,sizeof(d)},{&wd,sizeof(wd)}};
    int32_t pcm[2048];struct pt_sample samples[3];struct pt_sample_version *master[3];
    struct pt_pcm held;struct pt_sample_version *pin;unsigned i,n=0,calls,refs[2];size_t bytes;
    enum pt_mixed_owner_result r;struct pt_render_interval interval;
    assert(f);for(i=0;i<2048;++i)pcm[i]=(int32_t)(i%120)+1;
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0,4096,4096,f,bus_owned,bus_write));
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,16,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<16;++i)doc.project.channels.track[i].route=PT_AMIGUS;
    doc.project.channels.track[4].route=PT_PAULA;doc.project.channels.track[4].pan=0;
    for(i=0;i<3;++i){doc.project.samples[i].pcm=(struct pt_pcm){pcm,2048,2048,8000,1,(uint8_t)bits};doc.project.samples[i].volume=64;}
    doc.project.speed=1;doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[7]=doc.project.events[4];doc.project.events[16+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    doc.project.events[32+15].effect=15;
    o.rate=48000;o.bits=24;o.tracks=(1U<<4)|(1U<<7);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    pt_sampler_init(&sampler,&a,1024*1024);
    if(mode!=3)for(i=0;i<2;++i){assert(pt_sampler_pin(&sampler,&doc.project,i,sampler.generation,&held,&pin)==PT_EDIT_OK);pt_sampler_unpin(pin);}
    assert(pt_sampler_paula_bind(&pb,&sampler,&doc.project,&d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&ab,&sampler,&doc.project,&f->cache));
    assert(pt_paula_voices_bind(&pv,&pb,&pa)&&pt_wavetable_voices_bind(&av,&ab,&aa));
    assert(pt_paula_voices_bind_quiesce(&pv,quiesce,&d)&&pt_wavetable_voices_bind_quiesce(&av,wave_quiesce,&wd));
    d.start_result=d.control_result=d.quiesce_result=wd.start_result=wd.stop_result=wd.barrier_result=1;
    for(i=0;i<4;++i)d.stop_result[i]=1;
    output_count=0;ca.owner=&owner;ca.control=&c;ca.project=&doc.project;
    memcpy(samples,doc.project.samples,sizeof(samples));memcpy(master,sampler.current,sizeof(master));bytes=sampler.bytes;
    /* Constructor and INITIAL begin must not read semantic payloads. */
    POISON(doc.project.orders,doc.project.order_count*sizeof(*doc.project.orders));
    POISON(doc.project.events,doc.project.pattern_count*64*16*sizeof(*doc.project.events));
    for(i=0;i<3;++i)POISON(doc.project.samples[i].pcm.data,doc.project.samples[i].pcm.capacity*sizeof(int32_t));
    if(mode>=10&&mode<=18) {
        switch(mode){case 10:ca.alias=&pv;break;case 11:ca.alias=&av;break;case 12:ca.alias=&pb;break;
        case 13:ca.alias=&ab;break;case 14:ca.alias=&f->cache;break;case 15:ca.alias=&f->reservation;break;
        case 16:ca.alias=&c;break;case 17:ca.alias=&ca;break;case 18:ca.alias=doc.project.samples[2].pcm.data+2047;break;}
    }
    if(mode==19)ca.fail=1;
    if(mode==20)ca.mode=1;
    if(mode==21)ca.mode=4;
    r=pt_mixed_owner_established_begin(&c,&pv,&av,&o,&caps,&format,&checked,extra,3,77,8,&owner);
    if(mode>=10&&mode<=21){assert((r==PT_MIXED_OWNER_MEMORY||r==PT_MIXED_OWNER_STALE)&&!owner);goto unpoison;}
    assert(r==PT_MIXED_OWNER_PREPARING&&owner&&ca.calls==1&&ca.live==1&&!f->writes&&!d.starts&&!wd.starts);
    assert(!sampler.current[2]);
    assert(pt_mixed_owner_prepare(owner,&report)==PT_MIXED_OWNER_PREPARING&&c.startup&&ca.calls==3&&ca.live==3);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!report.samples[0][i]&&!report.samples[1][i]);
unpoison:
    UNPOISON(doc.project.orders,doc.project.order_count*sizeof(*doc.project.orders));
    UNPOISON(doc.project.events,doc.project.pattern_count*64*16*sizeof(*doc.project.events));
    for(i=0;i<3;++i)UNPOISON(doc.project.samples[i].pcm.data,doc.project.samples[i].pcm.capacity*sizeof(int32_t));
    if(!owner){assert(!ca.live&&!ca.alias_releases&&!pv.song_owner&&!av.song_owner);goto released;}
    calls=ca.calls;
    if(mode==1)goto close;
    if(mode==2){++doc.project.bpm;assert(pt_mixed_owner_current(owner)==PT_MIXED_OWNER_STALE);goto close;}
    if(mode==4){ca.mode=2; /* Reentry during checked-sequence allocation, after INITIAL READY. */
        while(pt_mixed_preflight_setup_get(c.startup,77,sampler.generation,NULL)!=PT_RENDER_SETUP_READY)
            assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_PREPARING&&++n<5000);
        assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_STALE);ca.mode=0;goto close;}
    if(mode==5){ca.mode=3;
        while(pt_mixed_preflight_setup_get(c.startup,77,sampler.generation,NULL)!=PT_RENDER_SETUP_READY)
            assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_PREPARING&&++n<5000);
        assert(pt_mixed_owner_prepare(owner,NULL)==PT_MIXED_OWNER_STALE);ca.mode=0;goto close;}
    if(mode==6){ca.fail=calls+1;
        do{r=pt_mixed_owner_prepare(owner,NULL);assert(++n<5000);}while(r==PT_MIXED_OWNER_PREPARING);
        assert(r==PT_MIXED_OWNER_MEMORY);goto close;}
    /* Refuse report aliases without advancing or overwriting any parent/source. */
    assert(pt_mixed_owner_prepare(owner,(struct pt_mixed_report *)(void *)&pv)==PT_MIXED_OWNER_INVALID);
    assert(pt_mixed_owner_prepare(owner,(struct pt_mixed_report *)(void *)doc.project.events)==PT_MIXED_OWNER_INVALID);
    assert(pt_mixed_owner_prepare(owner,(struct pt_mixed_report *)(void *)owner)==PT_MIXED_OWNER_INVALID);
    assert(ca.calls==calls);
    do{r=pt_mixed_owner_prepare(owner,&report);assert(++n<5000&&!f->writes&&!d.starts&&!wd.starts&&!output_count);
        assert(sampler.bytes==bytes&&!memcmp(master,sampler.current,sizeof(master))&&!memcmp(samples,doc.project.samples,sizeof(samples)));
    }while(r==PT_MIXED_OWNER_PREPARING);
    if(mode==3){assert(r==PT_MIXED_OWNER_STALE&&!sampler.current[0]&&!sampler.current[1]);goto close;}
    assert(r==PT_MIXED_OWNER_OK&&ca.calls==calls+1&&owner->sequence&&owner->analyzed&&owner->ready);
    assert(report.samples[0][0]&&report.samples[1][0]&&report.samples[1][1]&&!report.samples[0][2]&&!report.samples[1][2]);
    for(i=0;i<2;++i){assert(pt_sampler_pin_current(&sampler,&doc.project,i,sampler.generation,master[i],&held,&pin)==PT_EDIT_OK);pt_sampler_unpin(pin);}
    refs[0]=owner->pin[0]!=NULL;refs[1]=owner->pin[1]!=NULL;assert(refs[0]&&refs[1]&&!owner->pin[2]);
    assert(!pt_mixed_owner_established_finish(&c));
    if(mode==7){
        uint64_t deadline=0;
        assert(pt_mixed_owner_schedule_begin(owner,100)==PT_MIXED_OWNER_OK);
        do{r=pt_mixed_owner_schedule_step(owner,0,&deadline);assert(++n<6000);}while(r==PT_MIXED_OWNER_PREPARING);
        assert(r==PT_MIXED_OWNER_WAITING&&!output_count);
        assert(pt_mixed_owner_schedule_step(owner,101,&deadline)==PT_MIXED_OWNER_DEADLINE);
        assert(!d.starts&&!wd.starts);goto close;
    }
    assert(pt_mixed_owner_next(owner,(struct pt_render_interval *)(void *)&c)==PT_MIXED_OWNER_INVALID);
    assert(pt_mixed_owner_next(owner,&interval)==PT_MIXED_OWNER_OK);
    if(mode==8){while(pt_mixed_owner_prefetch(owner)==PT_MIXED_OWNER_PREPARING)assert(++n<6000);
        while(owner->remaining)assert(pt_mixed_owner_consume(owner,owner->remaining>256?256:owner->remaining)==PT_MIXED_OWNER_OK);
        assert(pt_mixed_owner_complete(owner)==PT_MIXED_OWNER_OK&&d.starts&&wd.starts);
    }
close:
    /* Neither a failed nor successful owner may drop pins before BOTH drains. */
    d.quiesce_result=wd.barrier_result=0;
    assert(!pt_mixed_owner_close(&owner)&&owner&&pv.song_owner==owner&&av.song_owner==owner);
    assert(!c.startup&&!owner->analysis&&!owner->sequence&&ca.live==1);
    if(mode!=3)assert(sampler.current[0]==master[0]&&sampler.current[1]==master[1]);
    d.quiesce_result=1;assert(!pt_mixed_owner_close(&owner)&&owner&&ca.live==1);
    wd.barrier_result=1;assert(pt_mixed_owner_close(&owner)&&!owner&&!ca.live);
released:
    ca.alias=NULL;ca.mode=0;
    assert(pt_mixed_owner_established_finish(&c)&&!c.storage.memory.active&&!ca.alias_releases);
    assert(pt_paula_voices_close(&pv)&&pt_wavetable_voices_close(&av)&&!d.live);
    assert(pt_amigus_wavetable_cache_detach(&f->cache)&&pt_amigus_reservation_close(&f->reservation));
    pt_sampler_release(&sampler);pt_document_release(&doc);free(f);
}
int main(void)
{
    unsigned bits,mode; /* Same binary retains the original 255 owner regressions. */
    assert(!mixed_owner_fixture());
    for(bits=8;bits<=24;bits+=8)for(mode=0;mode<=21;++mode)checked_fixture(bits,mode);
    puts("ESTABLISHED OWNER PASS:66 8/16/24 scenarios; cancellable actual setup/audit/same sequence; source-preserving required union pins; aliases/reentry/cancel/dual drain; unchanged exact late-start refusal; injected callbacks only");
    return 0;
}
