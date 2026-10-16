/* Private ordinary-TASK native RAM component only. No publication, backend
 * execution, receipt, output, IRQ, timer, filesystem or hardware-quiet claim. */
#if !defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY) || PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY != 1
#error "This RAM probe requires its exact native TASK probe-only opt-in"
#endif
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
#error "The native TASK RAM probe and HOST lifetime opt-ins are exclusive"
#endif
#include "../editor/prepared_paula_lifetime.h"
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

#define PASS_MARKER "PREPARED PAULA LIFETIME RAM PASS:mono8/16/24 Fast masters;3 Chip caches;genuine unpublished1C/1R local stop;7 deferred drain steps;queue freed;master/save preserved;zero ledgers;NO OUTPUT AUTHORITY"
enum {FRAMES=513,VALUES=521,PACKED=514,TAIL=32,STEP_LIMIT=64};
typedef char probe_fixed_abi[(CHAR_BIT==8&&sizeof(void *)==4&&sizeof(uintptr_t)==4&&
    sizeof(size_t)==4&&sizeof(uint32_t)==4&&sizeof(uint64_t)==8)?1:-1];
struct chip_block {void *data;size_t bytes;unsigned sample;};
struct chip_pool {
    struct pt_sampler_paula bridge;
    struct chip_block block[3];
    size_t bytes;
    unsigned allocated,released,hint,bound;
};
struct control_allocator {void *queue;size_t bytes;unsigned allocated,released;};
struct refusal_backend {unsigned calls;};
struct probe_state {
    struct ExecBase *exec;struct Task *task;int idnest,tdnest;
    struct pt_project project;
    struct pt_sample samples[3],saved[3];
    struct pt_event events[4*PT_PROJECT_ROWS];uint16_t order;
    struct pt_sampler sampler;struct pt_allocator allocator;
    struct pt_sample_version *pin[3];struct pt_pcm pcm[3];
    int32_t source[3][VALUES],source_before[3][VALUES];
    uint32_t markers[3][2],markers_before[3][2];
    uint8_t packed[3][PACKED];
    struct chip_pool pool;
    struct pt_prepared_paula_sources *owner;size_t owner_capacity;
    struct pt_prepared_paula_lifetime *lifetime;size_t lifetime_capacity;
    struct pt_prepared_paula_lifetime_config config;
    struct pt_readers_backend backend;
    struct control_allocator control;struct refusal_backend refusal;
    uint8_t *encoded,*scratch,*owner_before,*lifetime_before;size_t encoded_bytes;
    void *chip_ambiguous;size_t chip_ambiguous_bytes;
    unsigned chip_uncertain,sampler_live,preparation_steps,drain_steps;
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
    if(state.sampler.generation||
       state.sampler.current[0]!=state.pin[0]||state.sampler.current[1]!=state.pin[1]||
       state.sampler.current[2]!=state.pin[2]||memcmp(state.samples,state.saved,sizeof(state.saved))||
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
    size_t i,owner=pt_prepared_paula_sources_size(),life=pt_prepared_paula_lifetime_size();
    for(i=owner;i<state.owner_capacity;++i)REQUIRE(((uint8_t *)state.owner)[i]==0xa7);
    if(state.lifetime)for(i=life;i<state.lifetime_capacity;++i)REQUIRE(((uint8_t *)state.lifetime)[i]==0xa7);
}
static void ready_resources(void)
{
    struct pt_prepared_paula_sources_status value=source_status();
    struct chip_pool *p=&state.pool;unsigned i,j,found=0,seen=0;
    REQUIRE(value.phase==PT_PREPARED_PAULA_SOURCES_PREPARED&&value.failure==PT_PREPARED_PAULA_SOURCES_OK&&
        value.completed==3&&value.master_pins==3&&value.chip_leases==3&&!value.master_job&&!value.chip_job&&
        p->bytes==3U*PACKED&&p->bridge.cache.bytes==3U*PACKED&&cache_pins()==3&&
        !state.sampler.pending_count);
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
static void prepare(void)
{
    unsigned i;
    for(i=0;i<STEP_LIMIT;++i){
        struct pt_prepared_paula_sources_status before=source_status();
        enum pt_prepared_paula_sources_result result;
        REQUIRE(before.completed<3);state.pool.hint=before.completed;
        result=pt_prepared_paula_sources_step(state.owner);++state.preparation_steps;
        REQUIRE(normal()&&!memory.failed&&!state.chip_uncertain&&!state.refusal.calls);
        if(result==PT_PREPARED_PAULA_SOURCES_READY){ready_resources();return;}
        REQUIRE(result==PT_PREPARED_PAULA_SOURCES_PENDING&&masters_and_save_equal());
        {struct pt_prepared_paula_sources_status after=source_status();
            REQUIRE(after.master_job<=1&&state.sampler.pending_count==after.master_job);}
    }
    held();
}
static void refusal(unsigned operation)
{
    struct ledger before=ledger_now();enum pt_prepared_paula_sources_result result;
    /* Byte audit only: these buffers are never interpreted as owner objects. */
    memcpy(state.owner_before,state.owner,state.owner_capacity);
    memcpy(state.lifetime_before,state.lifetime,state.lifetime_capacity);
    if(operation==0)result=pt_prepared_paula_sources_cancel_step(state.owner);
    else if(operation==1)result=pt_prepared_paula_lifetime_enqueue_unpublished(state.lifetime);
    else if(operation==2)result=pt_prepared_paula_lifetime_drain_step(state.lifetime);
    else {REQUIRE(operation==3);result=pt_prepared_paula_lifetime_stop_local(state.lifetime);}
    REQUIRE(result==((operation==0||operation==2)?PT_PREPARED_PAULA_SOURCES_BUSY:
        PT_PREPARED_PAULA_SOURCES_INVALID));
    REQUIRE(!memcmp(state.owner_before,state.owner,state.owner_capacity)&&
        !memcmp(state.lifetime_before,state.lifetime,state.lifetime_capacity));
    ledger_equal(before,ledger_now());ready_resources();
}
static void drain(void)
{
    static const unsigned masters[7]={3,2,2,1,1,0,0},leases[7]={2,2,1,1,0,0,0};
    unsigned i;
    for(i=0;i<7;++i){
        struct ledger before=ledger_now();struct pt_prepared_paula_sources_status s;
        enum pt_prepared_paula_sources_result result=pt_prepared_paula_lifetime_drain_step(state.lifetime);
        ++state.drain_steps;
        REQUIRE(result==(i==6?PT_PREPARED_PAULA_SOURCES_CLOSED:PT_PREPARED_PAULA_SOURCES_PENDING));
        ledger_equal(before,ledger_now());s=source_status();
        REQUIRE(s.master_pins==masters[i]&&s.chip_leases==leases[i]&&!s.master_job&&!s.chip_job&&
            s.failure==PT_PREPARED_PAULA_SOURCES_OK&&cache_pins()==leases[i]&&
            s.phase==(i==6?PT_PREPARED_PAULA_SOURCES_FINISHED:PT_PREPARED_PAULA_SOURCES_CANCELLING));
        REQUIRE(pt_prepared_paula_lifetime_failure(state.lifetime)==PT_SCHEDULED_OK&&masters_and_save_equal());
        tails_equal();
    }
    REQUIRE(state.drain_steps==7);
}
int main(void)
{
    struct pt_private_memory_span guard={&state,sizeof(state)};
    struct pt_private_memory_stats stats;struct ledger before,after;
    struct pt_prepared_paula_sources_status finished;
    unsigned i,samples[3]={0,1,2};size_t owner_size,life_size,written,queue_bytes;
    state.exec=SysBase;REQUIRE(state.exec&&(state.task=FindTask(NULL))&&state.exec->ThisTask==state.task);
    state.idnest=(int)(signed char)state.exec->IDNestCnt;state.tdnest=(int)(signed char)state.exec->TDNestCnt;
    REQUIRE(state.idnest==-1&&state.tdnest==-1&&normal());
    assert(sizeof(uint64_t)==8);
    assert(PT_PREPARED_PAULA_SOURCES_COUNT==3);
    owner_size=pt_prepared_paula_sources_size();life_size=pt_prepared_paula_lifetime_size();
    REQUIRE(owner_size&&life_size&&owner_size<=SIZE_MAX-TAIL&&life_size<=SIZE_MAX-TAIL&&
        pt_prepared_paula_sources_alignment()&&pt_prepared_paula_lifetime_alignment());
    state.owner_capacity=owner_size+TAIL;state.lifetime_capacity=life_size+TAIL;
    project_init();REQUIRE(pt_project_validate(&state.project,NULL)==PT_PROJECT_OK);
    REQUIRE(pt_private_native_memory_init(&memory,512UL*1024,&guard,1)==PT_PRIVATE_MEMORY_OK&&normal());
    state.allocator=(struct pt_allocator){&memory,fast_allocate,fast_release};
    pt_sampler_init(&state.sampler,&state.allocator,128UL*1024);state.sampler_live=1;
    for(i=0;i<3;++i){
        REQUIRE(pt_sampler_pin(&state.sampler,&state.project,i,0,state.pcm+i,state.pin+i)==PT_EDIT_OK);
        REQUIRE(normal()&&!memory.failed&&state.pin[i]&&state.pin[i]==state.sampler.current[i]);
    }
    memcpy(state.saved,state.samples,sizeof(state.saved));
    REQUIRE(pt_project_size(&state.project,&state.encoded_bytes)==PT_PROJECT_OK);
    state.encoded=fast_allocate(&memory,state.encoded_bytes);state.scratch=fast_allocate(&memory,state.encoded_bytes);
    REQUIRE(state.encoded&&state.scratch&&pt_project_encode(&state.project,state.encoded,state.encoded_bytes,&written)==PT_PROJECT_OK&&written==state.encoded_bytes);
    state.owner=fast_allocate(&memory,state.owner_capacity);state.lifetime=fast_allocate(&memory,state.lifetime_capacity);
    state.owner_before=fast_allocate(&memory,state.owner_capacity);state.lifetime_before=fast_allocate(&memory,state.lifetime_capacity);
    REQUIRE(state.owner&&state.lifetime&&state.owner_before&&state.lifetime_before&&
        memory_class(state.owner,state.owner_capacity,MEMF_FAST)&&memory_class(state.lifetime,state.lifetime_capacity,MEMF_FAST)&&
        (uintptr_t)state.owner%pt_prepared_paula_sources_alignment()==0&&
        (uintptr_t)state.lifetime%pt_prepared_paula_lifetime_alignment()==0&&normal());
    memset(state.owner,0,owner_size);memset((uint8_t *)state.owner+owner_size,0xa7,TAIL);
    memset(state.lifetime,0,life_size);memset((uint8_t *)state.lifetime+life_size,0xa7,TAIL);
    REQUIRE(pt_sampler_paula_bind(&state.pool.bridge,&state.sampler,&state.project,&state.pool,
        chip_allocate,chip_release,3U*PACKED));state.pool.bound=1;
    REQUIRE(pt_prepared_paula_sources_init(state.owner,state.owner_capacity,&state.pool.bridge,0,samples)==PT_PREPARED_PAULA_SOURCES_OK);
    prepare();
    state.config.allocator=(struct pt_allocator){&state.control,control_allocate,control_release};
    state.config.grid=(struct pt_scheduled_grid){100,7,1000000,48000};
    state.config.session=UINT64_C(0x50544c4946450001);state.config.frame=100;
    state.config.control_budget=512UL*1024;state.config.slot=0;state.config.period=124;state.config.volume=48;
    state.config.context_count=2;
    state.config.context[0]=(struct pt_prepared_paula_context_extent){&state,sizeof(state)};
    state.config.context[1]=(struct pt_prepared_paula_context_extent){&memory,sizeof(memory)};
    /* Constructor-required declarations only. Every callback refuses by HOLD;
     * none is ever an implementation/qualification of clock or output. */
    state.backend=(struct pt_readers_backend){&state.refusal,sizeof(state.refusal),
        {PT_SCHEDULED_REQUIRED,1,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
        backend_clock,backend_submit,backend_command,backend_command,backend_reader,backend_reader};
    REQUIRE(pt_prepared_paula_lifetime_init(state.lifetime,state.lifetime_capacity,state.owner,
        &state.config,&state.backend)==PT_PREPARED_PAULA_SOURCES_OK&&normal());
    REQUIRE(state.control.allocated==1&&!state.control.released&&state.control.queue&&state.control.bytes&&
        registered(state.control.queue,state.control.bytes)&&memory_class(state.control.queue,state.control.bytes,MEMF_FAST));
    ready_resources();REQUIRE(pt_prepared_paula_lifetime_failure(state.lifetime)==PT_SCHEDULED_OK);
    REQUIRE(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK&&normal());
    before=ledger_now();
    REQUIRE(pt_prepared_paula_lifetime_enqueue_unpublished(state.lifetime)==PT_PREPARED_PAULA_SOURCES_OK&&normal());
    ledger_equal(before,ledger_now());ready_resources();refusal(0);refusal(1);refusal(2);
    before=ledger_now();
    REQUIRE(pt_prepared_paula_lifetime_stop_local(state.lifetime)==PT_PREPARED_PAULA_SOURCES_OK&&normal());
    ledger_equal(before,ledger_now());ready_resources();refusal(0);refusal(3);
    /* Actual core C-then-R callbacks have returned. Later top-level calls alone
     * dispose the captured three leases/three pins; no supplied receipt/quiet. */
    drain();
    before=ledger_now();queue_bytes=state.control.bytes;
    REQUIRE(pt_prepared_paula_lifetime_close(state.lifetime)==PT_PREPARED_PAULA_SOURCES_CLOSED&&normal());
    after=ledger_now();
    REQUIRE(state.control.allocated==1&&state.control.released==1&&!state.control.queue&&!state.control.bytes&&
        before.live==after.live+1&&before.fast_bytes==after.fast_bytes+queue_bytes&&
        after.released==before.released+1&&after.allocated==before.allocated&&
        after.chip_bytes==before.chip_bytes&&after.chip_allocated==before.chip_allocated&&
        after.chip_released==before.chip_released&&!after.backend_calls);
    REQUIRE(pt_prepared_paula_lifetime_failure(state.lifetime)==PT_SCHEDULED_OK);tails_equal();
    fast_release(&memory,state.lifetime);state.lifetime=NULL;
    /* Public CLOSED owner observations must not follow the now-freed lifetime.
     * This native free is not the HOST protected-page inaccessible oracle. */
    before=ledger_now();memcpy(state.owner_before,state.owner,state.owner_capacity);
    finished=source_status();
    REQUIRE(finished.phase==PT_PREPARED_PAULA_SOURCES_FINISHED&&!finished.master_pins&&!finished.chip_leases&&
        !finished.master_job&&!finished.chip_job&&pt_prepared_paula_sources_cancel_step(state.owner)==PT_PREPARED_PAULA_SOURCES_CLOSED&&
        !memcmp(state.owner_before,state.owner,state.owner_capacity));
    ledger_equal(before,ledger_now());REQUIRE(masters_and_save_equal());tails_equal();
    REQUIRE(!cache_pins()&&pt_sampler_paula_close(&state.pool.bridge));state.pool.bound=0;
    REQUIRE(!state.pool.bytes&&state.pool.allocated==3&&state.pool.released==3&&normal());
    for(i=0;i<3;++i){pt_sampler_unpin(state.pin[i]);state.pin[i]=NULL;REQUIRE(normal());}
    pt_sampler_release(&state.sampler);state.sampler_live=0;
    REQUIRE(normal()&&!memory.failed&&!state.sampler.bytes&&!state.sampler.live_count&&
        !state.sampler.pending_count&&!state.sampler.lifetime_fault);
    memset(state.samples,0,sizeof(state.samples));memset(state.saved,0,sizeof(state.saved));
    memset(state.pcm,0,sizeof(state.pcm));memset(&state.project,0,sizeof(state.project));
    fast_release(&memory,state.encoded);state.encoded=NULL;fast_release(&memory,state.scratch);state.scratch=NULL;
    fast_release(&memory,state.owner);state.owner=NULL;
    fast_release(&memory,state.owner_before);state.owner_before=NULL;
    fast_release(&memory,state.lifetime_before);state.lifetime_before=NULL;
    REQUIRE(!state.control.queue&&!state.control.bytes&&state.control.allocated==state.control.released&&
        !state.pool.bound&&!state.pool.bytes&&state.pool.allocated==state.pool.released&&
        !state.chip_uncertain&&!state.chip_ambiguous&&!state.chip_ambiguous_bytes&&!state.refusal.calls);
    for(i=0;i<3;++i)REQUIRE(!state.pool.block[i].data&&!state.pool.block[i].bytes);
    REQUIRE(pt_private_native_memory_finish(&memory)==PT_PRIVATE_MEMORY_OK&&
        pt_private_native_memory_stats(&memory,&stats)==PT_PRIVATE_MEMORY_OK&&
        !stats.live&&!stats.used&&!stats.failed&&stats.allocations==stats.releases&&normal());
    puts(PASS_MARKER);return 0;
}
