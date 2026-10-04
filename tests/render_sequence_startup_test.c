#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_commands.h"
#include "render_lookahead.h"
#include "document.h"
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define STARTUP_POISON(p,n) __asan_poison_memory_region((p),(n))
#define STARTUP_UNPOISON(p,n) __asan_unpoison_memory_region((p),(n))
#endif
#endif
#ifndef STARTUP_POISON
#define STARTUP_POISON(p,n) ((void)(p),(void)(n))
#define STARTUP_UNPOISON(p,n) ((void)(p),(void)(n))
#endif
#define STARTUP_VALUES 4096U
#define STARTUP_REVISION 17U
#define STARTUP_GENERATION 23U
struct startup_memory {
    unsigned calls,releases,live,fail_at,alias_releases,free_alias,hook_at;
    void *arena,*last;
    void *returned[16];size_t requested[16];
    void (*hook)(void *),(*release_hook)(void *);
    void *hook_context;
};
static void *startup_allocate(void *context,size_t bytes)
{
    struct startup_memory *m=context;void *p;++m->calls;
    if(m->hook&&(!m->hook_at||m->calls==m->hook_at)){void (*hook)(void *)=m->hook;m->hook=NULL;hook(m->hook_context);}
    if(m->calls==m->fail_at)return NULL;
    p=m->arena?m->arena:malloc(bytes);
    if(m->calls<=16){m->returned[m->calls-1]=p;m->requested[m->calls-1]=bytes;}
    if(p){if(p!=m->arena)++m->live;m->last=p;}
    return p;
}
static void startup_release(void *context,void *p)
{
    struct startup_memory *m=context;assert(p);
    if(p==m->arena){++m->alias_releases;if(!m->free_alias)return;}
    assert(m->live);--m->live;++m->releases;
    if(m->release_hook){void (*hook)(void *)=m->release_hook;m->release_hook=NULL;hook(m->hook_context);}
    free(p);
}
struct startup_fixture {
    struct startup_memory memory;struct pt_allocator allocator;
    struct pt_project project;struct pt_render_options options;
    struct pt_sample samples[2];uint16_t orders[2];
    struct pt_event events[2*PT_PROJECT_ROWS*PT_CHANNEL_LIMIT];
    struct pt_extension extension;unsigned char payload[8];uint32_t slices[2];
    union {long double alignment;int32_t values[STARTUP_VALUES];} pcm[2];
};
static void startup_init(struct startup_fixture *f,unsigned bits,unsigned channels)
{
    unsigned i,s;memset(f,0,sizeof(*f));
    f->allocator=(struct pt_allocator){&f->memory,startup_allocate,startup_release};
    pt_channels_init(&f->project.channels);f->project.channels.count=PT_CHANNEL_LIMIT;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)f->project.channels.track[i].route=PT_AMIGUS;
    f->project.channels.track[0].route=PT_PAULA;f->project.channels.track[0].pan=0;
    f->project.samples=f->samples;f->project.sample_count=2;
    f->project.orders=f->orders;f->project.order_count=f->project.pattern_count=2;f->orders[1]=1;
    f->project.events=f->events;f->project.speed=3;f->project.bpm=125;
    f->project.extensions=&f->extension;f->project.extension_count=1;
    f->extension=(struct pt_extension){0x58595a31UL,sizeof(f->payload),1,f->payload};
    f->slices[0]=0;f->slices[1]=32;
    for(s=0;s<2;++s){
        unsigned count=64*(s?2:channels);
        for(i=0;i<STARTUP_VALUES;++i)f->pcm[s].values[i]=0x12345678; /* Unused capacity is protected, not semantic audio. */
        for(i=0;i<count;++i)f->pcm[s].values[i]=bits==8?(int32_t)(i%201)-100:
            bits==16?((i&1)?-(int32_t)(0x1234+i):(int32_t)(0x1234+i)):
            ((i&1)?-(int32_t)(0x123457+i):(int32_t)(0x123457+i));
        f->samples[s].pcm=(struct pt_pcm){f->pcm[s].values,STARTUP_VALUES,64,8000,(uint8_t)(s?2:channels),(uint8_t)bits};
        f->samples[s].volume=64;
    }
    f->samples[0].slices=f->slices;f->samples[0].slice_count=2;
    f->events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    f->events[5]=(struct pt_event){320,0,PT_NOTE_PERIOD,2,0,0,0,0};
    f->events[16+15].effect=15;f->events[16+15].parameter=150;
    f->events[2*16+15].effect=14;f->events[2*16+15].parameter=0xe1;
    f->events[5*16+15].effect=15; /* F00 on an unselected track ends the genuine shared flow. */
    f->events[64*16]=(struct pt_event){428,1,PT_NOTE_PERIOD,1,0,0,0,0};
    f->events[64*16+4*16+15].effect=15;
    f->options=(struct pt_render_options){1000000,1000,48000,65536,(1U<<0)|(1U<<5),0,0,24,0,0,0,0,0};
}
static void startup_values_poison(struct startup_fixture *f,unsigned all)
{
    unsigned i;for(i=0;i<2;++i)STARTUP_POISON(f->pcm[i].values,sizeof(f->pcm[i].values));
    STARTUP_POISON(f->slices,sizeof(f->slices));STARTUP_POISON(f->payload,sizeof(f->payload));
    if(all){STARTUP_POISON(f->orders,sizeof(f->orders));STARTUP_POISON(f->events,sizeof(f->events));}
}
static void startup_values_unpoison(struct startup_fixture *f)
{
    unsigned i;for(i=0;i<2;++i)STARTUP_UNPOISON(f->pcm[i].values,sizeof(f->pcm[i].values));
    STARTUP_UNPOISON(f->slices,sizeof(f->slices));STARTUP_UNPOISON(f->payload,sizeof(f->payload));
    STARTUP_UNPOISON(f->orders,sizeof(f->orders));STARTUP_UNPOISON(f->events,sizeof(f->events));
}
static void startup_unplayed_poison(struct startup_fixture *f)
{
    /* Measurement must read played flow events, but a hidden offset rescan
     * would also read the second order/pattern or rows beyond this F00. */
    if(!f->options.pattern_only&&!f->options.start_order){
        STARTUP_POISON(f->orders+1,sizeof(f->orders[1]));
        STARTUP_POISON(f->events+64*16,64*16*sizeof(*f->events));
    }
    STARTUP_POISON(f->events+(f->options.pattern_only?f->options.pattern:f->options.start_order)*64*16+48*16,
        16*16*sizeof(*f->events));
}
static void startup_mode(struct startup_fixture *f,unsigned mode)
{
    if(mode==1)f->options.include_lead_in=1;
    if(mode==2){f->options.pattern_only=1;f->options.pattern=1;f->options.rate=44100;}
    if(mode==3){f->options.pattern_only=f->options.row_range=1;f->options.row_first=1;f->options.row_end=4;}
    if(mode==4)f->options.start_order=1;
}
static void startup_semantic_mutation(struct startup_fixture *f,unsigned kind)
{
    f->options.pattern_only=1; /* The complete original project still validates. */
    if(kind==0)f->pcm[1].values[127]=0x800000;
    if(kind==1)f->orders[1]=2;
    if(kind==2)f->events[2*64*16-1].flags=2;
    if(kind==3)f->slices[1]=64;
    if(kind==4){f->options.pattern=1;f->events[64*16+63*16].effect=9;f->events[64*16+63*16].parameter=1;}
    if(kind==5){f->options.pattern_only=0;f->events[64*16+63*16].effect=14;f->events[64*16+63*16].parameter=0xf1;}
}
static void startup_consume(struct pt_render_sequence *s,uint32_t frames)
{
    while(frames){unsigned n=frames>256?256:frames;assert(pt_render_sequence_consume(s,n)==PT_RENDER_OK);frames-=n;}
}
static void startup_same_voice(const struct pt_voice *a,const struct pt_voice *b)
{
    assert(a->pcm==b->pcm&&a->repeat_pcm==b->repeat_pcm&&a->phase==b->phase&&a->step==b->step&&a->cycle==b->cycle);
    assert(a->start==b->start&&a->end==b->end&&a->loop_start==b->loop_start&&a->loop_end==b->loop_end);
    assert(a->loop==b->loop&&a->looped==b->looped&&a->linear==b->linear&&a->active==b->active&&a->segment==b->segment);
}
static uint64_t startup_compare_sequences(struct pt_render_sequence *a,struct pt_render_sequence *b)
{
    struct pt_render_interval sa,sb;struct pt_render_plan pa,pb;unsigned intervals=0,i;uint64_t frames=0;
    do {
        assert(pt_render_sequence_next(a,&sa)==PT_RENDER_OK&&pt_render_sequence_next(b,&sb)==PT_RENDER_OK);
        assert(sa.frames==sb.frames&&sa.emit==sb.emit&&sa.end==sb.end);if(sa.emit)frames+=sa.frames;
        startup_consume(a,sa.frames);startup_consume(b,sb.frames);
        assert(pt_render_sequence_complete(a,&pa)==PT_RENDER_OK&&pt_render_sequence_complete(b,&pb)==PT_RENDER_OK);
        assert(pa.count==pb.count);
        for(i=0;i<pa.count;++i){assert(pa.action[i].kind==pb.action[i].kind&&pa.action[i].channel==pb.action[i].channel);
            startup_same_voice(&pa.action[i].voice,&pb.action[i].voice);
            assert(pa.action[i].gain[0]==pb.action[i].gain[0]&&pa.action[i].gain[1]==pb.action[i].gain[1]);}
        assert(++intervals<1000);
    }while(!sa.end);
    return frames;
}
static void startup_core_begin(struct startup_fixture *f,struct pt_render_sequence_setup **owner)
{
    assert(pt_render_sequence_setup_begin(&f->project,&f->options,&f->allocator,
        STARTUP_REVISION,STARTUP_GENERATION,NULL,0,owner)==PT_RENDER_SETUP_PENDING);
    assert(*owner);
}
static unsigned startup_core_complete(struct pt_render_sequence_setup *j,unsigned work)
{
    struct pt_render_setup_report report;enum pt_render_setup_result r;unsigned calls=0,phases=0;
    do {
        r=pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&report);
        assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);phases|=1U<<report.phase;
        if(r==PT_RENDER_SETUP_READY)break;
        r=pt_render_sequence_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,work);
        assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);assert(++calls<20000);
        assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&report)==r);
        assert(report.last_work<=work);phases|=1U<<report.phase;
    }while(r==PT_RENDER_SETUP_PENDING);
    return phases;
}
static void startup_core_parity(void)
{
    unsigned bits,channels,mode;const unsigned budget[3]={1,7,4096};
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)for(mode=0;mode<5;++mode){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL;
        struct pt_render_sequence *s=(void *)(uintptr_t)1,*reference=NULL;struct pt_render_report measured;
        struct pt_render_setup_report before,after;int32_t *saved=malloc(sizeof(f->pcm));unsigned ready=0,n=0,calls;
        assert(f&&saved);startup_init(f,bits,channels);startup_mode(f,mode);memcpy(saved,f->pcm,sizeof(f->pcm));
        assert(pt_render_measure(&f->project,&f->options,NULL,NULL,&measured)==PT_RENDER_OK);
        assert(pt_render_sequence_open(&f->project,&f->options,&f->allocator,&reference)==PT_RENDER_OK);
        calls=f->memory.calls;startup_values_poison(f,1);startup_core_begin(f,&j);startup_values_unpoison(f);
        assert(f->memory.calls==calls+1&&f->memory.live==2);
        assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&before)==PT_RENDER_SETUP_PENDING);
        assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&s)==PT_RENDER_SETUP_PENDING);
        assert(s==(void *)(uintptr_t)1&&f->memory.calls==calls+1);
        assert(pt_render_sequence_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,0)==PT_RENDER_SETUP_INVALID);
        assert(pt_render_sequence_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,4097)==PT_RENDER_SETUP_INVALID);
        assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&after)==PT_RENDER_SETUP_PENDING);
        assert(before.phase==after.phase&&before.last_work==after.last_work&&before.render_result==after.render_result);
        assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,
            (struct pt_render_setup_report *)&f->pcm[0].values[STARTUP_VALUES-8])==PT_RENDER_SETUP_ALIAS);
        startup_core_complete(j,budget[mode%3]);assert(f->memory.calls==calls+1);
        startup_values_poison(f,1);
        assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&s)==PT_RENDER_SETUP_READY);
        startup_values_unpoison(f);assert(!j&&s&&s!=(void *)(uintptr_t)1&&f->memory.live==2&&f->memory.calls==calls+2);
        assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&s)==PT_RENDER_SETUP_INVALID);
        assert(pt_render_sequence_prepare(s,1,(unsigned *)&f->pcm[0].values[STARTUP_VALUES-1])==PT_RENDER_INVALID);
        assert(!memcmp(saved,f->pcm,sizeof(f->pcm)));
        startup_values_poison(f,0);startup_unplayed_poison(f);
        do {assert(pt_render_sequence_prepare(s,1,&ready)==PT_RENDER_OK);assert(++n<1000);}while(!ready);
        startup_values_unpoison(f);assert(f->memory.calls==calls+2);
        assert(startup_compare_sequences(s,reference)==measured.frames);
        startup_values_poison(f,1);assert(pt_render_sequence_rewind(s)==PT_RENDER_OK);startup_values_unpoison(f);
        assert(pt_render_sequence_rewind(reference)==PT_RENDER_OK);
        assert(startup_compare_sequences(s,reference)==measured.frames);
        assert(!memcmp(saved,f->pcm,sizeof(f->pcm)));
        pt_render_sequence_close(s);pt_render_sequence_close(reference);assert(!f->memory.live);
        free(saved);free(f);
    }
}
static void startup_core_failures_and_cancellation(void)
{
    unsigned kind,wanted,seen=0;
    for(kind=0;kind<6;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL;
        struct pt_render_sequence *legacy=NULL,*out=(void *)(uintptr_t)1;
        struct pt_render_setup_report report;enum pt_render_result expected;enum pt_render_setup_result r;unsigned n=0;
        assert(f);startup_init(f,24,2);startup_semantic_mutation(f,kind);
        expected=pt_render_sequence_open(&f->project,&f->options,&f->allocator,&legacy);
        assert(expected==(kind<4?PT_RENDER_INVALID:PT_RENDER_EFFECT)&&!legacy&&!f->memory.live);
        startup_core_begin(f,&j); /* Late invalid values must be found by actual semantic steps. */
        do {r=pt_render_sequence_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,7);assert(++n<20000);}while(r==PT_RENDER_SETUP_PENDING);
        assert(r==PT_RENDER_SETUP_FAILED&&n>1);
        assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&report)==PT_RENDER_SETUP_FAILED);
        assert(report.render_result==expected);
        assert(pt_render_sequence_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,7)==PT_RENDER_SETUP_FAILED);
        assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&out)==PT_RENDER_SETUP_FAILED&&out==(void *)(uintptr_t)1);
        assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);free(f);
    }
    for(wanted=PT_RENDER_SETUP_VALIDATE;wanted<=PT_RENDER_SETUP_COMPLETE;++wanted){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL;
        struct pt_render_setup_report report;unsigned n=0;assert(f);startup_init(f,16,2);startup_core_begin(f,&j);
        do {
            enum pt_render_setup_result r=pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&report);
            assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);
            if((unsigned)report.phase==wanted)break;
            r=pt_render_sequence_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,1);
            assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);assert(++n<20000);
        }while(1);
        seen|=1U<<report.phase;
        assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);
        assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY);free(f);
    }
    assert(seen==31);
}
static void startup_core_stale(void)
{
    unsigned kind;
    for(kind=0;kind<5;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL,*same;
        struct pt_render_sequence *out=(void *)(uintptr_t)1;struct pt_render_setup_report report,sentinel;
        uint32_t revision=STARTUP_REVISION,generation=STARTUP_GENERATION;uint16_t *old=NULL;assert(f);startup_init(f,24,2);
        if(kind==2){old=malloc(sizeof(f->orders));assert(old);memcpy(old,f->orders,sizeof(f->orders));f->project.orders=old;}
        startup_core_begin(f,&j);same=j;
        f->project.channels.selected=15;assert(pt_render_sequence_setup_get(j,revision,generation,NULL)==PT_RENDER_SETUP_PENDING);
        if(kind==0)++revision;
        if(kind==1)++generation;
        if(kind==2){free(old);f->project.orders=(uint16_t *)(uintptr_t)1;}
        if(kind==3){STARTUP_POISON(f->samples,sizeof(f->samples));f->project.samples=(struct pt_sample *)(uintptr_t)1;}
        if(kind==4)f->allocator.release=NULL;
        memset(&sentinel,0x5a,sizeof(sentinel));memcpy(&report,&sentinel,sizeof(report));
        assert(pt_render_sequence_setup_get(j,revision,generation,&report)==PT_RENDER_SETUP_STALE&&!memcmp(&report,&sentinel,sizeof(report)));
        assert(pt_render_sequence_setup_step(j,revision,generation,7)==PT_RENDER_SETUP_STALE);
        assert(pt_render_sequence_setup_take(&j,revision,generation,NULL,0,&out)==PT_RENDER_SETUP_STALE&&j==same&&out==(void *)(uintptr_t)1);
        assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);
        STARTUP_UNPOISON(f->samples,sizeof(f->samples));free(f);
    }
}
struct startup_core_hook {
    struct startup_fixture *fixture;struct pt_render_sequence_setup **owner;unsigned kind;
};
static void startup_core_reenter(void *context)
{
    struct startup_core_hook *h=context;
    if(h->kind==0)assert(pt_render_sequence_setup_step(*h->owner,STARTUP_REVISION,STARTUP_GENERATION,1)==PT_RENDER_SETUP_BUSY);
    if(h->kind==1)assert(pt_render_sequence_setup_get(*h->owner,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_BUSY);
    if(h->kind==2)assert(pt_render_sequence_setup_cancel(h->owner)==PT_RENDER_SETUP_BUSY&&*h->owner);
}
static void startup_core_allocations(void)
{
    unsigned kind;
    for(kind=0;kind<7;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL,*same;
        struct pt_render_sequence *out=(void *)(uintptr_t)1;unsigned calls;
        struct startup_core_hook hook;assert(f);startup_init(f,24,1);
        if(kind==0){f->memory.fail_at=1;
            assert(pt_render_sequence_setup_begin(&f->project,&f->options,&f->allocator,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&j)==PT_RENDER_SETUP_CAPACITY);
            assert(!j&&!f->memory.live);free(f);continue;}
        startup_core_begin(f,&j);startup_core_complete(j,4096);same=j;calls=f->memory.calls;
        if(kind==1){f->memory.fail_at=calls+1;
            assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&out)==PT_RENDER_SETUP_CAPACITY);
            assert(j==same&&out==(void *)(uintptr_t)1&&f->memory.live==1);
            assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_READY);
            f->memory.fail_at=0;
            assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&out)==PT_RENDER_SETUP_READY);
            assert(!j&&f->memory.calls==calls+2&&f->memory.live==1);pt_render_sequence_close(out);
        }else if(kind==2){
            /* This would actually free the real startup if product cleanup
             * wrongly treated the returned alias as a fresh allocation. */
            f->memory.arena=j;f->memory.free_alias=1;
            assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&out)==PT_RENDER_SETUP_ALIAS);
            assert(j==same&&out==(void *)(uintptr_t)1&&!f->memory.alias_releases&&f->memory.live==1);
            f->memory.arena=NULL;
            assert(pt_render_sequence_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_READY);
            assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY);
        }else{
            hook=(struct startup_core_hook){f,&j,kind==6?0:kind-3};f->memory.hook_context=&hook;
            if(kind==6)f->memory.release_hook=startup_core_reenter;else f->memory.hook=startup_core_reenter;
            assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&out)==PT_RENDER_SETUP_FAILED);
            assert(out==(void *)(uintptr_t)1);
            if(kind==6)assert(!j&&!f->memory.live);
            else {assert(j==same&&f->memory.live==1);assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY);}
        }
        assert(!f->memory.live);free(f);
    }
}
static void startup_core_empty_and_aliases(void)
{
    struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL;
    struct pt_render_sequence *s=(void *)(uintptr_t)1;struct pt_render_setup_guard guard;
    unsigned calls;int32_t *saved;assert(f);startup_init(f,24,1);calls=f->memory.calls;
    assert(pt_render_sequence_setup_begin(&f->project,&f->options,&f->allocator,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,
        (struct pt_render_sequence_setup **)&f->pcm[0].values[STARTUP_VALUES-2])==PT_RENDER_SETUP_ALIAS);
    assert(f->memory.calls==calls);
    assert(pt_render_sequence_setup_begin(&f->project,&f->options,&f->allocator,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,
        (void *)(UINTPTR_MAX-sizeof(j)+1))==PT_RENDER_SETUP_INVALID);
    guard=(struct pt_render_setup_guard){f->payload,sizeof(f->payload)};
    assert(pt_render_sequence_setup_begin(&f->project,&f->options,&f->allocator,STARTUP_REVISION,STARTUP_GENERATION,&guard,9,&j)==PT_RENDER_SETUP_INVALID);
    startup_core_begin(f,&j);startup_core_complete(j,4096);saved=malloc(sizeof(f->pcm));assert(saved);memcpy(saved,f->pcm,sizeof(f->pcm));
    assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,
        (struct pt_render_sequence **)&f->pcm[0].values[STARTUP_VALUES-2])==PT_RENDER_SETUP_ALIAS);
    assert(!memcmp(saved,f->pcm,sizeof(f->pcm)));assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY);free(saved);
    /* Valid silent project with no masters still owns only setup then sequence. */
    f->project.samples=NULL;f->project.sample_count=0;memset(f->events,0,sizeof(f->events));f->events[15].effect=15;
    startup_core_begin(f,&j);startup_core_complete(j,7);
    assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&s)==PT_RENDER_SETUP_READY);
    {unsigned ready=0;do {assert(pt_render_sequence_prepare(s,1,&ready)==PT_RENDER_OK);}while(!ready);}
    pt_render_sequence_close(s);assert(!j&&!f->memory.live);
    f->project.order_count=0;
    assert(pt_render_sequence_setup_begin(&f->project,&f->options,&f->allocator,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&j)!=PT_RENDER_SETUP_PENDING);
    assert(!j&&!f->memory.live);free(f);
}
static void startup_core_guard_aliases(void)
{
    unsigned kind;
    for(kind=0;kind<4;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL,*same;
        struct pt_render_sequence *out=(void *)(uintptr_t)1;struct pt_render_setup_guard local,*guard=&local;
        unsigned char *arena=malloc(131072),*before;int32_t *masters;unsigned count=1;
        assert(f&&arena);startup_init(f,24,2);memset(arena,0x6d,131072);masters=malloc(sizeof(f->pcm));assert(masters);memcpy(masters,f->pcm,sizeof(f->pcm));
        startup_core_begin(f,&j);startup_core_complete(j,4096);same=j;before=malloc(f->memory.requested[0]);assert(before);memcpy(before,j,f->memory.requested[0]);
        local=(struct pt_render_setup_guard){arena+64,16};
        if(kind==0)f->memory.arena=arena+64;
        if(kind==1){guard=(void *)arena;*guard=local;f->memory.arena=arena;}
        if(kind==2){f->memory.arena=f->pcm[0].values+STARTUP_VALUES-2;count=0;}
        if(kind==3){f->memory.arena=&f->options;count=0;}
        assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,guard,count,&out)==PT_RENDER_SETUP_ALIAS);
        assert(j==same&&out==(void *)(uintptr_t)1&&!f->memory.alias_releases&&f->memory.live==1);
        assert(!memcmp(before,j,f->memory.requested[0])&&!memcmp(masters,f->pcm,sizeof(f->pcm)));
        assert(local.data==arena+64&&local.bytes==16);
        if(kind==1)assert(guard->data==local.data&&guard->bytes==local.bytes);
        f->memory.arena=NULL;assert(pt_render_sequence_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);
        free(masters);free(before);free(arena);free(f);
    }
}
struct startup_sequence_close_hook {
    struct startup_memory *memory;struct pt_render_sequence *sequence;unsigned releases;
};
static void startup_sequence_close_reenter(void *context)
{
    struct startup_sequence_close_hook *h=context;struct pt_render_interval interval,before;
    struct pt_render_plan *plan=malloc(sizeof(*plan)),*saved=malloc(sizeof(*saved));unsigned ready=91;
    assert(plan&&saved);memset(&interval,0x5a,sizeof(interval));memcpy(&before,&interval,sizeof(before));
    memset(plan,0x5a,sizeof(*plan));memcpy(saved,plan,sizeof(*saved));
    assert(h->memory->releases==h->releases+1);
    assert(pt_render_sequence_next(h->sequence,&interval)==PT_RENDER_INVALID&&!memcmp(&interval,&before,sizeof(interval)));
    assert(pt_render_sequence_prepare(h->sequence,1,&ready)==PT_RENDER_INVALID&&ready==91);
    assert(pt_render_sequence_complete(h->sequence,plan)==PT_RENDER_INVALID&&!memcmp(plan,saved,sizeof(*plan)));
    assert(pt_render_sequence_rewind(h->sequence)==PT_RENDER_INVALID);
    pt_render_sequence_close(h->sequence);assert(h->memory->releases==h->releases+1);
    free(saved);free(plan);
}
static void startup_core_close_reentry(void)
{
    struct startup_fixture *f=malloc(sizeof(*f));struct pt_render_sequence_setup *j=NULL;
    struct pt_render_sequence *s=NULL;struct startup_sequence_close_hook hook;unsigned ready=0,n=0;
    assert(f);startup_init(f,24,1);startup_core_begin(f,&j);startup_core_complete(j,4096);
    assert(pt_render_sequence_setup_take(&j,STARTUP_REVISION,STARTUP_GENERATION,NULL,0,&s)==PT_RENDER_SETUP_READY&&!j);
    do {assert(pt_render_sequence_prepare(s,1,&ready)==PT_RENDER_OK);assert(++n<1000);}while(!ready);
    hook=(struct startup_sequence_close_hook){&f->memory,s,f->memory.releases};
    f->memory.hook_context=&hook;f->memory.release_hook=startup_sequence_close_reenter;
    pt_render_sequence_close(s);assert(f->memory.releases==hook.releases+1&&!f->memory.live);free(f);
}
int main(void)
{
    startup_core_parity();startup_core_failures_and_cancellation();startup_core_stale();
    startup_core_allocations();startup_core_empty_and_aliases();startup_core_guard_aliases();startup_core_close_reentry();
    puts("RENDER STARTUP PASS: bounded original validation, exact legacy plans and scan-free checked reset; host software only");return 0;
}
