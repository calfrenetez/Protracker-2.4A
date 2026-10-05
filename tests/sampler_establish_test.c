#define PT_TEST_MIXED_OWNER_INCLUDED
#include "mixed_owner_test.c"
#include "../src/editor/sampler_establish.h"
#include "../src/editor/mixed_owner_established.h"
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
struct establishment_fixture {
    struct pt_sampler_establish c;struct pt_document document;struct pt_sampler sampler;
    struct pt_allocator document_allocator,master_allocator,control_allocator;
    struct pt_sampler_storage_span parent;
    struct pt_extension extension;uint8_t payload[8];uint32_t markers[2];
    int32_t *pcm;void *alias;
    unsigned control_calls,master_calls,live_controls,live_masters,releases,alias_releases,fail,hook;
};
static void hook(struct establishment_fixture *g)
{
    if(g->hook==1)assert(!pt_sampler_establish_cancel(&g->c));
    if(g->hook==2)assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_FAULT);
    if(g->hook==3)++g->document.project.bpm;
}
static void *control_allocate(void *context,size_t n)
{
    struct establishment_fixture *g=context;void *p;++g->control_calls;hook(g);
    if(g->alias)return g->alias;
    if(g->fail==1)return NULL;
    p=malloc(n);if(p)++g->live_controls;return p;
}
static void control_release(void *context,void *p)
{
    struct establishment_fixture *g=context;
    if(p==g->alias){++g->alias_releases;return;}
    assert(p&&g->live_controls);--g->live_controls;++g->releases;free(p);
}
static void *master_allocate(void *context,size_t n)
{
    struct establishment_fixture *g=context;void *p;++g->master_calls;
    assert(!g->c.validation.project); /* Actual validator borrow ended before ANY master. */
    hook(g);if(g->alias)return g->alias;
    if(g->fail==2)return NULL;
    p=malloc(n);if(p)++g->live_masters;return p;
}
static void master_release(void *context,void *p)
{
    struct establishment_fixture *g=context;
    if(p==g->alias){++g->alias_releases;return;}
    assert(p&&g->live_masters);--g->live_masters;++g->releases;free(p);
}
static struct establishment_fixture *make_fixture(unsigned bits,unsigned channels)
{
    struct establishment_fixture *g=calloc(1,sizeof(*g));unsigned i,j;assert(g);
    g->pcm=malloc(3*2048*sizeof(*g->pcm));assert(g->pcm);
    for(i=0;i<3;++i)for(j=0;j<2048;++j)g->pcm[i*2048+j]=((int32_t)(j%120)-60)*(bits==8?1:bits==16?251:65537)+(bits==8?0:bits==16?17:31);
    g->document_allocator=(struct pt_allocator){NULL,fast_alloc,fast_free};
    g->master_allocator=(struct pt_allocator){g,master_allocate,master_release};
    g->control_allocator=(struct pt_allocator){g,control_allocate,control_release};
    g->parent=(struct pt_sampler_storage_span){g,sizeof(*g)};
    pt_document_init(&g->document,&g->document_allocator);assert(pt_document_new(&g->document,16,SIZE_MAX)==PT_PROJECT_OK);
    g->document.project.sample_count=3;
    for(i=0;i<16;++i)g->document.project.channels.track[i].route=PT_AMIGUS;
    g->document.project.channels.track[4].route=PT_PAULA;g->document.project.channels.track[4].pan=0;
    for(i=0;i<3;++i){g->document.project.samples[i].pcm=(struct pt_pcm){g->pcm+i*2048,2048,64,8000,(uint8_t)channels,(uint8_t)bits};g->document.project.samples[i].volume=64;}
    g->markers[1]=32;g->document.project.samples[0].slices=g->markers;g->document.project.samples[0].slice_count=2;
    g->extension=(struct pt_extension){0x58595a31UL,sizeof(g->payload),1,g->payload};
    g->document.project.extensions=&g->extension;g->document.project.extension_count=1;
    g->document.project.speed=1;g->document.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    g->document.project.events[7]=g->document.project.events[4];g->document.project.events[16+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    g->document.project.events[32+15].effect=15;
    pt_sampler_init(&g->sampler,&g->master_allocator,1024*1024);return g;
}
static void drop_fixture(struct establishment_fixture *g)
{
    g->hook=0;g->alias=NULL;
    assert(pt_sampler_establish_cancel(&g->c)&&!g->live_controls&&!g->alias_releases);
    pt_sampler_release(&g->sampler);assert(!g->sampler.bytes&&!g->live_masters);
    pt_document_release(&g->document);free(g->pcm);free(g);
}
static void begin(struct establishment_fixture *g)
{
    unsigned i;struct pt_project *p=&g->document.project;
    POISON(g->pcm,3*2048*sizeof(*g->pcm));POISON(g->markers,sizeof(g->markers));POISON(g->payload,sizeof(g->payload));
    POISON(p->orders,p->order_count*sizeof(*p->orders));POISON(p->events,p->pattern_count*64*16*sizeof(*p->events));
    assert(pt_sampler_establish_begin(&g->c,&g->sampler,p,&g->control_allocator,&g->parent,1,123)==PT_ESTABLISH_PENDING);
    /* Descriptor copy and validator begin are still metadata-only. */
    assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,4096)==PT_ESTABLISH_PENDING);
    assert(g->c.validation.project==p&&!g->master_calls&&!g->sampler.bytes);
    UNPOISON(g->pcm,3*2048*sizeof(*g->pcm));UNPOISON(g->markers,sizeof(g->markers));UNPOISON(g->payload,sizeof(g->payload));
    UNPOISON(p->orders,p->order_count*sizeof(*p->orders));UNPOISON(p->events,p->pattern_count*64*16*sizeof(*p->events));
    for(i=0;i<3;++i)assert(!g->sampler.current[i]);
}
static void validation_done(struct establishment_fixture *g)
{
    unsigned n=0;
    while(g->c.validation.project){assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_PENDING&&++n<10000);assert(!g->master_calls);}
    assert(!g->sampler.bytes&&!g->c.slot);
}
static void handoff(struct establishment_fixture *g)
{
    struct pt_sampler_paula pb={0};struct pt_sampler_wavetable ab={0};struct pt_paula_voices pv={0};struct pt_wavetable_voices av={0};
    struct driver d={0};struct wave_driver wd={0};struct fixture *f=malloc(sizeof(*f));
    struct pt_paula_voice_api pa={&d,mixed_start,stop,mixed_control};struct pt_wavetable_voice_api aa={&wd,wave_start,wave_stop,wave_control,NULL};
    struct pt_render_options o={0};struct pt_paula_render_caps caps={3546895,124,65535};struct pt_playback_format format={8,0,0,0};
    struct pt_mixed_established c={0};struct pt_mixed_owner *owner=NULL;struct pt_mixed_report report;
    struct pt_allocator a={NULL,fast_alloc,fast_free};enum pt_mixed_owner_result r;unsigned n=0;struct pt_sample_version *masters[3];size_t bytes=g->sampler.bytes;
    assert(f);memcpy(masters,g->sampler.current,sizeof(masters));
    init(f,PT_AMIGUS_WAVETABLE);assert(pt_amigus_wavetable_cache_attach(&f->cache,&f->reservation,0,4096,4096,f,bus_owned,bus_write));
    assert(pt_sampler_paula_bind(&pb,&g->sampler,&g->document.project,&d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&ab,&g->sampler,&g->document.project,&f->cache));
    assert(pt_paula_voices_bind(&pv,&pb,&pa)&&pt_wavetable_voices_bind(&av,&ab,&aa));
    assert(pt_paula_voices_bind_quiesce(&pv,quiesce,&d)&&pt_wavetable_voices_bind_quiesce(&av,wave_quiesce,&wd));
    d.quiesce_result=wd.barrier_result=wd.stop_result=1;for(n=0;n<4;++n)d.stop_result[n]=1;n=0;
    o=(struct pt_render_options){100000,100,48000,65536,(1U<<4)|(1U<<7),0,0,24,0,0,0,0,0};
    assert(pt_mixed_owner_established_begin(&c,&pv,&av,&o,&caps,&format,&a,NULL,0,456,8,&owner)==PT_MIXED_OWNER_PREPARING);
    do{r=pt_mixed_owner_prepare(owner,&report);assert(++n<10000&&!d.starts&&!wd.starts&&!f->writes);}while(r==PT_MIXED_OWNER_PREPARING);
    assert(r==PT_MIXED_OWNER_OK&&report.samples[0][0]&&report.samples[1][1]);
    assert(g->sampler.bytes==bytes&&!memcmp(masters,g->sampler.current,sizeof(masters)));
    assert(pt_mixed_owner_close(&owner)&&pt_mixed_owner_established_finish(&c));
    assert(pt_paula_voices_close(&pv)&&pt_wavetable_voices_close(&av));
    assert(pt_amigus_wavetable_cache_detach(&f->cache)&&pt_amigus_reservation_close(&f->reservation));free(f);
}
static void success(unsigned bits,unsigned channels)
{
    PT_TEST_CASE_PROGRESS("MASTER","BEGIN",bits,channels);
    struct establishment_fixture *g=make_fixture(bits,channels);struct pt_project *p=&g->document.project;
    size_t bytes,done;uint8_t *before,*after;enum pt_establish_result r;unsigned n=0,i;
    assert(pt_project_size(p,&bytes)==PT_PROJECT_OK);before=malloc(bytes);after=malloc(bytes);assert(before&&after);
    assert(pt_project_encode(p,before,bytes,&done)==PT_PROJECT_OK&&done==bytes);
    begin(g);assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,0)==PT_ESTABLISH_INVALID);
    assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,4097)==PT_ESTABLISH_INVALID);
    do {p->channels.selected=(uint8_t)(n%16);r=pt_sampler_establish_step(&g->c,123,g->sampler.generation,8);assert(++n<12000);}while(r==PT_ESTABLISH_PENDING);
    assert(r==PT_ESTABLISH_READY&&g->master_calls==3&&g->live_masters==3&&g->live_controls==1);
    for(i=0;i<3;++i){assert(g->sampler.current[i]&&p->samples[i].pcm.bits==bits&&p->samples[i].pcm.channels==channels&&p->samples[i].pcm.capacity==64*channels);assert(!memcmp(p->samples[i].pcm.data,g->pcm+i*2048,64*channels*sizeof(int32_t)));}
    /* Selection is UI-only; canonical bytes remain exact after pointer promotion. */
    p->channels.selected=0;assert(pt_project_encode(p,after,bytes,&done)==PT_PROJECT_OK&&done==bytes&&!memcmp(before,after,bytes));
    assert(pt_sampler_establish_cancel(&g->c)&&!g->live_controls&&g->live_masters==3);
    /* A second actual establishment validates again and reuses owned versions. */
    assert(pt_sampler_establish_begin(&g->c,&g->sampler,p,&g->control_allocator,&g->parent,1,123)==PT_ESTABLISH_PENDING);
    n=0;do{r=pt_sampler_establish_step(&g->c,123,g->sampler.generation,32);assert(++n<12000);}while(r==PT_ESTABLISH_PENDING);
    assert(r==PT_ESTABLISH_READY&&g->master_calls==3&&g->live_masters==3);
    assert(pt_sampler_establish_cancel(&g->c)&&!g->live_controls);
    assert(pt_project_encode(p,after,bytes,&done)==PT_PROJECT_OK&&done==bytes&&!memcmp(before,after,bytes));
    if(channels==1)handoff(g);
    free(before);free(after);drop_fixture(g);
    PT_TEST_CASE_PROGRESS("MASTER","END",bits,channels);
}
static void faults(unsigned bits,unsigned mode)
{
    PT_TEST_CASE_PROGRESS("FAULT","BEGIN",bits,mode);
    struct establishment_fixture *g=make_fixture(bits,1);enum pt_establish_result r;unsigned n=0;struct pt_sample before[3];size_t used;
    memcpy(before,g->document.project.samples,sizeof(before));
    if(mode==0){g->alias=&g->c;r=pt_sampler_establish_begin(&g->c,&g->sampler,&g->document.project,&g->control_allocator,&g->parent,1,123);assert(r==PT_ESTABLISH_ALIAS&&!g->live_controls&&!g->alias_releases);goto end;}
    if(mode==1){g->fail=1;assert(pt_sampler_establish_begin(&g->c,&g->sampler,&g->document.project,&g->control_allocator,&g->parent,1,123)==PT_ESTABLISH_CAPACITY);goto end;}
    if(mode==2){g->hook=1;assert(pt_sampler_establish_begin(&g->c,&g->sampler,&g->document.project,&g->control_allocator,&g->parent,1,123)==PT_ESTABLISH_FAULT&&!g->live_controls);goto end;}
    if(mode==26)g->sampler.budget=0;
    begin(g);
    if(mode==3)goto end; /* Cancel actual unfinished validation. */
    if(mode==4){g->document.project.events[63*16].kind=99;
        do{r=pt_sampler_establish_step(&g->c,123,g->sampler.generation,32);assert(++n<2000);}while(r==PT_ESTABLISH_PENDING);
        assert(r==PT_ESTABLISH_SEMANTIC&&!g->master_calls&&!g->sampler.bytes);goto end;}
    if(mode==5){assert(pt_sampler_establish_step(&g->c,124,g->sampler.generation,32)==PT_ESTABLISH_STALE&&!g->master_calls);goto end;}
    validation_done(g);
    if(mode==26){assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_CAPACITY&&!g->master_calls&&!g->sampler.bytes);goto end;}
    if(mode>=6&&mode<=14){switch(mode){
        case 6:g->alias=&g->c;break;case 7:g->alias=&g->sampler;break;
        case 8:g->alias=g->document.project.samples;break;case 9:g->alias=g->document.project.events;break;
        case 10:g->alias=g->document.project.orders;break;case 11:g->alias=g->pcm+3*2048-1;break;
        case 12:g->alias=g->markers+1;break;case 13:g->alias=g->c.metadata;break;case 14:g->alias=g->payload+7;break;}
        assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_ALIAS&&!g->live_masters&&!g->alias_releases);goto end;}
    if(mode==15){g->fail=2;assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_CAPACITY);goto end;}
    if(mode==16){g->sampler.budget=0;assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_STALE&&!g->master_calls);goto end;}
    if(mode==17||mode==18||mode==19){g->hook=mode==17?1:mode==18?2:3;
        r=pt_sampler_establish_step(&g->c,123,g->sampler.generation,8);
        assert(r==(mode==19?PT_ESTABLISH_STALE:PT_ESTABLISH_FAULT)&&!g->live_masters);goto end;}
    assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_PENDING&&g->live_masters==1&&g->c.job.owner);
    assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_PENDING&&!g->sampler.current[0]);
    if(mode==20)goto end; /* Discard partial unpublished copy. */
    if(mode==21){++g->sampler.generation;assert(pt_sampler_establish_get(&g->c,123,g->c.generation)==PT_ESTABLISH_STALE&&!g->live_masters);goto end;}
    while(!g->sampler.current[0])assert(pt_sampler_establish_step(&g->c,123,g->sampler.generation,8)==PT_ESTABLISH_PENDING&&++n<1000);
    assert(g->live_masters==1&&!g->sampler.current[1]);used=g->sampler.bytes;
    if(mode==22)goto end; /* Completed slot retained on cancel. */
    if(mode==23)g->alias=g->pcm+2047; /* No longer named by current project: frozen initial capacity still guarded. */
    if(mode==24)g->alias=g->sampler.current[0];
    if(mode==25)g->fail=2;
    r=pt_sampler_establish_step(&g->c,123,g->sampler.generation,8);
    assert(r==(mode==25?PT_ESTABLISH_CAPACITY:PT_ESTABLISH_ALIAS)&&g->live_masters==1&&g->sampler.bytes==used&&!g->sampler.current[1]);
end:
    if(!g->sampler.current[0])assert(!memcmp(before,g->document.project.samples,sizeof(before)));
    assert(!g->alias_releases);drop_fixture(g);
    PT_TEST_CASE_PROGRESS("FAULT","END",bits,mode);
}
static int sampler_establish_fixture(void)
{
    unsigned bits,mode;assert(!mixed_owner_fixture());
    for(bits=8;bits<=24;bits+=8){success(bits,1);success(bits,2);for(mode=0;mode<=26;++mode)faults(bits,mode);}
    puts("ESTABLISH PASS:6 mono/stereo8/16/24 exact save/PCM cases;6 owned-master reuse cases;81 cancellation/alias/late-validation/budget/reentry/stale/partial-copy cases;3 actual established mixed handoffs;255 original owner regressions; host callbacks only");
    return 0;
}
#ifndef PT_TEST_ESTABLISH_INCLUDED
int main(void){return sampler_establish_fixture();}
#endif
