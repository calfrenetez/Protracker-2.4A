#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_paula_readers.c"
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define POISON(p,n) __asan_poison_memory_region((p),(n))
#define UNPOISON(p,n) __asan_unpoison_memory_region((p),(n))
#endif
#endif
#ifndef POISON
#define POISON(p,n) ((void)(p),(void)(n))
#define UNPOISON(p,n) ((void)(p),(void)(n))
#endif
#define STORAGE 2048
struct memory {unsigned calls,releases,live,fail,chip_calls;void *arena;void (*hook)(void *);void *hook_context;};
static void *allocate(void *context,size_t n)
{
    struct memory *m=context;void *p;++m->calls;
    if(m->hook){void (*f)(void *)=m->hook;m->hook=NULL;f(m->hook_context);}
    if(m->calls==m->fail)return NULL;
    p=m->arena?m->arena:malloc(n);if(p)++m->live;return p;
}
static void release(void *context,void *p)
{
    struct memory *m=context;assert(p&&m->live);++m->releases;--m->live;
    if(p!=m->arena)free(p);
}
static void *chip_allocate(void *context,size_t n)
{struct memory *m=context;++m->chip_calls;return allocate(context,n);}
static void chip_release(void *context,void *p,size_t n)
{(void)n;release(context,p);}
struct fixture {
    struct memory memory;
    struct pt_allocator allocator;
    struct pt_sampler sampler;
    struct pt_project project;
    struct pt_paula_readers_config config;
    uint16_t orders[1];struct pt_event events[256];struct pt_sample samples[1];
    struct pt_extension extensions[1];uint8_t payload[4];uint32_t slices[3];
    union {int32_t values[STORAGE];struct pt_paula_readers_preparation workspace;
        struct {int32_t prefix[STORAGE-2];struct pt_paula_readers_pool *out;} scalar;} master;
};
static void init(struct fixture *f,unsigned bits,unsigned channels)
{
    unsigned i;memset(f,0,sizeof(*f));f->allocator=(struct pt_allocator){&f->memory,allocate,release};
    pt_sampler_init(&f->sampler,&f->allocator,1024*1024);pt_channels_init(&f->project.channels);
    f->project.order_count=f->project.pattern_count=f->project.sample_count=1;
    f->project.orders=f->orders;f->project.events=f->events;f->project.samples=f->samples;
    f->project.bpm=125;f->project.speed=6;
    f->project.extensions=f->extensions;f->project.extension_count=1;
    f->extensions[0]=(struct pt_extension){0x58595a31UL,4,1,f->payload};
    for(i=0;i<STORAGE;++i)f->master.values[i]=0x12345678; /* padding is not audio */
    for(i=0;i<33*channels;++i)f->master.values[i]=bits==8?(int32_t)(i%200)-100:
        bits==16?((i&1)?-0x1234:0x1234):((i&1)?-0x123456:0x123456);
    f->samples[0].pcm=(struct pt_pcm){f->master.values,STORAGE,33,48000,(uint8_t)channels,(uint8_t)bits};
    f->samples[0].volume=64;f->samples[0].finetune=-3;f->samples[0].interpolation=1;
    f->samples[0].loop=PT_LOOP_FORWARD;f->samples[0].loop_start=3;f->samples[0].loop_end=30;
    f->slices[0]=0;f->slices[1]=17;f->slices[2]=32;f->samples[0].slices=f->slices;f->samples[0].slice_count=3;
    f->events[255]=(struct pt_event){428,3,PT_NOTE_PERIOD,1,0,0,0,0};
    f->config=(struct pt_paula_readers_config){2,8,1024*1024,32768,UINT64_C(0x100000007),
        &f->memory,chip_allocate,chip_release};
}
static void begin_job(struct fixture *f,struct pt_paula_readers_preparation *j)
{assert(pt_paula_readers_prepare_begin(j,&f->allocator,&f->sampler,&f->project,&f->config,11)==PT_PAULA_READERS_OK);}
static unsigned complete(struct fixture *f,struct pt_paula_readers_preparation *j,unsigned work)
{
    enum pt_paula_readers_result r;unsigned calls=0;
    do {r=pt_paula_readers_prepare_step(j,11,work);assert(++calls<=1024);assert(j->validation.last_work<=work);
        assert(!f->memory.calls&&!f->memory.live&&!f->memory.chip_calls&&!f->sampler.bytes);
        assert(r==PT_PAULA_READERS_PENDING||r==PT_PAULA_READERS_OK);
    }while(r==PT_PAULA_READERS_PENDING);
    return calls;
}
static void precision_and_bounds(void)
{
    unsigned bits,channels,w;const unsigned work[3]={1,7,4096};
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)for(w=0;w<3;++w) {
        struct fixture f;struct pt_paula_readers_preparation j={0},before;
        struct pt_paula_readers_pool *p=(void *)1;int32_t master[STORAGE];unsigned calls;
        init(&f,bits,channels);memcpy(master,f.master.values,sizeof(master));
        POISON(f.master.values,33*channels*sizeof(int32_t));POISON(f.slices,sizeof(f.slices));
        POISON(f.events,sizeof(f.events));POISON(f.orders,sizeof(f.orders));POISON(f.payload,sizeof(f.payload));
        begin_job(&f,&j); /* ASan proves begin never scans semantic values. */
        UNPOISON(f.master.values,33*channels*sizeof(int32_t));UNPOISON(f.slices,sizeof(f.slices));
        UNPOISON(f.events,sizeof(f.events));UNPOISON(f.orders,sizeof(f.orders));UNPOISON(f.payload,sizeof(f.payload));
        assert(!f.memory.calls&&!f.sampler.bytes);before=j;
        assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_PENDING&&p==(void *)1&&!memcmp(&j,&before,sizeof(j)));
        assert(pt_paula_readers_prepare_step(&j,11,0)==PT_PAULA_READERS_INVALID&&!memcmp(&j,&before,sizeof(j)));
        assert(pt_paula_readers_prepare_step(&j,11,4097)==PT_PAULA_READERS_INVALID&&!memcmp(&j,&before,sizeof(j)));
        calls=complete(&f,&j,work[w]);assert(work[w]!=1||calls>256);
        /* Completed transfer also avoids both legacy PCM/event rescans. */
        POISON(f.master.values,33*channels*sizeof(int32_t));POISON(f.slices,sizeof(f.slices));POISON(f.events,sizeof(f.events));POISON(f.orders,sizeof(f.orders));
        assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_OK&&p!=(void *)1);
        UNPOISON(f.master.values,33*channels*sizeof(int32_t));UNPOISON(f.slices,sizeof(f.slices));UNPOISON(f.events,sizeof(f.events));UNPOISON(f.orders,sizeof(f.orders));
        before=(struct pt_paula_readers_preparation){0};assert(!memcmp(&j,&before,sizeof(j)));
        assert(f.memory.calls==1&&f.memory.live==1&&!f.memory.chip_calls&&!f.sampler.bytes);
        assert(p->config.generation==f.config.generation&&p->generation==f.sampler.generation);
        assert(!memcmp(&p->original[0],&f.samples[0],sizeof(f.samples[0]))&&!memcmp(master,f.master.values,sizeof(master)));
        assert(pt_paula_readers_close(p)&&!f.memory.live&&f.memory.releases==1);
        assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_INVALID);
    }
}
static void semantic_failures(void)
{
    unsigned kind;
    for(kind=0;kind<5;++kind) {
        struct fixture f;struct pt_paula_readers_preparation j={0};struct pt_paula_readers_pool *p=(void *)1;
        enum pt_paula_readers_result r;unsigned calls=0;init(&f,24,2);
        if(kind==0)f.master.values[65]=0x800000;
        if(kind==1)f.slices[2]=33;
        if(kind==2)f.events[255].flags=2;
        if(kind==3)f.extensions[0].id=0x48454144UL;
        if(kind==4)f.samples[0].loop_end=34;
        assert(pt_project_validate(&f.project,NULL)==PT_PROJECT_INVALID);begin_job(&f,&j);
        do {r=pt_paula_readers_prepare_step(&j,11,7);assert(++calls<=1024);}while(r==PT_PAULA_READERS_PENDING);
        assert(r==PT_PAULA_READERS_INVALID&&!f.memory.calls&&!f.sampler.bytes);
        assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_INVALID&&p==(void *)1&&!f.memory.calls);
        assert(pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK);
    }
}
static void cancel_every_phase(void)
{
    unsigned wanted,seen=0;
    for(wanted=1;wanted<=8;++wanted) {
        struct fixture f;struct pt_paula_readers_preparation j={0},zero={0};unsigned n=0;init(&f,16,2);begin_job(&f,&j);
        while(j.validation.phase<wanted) {
            enum pt_paula_readers_result r=pt_paula_readers_prepare_step(&j,11,1);
            assert(++n<=1024&&(r==PT_PAULA_READERS_OK||r==PT_PAULA_READERS_PENDING));
        }
        /* SAMPLE is an atomic metadata item, never an externally cancellable
         * step boundary; all other pending/completed boundaries are visited. */
        if(wanted==3){assert(j.validation.phase==4);}else {assert(j.validation.phase==wanted);++seen;}
        /* No source lifetime is needed by cancellation, even after replacement. */
        f.project.samples=NULL;f.project.extensions=NULL;f.project.events=NULL;
        assert(pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK&&!memcmp(&j,&zero,sizeof(j)));
        assert(!f.memory.calls&&!f.memory.live&&!f.memory.chip_calls&&!f.sampler.bytes);
    }
    assert(seen==7);
}
static void stale_boundaries(void)
{
    unsigned kind;
    for(kind=0;kind<8;++kind) {
        struct fixture f;struct pt_paula_readers_preparation j={0},before;
        struct pt_paula_readers_pool *p=(void *)1;unsigned calls=0;enum pt_paula_readers_result r;
        init(&f,24,2);begin_job(&f,&j);
        do {r=pt_paula_readers_prepare_step(&j,11,1);assert(++calls<=1024);}while(j.validation.phase!=4);
        assert(r==PT_PAULA_READERS_PENDING);before=j;
        if(kind==0)++f.sampler.generation;
        if(kind==1)f.project.samples=NULL;
        if(kind==2)++f.project.sample_count;
        if(kind==3)++f.samples[0].pcm.rate;
        if(kind==4)f.project.events=NULL;
        if(kind==5)f.sampler.table_bytes=1;
        if(kind==6)f.sampler.allocator.allocate=NULL;
        assert(pt_paula_readers_prepare_step(&j,kind==7?12:11,7)==PT_PAULA_READERS_STALE&&!memcmp(&j,&before,sizeof(j)));
        assert(pt_paula_readers_prepare_transfer(&j,kind==7?12:11,&p)==PT_PAULA_READERS_STALE&&p==(void *)1&&!memcmp(&j,&before,sizeof(j)));
        assert(!f.memory.calls&&pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK);
    }
    {struct fixture f;struct pt_paula_readers_preparation j={0};struct pt_sample *old;struct pt_extension *ext;
     struct pt_event *events;struct pt_paula_readers_pool *p=(void *)1;init(&f,8,1);
     old=malloc(sizeof(f.samples));ext=malloc(sizeof(f.extensions));events=malloc(sizeof(f.events));assert(old&&ext&&events);
     memcpy(old,f.samples,sizeof(f.samples));memcpy(ext,f.extensions,sizeof(f.extensions));memcpy(events,f.events,sizeof(f.events));
     f.project.samples=old;f.project.extensions=ext;f.project.events=events;begin_job(&f,&j);
     free(old);free(ext);free(events);f.project.samples=NULL;f.project.extensions=NULL;f.project.events=NULL;
     assert(pt_paula_readers_prepare_step(&j,11,1)==PT_PAULA_READERS_STALE);
     assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_STALE&&p==(void *)1);
     assert(pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK&&!f.memory.calls);}
}
static void alias_and_wrap(void)
{
    struct fixture f;struct pt_paula_readers_preparation j={0},before;unsigned i;
    struct pt_paula_readers_pool *p=(void *)1;int32_t master[STORAGE];struct pt_sampler sampler;
    struct pt_project project;struct pt_sample sample;struct pt_allocator allocator;struct pt_paula_readers_config config;
    init(&f,16,2);memcpy(master,f.master.values,sizeof(master));
    assert(pt_paula_readers_prepare_begin(&f.master.workspace,&f.allocator,&f.sampler,&f.project,&f.config,11)==PT_PAULA_READERS_INVALID);
    assert(!memcmp(master,f.master.values,sizeof(master))&&!f.memory.calls);
    begin_job(&f,&j);complete(&f,&j,4096);before=j;sampler=f.sampler;project=f.project;sample=f.samples[0];allocator=f.allocator;config=f.config;
    for(i=0;i<8;++i) {
        struct pt_paula_readers_pool **bad=i==0?&f.master.scalar.out:i==1?(void *)&f.samples[0]:
            i==2?(void *)&f.project:i==3?(void *)&f.sampler:i==4?(void *)&j.config:
            i==5?(void *)&f.allocator:i==6?(void *)&f.config:(void *)(UINTPTR_MAX-sizeof(*bad)+1);
        assert(pt_paula_readers_prepare_transfer(&j,11,bad)==PT_PAULA_READERS_INVALID&&!f.memory.calls);
        assert(!memcmp(&j,&before,sizeof(j))&&!memcmp(master,f.master.values,sizeof(master)));
        assert(!memcmp(&sampler,&f.sampler,sizeof(sampler))&&!memcmp(&project,&f.project,sizeof(project)));
        assert(!memcmp(&sample,&f.samples[0],sizeof(sample))&&!memcmp(&allocator,&f.allocator,sizeof(allocator))&&!memcmp(&config,&f.config,sizeof(config)));
    }
    assert(pt_paula_readers_prepare_step((void *)(UINTPTR_MAX-sizeof(j)+1),11,1)==PT_PAULA_READERS_INVALID);
    assert(pt_paula_readers_prepare_transfer((void *)(UINTPTR_MAX-sizeof(j)+1),11,&p)==PT_PAULA_READERS_INVALID&&p==(void *)1);
    assert(pt_paula_readers_prepare_cancel((void *)(UINTPTR_MAX-sizeof(j)+1))==PT_PAULA_READERS_INVALID);
    assert(pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK);
    /* Missing/overflow/wrapped declared storage fails closed before workspace writes. */
    for(i=0;i<3;++i) {
        init(&f,8,1);before=j;
        if(i==0)f.samples[0].pcm.data=NULL;
        if(i==1)f.samples[0].pcm.capacity=SIZE_MAX/sizeof(int32_t)+1;
        if(i==2)f.samples[0].pcm.data=(void *)(UINTPTR_MAX-3);
        assert(pt_paula_readers_prepare_begin(&j,&f.allocator,&f.sampler,&f.project,&f.config,11)==PT_PAULA_READERS_INVALID&&!memcmp(&j,&before,sizeof(j)));
    }
}
struct callback {struct fixture *f;struct pt_paula_readers_preparation *j;unsigned kind,calls;};
static void mutation(void *context)
{
    struct callback *c=context;struct pt_paula_readers_pool *out=(void *)1;++c->calls;
    if(c->kind==0)c->f->project.samples=NULL;
    if(c->kind==1)++c->f->sampler.generation;
    if(c->kind==2)c->f->sampler.allocator.allocate=NULL;
    if(c->kind==3)assert(pt_paula_readers_prepare_cancel(c->j)==PT_PAULA_READERS_BUSY);
    if(c->kind==4)assert(pt_paula_readers_prepare_transfer(c->j,11,&out)==PT_PAULA_READERS_BUSY&&out==(void *)1);
}
static void allocation_and_callbacks(void)
{
    unsigned kind;
    for(kind=0;kind<5;++kind) {
        struct fixture f;struct pt_paula_readers_preparation j={0};struct pt_paula_readers_pool *p=(void *)1;
        struct callback callback;init(&f,24,2);begin_job(&f,&j);complete(&f,&j,4096);
        callback=(struct callback){&f,&j,kind,0};f.memory.hook=mutation;f.memory.hook_context=&callback;
        assert(pt_paula_readers_prepare_transfer(&j,11,&p)==(kind<3?PT_PAULA_READERS_STALE:PT_PAULA_READERS_INVALID));
        assert(p==(void *)1&&callback.calls==1&&f.memory.calls==1&&f.memory.releases==1&&!f.memory.live&&!f.memory.chip_calls&&!f.sampler.bytes);
        assert(pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK);
    }
    {struct fixture f;struct pt_paula_readers_preparation j={0},before;struct pt_paula_readers_pool *p=(void *)1;
     init(&f,16,1);f.config.control_budget=sizeof(struct pt_paula_readers_pool)-1;begin_job(&f,&j);complete(&f,&j,4096);before=j;
     assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_CAPACITY&&p==(void *)1&&!memcmp(&j,&before,sizeof(j))&&!f.memory.calls);
     assert(pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK);
     f.config.control_budget=sizeof(struct pt_paula_readers_pool);begin_job(&f,&j);complete(&f,&j,4096);f.memory.fail=1;before=j;
     assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_CAPACITY&&p==(void *)1&&!memcmp(&j,&before,sizeof(j))&&!f.memory.live);
     f.memory.fail=0;assert(pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_OK&&pt_paula_readers_close(p)&&!f.memory.live);}
    {struct fixture f;struct pt_paula_readers_preparation j={0};
     union {struct pt_paula_readers_pool pool;struct {struct pt_paula_readers_pool *out;} scalar;} arena;
     unsigned char before[sizeof(arena)];init(&f,8,1);memset(&arena,0xa5,sizeof(arena));memcpy(before,&arena,sizeof(arena));
     begin_job(&f,&j);complete(&f,&j,4096);f.memory.arena=&arena;
     assert(pt_paula_readers_prepare_transfer(&j,11,&arena.scalar.out)==PT_PAULA_READERS_INVALID&&!memcmp(&arena,before,sizeof(arena)));
     assert(!f.memory.live&&f.memory.releases==1&&pt_paula_readers_prepare_cancel(&j)==PT_PAULA_READERS_OK);}
}
static void repeated_lifecycle(void)
{
    struct fixture f;struct pt_paula_readers_preparation j={0};unsigned i;init(&f,24,2);
    for(i=0;i<20;++i) {
        struct pt_paula_readers_pool *p=NULL;unsigned n=0;enum pt_paula_readers_result r;
        begin_job(&f,&j);do {r=pt_paula_readers_prepare_step(&j,11,4096);assert(++n<=10);}while(r==PT_PAULA_READERS_PENDING);
        assert(r==PT_PAULA_READERS_OK&&pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_OK);
        assert(pt_paula_readers_close(p)&&!f.memory.live&&!f.memory.chip_calls&&!f.sampler.bytes);
    }
    assert(f.memory.calls==20&&f.memory.releases==20);
}
static void empty_and_begin_refusals(void)
{
    struct fixture f;struct pt_paula_readers_preparation j={0},before;
    struct pt_paula_readers_pool *p=NULL;unsigned kind,n=0;enum pt_paula_readers_result result;
    init(&f,8,1);f.project.sample_count=0;f.project.samples=NULL;memset(f.events,0,sizeof(f.events));
    begin_job(&f,&j);
    do {result=pt_paula_readers_prepare_step(&j,11,7);assert(++n<=128);}while(result==PT_PAULA_READERS_PENDING);
    assert(result==PT_PAULA_READERS_OK&&pt_paula_readers_prepare_transfer(&j,11,&p)==PT_PAULA_READERS_OK);
    assert(p&&p->bridge.count==0&&pt_paula_readers_close(p)&&!f.memory.live&&!f.sampler.bytes&&!f.memory.chip_calls);
    for(kind=0;kind<4;++kind) {
        init(&f,8,1);before=j;
        if(kind==0)f.allocator.allocate=NULL;
        if(kind==1)f.sampler.allocator.release=NULL;
        if(kind==2)f.config.chip_allocate=NULL;
        if(kind==3)f.config.maximum_readers=9;
        assert(pt_paula_readers_prepare_begin(&j,&f.allocator,&f.sampler,&f.project,&f.config,11)==PT_PAULA_READERS_INVALID);
        assert(!memcmp(&j,&before,sizeof(j))&&!f.memory.calls&&!f.memory.live);
    }
    init(&f,8,1);before=j;
    assert(pt_paula_readers_prepare_begin((void *)(UINTPTR_MAX-sizeof(j)+1),&f.allocator,&f.sampler,&f.project,&f.config,11)==PT_PAULA_READERS_INVALID);
    assert(pt_paula_readers_prepare_begin(&j,(void *)(UINTPTR_MAX-sizeof(f.allocator)+1),&f.sampler,&f.project,&f.config,11)==PT_PAULA_READERS_INVALID);
    assert(pt_paula_readers_prepare_begin(&j,&f.allocator,(void *)(UINTPTR_MAX-sizeof(f.sampler)+1),&f.project,&f.config,11)==PT_PAULA_READERS_INVALID);
    assert(pt_paula_readers_prepare_begin(&j,&f.allocator,&f.sampler,(void *)(UINTPTR_MAX-sizeof(f.project)+1),&f.config,11)==PT_PAULA_READERS_INVALID);
    assert(pt_paula_readers_prepare_begin(&j,&f.allocator,&f.sampler,&f.project,(void *)(UINTPTR_MAX-sizeof(f.config)+1),11)==PT_PAULA_READERS_INVALID);
    assert(!memcmp(&j,&before,sizeof(j))&&!f.memory.calls);
}
int main(void)
{
    precision_and_bounds();semantic_failures();cancel_every_phase();stale_boundaries();alias_and_wrap();
    allocation_and_callbacks();repeated_lifecycle();empty_and_begin_refusals();
    printf("STARTUP WORKSPACE: %lu bytes; max step work %u\n",(unsigned long)sizeof(struct pt_paula_readers_preparation),PT_PROJECT_VALIDATION_WORK_MAX);
    puts("PAULA READERS STARTUP PASS: bounded validation, cancellable preparation and checked transfer; software ownership only");return 0;
}
