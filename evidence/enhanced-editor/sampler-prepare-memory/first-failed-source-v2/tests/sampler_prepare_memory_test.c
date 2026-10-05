/* Genuine existing mixed/renderer helpers; inherited suite is not replayed. */
#define main inherited_prepare_memory_mixed_main
#include "mixed_preflight_startup_test.c"
#undef main
#include "../src/editor/sampler_prepare_memory.h"
struct guard_master_memory {unsigned live;};
static void *guard_master_allocate(void *context,size_t bytes)
{struct guard_master_memory *m=context;void *p=malloc(bytes);if(p)++m->live;return p;}
static void guard_master_release(void *context,void *p)
{struct guard_master_memory *m=context;assert(m->live&&p);--m->live;free(p);}
struct guard_memory {
    unsigned calls,releases,live,alias_releases,mode,fail;
    void *alias,*releasing;
    struct pt_sampler_prepare_memory *owner;struct pt_sampler *sampler;
    struct pt_sample_version *replacement;
};
static void *guard_allocate(void *context,size_t bytes)
{
    struct guard_memory *m=context;void *p;++m->calls;
    if(m->mode==1)++m->sampler->generation;
    if(m->mode==2)assert(!m->owner->allocator.allocate(m->owner,8));
    if(m->mode==4)m->sampler->current[0]=m->replacement;
    if(m->fail)return NULL;
    if(m->alias)return m->alias;
    p=malloc(bytes);if(p)++m->live;return p;
}
static void guard_release(void *context,void *p)
{
    struct guard_memory *m=context;
    if(p==m->alias){++m->alias_releases;return;}
    assert(p&&m->live);--m->live;++m->releases;m->releasing=p;
    if(m->mode==3)m->owner->allocator.release(m->owner,p);
    free(p);m->releasing=NULL;
}
struct guard_fixture {
    struct startup_fixture *source;struct guard_master_memory masters;
    struct pt_sampler sampler;struct pt_allocator master_allocator;
    struct pt_pattern_history history;struct pt_pattern_command commands[8];struct pt_event_change changes[8];
    struct pt_sample_version *held[2];
    struct guard_memory memory;struct pt_sampler_prepare_memory owner;
    unsigned char parent[64];
};
static struct guard_fixture *guard_init(unsigned bits)
{
    struct guard_fixture *g=calloc(1,sizeof(*g));struct pt_pcm pcm,owned;unsigned i;
    assert(g);g->source=malloc(sizeof(*g->source));assert(g->source);
    startup_mixed_init(g->source,bits,1);memset(g->parent,0x5a,sizeof(g->parent));
    g->master_allocator=(struct pt_allocator){&g->masters,guard_master_allocate,guard_master_release};
    pt_sampler_init(&g->sampler,&g->master_allocator,1024*1024);
    for(i=0;i<2;++i)assert(pt_sampler_pin(&g->sampler,&g->source->project,i,g->sampler.generation,&pcm,g->held+i)==PT_EDIT_OK);
    assert(pt_pattern_history_init(&g->history,&g->source->project,g->commands,8,g->changes,8)==PT_EDIT_OK);
    owned=g->source->project.samples[0].pcm;owned.capacity=STARTUP_VALUES;owned.data=guard_master_allocate(&g->masters,owned.capacity*sizeof(*owned.data));
    assert(owned.data);for(i=0;i<owned.capacity;++i)owned.data[i]=i<owned.frames?(int32_t)(i%101)-50:0x12345678;
    assert(pt_sampler_append_owned(&g->sampler,&g->source->project,&g->history,&owned,&g->master_allocator,"unused")==PT_EDIT_OK&&!owned.data);
    assert(pt_sampler_attributes(&g->sampler,&g->source->project,&g->history,2,"metadata",64,0)==PT_EDIT_OK);
    g->memory.owner=&g->owner;g->memory.sampler=&g->sampler;return g;
}
static void guard_begin(struct guard_fixture *g)
{
    struct pt_allocator a={&g->memory,guard_allocate,guard_release};
    struct pt_sampler_storage_span p[2]={{g,sizeof(*g)},{g->source,sizeof(*g->source)}};
    assert(pt_sampler_prepare_memory_begin(&g->owner,&g->sampler,&a,p,2));
    /* The source allocator and descriptor arguments have been copied. */
    memset(&a,0,sizeof(a));memset(p,0,sizeof(p));
}
static void guard_close(struct guard_fixture *g)
{
    unsigned i;assert(pt_sampler_prepare_memory_finish(&g->owner));
    assert(!g->memory.live&&!g->memory.alias_releases);
    for(i=0;i<sizeof(g->parent);++i)assert(g->parent[i]==0x5a);
    pt_pattern_history_release(&g->history);
    for(i=0;i<2;++i)pt_sampler_unpin(g->held[i]);
    pt_sampler_release(&g->sampler);assert(!g->sampler.bytes&&!g->masters.live);
    free(g->source);free(g);
}
static void guard_poison(struct guard_fixture *g,unsigned poison)
{
    unsigned i;for(i=0;i<g->source->project.sample_count;++i) {
        struct pt_sample *s=g->source->project.samples+i;
        if(poison){STARTUP_POISON(s->pcm.data,s->pcm.capacity*sizeof(int32_t));STARTUP_POISON(s->slices,s->slice_count*sizeof(uint32_t));}
        else {STARTUP_UNPOISON(s->pcm.data,s->pcm.capacity*sizeof(int32_t));STARTUP_UNPOISON(s->slices,s->slice_count*sizeof(uint32_t));}
    }
    if(poison){STARTUP_POISON(g->source->orders,sizeof(g->source->orders));STARTUP_POISON(g->source->events,sizeof(g->source->events));}
    else {STARTUP_UNPOISON(g->source->orders,sizeof(g->source->orders));STARTUP_UNPOISON(g->source->events,sizeof(g->source->events));}
}
static void guard_alias_cases(void)
{
    unsigned bits,kind;
    for(bits=8;bits<=24;bits+=8)for(kind=0;kind<7;++kind) {
        struct guard_fixture *g=guard_init(bits);struct pt_mixed_preflight_setup *j=NULL;
        struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned count;void *child=NULL;
        struct pt_sampler before=g->sampler;struct pt_project project=g->source->project;
        assert(pt_sampler_version_spans(g->sampler.current[2],spans,PT_SAMPLER_VERSION_SPANS,&count)&&count==4);
        guard_begin(g);
        switch(kind) {
        case 0:g->memory.alias=g->parent+sizeof(g->parent)-1;break;
        case 1:g->memory.alias=(void *)g->sampler.current[2];break; /* Unused version header. */
        case 2:g->memory.alias=g->source->project.samples[2].pcm.data+STARTUP_VALUES-1;break;
        case 3:g->memory.alias=(void *)spans[2].data;break; /* Flat backing header. */
        case 4:g->memory.alias=g->source->project.samples[0].slices+1;break;
        case 5:g->memory.alias=&g->owner;break;
        case 6:child=g->owner.allocator.allocate(&g->owner,32);assert(child);g->memory.alias=child;break;
        }
        guard_poison(g,1);
        assert(pt_mixed_preflight_setup_begin(&g->source->project,&g->source->options,NULL,&startup_caps,&startup_format,1,1,
            &g->owner.allocator,STARTUP_REVISION,g->sampler.generation,&j)==PT_RENDER_SETUP_CAPACITY&&!j);
        guard_poison(g,0);
        assert(g->owner.faulted&&!g->memory.alias_releases&&!g->memory.releases);
        assert(!memcmp(&before,&g->sampler,sizeof(before))&&!memcmp(&project,&g->source->project,sizeof(project)));
        assert(!g->owner.allocator.allocate(&g->owner,16));
        if(child){g->memory.alias=NULL;g->owner.allocator.release(&g->owner,child);assert(g->memory.releases==1);}
        guard_close(g);
    }
}
static void guard_mixed_success(void)
{
    unsigned bits;for(bits=8;bits<=24;bits+=8) {
        struct guard_fixture *g=guard_init(bits);struct pt_mixed_preflight_setup *j=NULL;
        struct pt_mixed_preflight *audit=NULL;struct pt_mixed_report report;struct pt_render_sequence *sequence=NULL;
        struct pt_sampler before=g->sampler;struct pt_project project=g->source->project;unsigned n=0;
        guard_begin(g);guard_poison(g,1);
        assert(pt_mixed_preflight_setup_begin(&g->source->project,&g->source->options,NULL,&startup_caps,&startup_format,1,1,
            &g->owner.allocator,STARTUP_REVISION,STARTUP_GENERATION,&j)==PT_RENDER_SETUP_PENDING&&j);
        guard_poison(g,0);assert(g->memory.calls==2&&g->memory.live==2&&!g->owner.faulted);
        assert(!pt_sampler_prepare_memory_finish(&g->owner));
        startup_mixed_complete(j,4096);
        assert(pt_mixed_preflight_setup_transfer(&j,STARTUP_REVISION,STARTUP_GENERATION,&audit)==PT_RENDER_SETUP_READY&&!j&&audit);
        assert(g->memory.calls==3&&g->memory.live==2);
        do{assert(++n<20000);}while(pt_mixed_preflight_step(audit,&report)==PT_MIXED_PENDING);
        assert(report.result==PT_MIXED_OK&&pt_mixed_preflight_take(audit,&sequence)&&sequence);
        pt_mixed_preflight_close(&audit);assert(g->memory.live==1);
        pt_render_sequence_close(sequence);assert(!g->memory.live&&g->memory.releases==3);
        assert(!memcmp(&before,&g->sampler,sizeof(before))&&!memcmp(&project,&g->source->project,sizeof(project)));
        guard_close(g);
    }
}
static void guard_fault_cases(void)
{
    unsigned kind;for(kind=0;kind<7;++kind) {
        struct guard_fixture *g=guard_init(24);void *p=NULL;unsigned generation=g->sampler.generation;
        guard_begin(g);
        if(kind==0){++g->sampler.generation;assert(!g->owner.allocator.allocate(&g->owner,32)&&!g->memory.calls);}
        if(kind==1||kind==2){g->memory.mode=kind;assert(!g->owner.allocator.allocate(&g->owner,32));assert(g->memory.calls==1&&g->memory.releases==1&&!g->memory.live);}
        if(kind>=3) {
            p=g->owner.allocator.allocate(&g->owner,32);assert(p);
            if(kind==3)g->owner.allocator.release(&g->owner,g->sampler.current[0]);
            if(kind==4){g->owner.allocator.release(&g->owner,p);g->owner.allocator.release(&g->owner,p);p=NULL;}
            if(kind==5){++g->sampler.generation;assert(!g->owner.allocator.allocate(&g->owner,16));}
            if(kind==6){g->memory.mode=3;g->owner.allocator.release(&g->owner,p);p=NULL;}
        }
        assert(g->owner.faulted);g->sampler.generation=generation;
        if(p){g->memory.mode=0;g->owner.allocator.release(&g->owner,p);}
        assert(!g->memory.live);guard_close(g);
    }
}
static void guard_capacity_and_begin(void)
{
    struct guard_fixture *g=guard_init(24);void *p[PT_SAMPLER_PREPARE_BLOCKS];unsigned i,calls;
    struct pt_allocator a={&g->memory,guard_allocate,guard_release};struct pt_sampler_prepare_memory before=g->owner;
    struct pt_sampler_storage_span invalid={(void *)(uintptr_t)(UINTPTR_MAX-1),4};
    assert(!pt_sampler_prepare_memory_begin(&g->owner,&g->sampler,&a,&invalid,1)&&!memcmp(&before,&g->owner,sizeof(before)));
    assert(!pt_sampler_prepare_memory_begin((struct pt_sampler_prepare_memory *)g->sampler.current[0],&g->sampler,&a,NULL,0));
    guard_begin(g);assert(!pt_sampler_prepare_memory_begin(&g->owner,&g->sampler,&a,NULL,0));
    g->memory.fail=1;assert(!g->owner.allocator.allocate(&g->owner,16)&&!g->owner.faulted);g->memory.fail=0;
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS;++i){p[i]=g->owner.allocator.allocate(&g->owner,16);assert(p[i]);}
    calls=g->memory.calls;assert(!g->owner.allocator.allocate(&g->owner,16)&&g->memory.calls==calls&&!g->owner.faulted);
    assert(!pt_sampler_prepare_memory_finish(&g->owner));
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS;++i)g->owner.allocator.release(&g->owner,p[i]);
    assert(!g->owner.faulted&&g->memory.releases==PT_SAMPLER_PREPARE_BLOCKS);guard_close(g);
}
static void guard_changed_source_alias(void)
{
    struct guard_fixture *g=guard_init(24);struct pt_sampler other;
    struct pt_project project=g->source->project;struct pt_sample samples[3];
    struct pt_pcm pcm;struct pt_sample_version *pin=NULL,*original=g->sampler.current[0];
    memcpy(samples,project.samples,sizeof(samples));project.samples=samples;
    pt_sampler_init(&other,&g->master_allocator,1024*1024);
    assert(pt_sampler_pin(&other,&project,0,other.generation,&pcm,&pin)==PT_EDIT_OK);
    guard_begin(g);g->memory.mode=4;g->memory.replacement=pin;g->memory.alias=pin;
    /* Test-only controlled callback mutation: newly named genuine source must
     * also be classified before stale cleanup. Restore before any consumer. */
    assert(!g->owner.allocator.allocate(&g->owner,32));
    assert(g->owner.faulted&&!g->memory.alias_releases&&!g->memory.releases);
    g->sampler.current[0]=original;pt_sampler_unpin(pin);pt_sampler_release(&other);
    guard_close(g);
}
int main(void)
{
    guard_alias_cases();guard_mixed_success();guard_fault_cases();guard_capacity_and_begin();guard_changed_source_alias();
    puts("PREPARE MEMORY PASS:21 known aliases untouched/unreleased,3 genuine checked mixed transfers,7 fault cleanup cases, bounded capacity/begin refusal and changed-source alias; genuine sampler masters/fake allocators; host only");return 0;
}
