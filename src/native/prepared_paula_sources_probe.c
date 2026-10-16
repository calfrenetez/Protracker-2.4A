/* Real normal-TASK RAM probe only: no output/IRQ/gate/device or filesystem.
 * This native subset is not the full HOST25-cut/36-stale/fault matrix. */
#include "../editor/prepared_paula_sources.h"
#include "../editor/sampler_internal.h"
#include "../editor/sampler_paula_internal.h"
#include "native_checked_memory.h"
#include "paula_memory.h"
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PASS_MARKER "PREPARED PAULA SOURCES RAM PASS:mono8/16/24 Fast masters;3 selective Chip caches;cold/HIT/cancel5;exact master/save preservation;zero accounting;NO OUTPUT AUTHORITY"
enum {FRAMES=513,VALUES=521,PACKED=514,OWNERS=7,POOLS=6,TAIL=32,STEP_LIMIT=64};
struct chip_block {void *data;size_t bytes;unsigned sample;};
struct chip_pool {
    struct pt_sampler_paula bridge;
    struct chip_block block[3];
    size_t bytes;
    unsigned allocated,released,hint,bound;
};
struct probe_state {
    struct pt_project project;
    struct pt_sample samples[3],saved[3];
    struct pt_event events[4*PT_PROJECT_ROWS];uint16_t order;
    struct pt_sampler sampler;struct pt_allocator allocator;
    struct pt_sample_version *pin[3];struct pt_pcm pcm[3];
    int32_t source[3][VALUES],source_before[3][VALUES];
    uint32_t markers[3][2],markers_before[3][2];
    uint8_t packed[3][PACKED];
    struct chip_pool pool[POOLS];
    struct pt_prepared_paula_sources *owner[OWNERS];
    unsigned initialized[OWNERS];size_t owner_capacity;
    uint8_t *encoded,*scratch;size_t encoded_bytes;
    void *chip_ambiguous;size_t chip_ambiguous_bytes;
    unsigned sampler_live,chip_uncertain;
};
static struct probe_state state;
static struct pt_private_native_memory memory;
static struct ExecBase *original_exec;
static struct Task *original_task;
static int original_idnest,original_tdnest;

static int held(void) __attribute__((noreturn));
static int held(void)
{
    /* Retain every surviving RAM/context/HUNK obligation on uncertainty.
     * No corrective cleanup loop, source reset, Wait or implicit Exit. */
    for(;;)__asm__ volatile ("" ::: "memory");
    __builtin_unreachable();
}
static int normal(void)
{
    return SysBase==original_exec&&original_exec->ThisTask==original_task&&
        (int)(signed char)original_exec->IDNestCnt==original_idnest&&
        (int)(signed char)original_exec->TDNestCnt==original_tdnest;
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
static void *chip_allocate(void *context,size_t bytes)
{
    struct chip_pool *pool=context;void *p;unsigned i,j,k;
    if(state.chip_uncertain||pool->hint>=3||bytes!=PACKED)return NULL;
    for(i=0;i<3&&pool->block[i].data;++i){}
    if(i==3)return NULL;
    p=pt_paula_chip_allocate(NULL,bytes);if(!p)return NULL;
    if(!memory_class(p,bytes,MEMF_CHIP)||((uintptr_t)p&1)||
       !apart(p,bytes,&state,sizeof(state))||!apart(p,bytes,&memory,sizeof(memory)))
        goto uncertain;
    for(j=0;j<PT_PRIVATE_MEMORY_ALLOCATIONS;++j)
        if(!apart(p,bytes,memory.allocation[j].pointer,memory.allocation[j].bytes))goto uncertain;
    for(k=0;k<POOLS;++k)for(j=0;j<3;++j)
        if(!apart(p,bytes,state.pool[k].block[j].data,state.pool[k].block[j].bytes))goto uncertain;
    pool->block[i]=(struct chip_block){p,bytes,pool->hint};
    pool->bytes+=bytes;++pool->allocated;return p;
uncertain:
    /* An unclassified interval must never be freed as proved fresh storage. */
    state.chip_ambiguous=p;state.chip_ambiguous_bytes=bytes;
    state.chip_uncertain=1;return NULL;
}
static void chip_release(void *context,void *p,size_t bytes)
{
    struct chip_pool *pool=context;unsigned i;
    if(state.chip_uncertain)return;
    for(i=0;i<3&&pool->block[i].data!=p;++i){}
    if(i==3||pool->block[i].bytes!=bytes||pool->bytes<bytes||
       !memory_class(p,bytes,MEMF_CHIP)){state.chip_uncertain=1;return;}
    pool->block[i]=(struct chip_block){NULL,0,0};pool->bytes-=bytes;++pool->released;
    pt_paula_chip_release(NULL,p,bytes);
}
static uint8_t signed8(int32_t value,unsigned bits)
{
    int64_t n=value;unsigned shift=bits-8;
    if(shift){int64_t half=INT64_C(1)<<(shift-1);
        n=n>=0?(n+half)>>shift:-(((-n)+half)>>shift);}
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
    memcpy(state.project.title,"Three genuine sources",22);
    for(i=0;i<3;++i){unsigned bits=8+8*i;
        int32_t low=-(int32_t)(UINT32_C(1)<<(bits-1)),high=-low-1;
        struct pt_sample *s=state.samples+i;
        for(j=0;j<VALUES;++j)
            state.source[i][j]=(int32_t)(((uint64_t)j*65357+17*i)%((UINT64_C(1)<<bits)-1))+low;
        state.source[i][0]=low;state.source[i][1]=high;
        state.source[i][2]=-1;state.source[i][3]=1;
        state.source[i][4]=i?(int32_t)(UINT32_C(1)<<(bits-9)):1;
        state.source[i][5]=-state.source[i][4];
        state.markers[i][0]=16;state.markers[i][1]=256;
        s->pcm=(struct pt_pcm){state.source[i],VALUES,FRAMES,48000,1,(uint8_t)bits};
        s->slices=state.markers[i];s->slice_count=2;s->volume=48;
        s->name[0]=(char)('A'+i);
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
    if(state.sampler.generation||state.sampler.pending_count||
       state.sampler.current[0]!=state.pin[0]||state.sampler.current[1]!=state.pin[1]||
       state.sampler.current[2]!=state.pin[2]||
       memcmp(state.samples,state.saved,sizeof(state.saved))||
       memcmp(state.source,state.source_before,sizeof(state.source))||
       memcmp(state.markers,state.markers_before,sizeof(state.markers)))return 0;
    for(i=0;i<3;++i){
        if(state.samples[i].pcm.bits!=8+8*i||state.samples[i].pcm.capacity!=FRAMES||
           memcmp(state.samples[i].pcm.data,state.source[i],FRAMES*sizeof(int32_t))||
           memcmp(state.samples[i].slices,state.markers[i],2*sizeof(uint32_t)))return 0;
        n=0;if(!pt_sampler_version_spans(state.pin[i],spans,PT_SAMPLER_VERSION_SPANS,&n)||!n)return 0;
        for(j=0;j<n;++j)if(!memory_class(spans[j].data,spans[j].bytes,MEMF_FAST))return 0;
    }
    if(pt_project_size(&state.project,&bytes)!=PT_PROJECT_OK||bytes!=state.encoded_bytes||
       pt_project_encode(&state.project,state.scratch,bytes,&written)!=PT_PROJECT_OK||
       written!=bytes||memcmp(state.encoded,state.scratch,bytes))return 0;
    return normal();
}
static int status(unsigned owner,struct pt_prepared_paula_sources_status *out)
{
    return pt_prepared_paula_sources_get(state.owner[owner],out)==PT_PREPARED_PAULA_SOURCES_OK&&
        normal()&&!state.chip_uncertain&&!memory.failed;
}
static int owner_init(unsigned owner,unsigned pool)
{
    unsigned samples[3]={0,1,2};enum pt_prepared_paula_sources_result result;
    result=pt_prepared_paula_sources_init(state.owner[owner],state.owner_capacity,
                                        &state.pool[pool].bridge,0,samples);
    if(result==PT_PREPARED_PAULA_SOURCES_OK)state.initialized[owner]=1;
    return result==PT_PREPARED_PAULA_SOURCES_OK&&normal();
}
static int step(unsigned owner,unsigned pool)
{
    struct pt_prepared_paula_sources_status before;
    enum pt_prepared_paula_sources_result result;
    if(!status(owner,&before)||before.completed>=3)return -1;
    state.pool[pool].hint=before.completed;
    result=pt_prepared_paula_sources_step(state.owner[owner]);
    if(!normal()||state.chip_uncertain||memory.failed||result==PT_PREPARED_PAULA_SOURCES_FAULT)return held();
    if(result==PT_PREPARED_PAULA_SOURCES_READY)return 1;
    return result==PT_PREPARED_PAULA_SOURCES_PENDING?0:-1;
}
static int prepare(unsigned owner,unsigned pool)
{
    unsigned i;int result;
    for(i=0;i<STEP_LIMIT;++i){result=step(owner,pool);if(result)return result==1;}
    return 0;
}
static int ready_resources(unsigned owner,unsigned pool)
{
    struct pt_prepared_paula_sources_status value;struct chip_pool *c=state.pool+pool;
    unsigned i,j,found=0,seen=0;
    if(!status(owner,&value)||value.phase!=PT_PREPARED_PAULA_SOURCES_PREPARED||
       value.failure!=PT_PREPARED_PAULA_SOURCES_OK||value.completed!=3||
       value.master_pins!=3||value.chip_leases!=3||value.master_job||value.chip_job||
       c->bytes!=3U*PACKED||c->bridge.cache.bytes!=3U*PACKED)return 0;
    for(i=0;i<3;++i){const struct chip_block *b=c->block+i;
        if(!b->data||b->sample>=3||(seen&(1U<<b->sample))||b->bytes!=PACKED||
           !memory_class(b->data,b->bytes,MEMF_CHIP)||((uintptr_t)b->data&1)||
           memcmp(b->data,state.packed[b->sample],PACKED))return 0;
        seen|=1U<<b->sample;
        for(j=0;j<PT_CACHE_SLOTS;++j)if(c->bridge.cache.entry[j].data==b->data){
            const struct pt_cache_entry *e=c->bridge.cache.entry+j;
            if(e->bytes!=PACKED||e->pins!=1||e->valid!=1||!e->serial)return 0;
            ++found;break;
        }
    }
    return found==3&&seen==7&&masters_and_save_equal();
}
static int cancel(unsigned owner)
{
    unsigned i;enum pt_prepared_paula_sources_result result;
    struct pt_prepared_paula_sources_status value;
    for(i=0;i<STEP_LIMIT;++i){
        result=pt_prepared_paula_sources_cancel_step(state.owner[owner]);
        if(!normal()||state.chip_uncertain||memory.failed||result==PT_PREPARED_PAULA_SOURCES_FAULT)return held();
        if(result==PT_PREPARED_PAULA_SOURCES_CLOSED){
            if(!status(owner,&value)||value.phase!=PT_PREPARED_PAULA_SOURCES_FINISHED||
               value.master_pins||value.chip_leases||value.master_job||value.chip_job)return held();
            return 1;
        }
        if(result!=PT_PREPARED_PAULA_SOURCES_PENDING)return held();
    }
    return held();
}
static int pool_bind(unsigned index)
{
    struct chip_pool *p=state.pool+index;
    if(!pt_sampler_paula_bind(&p->bridge,&state.sampler,&state.project,p,
                              chip_allocate,chip_release,3U*PACKED))return 0;
    p->bound=1;return normal();
}
static int pool_close(unsigned index)
{
    struct chip_pool *p=state.pool+index;
    if(!pt_sampler_paula_close(&p->bridge))return 0;
    p->bound=0;return !p->bytes&&p->allocated==p->released&&normal()&&!state.chip_uncertain;
}

int main(void)
{
    struct pt_private_memory_span guard={&state,sizeof(state)};
    struct pt_private_memory_stats stats;
    unsigned i,j,cuts[5]={0,1,2,3,4},old_allocations;
    size_t owner_size=pt_prepared_paula_sources_size(),written;
    int bad=0,initialized=0;
    original_exec=SysBase;if(!original_exec||!(original_task=FindTask(NULL))||
       original_exec->ThisTask!=original_task)return held();
    original_idnest=(int)(signed char)original_exec->IDNestCnt;
    original_tdnest=(int)(signed char)original_exec->TDNestCnt;
    if(original_idnest!=-1||original_tdnest!=-1)return held();
    assert(sizeof(uint64_t)==8);
    assert(PT_PREPARED_PAULA_SOURCES_COUNT==3);
    project_init();
    if(pt_private_native_memory_init(&memory,512UL*1024,&guard,1)!=PT_PRIVATE_MEMORY_OK){
        if(!normal())return held();
        puts("PREPARED PAULA SOURCES RAM FAIL:strict Fast admission");return 20;
    }
    initialized=1;
#define CHECK(condition) do{if(!(condition)){bad=1;goto cleanup;}}while(0)
    CHECK(owner_size&&owner_size<=UINT32_MAX-TAIL&&pt_prepared_paula_sources_alignment());
    state.owner_capacity=owner_size+TAIL;
    state.allocator=(struct pt_allocator){&memory,pt_private_native_allocate,pt_private_native_release};
    CHECK(pt_project_validate(&state.project,NULL)==PT_PROJECT_OK);
    pt_sampler_init(&state.sampler,&state.allocator,128UL*1024);state.sampler_live=1;
    for(i=0;i<3;++i){
        CHECK(pt_sampler_pin(&state.sampler,&state.project,i,0,state.pcm+i,state.pin+i)==PT_EDIT_OK);
        CHECK(normal()&&!memory.failed&&state.pin[i]&&state.pin[i]==state.sampler.current[i]);
    }
    memcpy(state.saved,state.samples,sizeof(state.saved));
    CHECK(pt_project_size(&state.project,&state.encoded_bytes)==PT_PROJECT_OK);
    state.encoded=pt_private_native_allocate(&memory,state.encoded_bytes);
    state.scratch=pt_private_native_allocate(&memory,state.encoded_bytes);
    CHECK(state.encoded&&state.scratch&&!memory.failed);
    CHECK(pt_project_encode(&state.project,state.encoded,state.encoded_bytes,&written)==PT_PROJECT_OK&&written==state.encoded_bytes);
    for(i=0;i<OWNERS;++i){
        state.owner[i]=pt_private_native_allocate(&memory,state.owner_capacity);
        CHECK(state.owner[i]&&!memory.failed&&memory_class(state.owner[i],state.owner_capacity,MEMF_FAST));
        CHECK((uintptr_t)state.owner[i]%pt_prepared_paula_sources_alignment()==0);
        memset(state.owner[i],0,owner_size);memset((unsigned char *)state.owner[i]+owner_size,0xa7,TAIL);
    }
    CHECK(pt_private_native_memory_seal(&memory)==PT_PRIVATE_MEMORY_OK);
    CHECK(masters_and_save_equal()&&pool_bind(0)&&owner_init(0,0)&&prepare(0,0)&&ready_resources(0,0));
    CHECK(cancel(0)&&masters_and_save_equal());
    old_allocations=state.pool[0].allocated;
    CHECK(owner_init(1,0)&&prepare(1,0)&&ready_resources(1,0));
    CHECK(state.pool[0].allocated==old_allocations&&cancel(1)&&masters_and_save_equal()&&pool_close(0));
    for(i=0;i<5;++i){unsigned owner=i+2,pool=i+1;
        CHECK(pool_bind(pool)&&owner_init(owner,pool));
        for(j=0;j<cuts[i];++j)CHECK(step(owner,pool)==0);
        CHECK(cancel(owner)&&masters_and_save_equal()&&pool_close(pool));
    }
    for(i=0;i<OWNERS;++i)for(j=0;j<TAIL;++j)
        CHECK(((unsigned char *)state.owner[i])[owner_size+j]==0xa7);
cleanup:
    if(!normal()||state.chip_uncertain||memory.failed)return held();
    /* Only exact locally acquired RAM is eligible for this cleanup. Any child
     * HOLD/refused restoration retains its original owner and whole process. */
    for(i=0;i<OWNERS;++i)if(state.initialized[i]){
        if(!cancel(i))return held();
        state.initialized[i]=0;
    }
    for(i=0;i<POOLS;++i)if(state.pool[i].bound&&!pool_close(i))return held();
    for(i=0;i<3;++i)if(state.pin[i]){pt_sampler_unpin(state.pin[i]);state.pin[i]=NULL;}
    if(state.sampler_live){pt_sampler_release(&state.sampler);state.sampler_live=0;}
    if(!normal()||memory.failed||state.sampler.bytes||state.sampler.live_count||
       state.sampler.pending_count||state.sampler.lifetime_fault)return held();
    memset(state.samples,0,sizeof(state.samples));memset(state.saved,0,sizeof(state.saved));
    memset(state.pcm,0,sizeof(state.pcm));memset(&state.project,0,sizeof(state.project));
    if(state.encoded){if(pt_private_native_release_checked(&memory,state.encoded)!=PT_PRIVATE_MEMORY_OK)return held();state.encoded=NULL;}
    if(state.scratch){if(pt_private_native_release_checked(&memory,state.scratch)!=PT_PRIVATE_MEMORY_OK)return held();state.scratch=NULL;}
    for(i=0;i<OWNERS;++i)if(state.owner[i]){
        if(pt_private_native_release_checked(&memory,state.owner[i])!=PT_PRIVATE_MEMORY_OK)return held();
        state.owner[i]=NULL;
    }
    for(i=0;i<POOLS;++i)if(state.pool[i].bytes||state.pool[i].allocated!=state.pool[i].released)return held();
    if(!initialized||pt_private_native_memory_finish(&memory)!=PT_PRIVATE_MEMORY_OK||
       pt_private_native_memory_stats(&memory,&stats)!=PT_PRIVATE_MEMORY_OK||
       stats.live||stats.used||stats.failed||stats.allocations!=stats.releases||!normal())return held();
    if(bad){puts("PREPARED PAULA SOURCES RAM FAIL:known local RAM cleaned");return 20;}
    puts(PASS_MARKER);return 0;
#undef CHECK
}
