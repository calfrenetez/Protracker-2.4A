/* Private ordinary-TASK native RAM component only. No publication, backend
 * execution, receipt, output, IRQ, timer, filesystem or hardware-quiet claim. */
#if !defined(PT_PREPARED_PAULA_MUTATION_NATIVE_TASK_PROBE_ONLY) || PT_PREPARED_PAULA_MUTATION_NATIVE_TASK_PROBE_ONLY != 1
#error "This compositor RAM probe requires its exact native TASK option"
#endif
#if !defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY) || PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY != 1
#error "This compositor RAM probe requires the original native lifetime TASK option"
#endif
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME) || defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
#error "Native TASK RAM probe and HOST opt-ins are exclusive"
#endif
#include "../editor/prepared_paula_mutation_barrier.h"
#include "../editor/editor.h"
#include "../editor/sampler_internal.h"
#include "../editor/sampler_paula_internal.h"
#include "native_checked_memory.h"
#include "paula_memory.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "PREPARED PAULA MUTATION RAM PASS:real editor;3 Fast mono8/16/24 masters;3 Chip caches;unpublished1C/1R;veto then deferred7-drain cleanup;separate detach;master/save preserved;zero ledgers;NO OUTPUT AUTHORITY"
enum {FRAMES=513,VALUES=521,PACKED=514,TAIL=32,STEP_LIMIT=64};
typedef char probe_fixed_abi[(CHAR_BIT==8&&sizeof(void *)==4&&sizeof(uintptr_t)==4&&
    sizeof(size_t)==4&&sizeof(uint32_t)==4&&sizeof(uint64_t)==8)?1:-1];
struct chip_block {void *data;size_t bytes;unsigned sample;};
struct chip_pool {
    struct pt_sampler_paula bridge;
    struct chip_block block[3];
    size_t bytes;
    unsigned allocated,released,hint;
};
struct control_allocator {void *queue;size_t bytes;unsigned allocated,released;};
struct refusal_backend {unsigned calls;};
struct probe_state {
    struct ExecBase *exec;struct Task *task;int idnest,tdnest;
    struct pt_project project;
    struct pt_sample samples[3],saved[3];
    struct pt_event events[4*PT_PROJECT_ROWS];uint16_t order;
    struct pt_editor *editor;struct pt_allocator allocator;
    struct pt_sample_version *pin[3];struct pt_pcm pcm[3];
    int32_t source[3][VALUES],source_before[3][VALUES];
    uint32_t markers[3][2],markers_before[3][2];
    uint8_t packed[3][PACKED];
    struct chip_pool pool;
    struct pt_prepared_paula_mutation_barrier *barrier;size_t barrier_capacity;
    struct pt_prepared_paula_sources *owner;size_t owner_capacity;
    struct pt_prepared_paula_lifetime *lifetime;size_t lifetime_capacity;
    struct pt_prepared_paula_mutation_config config;
    struct pt_readers_backend backend;
    struct control_allocator control;struct refusal_backend refusal;
    uint8_t *encoded,*scratch,*owner_before,*lifetime_before;size_t encoded_bytes;
    void *chip_ambiguous;size_t chip_ambiguous_bytes;
    unsigned chip_uncertain,preparation_steps,drain_steps,admission_refusals,post_detach_edit;
    uint8_t context_tail[TAIL];
};
struct ledger {
    size_t fast_bytes,chip_bytes;
    unsigned live,allocated,released,chip_allocated,chip_released,backend_calls;
};
static struct probe_state state;
static struct pt_private_native_memory memory;

static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* Preserve HUNK/task/all original surviving contexts on uncertainty.
     * No retry, force cleanup, source reset, printing, Wait or implicit Exit. */
    for(;;)__asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
#define REQUIRE(condition) do {if(!(condition))held();} while(0)
static int normal(void)
{
    return state.exec&&SysBase==state.exec&&state.exec->ThisTask==state.task&&
        (int)(signed char)state.exec->IDNestCnt==state.idnest&&
        (int)(signed char)state.exec->TDNestCnt==state.tdnest;
}
static int memory_class(const void *p,size_t bytes,ULONG flags)
{
    uintptr_t a=(uintptr_t)p;
    return p&&bytes&&bytes<=UINTPTR_MAX-a&&
        (TypeOfMem((APTR)p)&(MEMF_FAST|MEMF_CHIP))==flags&&
        (TypeOfMem((APTR)(a+bytes-1))&(MEMF_FAST|MEMF_CHIP))==flags;
}
static int apart(const void *p,size_t n,const void *q,size_t m)
{
    uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return (!n||(p&&n<=UINTPTR_MAX-a))&&(!m||(q&&m<=UINTPTR_MAX-b))&&
        (!n||!m||a+n<=b||b+m<=a);
}
static int registered(const void *p,size_t bytes)
{
    unsigned i;
    for(i=0;i<PT_PRIVATE_MEMORY_ALLOCATIONS;++i)
        if(memory.allocation[i].pointer==p)return memory.allocation[i].bytes==bytes&&bytes!=0;
    return 0;
}
static void *fast_allocate(void *context,size_t bytes)
{
    void *p;
    REQUIRE(context==&memory&&normal()&&!state.refusal.calls);
    p=pt_private_native_allocate(&memory,bytes);
    REQUIRE(normal()&&!memory.failed&&!state.refusal.calls);
    return p;
}
static void fast_release(void *context,void *p)
{
    REQUIRE(context==&memory&&normal()&&!state.refusal.calls);
    REQUIRE(pt_private_native_release_checked(&memory,p)==PT_PRIVATE_MEMORY_OK);
    REQUIRE(normal()&&!memory.failed&&!state.refusal.calls);
}
static void *control_allocate(void *context,size_t bytes)
{
    struct control_allocator *c=context;void *p;
    REQUIRE(c==&state.control&&normal()&&!c->queue&&!c->bytes&&!c->allocated&&!c->released);
    p=fast_allocate(&memory,bytes);if(!p)return NULL;
    REQUIRE(registered(p,bytes)&&memory_class(p,bytes,MEMF_FAST)&&normal());
    c->queue=p;c->bytes=bytes;++c->allocated;return p;
}
static void control_release(void *context,void *p)
{
    struct control_allocator *c=context;
    REQUIRE(c==&state.control&&normal()&&p&&p==c->queue&&c->bytes&&
        c->allocated==1&&!c->released&&registered(p,c->bytes));
    /* Consume this original allocator receipt before its exact FreeMem path.
     * On any refusal the checked registry and whole process stay retained. */
    c->queue=NULL;c->bytes=0;fast_release(&memory,p);++c->released;
    REQUIRE(normal());
}
static void *chip_allocate(void *context,size_t bytes)
{
    struct chip_pool *pool=context;void *p;unsigned i,j;
    REQUIRE(pool==&state.pool&&normal()&&!state.chip_uncertain&&pool->hint<3&&bytes==PACKED);
    for(i=0;i<3&&pool->block[i].data;++i){}
    REQUIRE(i<3);
    p=pt_paula_chip_allocate(NULL,bytes);REQUIRE(normal());if(!p)return NULL;
    if(!memory_class(p,bytes,MEMF_CHIP)||((uintptr_t)p&1)||
       !apart(p,bytes,&state,sizeof(state))||!apart(p,bytes,&memory,sizeof(memory)))goto uncertain;
    for(j=0;j<PT_PRIVATE_MEMORY_ALLOCATIONS;++j)
        if(!apart(p,bytes,memory.allocation[j].pointer,memory.allocation[j].bytes))goto uncertain;
    for(j=0;j<3;++j)
        if(!apart(p,bytes,pool->block[j].data,pool->block[j].bytes))goto uncertain;
    REQUIRE(normal());pool->block[i]=(struct chip_block){p,bytes,pool->hint};
    pool->bytes+=bytes;++pool->allocated;return p;
uncertain:
    state.chip_ambiguous=p;state.chip_ambiguous_bytes=bytes;state.chip_uncertain=1;
    held();
}
static void chip_release(void *context,void *p,size_t bytes)
{
    struct chip_pool *pool=context;unsigned i;
    REQUIRE(pool==&state.pool&&normal()&&!state.chip_uncertain);
    for(i=0;i<3&&pool->block[i].data!=p;++i){}
    REQUIRE(i<3&&pool->block[i].bytes==bytes&&pool->bytes>=bytes&&memory_class(p,bytes,MEMF_CHIP));
    pool->block[i]=(struct chip_block){NULL,0,0};pool->bytes-=bytes;++pool->released;
    pt_paula_chip_release(NULL,p,bytes);REQUIRE(normal());
}
static int backend_clock(void *context,uint64_t *tick,uint32_t *frequency)
{
    (void)tick;(void)frequency;REQUIRE(context==&state.refusal);++state.refusal.calls;
    return held();
}
static int backend_submit(void *context,const struct pt_readers_event *event)
{
    (void)event;REQUIRE(context==&state.refusal);++state.refusal.calls;return held();
}
static enum pt_readers_reply backend_command(void *context,uint64_t ticket,
    struct pt_readers_command_receipt *out)
{
    (void)ticket;(void)out;REQUIRE(context==&state.refusal);++state.refusal.calls;
    return held();
}
static enum pt_readers_reply backend_reader(void *context,const struct pt_readers_domain *reference,
    struct pt_readers_reader_receipt *out)
{
    (void)reference;(void)out;REQUIRE(context==&state.refusal);++state.refusal.calls;
    return held();
}
static uint8_t signed8(int32_t value,unsigned bits)
{
    int64_t n=value;unsigned shift=bits-8;
    if(shift){int64_t half=INT64_C(1)<<(shift-1);n=n>=0?(n+half)>>shift:-(((-n)+half)>>shift);}
    if(n>127)n=127;
    if(n< -128)n=-128;
    return (uint8_t)(int8_t)n;
}
static void project_init(void)
{
    unsigned i,j;
    pt_channels_init(&state.project.channels);
    state.project.bpm=125;state.project.speed=6;state.project.mode=PT_MODE_WAVETABLE;
    state.project.order_count=state.project.pattern_count=1;state.project.sample_count=3;
    state.project.orders=&state.order;state.project.events=state.events;state.project.samples=state.samples;
    memcpy(state.project.title,"Unpublished three sources",25);
    for(i=0;i<3;++i){unsigned bits=8+8*i;
        int32_t low=-(int32_t)(UINT32_C(1)<<(bits-1)),high=-low-1;
        struct pt_sample *s=state.samples+i;
        for(j=0;j<VALUES;++j)
            state.source[i][j]=(int32_t)(((uint64_t)j*65357+17*i)%((UINT64_C(1)<<bits)-1))+low;
        state.source[i][0]=low;state.source[i][1]=high;state.source[i][2]=-1;state.source[i][3]=1;
        state.source[i][4]=i?(int32_t)(UINT32_C(1)<<(bits-9)):1;state.source[i][5]=-state.source[i][4];
        state.markers[i][0]=16;state.markers[i][1]=256;
        s->pcm=(struct pt_pcm){state.source[i],VALUES,FRAMES,48000,1,(uint8_t)bits};
        s->slices=state.markers[i];s->slice_count=2;s->volume=48;s->name[0]=(char)('A'+i);
        for(j=0;j<FRAMES;++j)state.packed[i][j]=signed8(state.source[i][j],bits);
        state.packed[i][FRAMES]=0;
    }
    memcpy(state.source_before,state.source,sizeof(state.source));
    memcpy(state.markers_before,state.markers,sizeof(state.markers));
}
static int masters_and_save_equal(void)
{
    size_t bytes,written;unsigned i,j,n;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];
    if(state.editor->sampler.generation||
       state.editor->sampler.current[0]!=state.pin[0]||state.editor->sampler.current[1]!=state.pin[1]||
       state.editor->sampler.current[2]!=state.pin[2]||memcmp(state.samples,state.saved,sizeof(state.saved))||
       memcmp(state.source,state.source_before,sizeof(state.source))||
       memcmp(state.markers,state.markers_before,sizeof(state.markers)))return 0;
    for(i=0;i<3;++i){
        if(state.samples[i].pcm.bits!=8+8*i||state.samples[i].pcm.capacity!=FRAMES||
           memcmp(state.samples[i].pcm.data,state.source[i],FRAMES*sizeof(int32_t))||
           memcmp(state.samples[i].slices,state.markers[i],2*sizeof(uint32_t)))return 0;
        n=0;if(!pt_sampler_version_spans(state.pin[i],spans,PT_SAMPLER_VERSION_SPANS,&n)||!n)return 0;
        for(j=0;j<n;++j)if(!memory_class(spans[j].data,spans[j].bytes,MEMF_FAST))return 0;
    }
    return pt_project_size(&state.project,&bytes)==PT_PROJECT_OK&&bytes==state.encoded_bytes&&
        pt_project_encode(&state.project,state.scratch,bytes,&written)==PT_PROJECT_OK&&written==bytes&&
        !memcmp(state.encoded,state.scratch,bytes)&&normal()&&!state.refusal.calls;
}
static struct pt_prepared_paula_sources_status source_status(void)
{
    struct pt_prepared_paula_sources_status out;
    REQUIRE(pt_prepared_paula_sources_get(state.owner,&out)==PT_PREPARED_PAULA_SOURCES_OK&&
        normal()&&!memory.failed&&!state.chip_uncertain&&!state.refusal.calls);
    return out;
}
static unsigned cache_pins(void)
{
    unsigned i,total=0;for(i=0;i<PT_CACHE_SLOTS;++i)total+=state.pool.bridge.cache.entry[i].pins;
    return total;
}
static struct ledger ledger_now(void)
{
    struct pt_private_memory_stats s;
    REQUIRE(pt_private_native_memory_stats(&memory,&s)==PT_PRIVATE_MEMORY_OK&&normal()&&!s.failed);
    return (struct ledger){s.used,state.pool.bytes,s.live,s.allocations,s.releases,
        state.pool.allocated,state.pool.released,state.refusal.calls};
}
static void ledger_equal(struct ledger a,struct ledger b)
{
    REQUIRE(a.fast_bytes==b.fast_bytes&&a.chip_bytes==b.chip_bytes&&a.live==b.live&&
        a.allocated==b.allocated&&a.released==b.released&&a.chip_allocated==b.chip_allocated&&
        a.chip_released==b.chip_released&&a.backend_calls==b.backend_calls);
}
static void tails_equal(void)
{
    size_t i;
    if(state.barrier)for(i=pt_prepared_paula_mutation_barrier_size();i<state.barrier_capacity;++i)
        REQUIRE(((uint8_t *)state.barrier)[i]==0xa7);
    if(state.owner)for(i=pt_prepared_paula_sources_size();i<state.owner_capacity;++i)
        REQUIRE(((uint8_t *)state.owner)[i]==0xa7);
    if(state.lifetime)for(i=pt_prepared_paula_lifetime_size();i<state.lifetime_capacity;++i)
        REQUIRE(((uint8_t *)state.lifetime)[i]==0xa7);
    for(i=0;i<TAIL;++i)REQUIRE(state.context_tail[i]==0xa7);
}
static void ready_resources(void)
{
    struct pt_prepared_paula_sources_status value=source_status();
    struct chip_pool *p=&state.pool;unsigned i,j,found=0,seen=0;
    REQUIRE(value.phase==PT_PREPARED_PAULA_SOURCES_PREPARED&&value.failure==PT_PREPARED_PAULA_SOURCES_OK&&
        value.completed==3&&value.master_pins==3&&value.chip_leases==3&&!value.master_job&&!value.chip_job&&
        p->bytes==3U*PACKED&&p->bridge.cache.bytes==3U*PACKED&&cache_pins()==3&&
        !state.editor->sampler.pending_count);
    for(i=0;i<3;++i){const struct chip_block *b=p->block+i;
        REQUIRE(b->data&&b->sample<3&&!(seen&(1U<<b->sample))&&b->bytes==PACKED&&
            memory_class(b->data,b->bytes,MEMF_CHIP)&&!((uintptr_t)b->data&1)&&
            !memcmp(b->data,state.packed[b->sample],PACKED));seen|=1U<<b->sample;
        for(j=0;j<PT_CACHE_SLOTS;++j)if(p->bridge.cache.entry[j].data==b->data){
            const struct pt_cache_entry *e=p->bridge.cache.entry+j;
            REQUIRE(e->bytes==PACKED&&e->pins==1&&e->valid==1&&e->serial);++found;break;}
    }
    REQUIRE(found==3&&seen==7&&masters_and_save_equal());tails_equal();
}
static enum pt_prepared_paula_mutation_result step(void)
{
    enum pt_prepared_paula_mutation_result r;
    REQUIRE(normal()&&!state.refusal.calls&&!memory.failed&&!state.chip_uncertain);
    r=pt_prepared_paula_mutation_barrier_step(state.barrier);
    REQUIRE(normal()&&!state.refusal.calls&&!memory.failed&&!state.chip_uncertain);
    return r;
}
static void prepare(void)
{
    unsigned i;
    /* The first actual compositor step constructs this exact original owner.
     * Only then may its public metadata be observed; no alleged owner cast. */
    REQUIRE(step()==PT_PREPARED_PAULA_MUTATION_PENDING&&
        pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_PREPARING);
    for(i=0;i<STEP_LIMIT;++i){
        struct pt_prepared_paula_sources_status before=source_status();
        enum pt_prepared_paula_mutation_result r;
        REQUIRE(before.completed<=3);if(before.completed<3)state.pool.hint=before.completed;
        r=step();++state.preparation_steps;
        if(r==PT_PREPARED_PAULA_MUTATION_READY){
            REQUIRE(pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_EMPTY);
            ready_resources();return;}
        REQUIRE(r==PT_PREPARED_PAULA_MUTATION_PENDING&&masters_and_save_equal());
        {struct pt_prepared_paula_sources_status after=source_status();
            REQUIRE(after.master_job<=1&&state.editor->sampler.pending_count==after.master_job);}
    }
    held();
}
static void refused_editor_changes(void)
{
    struct pt_editor *e=state.editor;struct ledger before=ledger_now();
    unsigned revision=e->history.revision,generation=e->sampler.generation;
    REQUIRE(!e->number_field&&!e->name_entry&&!e->workflow.transaction&&normal());
    e->panel=0;e->editing=1;e->field=0;e->octave=1;
    e->pattern=e->row=0;e->project->channels.selected=0;
    /* Byte comparison buffers only: never interpreted as owner objects,
     * substituted for originals, or used as custody/release authority. */
    memcpy(state.owner_before,state.owner,state.owner_capacity);
    memcpy(state.lifetime_before,state.lifetime,state.lifetime_capacity);
    REQUIRE(!pt_editor_prepare_change(e)&&normal());
    REQUIRE(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    REQUIRE(pt_editor_key(e,0x31,0)==PT_UI_NONE&&normal());
    REQUIRE(!strcmp(e->status,"INVALID EVENT - NO CHANGE"));
    REQUIRE(pt_editor_key(e,0x31,8)==PT_UI_NONE&&normal());
    REQUIRE(!strcmp(e->status,"UNDO CONFLICT - EDIT PRESERVED"));
    e->panel=5;e->sample=1;pt_editor_sample_all(e);
    REQUIRE(pt_editor_key(e,0x4c,0x10)==PT_UI_NONE&&normal());
    REQUIRE(!strcmp(e->status,"SAMPLE EDIT REFUSED - NO CHANGE"));
    REQUIRE(pt_editor_key(e,0x0c,9)==PT_UI_NONE&&normal());
    REQUIRE(!strcmp(e->status,"SLOT ADD REFUSED - NO CHANGE"));
    REQUIRE(!pt_editor_dispose(e)&&normal());
    REQUIRE(!strcmp(e->status,"PLAYBACK STOP UNCONFIRMED - RETRY BEFORE CHANGING PROJECT"));
    REQUIRE(e->history.revision==revision&&e->sampler.generation==generation&&
        !memcmp(state.owner_before,state.owner,state.owner_capacity)&&
        !memcmp(state.lifetime_before,state.lifetime,state.lifetime_capacity));
    ledger_equal(before,ledger_now());REQUIRE(masters_and_save_equal());tails_equal();
}
static void drain(void)
{
    static const unsigned masters[7]={3,2,2,1,1,0,0},leases[7]={2,2,1,1,0,0,0};
    unsigned i;
    for(i=0;i<7;++i){
        struct ledger before=ledger_now();struct pt_prepared_paula_sources_status s;
        REQUIRE(step()==PT_PREPARED_PAULA_MUTATION_PENDING);++state.drain_steps;
        ledger_equal(before,ledger_now());s=source_status();
        REQUIRE(s.master_pins==masters[i]&&s.chip_leases==leases[i]&&!s.master_job&&!s.chip_job&&
            s.failure==PT_PREPARED_PAULA_SOURCES_OK&&cache_pins()==leases[i]&&
            s.phase==(i==6?PT_PREPARED_PAULA_SOURCES_FINISHED:PT_PREPARED_PAULA_SOURCES_CANCELLING));
        REQUIRE(pt_prepared_paula_mutation_barrier_failure(state.barrier)==PT_PREPARED_PAULA_SOURCES_OK&&
            masters_and_save_equal());tails_equal();
    }
    REQUIRE(state.drain_steps==7);
}
static void *workspace(size_t prefix,size_t *capacity,size_t alignment)
{
    uint8_t *p;
    REQUIRE(prefix&&prefix<=SIZE_MAX-TAIL&&alignment);*capacity=prefix+TAIL;
    p=fast_allocate(&memory,*capacity);
    REQUIRE(p&&registered(p,*capacity)&&memory_class(p,*capacity,MEMF_FAST)&&
        (uintptr_t)p%alignment==0&&normal());
    memset(p,0,prefix);memset(p+prefix,0xa7,TAIL);return p;
}
int main(void)
{
    struct pt_private_memory_span guard={&state,sizeof(state)};
    struct pt_private_memory_stats stats;struct ledger before,after;
    struct pt_prepared_paula_sources_status finished;
    unsigned i;size_t written,queue_bytes,barrier_size;
    state.exec=SysBase;REQUIRE(state.exec&&(state.task=FindTask(NULL))&&state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1&&state.tdnest==-1&&normal());
    assert(sizeof(uint64_t)==8);
    assert(PT_PREPARED_PAULA_SOURCES_COUNT==3);
    /* No linker/HUNK placement assumption: these two genuine whole static
     * context arenas must actually be Fast in this selected native run. */
    REQUIRE(memory_class(&state,sizeof(state),MEMF_FAST)&&memory_class(&memory,sizeof(memory),MEMF_FAST));
    memset(state.context_tail,0xa7,TAIL);project_init();
    REQUIRE(pt_project_validate(&state.project,NULL)==PT_PROJECT_OK&&normal());
    REQUIRE(pt_private_native_memory_init(&memory,512UL*1024,&guard,1)==PT_PRIVATE_MEMORY_OK&&normal());
    state.allocator=(struct pt_allocator){&memory,fast_allocate,fast_release};
    state.editor=fast_allocate(&memory,sizeof(*state.editor));
    REQUIRE(state.editor&&registered(state.editor,sizeof(*state.editor))&&
        memory_class(state.editor,sizeof(*state.editor),MEMF_FAST));
    memset(state.editor,0,sizeof(*state.editor));
    REQUIRE(pt_editor_init(state.editor,&state.project)&&normal());
    REQUIRE(!state.editor->sampler.bytes&&!state.editor->sampler.live_count&&!state.editor->sampler.pending_count&&
        !state.editor->sampler.table&&!state.editor->song.bytes&&!state.editor->song.current&&
        !state.editor->sample_source.allocated_bytes&&!state.editor->sample_source.loaded);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)REQUIRE(!state.editor->sampler.current[i]);
    /* Genuine initializer first, then ONLY still-empty owners: same pattern as
     * native editor_init_memory. Never exchange a live owner's allocator. */
    pt_sampler_init(&state.editor->sampler,&state.allocator,128UL*1024);
    pt_document_init(&state.editor->sample_source,&state.allocator);
    pt_song_init(&state.editor->song,&state.allocator,64UL*1024);
    /* Observe actual void-initializer effects, especially document_init's
     * guarded refusal: never proceed with its old libc allocator descriptor. */
    {
        const struct pt_sampler *s=&state.editor->sampler;
        const struct pt_document *d=&state.editor->sample_source;
        const struct pt_song *song=&state.editor->song;
        REQUIRE(s->allocator.context==&memory&&s->allocator.allocate==fast_allocate&&
            s->allocator.release==fast_release&&s->budget==128UL*1024&&!s->bytes&&!s->generation&&
            !s->table&&!s->table_original&&!s->table_bytes&&!s->live_head&&!s->live_count&&
            !s->pending_head&&!s->pending_count&&!s->lifetime_fault&&!s->progress&&!s->progress_context);
        for(i=0;i<PT_PROJECT_SAMPLES;++i)REQUIRE(!s->current[i]);
        REQUIRE(d->allocator.context==&memory&&d->allocator.allocate==fast_allocate&&
            d->allocator.release==fast_release&&!d->loaded&&!d->dirty&&!d->allocated_bytes&&
            !d->storage.orders&&!d->storage.order_capacity&&!d->storage.events&&!d->storage.event_capacity&&
            !d->storage.samples&&!d->storage.sample_capacity&&!d->storage.pcm&&!d->storage.pcm_capacity&&
            !d->storage.slices&&!d->storage.slice_capacity&&!d->storage.extensions&&!d->storage.extension_capacity&&
            !d->storage.extension_data&&!d->storage.extension_bytes);
        REQUIRE(song->allocator.context==&memory&&song->allocator.allocate==fast_allocate&&
            song->allocator.release==fast_release&&song->budget==64UL*1024&&
            !song->current&&!song->bytes&&!song->generation);
        REQUIRE(normal()&&!memory.failed&&!state.refusal.calls&&!state.chip_uncertain);
    }
    for(i=0;i<3;++i){
        REQUIRE(pt_sampler_pin(&state.editor->sampler,&state.project,i,0,state.pcm+i,state.pin+i)==PT_EDIT_OK);
        REQUIRE(normal()&&!memory.failed&&state.pin[i]&&state.pin[i]==state.editor->sampler.current[i]);
        if(i)REQUIRE(state.pin[i]!=state.pin[i-1]&&state.pcm[i].data!=state.pcm[i-1].data);
    }
    memcpy(state.saved,state.samples,sizeof(state.saved));
    REQUIRE(pt_project_size(&state.project,&state.encoded_bytes)==PT_PROJECT_OK);
    state.encoded=fast_allocate(&memory,state.encoded_bytes);state.scratch=fast_allocate(&memory,state.encoded_bytes);
    REQUIRE(state.encoded&&state.scratch&&pt_project_encode(&state.project,state.encoded,state.encoded_bytes,&written)==PT_PROJECT_OK&&written==state.encoded_bytes);
    state.barrier=workspace(pt_prepared_paula_mutation_barrier_size(),&state.barrier_capacity,
        pt_prepared_paula_mutation_barrier_alignment());
    state.owner=workspace(pt_prepared_paula_sources_size(),&state.owner_capacity,pt_prepared_paula_sources_alignment());
    state.lifetime=workspace(pt_prepared_paula_lifetime_size(),&state.lifetime_capacity,pt_prepared_paula_lifetime_alignment());
    state.owner_before=fast_allocate(&memory,state.owner_capacity);state.lifetime_before=fast_allocate(&memory,state.lifetime_capacity);
    REQUIRE(state.owner_before&&state.lifetime_before&&normal());
    REQUIRE(pt_sampler_paula_bind(&state.pool.bridge,&state.editor->sampler,&state.project,&state.pool,
        chip_allocate,chip_release,3U*PACKED)&&normal());
    state.config.editor=state.editor;state.config.bridge=&state.pool.bridge;state.config.sources=state.owner;
    state.config.source_capacity=state.owner_capacity;state.config.lifetime=state.lifetime;
    state.config.lifetime_capacity=state.lifetime_capacity;state.config.track=0;
    for(i=0;i<3;++i)state.config.sample[i]=i;
    state.config.queue.allocator=(struct pt_allocator){&state.control,control_allocate,control_release};
    state.config.queue.grid=(struct pt_scheduled_grid){100,7,1000000,48000};
    state.config.queue.session=UINT64_C(0x50544d5554410001);state.config.queue.frame=100;
    state.config.queue.control_budget=512UL*1024;state.config.queue.slot=0;
    state.config.queue.period=124;state.config.queue.volume=48;state.config.queue.context_count=2;
    state.config.queue.context[0]=(struct pt_prepared_paula_context_extent){&state,sizeof(state)};
    state.config.queue.context[1]=(struct pt_prepared_paula_context_extent){&memory,sizeof(memory)};
    state.backend=(struct pt_readers_backend){&state.refusal,sizeof(state.refusal),
        {PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        backend_clock,backend_submit,backend_command,backend_command,backend_reader,backend_reader};
    state.config.backend=state.backend;
    REQUIRE(masters_and_save_equal());before=ledger_now();
    barrier_size=pt_prepared_paula_mutation_barrier_size();
    REQUIRE(barrier_size>1&&pt_prepared_paula_mutation_barrier_begin(state.barrier,barrier_size-1,&state.config)==PT_PREPARED_PAULA_MUTATION_INVALID&&normal());
    for(i=0;i<barrier_size;++i)REQUIRE(!((uint8_t *)state.barrier)[i]);
    REQUIRE(!state.editor->before_change&&!state.editor->change_ready&&!state.editor->before_change_context);
    ++state.admission_refusals;ledger_equal(before,ledger_now());tails_equal();
    REQUIRE(pt_prepared_paula_mutation_barrier_begin(state.barrier,state.barrier_capacity,&state.config)==PT_PREPARED_PAULA_MUTATION_OK&&normal());
    ledger_equal(before,ledger_now());
    REQUIRE(!state.editor->before_change&&state.editor->change_ready&&state.editor->before_change_context==state.barrier&&
        pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_ATTACHED);
    prepare();
    REQUIRE(state.control.allocated==1&&!state.control.released&&state.control.queue&&state.control.bytes&&
        registered(state.control.queue,state.control.bytes)&&memory_class(state.control.queue,state.control.bytes,MEMF_FAST));
    REQUIRE(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK&&normal());
    before=ledger_now();
    REQUIRE(pt_prepared_paula_mutation_barrier_enqueue_once(state.barrier)==PT_PREPARED_PAULA_MUTATION_QUEUED&&normal());
    ledger_equal(before,ledger_now());ready_resources();
    REQUIRE(pt_prepared_paula_mutation_barrier_enqueue_once(state.barrier)==PT_PREPARED_PAULA_MUTATION_BUSY&&normal());
    ledger_equal(before,ledger_now());ready_resources();
    refused_editor_changes();
    /* Cancellation was only latched on the actual editor stacks. Only this
     * separate later normal-TASK call performs the original local C->R stop. */
    before=ledger_now();REQUIRE(step()==PT_PREPARED_PAULA_MUTATION_PENDING&&
        pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_STOPPED);
    ledger_equal(before,ledger_now());ready_resources();drain();
    before=ledger_now();queue_bytes=state.control.bytes;
    REQUIRE(step()==PT_PREPARED_PAULA_MUTATION_PENDING);after=ledger_now();
    REQUIRE(state.control.allocated==1&&state.control.released==1&&!state.control.queue&&!state.control.bytes&&
        before.live==after.live+1&&before.fast_bytes==after.fast_bytes+queue_bytes&&
        after.released==before.released+1&&after.allocated==before.allocated&&
        after.chip_bytes==before.chip_bytes&&after.chip_allocated==before.chip_allocated&&
        after.chip_released==before.chip_released&&!after.backend_calls);
    before=ledger_now();REQUIRE(step()==PT_PREPARED_PAULA_MUTATION_CLEANED&&
        pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_DETACHABLE);
    after=ledger_now();
    REQUIRE(before.fast_bytes==after.fast_bytes&&before.live==after.live&&before.allocated==after.allocated&&
        before.released==after.released&&before.chip_bytes==3U*PACKED&&!after.chip_bytes&&
        after.chip_released==before.chip_released+3&&after.chip_allocated==before.chip_allocated&&
        state.pool.allocated==3&&state.pool.released==3&&!cache_pins()&&!state.pool.bridge.sampler&&!state.pool.bridge.project);
    for(i=0;i<PT_CACHE_SLOTS;++i)REQUIRE(!state.pool.bridge.cache.entry[i].data&&!state.pool.bridge.cache.entry[i].bytes&&
        !state.pool.bridge.cache.entry[i].pins&&!state.pool.bridge.cache.entry[i].valid);
    refused_editor_changes();before=ledger_now();
    REQUIRE(pt_prepared_paula_mutation_barrier_detach(state.barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED&&normal());
    ledger_equal(before,ledger_now());
    REQUIRE(!state.editor->before_change&&!state.editor->change_ready&&!state.editor->before_change_context&&
        pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_FINISHED&&
        pt_prepared_paula_mutation_barrier_failure(state.barrier)==PT_PREPARED_PAULA_SOURCES_OK);
    REQUIRE(masters_and_save_equal());tails_equal();
    /* Original constructor descriptors are no longer borrowed after the
     * positive terminal detach. Remove their obsolete child/editor pointers. */
    memset(&state.config,0,sizeof(state.config));memset(&state.backend,0,sizeof(state.backend));
    fast_release(&memory,state.lifetime);state.lifetime=NULL;
    before=ledger_now();memcpy(state.owner_before,state.owner,state.owner_capacity);finished=source_status();
    REQUIRE(finished.phase==PT_PREPARED_PAULA_SOURCES_FINISHED&&!finished.master_pins&&!finished.chip_leases&&
        !finished.master_job&&!finished.chip_job&&pt_prepared_paula_sources_cancel_step(state.owner)==PT_PREPARED_PAULA_SOURCES_CLOSED&&
        !memcmp(state.owner_before,state.owner,state.owner_capacity));
    ledger_equal(before,ledger_now());fast_release(&memory,state.owner);state.owner=NULL;
    /* Own-only CLOSED observations after actual native child free; not the
     * HOST inaccessible-page oracle and never hardware/source quiet proof. */
    before=ledger_now();
    REQUIRE(pt_prepared_paula_mutation_barrier_state(state.barrier)==PT_PREPARED_PAULA_MUTATION_FINISHED&&
        pt_prepared_paula_mutation_barrier_failure(state.barrier)==PT_PREPARED_PAULA_SOURCES_OK&&
        pt_prepared_paula_mutation_barrier_detach(state.barrier)==PT_PREPARED_PAULA_MUTATION_CLOSED&&normal());
    ledger_equal(before,ledger_now());
    REQUIRE(masters_and_save_equal());fast_release(&memory,state.barrier);state.barrier=NULL;
    /* Deliberate post-detach edit only AFTER preserved master/project-save
     * gold. It is an independent no-stale-hook check, not forced cleanup. */
    state.editor->panel=0;state.editor->editing=1;state.editor->field=0;state.editor->octave=1;
    state.editor->pattern=state.editor->row=0;
    REQUIRE(pt_editor_prepare_change(state.editor)&&normal());
    {unsigned revision=state.editor->history.revision;before=ledger_now();
        REQUIRE(pt_editor_key(state.editor,0x31,0)==PT_UI_NONE&&normal());
        REQUIRE(state.editor->history.revision!=revision&&state.events[0].kind==PT_NOTE_PERIOD&&
            !strcmp(state.editor->status,"PATTERN EDITED"));ledger_equal(before,ledger_now());}
    state.post_detach_edit=1;
    REQUIRE(!memcmp(state.samples,state.saved,sizeof(state.saved))&&
        !memcmp(state.source,state.source_before,sizeof(state.source))&&
        !memcmp(state.markers,state.markers_before,sizeof(state.markers)));
    for(i=0;i<3;++i)REQUIRE(!memcmp(state.samples[i].pcm.data,state.source[i],FRAMES*sizeof(int32_t)));
    REQUIRE(pt_editor_dispose(state.editor)&&normal());
    /* Real versions still reference editor.sampler while external pins remain.
     * Keep editor alive until ALL original pins and sampler ledgers ended. */
    for(i=0;i<3;++i){pt_sampler_unpin(state.pin[i]);state.pin[i]=NULL;REQUIRE(normal());}
    REQUIRE(!state.editor->sampler.bytes&&!state.editor->sampler.live_count&&!state.editor->sampler.pending_count&&
        !state.editor->sampler.lifetime_fault&&!state.editor->song.bytes&&!state.editor->song.current&&
        !state.editor->sample_source.allocated_bytes&&!memory.failed&&normal());
    memset(state.samples,0,sizeof(state.samples));memset(state.saved,0,sizeof(state.saved));
    memset(state.pcm,0,sizeof(state.pcm));memset(&state.project,0,sizeof(state.project));
    fast_release(&memory,state.editor);state.editor=NULL;
    fast_release(&memory,state.encoded);state.encoded=NULL;fast_release(&memory,state.scratch);state.scratch=NULL;
    fast_release(&memory,state.owner_before);state.owner_before=NULL;
    fast_release(&memory,state.lifetime_before);state.lifetime_before=NULL;
    REQUIRE(!state.control.queue&&!state.control.bytes&&state.control.allocated==state.control.released&&
        !state.pool.bytes&&state.pool.allocated==state.pool.released&&
        !state.chip_uncertain&&!state.chip_ambiguous&&!state.chip_ambiguous_bytes&&!state.refusal.calls&&
        state.admission_refusals==1&&state.post_detach_edit==1);
    for(i=0;i<3;++i)REQUIRE(!state.pool.block[i].data&&!state.pool.block[i].bytes);
    tails_equal();
    REQUIRE(pt_private_native_memory_finish(&memory)==PT_PRIVATE_MEMORY_OK&&
        pt_private_native_memory_stats(&memory,&stats)==PT_PRIVATE_MEMORY_OK&&
        !stats.live&&!stats.used&&!stats.failed&&stats.allocations==stats.releases&&normal());
    puts(PASS_MARKER);return 0;
}
