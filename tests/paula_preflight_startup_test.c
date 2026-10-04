#define main inherited_render_startup_fixture_main
#include "render_sequence_startup_test.c"
#undef main
#include "../src/editor/paula_preflight.h"
static const struct pt_paula_render_caps startup_caps={3546895,124,65535};
static void startup_paula_begin(struct startup_fixture *f,const int8_t *previous,
    const struct pt_paula_render_caps *caps,struct pt_paula_preflight_setup **j)
{
    assert(pt_paula_preflight_setup_begin(&f->project,&f->options,previous,caps,1,&f->allocator,
        STARTUP_REVISION,STARTUP_GENERATION,j)==PT_RENDER_SETUP_PENDING&&*j);
}
static unsigned startup_paula_complete(struct pt_paula_preflight_setup *j,unsigned work)
{
    struct pt_render_setup_report r;enum pt_render_setup_result result;unsigned n=0,phases=0;
    do {
        result=pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&r);
        assert(result==PT_RENDER_SETUP_PENDING||result==PT_RENDER_SETUP_READY);phases|=1U<<r.phase;
        if(result==PT_RENDER_SETUP_READY)break;
        result=pt_paula_preflight_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,work);
        assert(result==PT_RENDER_SETUP_PENDING||result==PT_RENDER_SETUP_READY);assert(++n<20000);
        assert(pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&r)==result);
        assert(r.last_work<=work);phases|=1U<<r.phase;
    }while(result==PT_RENDER_SETUP_PENDING);
    return phases;
}
static void startup_same_paula_report(const struct pt_paula_preflight_report *a,
    const struct pt_paula_preflight_report *b)
{
    assert(a->result==b->result&&a->render_result==b->render_result&&a->intervals==b->intervals&&a->frames==b->frames);
    assert(a->action==b->action&&a->channel==b->channel&&a->kind==b->kind);
    assert(!memcmp(a->map,b->map,sizeof(a->map))&&!memcmp(a->samples,b->samples,sizeof(a->samples)));
}
static enum pt_paula_capability startup_paula_audit(struct startup_fixture *f,
    struct pt_paula_preflight *w,struct pt_paula_preflight_report *r,unsigned poison)
{
    enum pt_paula_capability result=PT_PAULA_PENDING;unsigned n=0,i,calls=f->memory.calls;
    struct pt_paula_preflight_report before;
    memset(r,0,sizeof(*r));r->result=PT_PAULA_PENDING;
    if(poison){startup_values_poison(f,0);startup_unplayed_poison(f);}
    while(result==PT_PAULA_PENDING){
        before=*r;result=pt_paula_preflight_step(w,r);assert(++n<20000&&f->memory.calls==calls);
        assert(r->intervals>=before.intervals&&r->intervals-before.intervals<=1);
        assert(r->frames>=before.frames&&r->frames-before.frames<=256);
        assert(!(r->intervals!=before.intervals&&r->frames!=before.frames));
        if(poison&&r->intervals){startup_values_unpoison(f);poison=0;}
        if(result!=PT_PAULA_COMPATIBLE)for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!r->samples[i]);
    }
    if(poison)startup_values_unpoison(f);
    return result;
}
static void startup_paula_parity(void)
{
    unsigned bits,mode;
    for(bits=8;bits<=24;bits+=8)for(mode=0;mode<3;++mode){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
        struct pt_paula_preflight *w=(void *)(uintptr_t)1;struct pt_render_sequence *s=(void *)(uintptr_t)1,*reference=NULL;
        struct pt_paula_preflight_report legacy,report;struct pt_render_setup_report pending;
        int32_t *saved=malloc(sizeof(f->pcm));int8_t map[PT_CHANNEL_LIMIT];unsigned calls;
        assert(f&&saved);startup_init(f,bits,1);startup_mode(f,mode);f->options.tracks=1;
        memcpy(saved,f->pcm,sizeof(f->pcm));assert(pt_channels_paula_map(&f->project.channels,NULL,map)==PT_CHANNEL_OK);
        assert(pt_paula_preflight(&f->project,&f->options,map,&startup_caps,1,&f->allocator,&legacy)==PT_PAULA_COMPATIBLE);
        assert(!f->memory.live&&legacy.samples[0]&&!legacy.samples[1]);
        assert(pt_render_sequence_open(&f->project,&f->options,&f->allocator,&reference)==PT_RENDER_OK);
        calls=f->memory.calls;startup_values_poison(f,1);startup_paula_begin(f,map,&startup_caps,&j);startup_values_unpoison(f);
        assert(f->memory.calls==calls+2&&f->memory.live==3);
        assert(pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&pending)==PT_RENDER_SETUP_PENDING);
        assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_PENDING);
        assert(w==(void *)(uintptr_t)1&&f->memory.calls==calls+2);
        assert(pt_paula_preflight_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,0)==PT_RENDER_SETUP_INVALID);
        assert(pt_paula_preflight_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,4097)==PT_RENDER_SETUP_INVALID);
        startup_paula_complete(j,mode==0?1:mode==1?7:4096);assert(f->memory.calls==calls+2);
        startup_values_poison(f,1);
        assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_READY);
        startup_values_unpoison(f);assert(!j&&w&&w!=(void *)(uintptr_t)1&&f->memory.calls==calls+3&&f->memory.live==3);
        assert(!pt_paula_preflight_transfer(w,&s)&&s==(void *)(uintptr_t)1); /* Initial READY is not timeline capability. */
        assert(startup_paula_audit(f,w,&report,1)==PT_PAULA_COMPATIBLE);startup_same_paula_report(&report,&legacy);
        startup_values_poison(f,1);assert(pt_paula_preflight_transfer(w,&s));startup_values_unpoison(f);
        assert((void *)s==f->memory.returned[calls+2]&&f->memory.calls==calls+3);
        {struct pt_render_sequence *second=(void *)(uintptr_t)1;assert(!pt_paula_preflight_transfer(w,&second)&&second==(void *)(uintptr_t)1);}
        pt_paula_preflight_close(&w);assert(!w&&f->memory.live==2);pt_paula_preflight_close(&w);
        assert(startup_compare_sequences(s,reference)==legacy.frames);assert(!memcmp(saved,f->pcm,sizeof(f->pcm)));
        pt_render_sequence_close(s);pt_render_sequence_close(reference);assert(!f->memory.live);free(saved);free(f);
    }
}
static void startup_paula_refusals_and_cancel(void)
{
    unsigned kind,wanted,seen=0;
    for(kind=0;kind<8;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
        struct pt_paula_preflight *w=(void *)(uintptr_t)1;struct pt_render_sequence *out=(void *)(uintptr_t)1;
        struct pt_paula_preflight_report expected,actual;enum pt_render_setup_result result;unsigned n=0;
        assert(f);startup_init(f,24,1);f->options.tracks=1;
        if(kind<6)startup_semantic_mutation(f,kind);
        if(kind==6)f->events[2*16]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0}; /* Later selected stereo. */
        if(kind==7)startup_mode(f,3);
        assert(pt_paula_preflight(&f->project,&f->options,NULL,&startup_caps,1,&f->allocator,&expected)!=PT_PAULA_COMPATIBLE&&!f->memory.live);
        startup_paula_begin(f,NULL,&startup_caps,&j);
        do {result=pt_paula_preflight_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,7);assert(++n<20000);}while(result==PT_RENDER_SETUP_PENDING);
        if(kind<6){
            assert(result==PT_RENDER_SETUP_FAILED&&n>1);
            assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_FAILED&&w==(void *)(uintptr_t)1);
        }else{
            assert(result==PT_RENDER_SETUP_READY); /* Readiness does not pre-approve a later stereo trigger or row range. */
            result=pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w);
            if(kind==7)assert(result==PT_RENDER_SETUP_FAILED&&j&&w==(void *)(uintptr_t)1);
            else {
                assert(result==PT_RENDER_SETUP_READY&&!j);
                assert(startup_paula_audit(f,w,&actual,0)==PT_PAULA_GEOMETRY);startup_same_paula_report(&actual,&expected);
                assert(!pt_paula_preflight_transfer(w,&out)&&out==(void *)(uintptr_t)1);pt_paula_preflight_close(&w);
            }
        }
        assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);free(f);
    }
    for(wanted=PT_RENDER_SETUP_VALIDATE;wanted<=PT_RENDER_SETUP_COMPLETE;++wanted){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
        struct pt_render_setup_report report;unsigned n=0;assert(f);startup_init(f,16,1);f->options.tracks=1;startup_paula_begin(f,NULL,&startup_caps,&j);
        do {
            enum pt_render_setup_result r=pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,&report);
            assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);if((unsigned)report.phase==wanted)break;
            r=pt_paula_preflight_setup_step(j,STARTUP_REVISION,STARTUP_GENERATION,1);
            assert(r==PT_RENDER_SETUP_PENDING||r==PT_RENDER_SETUP_READY);assert(++n<20000);
        }while(1);
        seen|=1U<<report.phase;assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);
        assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY);free(f);
    }
    assert(seen==31);
    /* Genuine subsequent audit work can be closed before measurement, interval,
     * advancement or complete-boundary gate finishes; no sample mask escapes. */
    for(kind=0;kind<5;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
        struct pt_paula_preflight *w=NULL;struct pt_paula_preflight_report r;unsigned n=0,i;assert(f);startup_init(f,8,1);f->options.tracks=1;
        startup_paula_begin(f,NULL,&startup_caps,&j);startup_paula_complete(j,4096);
        assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_READY);
        if(kind)do {
            assert(pt_paula_preflight_step(w,&r)==PT_PAULA_PENDING);for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!r.samples[i]);
            assert(++n<20000);
        }while(!((kind==1&&!r.intervals)||(kind==2&&r.intervals)||(kind==3&&r.frames)||(kind==4&&r.intervals>1&&r.frames)));
        pt_paula_preflight_close(&w);assert(!w&&!f->memory.live);free(f);
    }
}
struct startup_paula_hook {struct pt_paula_preflight_setup **owner;unsigned kind;};
static void startup_paula_reenter(void *context)
{
    struct startup_paula_hook *h=context;
    if(h->kind==0)assert(pt_paula_preflight_setup_step(*h->owner,STARTUP_REVISION,STARTUP_GENERATION,1)==PT_RENDER_SETUP_BUSY);
    if(h->kind==1)assert(pt_paula_preflight_setup_get(*h->owner,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_BUSY);
    if(h->kind==2)assert(pt_paula_preflight_setup_cancel(h->owner)==PT_RENDER_SETUP_BUSY&&*h->owner);
}
static void startup_paula_allocations(void)
{
    unsigned kind;
    for(kind=0;kind<7;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL,*same;
        struct pt_paula_preflight *out=(void *)(uintptr_t)1;struct startup_paula_hook hook;unsigned calls;
        assert(f);startup_init(f,24,1);f->options.tracks=1;
        if(kind<2){f->memory.fail_at=kind+1;
            assert(pt_paula_preflight_setup_begin(&f->project,&f->options,NULL,&startup_caps,1,&f->allocator,
                STARTUP_REVISION,STARTUP_GENERATION,&j)==PT_RENDER_SETUP_CAPACITY);
            assert(!j&&!f->memory.live);free(f);continue;}
        startup_paula_begin(f,NULL,&startup_caps,&j);startup_paula_complete(j,4096);same=j;calls=f->memory.calls;
        if(kind==2){
            f->memory.fail_at=calls+1;
            assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&out)==PT_RENDER_SETUP_CAPACITY);
            assert(j==same&&out==(void *)(uintptr_t)1&&f->memory.live==2);
            assert(pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_READY);f->memory.fail_at=0;
            assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&out)==PT_RENDER_SETUP_READY);
            assert(!j&&f->memory.calls==calls+2&&f->memory.live==2);pt_paula_preflight_close(&out);
        }else {
            hook=(struct startup_paula_hook){&j,kind==6?0:kind-3};f->memory.hook_context=&hook;
            if(kind==6)f->memory.release_hook=startup_paula_reenter;else f->memory.hook=startup_paula_reenter;
            assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&out)==PT_RENDER_SETUP_FAILED);
            assert(j==same&&out==(void *)(uintptr_t)1);
            assert(pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_FAILED);
            assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j);
        }
        assert(!f->memory.live);free(f);
    }
}
static void startup_paula_stale_and_aliases(void)
{
    unsigned kind;
    for(kind=0;kind<7;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL,*same;
        struct pt_paula_preflight *out=(void *)(uintptr_t)1;struct pt_render_setup_report r,sentinel;
        struct pt_paula_render_caps caps=startup_caps;int8_t map[PT_CHANNEL_LIMIT];uint16_t *old=NULL;
        uint32_t revision=STARTUP_REVISION,generation=STARTUP_GENERATION;assert(f);startup_init(f,24,1);f->options.tracks=1;
        assert(pt_channels_paula_map(&f->project.channels,NULL,map)==PT_CHANNEL_OK);
        if(kind==5){old=malloc(sizeof(f->orders));assert(old);memcpy(old,f->orders,sizeof(f->orders));f->project.orders=old;}
        startup_paula_begin(f,map,&caps,&j);same=j;f->project.channels.selected=15;
        assert(pt_paula_preflight_setup_get(j,revision,generation,NULL)==PT_RENDER_SETUP_PENDING);
        if(kind==0)++revision;
        if(kind==1)++generation;
        if(kind==2)++caps.clock_hz;
        if(kind==3)map[0]=3;
        if(kind==4)f->allocator.release=NULL;
        if(kind==5){free(old);f->project.orders=(uint16_t *)(uintptr_t)1;}
        if(kind==6){STARTUP_POISON(f->samples,sizeof(f->samples));f->project.samples=(struct pt_sample *)(uintptr_t)1;}
        memset(&sentinel,0x5a,sizeof(sentinel));memcpy(&r,&sentinel,sizeof(r));
        assert(pt_paula_preflight_setup_get(j,revision,generation,&r)==PT_RENDER_SETUP_STALE&&!memcmp(&r,&sentinel,sizeof(r)));
        assert(pt_paula_preflight_setup_step(j,revision,generation,1)==PT_RENDER_SETUP_STALE);
        assert(pt_paula_preflight_setup_transfer(&j,revision,generation,&out)==PT_RENDER_SETUP_STALE&&j==same&&out==(void *)(uintptr_t)1);
        assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);
        STARTUP_UNPOISON(f->samples,sizeof(f->samples));free(f);
    }
    for(kind=0;kind<6;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL,*same;
        struct pt_paula_preflight *out=(void *)(uintptr_t)1;unsigned char *arena=malloc(131072),*before[2];
        struct pt_paula_render_caps *caps=(void *)arena;int8_t *map=(void *)(arena+64);unsigned i;
        assert(f&&arena);startup_init(f,24,1);f->options.tracks=1;memset(arena,0x6d,131072);*caps=startup_caps;
        assert(pt_channels_paula_map(&f->project.channels,NULL,map)==PT_CHANNEL_OK);
        startup_paula_begin(f,map,caps,&j);startup_paula_complete(j,4096);same=j;
        for(i=0;i<2;++i){before[i]=malloc(f->memory.requested[i]);assert(before[i]);memcpy(before[i],f->memory.returned[i],f->memory.requested[i]);}
        if(kind==0)f->memory.arena=caps;
        if(kind==1)f->memory.arena=map;
        if(kind==2)f->memory.arena=f->memory.returned[0]; /* Actual live startup; release would truly free it. */
        if(kind==3)f->memory.arena=f->memory.returned[1]; /* Whole parent. */
        if(kind==4)f->memory.arena=(unsigned char *)f->memory.returned[0]+f->memory.requested[0]-8;
        if(kind==5)f->memory.arena=(unsigned char *)f->memory.returned[1]+f->memory.requested[1]-8;
        f->memory.free_alias=kind==2||kind==3;
        assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&out)==PT_RENDER_SETUP_ALIAS);
        assert(j==same&&out==(void *)(uintptr_t)1&&!f->memory.alias_releases&&f->memory.live==2);
        assert(!memcmp(caps,&startup_caps,sizeof(*caps)));
        for(i=0;i<2;++i){assert(!memcmp(before[i],f->memory.returned[i],f->memory.requested[i]));free(before[i]);}
        f->memory.arena=NULL;assert(pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,NULL)==PT_RENDER_SETUP_READY);
        assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);free(arena);free(f);
    }
    {
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;struct pt_paula_preflight *w=(void *)(uintptr_t)1;
        int32_t *before;assert(f);startup_init(f,24,1);f->options.tracks=1;before=malloc(sizeof(f->pcm));assert(before);memcpy(before,f->pcm,sizeof(f->pcm));
        assert(pt_paula_preflight_setup_begin(&f->project,&f->options,NULL,&startup_caps,1,&f->allocator,
            STARTUP_REVISION,STARTUP_GENERATION,(struct pt_paula_preflight_setup **)&f->pcm[0].values[STARTUP_VALUES-2])==PT_RENDER_SETUP_ALIAS);
        assert(!f->memory.live&&!memcmp(before,f->pcm,sizeof(f->pcm)));
        startup_paula_begin(f,NULL,&startup_caps,&j);startup_paula_complete(j,4096);
        assert(pt_paula_preflight_setup_get(j,STARTUP_REVISION,STARTUP_GENERATION,
            (struct pt_render_setup_report *)&f->pcm[0].values[STARTUP_VALUES-8])==PT_RENDER_SETUP_ALIAS);
        assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,
            (struct pt_paula_preflight **)&f->pcm[0].values[STARTUP_VALUES-2])==PT_RENDER_SETUP_ALIAS);
        assert(!memcmp(before,f->pcm,sizeof(f->pcm))&&w==(void *)(uintptr_t)1);
        assert(pt_paula_preflight_setup_cancel(&j)==PT_RENDER_SETUP_READY&&!j&&!f->memory.live);free(before);free(f);
    }
}
static void startup_paula_second_alias(void *context)
{
    struct startup_memory *m=context;m->arena=m->returned[0];m->free_alias=1;
}
struct startup_paula_stale_alias_hook {struct startup_fixture *fixture;unsigned options;};
static void startup_paula_second_stale_alias(void *context)
{
    struct startup_paula_stale_alias_hook *h=context;struct startup_fixture *f=h->fixture;
    f->memory.arena=f->memory.returned[0];f->memory.free_alias=1;
    if(h->options)f->options.rate=44100;else ++f->project.bpm;
}
static void startup_paula_begin_aliases(void)
{
    unsigned kind;
    for(kind=0;kind<5;++kind){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
        unsigned char *arena=malloc(131072),*before=malloc(131072);struct pt_paula_render_caps *caps=(void *)arena;
        struct startup_paula_stale_alias_hook hook;int8_t *map=(void *)(arena+64);
        assert(f&&arena&&before);startup_init(f,24,1);f->options.tracks=1;
        memset(arena,0x6d,131072);*caps=startup_caps;assert(pt_channels_paula_map(&f->project.channels,NULL,map)==PT_CHANNEL_OK);memcpy(before,arena,131072);
        if(kind<2)f->memory.arena=kind?map:(void *)caps;
        else {
            f->memory.hook_at=2;
            if(kind==2){f->memory.hook=startup_paula_second_alias;f->memory.hook_context=&f->memory;}
            else {hook=(struct startup_paula_stale_alias_hook){f,kind==4};f->memory.hook=startup_paula_second_stale_alias;f->memory.hook_context=&hook;}
        }
        assert(pt_paula_preflight_setup_begin(&f->project,&f->options,map,caps,1,&f->allocator,
            STARTUP_REVISION,STARTUP_GENERATION,&j)==PT_RENDER_SETUP_ALIAS);
        assert(!j&&!f->memory.live&&!memcmp(before,arena,131072));
        if(kind<2)assert(f->memory.calls==1&&!f->memory.alias_releases&&!f->memory.releases);
        else assert(f->memory.calls==2&&f->memory.alias_releases==1&&f->memory.releases==1);
        if(kind==3)assert(f->project.bpm==126&&f->options.rate==48000);
        if(kind==4)assert(f->project.bpm==125&&f->options.rate==44100);
        /* The second-return alias acquired no second ownership. Its one release
         * above belongs to cancellation of the genuinely allocated inner owner.
         * A premature release of the alias followed by cancellation is actual
         * freed-storage misuse, detectable by the host sanitizer. */
        free(before);free(arena);free(f);
    }
}
static void startup_paula_reuse_original_controls(void)
{
    struct startup_fixture *f=malloc(sizeof(*f));unsigned char *controls=malloc(4096);
    struct pt_paula_render_caps *caps=(void *)controls;int8_t *map=(void *)(controls+64);
    struct pt_paula_preflight_setup *j=NULL;struct pt_paula_preflight *w=NULL;struct pt_render_sequence *s;
    struct pt_paula_preflight_report expected,*report=(void *)controls;
    struct pt_render_sequence **sequence_out=(void *)(controls+64);struct pt_paula_preflight **work_slot=(void *)controls;unsigned calls;
    assert(f&&controls);startup_init(f,24,1);f->options.tracks=1;memset(controls,0x6d,4096);*caps=startup_caps;
    assert(pt_channels_paula_map(&f->project.channels,NULL,map)==PT_CHANNEL_OK);
    assert(pt_paula_preflight(&f->project,&f->options,map,caps,1,&f->allocator,&expected)==PT_PAULA_COMPATIBLE&&!f->memory.live);
    calls=f->memory.calls;startup_paula_begin(f,map,caps,&j);startup_paula_complete(j,7);
    assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_READY&&!j&&f->memory.live==2);
    /* Both original borrowed controls become ordinary caller output storage as
     * soon as setup transfer succeeds. The genuine audit uses its own copies. */
    memset(controls,0xa5,4096);assert(startup_paula_audit(f,w,report,0)==PT_PAULA_COMPATIBLE);
    startup_same_paula_report(report,&expected);assert(f->memory.calls==calls+3);
    *sequence_out=(void *)(uintptr_t)1;assert(pt_paula_preflight_transfer(w,sequence_out));s=*sequence_out;
    *work_slot=w;pt_paula_preflight_close(work_slot);assert(!*work_slot&&f->memory.live==1);
    free(controls);pt_render_sequence_close(s);assert(!f->memory.live);free(f);
}
static void startup_paula_empty(void)
{
    struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
    struct pt_paula_preflight *w=NULL;struct pt_paula_preflight_report expected,actual;struct pt_render_sequence *s=(void *)(uintptr_t)1;
    assert(f);startup_init(f,24,1);f->options.tracks=1;f->project.samples=NULL;f->project.sample_count=0;
    memset(f->events,0,sizeof(f->events));f->events[15].effect=15;
    assert(pt_paula_preflight(&f->project,&f->options,NULL,&startup_caps,1,&f->allocator,&expected)==PT_PAULA_INVALID&&!f->memory.live);
    startup_paula_begin(f,NULL,&startup_caps,&j);startup_paula_complete(j,7);
    assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_READY&&!j);
    assert(startup_paula_audit(f,w,&actual,0)==expected.result);startup_same_paula_report(&actual,&expected);
    assert(!pt_paula_preflight_transfer(w,&s)&&s==(void *)(uintptr_t)1);pt_paula_preflight_close(&w);assert(!w&&!f->memory.live);
    f->project.order_count=0;
    assert(pt_paula_preflight_setup_begin(&f->project,&f->options,NULL,&startup_caps,1,&f->allocator,
        STARTUP_REVISION,STARTUP_GENERATION,&j)!=PT_RENDER_SETUP_PENDING);
    assert(!j&&!f->memory.live);free(f);
}
struct startup_paula_close_hook {
    struct startup_memory *memory;struct pt_paula_preflight **owner;
    struct pt_render_sequence *sequence;unsigned releases,inspect_sequence;
};
static void startup_paula_close_reenter(void *context)
{
    struct startup_paula_close_hook *h=context;struct pt_paula_preflight_report report,before;
    struct pt_paula_preflight *same=*h->owner;struct pt_render_sequence *out=(void *)(uintptr_t)1;
    assert(same&&h->memory->releases==h->releases+1);memset(&report,0x5a,sizeof(report));memcpy(&before,&report,sizeof(before));
    assert(pt_paula_preflight_step(same,&report)==PT_PAULA_INVALID&&!memcmp(&report,&before,sizeof(report)));
    assert(!pt_paula_preflight_transfer(same,&out)&&out==(void *)(uintptr_t)1);
    pt_paula_preflight_close(h->owner);assert(*h->owner==same&&h->memory->releases==h->releases+1);
    if(h->inspect_sequence){
        struct startup_sequence_close_hook nested={h->memory,h->sequence,h->releases};
        startup_sequence_close_reenter(&nested);
    }
}
static void startup_paula_close_reentry(void)
{
    unsigned transfer;
    for(transfer=0;transfer<2;++transfer){
        struct startup_fixture *f=malloc(sizeof(*f));struct pt_paula_preflight_setup *j=NULL;
        struct pt_paula_preflight *w=NULL;struct pt_render_sequence *s;struct pt_paula_preflight_report report;
        struct startup_paula_close_hook hook;struct startup_sequence_close_hook sequence_hook;unsigned releases;
        assert(f);startup_init(f,24,1);f->options.tracks=1;startup_paula_begin(f,NULL,&startup_caps,&j);startup_paula_complete(j,4096);
        assert(pt_paula_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&w)==PT_RENDER_SETUP_READY&&!j);
        assert(startup_paula_audit(f,w,&report,0)==PT_PAULA_COMPATIBLE);s=f->memory.returned[2];
        if(transfer){struct pt_render_sequence *out=NULL;assert(pt_paula_preflight_transfer(w,&out)&&out==s);}
        releases=f->memory.releases;hook=(struct startup_paula_close_hook){&f->memory,&w,s,releases,!transfer};
        f->memory.hook_context=&hook;f->memory.release_hook=startup_paula_close_reenter;
        pt_paula_preflight_close(&w);assert(!w&&f->memory.releases==releases+(transfer?1:2));
        if(transfer){
            assert(f->memory.live==1);sequence_hook=(struct startup_sequence_close_hook){&f->memory,s,f->memory.releases};
            f->memory.hook_context=&sequence_hook;f->memory.release_hook=startup_sequence_close_reenter;
            pt_render_sequence_close(s);assert(f->memory.releases==releases+2);
        }
        assert(!f->memory.live);pt_paula_preflight_close(&w);free(f);
    }
}
int main(void)
{
    startup_paula_parity();startup_paula_refusals_and_cancel();startup_paula_allocations();startup_paula_stale_and_aliases();startup_paula_begin_aliases();startup_paula_reuse_original_controls();startup_paula_empty();startup_paula_close_reentry();
    puts("PAULA STARTUP PASS: initial readiness, complete timeline audit and same checked sequence transfer; host software only");return 0;
}
