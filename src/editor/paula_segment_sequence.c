#include "paula_segment_sequence.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include <stddef.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char c;T value;},value)
#define ALLOCATIONS 16U
#define SOURCE_GUARDS 128U
struct source_guard {const void *address;size_t bytes;};
struct allocation {void *address;size_t bytes;};
struct pt_paula_segment_sequence {
    struct pt_allocator allocator,children;
    struct pt_sampler_paula *bridge;struct pt_sampler *sampler;struct pt_project *project,header;
    struct pt_paula_segment_sequence_config config;struct pt_paula_segment_backend backend;
    struct pt_pcm descriptor[3];struct pt_sample_version *expected[3];
    uint32_t revision,generation;uint64_t bridge_version;
    void *workspace;size_t capacity,bytes,peak;struct allocation allocation[ALLOCATIONS];
    struct source_guard guard[SOURCE_GUARDS];unsigned guard_count;
    struct source_guard constructor_argument[5];unsigned constructor_active;
    struct pt_render_sequence_setup *setup;struct pt_render_sequence *sequence;
    struct pt_paula_segment_readers *pool;struct pt_render_lookahead lookahead;
    struct pt_render_interval interval;struct pt_render_plan plan;
    struct pt_render_action action;struct pt_readers_key original,actual;
    struct pt_paula_segment_boundary_scope scope;
    enum pt_paula_segment_sequence_phase phase;
    enum pt_paula_segment_sequence_result result;
    enum pt_render_result render_result;enum pt_scheduled_result scheduled_result;
    uint64_t frame,boundary,intervals,ticket[4];uint32_t remaining;
    unsigned audit,events,audit_events,forecast_ready,command,published,detached,reader_retired;
    unsigned original_seen,busy,callback_active,failed,cancelled,unowned_return,callback_faults;
};
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int object(const void *p,size_t n,size_t alignment)
{return p&&span(p,n)&&!((uintptr_t)p%alignment);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int cache_apart(const struct pt_sampler_paula *b,const void *p,size_t n)
{unsigned i;for(i=0;i<PT_CACHE_SLOTS;++i){
 if(!apart(p,n,b->cache.entry[i].data,b->cache.entry[i].bytes))return 0;}return 1;}
static int product(size_t a,size_t b,size_t *n)
{if(b&&a>SIZE_MAX/b)return 0;*n=a*b;return 1;}
/* Valid immutable project metadata is a constructor borrowing precondition.
 * Protect complete PCM capacities, not only audible frames. */
static int source_apart(const struct pt_project *p,const void *out,size_t bytes)
{
    size_t n,i;
    if(!apart(out,bytes,p,sizeof(*p))||!product(p->sample_count,sizeof(*p->samples),&n)||
       !apart(out,bytes,p->samples,n)||!product(p->order_count,sizeof(*p->orders),&n)||
       !apart(out,bytes,p->orders,n)||!product((size_t)p->pattern_count*64U,p->channels.count,&n)||
       !product(n,sizeof(*p->events),&n)||!apart(out,bytes,p->events,n)||
       !product(p->extension_count,sizeof(*p->extensions),&n)||!apart(out,bytes,p->extensions,n))return 0;
    for(i=0;i<p->sample_count;++i){const struct pt_sample *s=p->samples+i;
        if(!product(s->pcm.capacity,sizeof(*s->pcm.data),&n)||!apart(out,bytes,s->pcm.data,n)||
           !product(s->slice_count,sizeof(*s->slices),&n)||!apart(out,bytes,s->slices,n))return 0;}
    for(i=0;i<p->extension_count;++i)if(!apart(out,bytes,p->extensions[i].data,p->extensions[i].length))return 0;
    return 1;
}
static int guard_add(struct pt_paula_segment_sequence *s,const void *p,size_t n)
{if(!n)return 1;if(!span(p,n)||s->guard_count==SOURCE_GUARDS)return 0;
 s->guard[s->guard_count++]=(struct source_guard){p,n};return 1;}
static int capture_guards(struct pt_paula_segment_sequence *s)
{
    struct pt_project *p=s->project;size_t i,n;
    if(!guard_add(s,p,sizeof(*p))||!product(p->sample_count,sizeof(*p->samples),&n)||!guard_add(s,p->samples,n)||
       !product(p->order_count,sizeof(*p->orders),&n)||!guard_add(s,p->orders,n)||
       !product((size_t)p->pattern_count*64U,p->channels.count,&n)||!product(n,sizeof(*p->events),&n)||!guard_add(s,p->events,n)||
       !product(p->extension_count,sizeof(*p->extensions),&n)||!guard_add(s,p->extensions,n))return 0;
    for(i=0;i<p->sample_count;++i){const struct pt_sample *sample=p->samples+i;
        if(!product(sample->pcm.capacity,sizeof(*sample->pcm.data),&n)||!guard_add(s,sample->pcm.data,n)||
           !product(sample->slice_count,sizeof(*sample->slices),&n)||!guard_add(s,sample->slices,n))return 0;}
    for(i=0;i<p->extension_count;++i)if(!guard_add(s,p->extensions[i].data,p->extensions[i].length))return 0;
    for(i=0;i<3;++i){struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned count=0,j;
        if(!pt_sampler_version_spans(s->expected[i],spans,PT_SAMPLER_VERSION_SPANS,&count))return 0;
        for(j=0;j<count;++j)if(!guard_add(s,spans[j].data,spans[j].bytes))return 0;}
    return 1;
}
static int same_key(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
 a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static int current(struct pt_paula_segment_sequence *s,uint32_t revision)
{
    unsigned i;
    if(revision!=s->revision||s->sampler->generation!=s->generation||s->bridge->sampler!=s->sampler||
       s->bridge->project!=s->project||s->bridge->version!=s->bridge_version||
       memcmp(s->project,&s->header,sizeof(s->header))||!pt_sampler_paula_prepared_current(s->bridge))return 0;
    for(i=0;i<3;++i){unsigned sample=s->config.readers.dependency[i].sample;
        if(s->sampler->current[sample]!=s->expected[i]||
           memcmp(&s->project->samples[sample].pcm,s->descriptor+i,sizeof(s->descriptor[i])))return 0;}
    return 1;
}
static int output_apart(struct pt_paula_segment_sequence *s,const void *p,size_t n)
{
    unsigned i;
    if(!apart(p,n,s->workspace,s->capacity)||!apart(p,n,s->bridge,sizeof(*s->bridge))||
       !apart(p,n,s->sampler,sizeof(*s->sampler))||!apart(p,n,s->backend.readers.context,s->backend.readers.context_bytes))return 0;
    for(i=0;i<ALLOCATIONS;++i)if(s->allocation[i].address&&!apart(p,n,s->allocation[i].address,s->allocation[i].bytes))return 0;
    if(s->constructor_active)for(i=0;i<5;++i)
        if(!apart(p,n,s->constructor_argument[i].address,s->constructor_argument[i].bytes))return 0;
    for(i=0;i<s->guard_count;++i)if(!apart(p,n,s->guard[i].address,s->guard[i].bytes))return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(!apart(p,n,s->bridge->cache.entry[i].data,s->bridge->cache.entry[i].bytes))return 0;
    return 1;
}
static enum pt_paula_segment_sequence_result fail(struct pt_paula_segment_sequence *s,enum pt_paula_segment_sequence_result r)
{s->failed=1;s->phase=PT_PAULA_SEQUENCE_ERROR;s->result=r;return r;}
static int enter(struct pt_paula_segment_sequence *s)
{if(!s)return 0;if(s->busy||s->callback_active){s->failed=1;++s->callback_faults;return 0;}s->busy=1;return 1;}
static void *child_allocate(void *context,size_t n)
{
    struct pt_paula_segment_sequence *s=context;unsigned i;void *p;
    if(s->failed||s->cancelled||s->callback_active||!current(s,s->revision)||n>s->config.control_budget-s->bytes)return NULL;
    for(i=0;i<ALLOCATIONS&&s->allocation[i].address;++i){}if(i==ALLOCATIONS)return NULL;
    s->callback_active=1;p=s->allocator.allocate(s->allocator.context,n);s->callback_active=0;
    if(!p)return NULL;
    /* An overlapping return was never acquired; never release a source/live
     * alias as if it were a fresh allocation. Contract breach retains failure. */
    if(!object(p,n,ALIGNMENT(long double))||!output_apart(s,p,n)){s->failed=1;s->unowned_return=1;return NULL;}
    s->allocation[i]=(struct allocation){p,n};s->bytes+=n;if(s->bytes>s->peak)s->peak=s->bytes;
    if(s->failed||!current(s,s->revision)){
        s->callback_active=1;s->allocator.release(s->allocator.context,p);s->callback_active=0;
        s->allocation[i]=(struct allocation){0};s->bytes-=n;return NULL;}
    return p;
}
static void child_release(void *context,void *p)
{
    struct pt_paula_segment_sequence *s=context;unsigned i;size_t n;
    for(i=0;i<ALLOCATIONS;++i)if(s->allocation[i].address==p)break;
    if(!p||i==ALLOCATIONS||s->callback_active){s->failed=1;++s->callback_faults;return;}
    n=s->allocation[i].bytes;s->allocation[i]=(struct allocation){0};s->bytes-=n;
    s->callback_active=1;s->allocator.release(s->allocator.context,p);s->callback_active=0;
}
size_t pt_paula_segment_sequence_control_size(void){return sizeof(struct pt_paula_segment_sequence);}
size_t pt_paula_segment_sequence_control_alignment(void){return ALIGNMENT(struct pt_paula_segment_sequence);}
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_begin(const struct pt_allocator *allocator,
    struct pt_sampler_paula *bridge,const struct pt_paula_segment_sequence_config *config,
    const struct pt_paula_segment_backend *backend,uint32_t revision,void *workspace,size_t capacity,
    struct pt_paula_segment_sequence **out)
{
    struct pt_paula_segment_sequence *s;struct pt_project *project;unsigned i,j;uint64_t offset;
    const void *fixed[5]={allocator,bridge,config,backend,out};size_t sizes[5]={sizeof(*allocator),sizeof(*bridge),sizeof(*config),sizeof(*backend),sizeof(*out)};
    if(!object(workspace,capacity,pt_paula_segment_sequence_control_alignment())||capacity<sizeof(*s))return PT_PAULA_SEQUENCE_CAPACITY;
    if(!object(allocator,sizeof(*allocator),ALIGNMENT(struct pt_allocator))||!object(bridge,sizeof(*bridge),ALIGNMENT(struct pt_sampler_paula))||
       !object(config,sizeof(*config),ALIGNMENT(struct pt_paula_segment_sequence_config))||!object(backend,sizeof(*backend),ALIGNMENT(struct pt_paula_segment_backend))||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_paula_segment_sequence *)))return PT_PAULA_SEQUENCE_INVALID;
    for(i=0;i<5;++i){if(!apart(workspace,capacity,fixed[i],sizes[i]))return PT_PAULA_SEQUENCE_INVALID;
        for(j=0;j<i;++j)if(!apart(fixed[i],sizes[i],fixed[j],sizes[j]))return PT_PAULA_SEQUENCE_INVALID;}
    /* Known genuine cache extents are refused before interpreting a new
     * output slot or writing any workspace byte. Cache data is never read. */
    if(!pt_sampler_paula_prepared_current(bridge)||!cache_apart(bridge,workspace,capacity))return PT_PAULA_SEQUENCE_INVALID;
    for(i=0;i<5;++i)if(!cache_apart(bridge,fixed[i],sizes[i]))return PT_PAULA_SEQUENCE_INVALID;
    if(*out||!allocator->allocate||!allocator->release)return PT_PAULA_SEQUENCE_INVALID;
    project=bridge->project;
    if(!project||!bridge->sampler||config->origin||!config->control_budget||capacity>config->control_budget||
       project->order_count!=1||project->pattern_count!=1||project->orders[0]||config->render.row_range||config->render.pattern_only||
       config->render.start_order||config->render.include_lead_in||config->render.rate!=48000||
       config->readers.grid.rate!=config->render.rate||config->readers.dependency_count!=3||
       config->readers.track>=project->channels.count||config->render.tracks!=(1U<<config->readers.track)||
       !config->render.tick_limit||!config->render.frame_limit||!revision||
       !source_apart(project,workspace,capacity)||!pt_sampler_output_disjoint(bridge->sampler,workspace,capacity)||
       !apart(workspace,capacity,bridge->sampler,sizeof(*bridge->sampler))||
       !apart(workspace,capacity,backend->readers.context,backend->readers.context_bytes))return PT_PAULA_SEQUENCE_INVALID;
    for(i=0;i<5;++i)if(!source_apart(project,fixed[i],sizes[i])||!pt_sampler_output_disjoint(bridge->sampler,fixed[i],sizes[i])||!apart(fixed[i],sizes[i],bridge->sampler,sizeof(*bridge->sampler))||
        !apart(fixed[i],sizes[i],backend->readers.context,backend->readers.context_bytes))return PT_PAULA_SEQUENCE_INVALID;
    for(i=0;i<4;++i){offset=(uint64_t)(i+1U)*960U;if(config->origin>UINT64_MAX-offset||
        config->origin+offset==UINT64_MAX||config->readers.frame[i]!=config->origin+offset)return PT_PAULA_SEQUENCE_INVALID;}
    for(i=0;i<3;++i){unsigned sample=config->readers.dependency[i].sample;
        if(sample>=project->sample_count||!config->readers.dependency[i].expected||
           bridge->sampler->current[sample]!=config->readers.dependency[i].expected)return PT_PAULA_SEQUENCE_STALE;
        for(j=0;j<i;++j)if(sample==config->readers.dependency[j].sample)return PT_PAULA_SEQUENCE_INVALID;}
    memset(workspace,0,capacity);s=workspace;s->workspace=workspace;s->capacity=capacity;s->bytes=s->peak=capacity;
    s->constructor_active=1;for(i=0;i<5;++i)s->constructor_argument[i]=(struct source_guard){fixed[i],sizes[i]};
    memcpy(&s->allocator,allocator,sizeof(s->allocator));s->children=(struct pt_allocator){s,child_allocate,child_release};s->bridge=bridge;s->sampler=bridge->sampler;
    s->project=project;memcpy(&s->header,project,sizeof(s->header));memcpy(&s->config,config,sizeof(s->config));memcpy(&s->backend,backend,sizeof(s->backend));s->revision=revision;
    s->generation=s->sampler->generation;s->bridge_version=bridge->version;s->scope.track=config->readers.track;s->scope.dependency_count=3;
    for(i=0;i<3;++i){unsigned sample=config->readers.dependency[i].sample;s->expected[i]=config->readers.dependency[i].expected;
        memcpy(s->descriptor+i,&project->samples[sample].pcm,sizeof(s->descriptor[i]));s->scope.dependency[i]=&project->samples[sample].pcm;}
    if(!capture_guards(s)){s->constructor_active=0;memset(s->constructor_argument,0,sizeof(s->constructor_argument));return PT_PAULA_SEQUENCE_CAPACITY;}
    memcpy(s->scope.frame,config->readers.frame,sizeof(s->scope.frame));s->audit=1;s->frame=config->origin;s->phase=PT_PAULA_SEQUENCE_SETUP;
    s->busy=1;
    if(pt_render_sequence_setup_begin(project,&s->config.render,&s->children,revision,s->generation,NULL,0,&s->setup)!=PT_RENDER_SETUP_PENDING||s->failed){
        if(s->setup)pt_render_sequence_setup_cancel(&s->setup);s->constructor_active=0;
        memset(s->constructor_argument,0,sizeof(s->constructor_argument));s->busy=0;return fail(s,PT_PAULA_SEQUENCE_FAILED);}
    if(*out||memcmp(allocator,&s->allocator,sizeof(*allocator))||memcmp(config,&s->config,sizeof(*config))||
       memcmp(backend,&s->backend,sizeof(*backend))||!current(s,revision)){
        if(s->setup)pt_render_sequence_setup_cancel(&s->setup);s->constructor_active=0;
        memset(s->constructor_argument,0,sizeof(s->constructor_argument));s->busy=0;return fail(s,PT_PAULA_SEQUENCE_FAILED);}
    s->constructor_active=0;memset(s->constructor_argument,0,sizeof(s->constructor_argument));
    s->busy=0;s->result=PT_PAULA_SEQUENCE_PENDING;*out=s;return s->result;
}
/* Audit is a shape/time/dependency pass, never an ACTIVE key certificate or
 * boundary admission. Only live admission can use actual original keys. */
static int audit_plan(struct pt_paula_segment_sequence *s)
{
    unsigned i,j;enum pt_render_action_kind kind;
    if(!s->plan.count)return s->events==0||s->events==4;
    if(s->events>=4||s->boundary!=s->config.readers.frame[s->events]||s->plan.count!=(s->events==3?1U:2U))return 0;
    kind=s->events==0?PT_RENDER_SEGMENT:s->events==3?PT_RENDER_STOP:PT_RENDER_REPEAT;
    if(s->plan.action[0].kind!=kind||s->plan.action[0].channel!=s->scope.track||
       (s->events!=3&&(s->plan.action[1].kind!=PT_RENDER_CONTROL||s->plan.action[1].channel!=s->scope.track)))return 0;
    for(i=0;i<s->plan.count;++i){const struct pt_voice *v=&s->plan.action[i].voice;int found=0;
        for(j=0;j<3;++j)if(v->pcm==s->scope.dependency[j])found=1;if(!found)return 0;
        if(v->repeat_pcm){found=0;for(j=0;j<3;++j)if(v->repeat_pcm==s->scope.dependency[j])found=1;if(!found)return 0;}}
    ++s->events;return 1;
}
static enum pt_paula_segment_sequence_result step_inner(struct pt_paula_segment_sequence *s,unsigned work)
{
    enum pt_render_setup_result setup;enum pt_paula_segment_readers_result readers;unsigned ready=0;
    switch(s->phase){
    case PT_PAULA_SEQUENCE_SETUP:
        setup=pt_render_sequence_setup_step(s->setup,s->revision,s->generation,work);
        if(setup==PT_RENDER_SETUP_READY)s->phase=PT_PAULA_SEQUENCE_TAKE;
        else if(setup!=PT_RENDER_SETUP_PENDING)return fail(s,PT_PAULA_SEQUENCE_FAILED);break;
    case PT_PAULA_SEQUENCE_TAKE:
        setup=pt_render_sequence_setup_take(&s->setup,s->revision,s->generation,NULL,0,&s->sequence);
        if(setup==PT_RENDER_SETUP_CAPACITY)return PT_PAULA_SEQUENCE_WAIT_PRESSURE;
        if(setup!=PT_RENDER_SETUP_READY)return fail(s,PT_PAULA_SEQUENCE_FAILED);s->phase=PT_PAULA_SEQUENCE_MEASURE;break;
    case PT_PAULA_SEQUENCE_MEASURE:
        s->render_result=pt_render_sequence_prepare(s->sequence,work,&ready);
        if(s->render_result!=PT_RENDER_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        if(ready)s->phase=PT_PAULA_SEQUENCE_NEXT;break;
    case PT_PAULA_SEQUENCE_NEXT:
        s->render_result=pt_render_sequence_next(s->sequence,&s->interval);
        if(s->render_result!=PT_RENDER_OK||s->interval.frames>UINT64_MAX-s->frame||s->frame+s->interval.frames==UINT64_MAX)
            return fail(s,PT_PAULA_SEQUENCE_FAILED);
        /* Returned interval PRECEDES its plan. Its end is the beginning of the
         * next voice interval, not an extra tick added to an already timed plan. */
        s->boundary=s->frame+s->interval.frames;s->remaining=s->interval.frames;++s->intervals;s->forecast_ready=0;
        s->render_result=pt_render_lookahead_begin(&s->lookahead,s->sequence);
        if(s->render_result!=PT_RENDER_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);s->phase=PT_PAULA_SEQUENCE_FORECAST;break;
    case PT_PAULA_SEQUENCE_FORECAST:
        s->render_result=pt_render_lookahead_step(&s->lookahead,work,&s->plan,&s->forecast_ready);
        if(s->render_result!=PT_RENDER_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        if(s->forecast_ready)s->phase=PT_PAULA_SEQUENCE_CONSUME;break;
    case PT_PAULA_SEQUENCE_CONSUME:
        if(s->remaining){unsigned n=s->remaining<work?s->remaining:work;
            s->render_result=pt_render_sequence_consume(s->sequence,n);
            if(s->render_result!=PT_RENDER_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);s->remaining-=n;break;}
        if(s->audit)s->phase=PT_PAULA_SEQUENCE_AUDIT;
        else if(!s->plan.count)s->phase=PT_PAULA_SEQUENCE_COMMIT;
        else s->phase=PT_PAULA_SEQUENCE_KEY;break;
    case PT_PAULA_SEQUENCE_AUDIT:
        if(!audit_plan(s)||(s->interval.end&&s->events!=4))return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->phase=PT_PAULA_SEQUENCE_COMMIT;break;
    case PT_PAULA_SEQUENCE_REWIND:
        if(s->events!=4||!current(s,s->revision))return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->render_result=pt_render_sequence_rewind(s->sequence);
        if(s->render_result!=PT_RENDER_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->audit_events=s->events;s->events=0;s->audit=0;s->frame=s->config.origin;s->intervals=0;s->phase=PT_PAULA_SEQUENCE_POOL_OPEN;break;
    case PT_PAULA_SEQUENCE_POOL_OPEN:
        readers=pt_paula_segment_readers_open(&s->children,s->bridge,&s->config.readers,&s->backend,&s->pool);
        if(readers!=PT_PAULA_SEGMENT_READERS_OK)return fail(s,readers==PT_PAULA_SEGMENT_READERS_CAPACITY?PT_PAULA_SEQUENCE_CAPACITY:PT_PAULA_SEQUENCE_FAILED);
        s->phase=PT_PAULA_SEQUENCE_POOL_PREPARE;break;
    case PT_PAULA_SEQUENCE_POOL_PREPARE:
        readers=pt_paula_segment_readers_step(s->pool,&ready);
        if(readers!=PT_PAULA_SEGMENT_READERS_OK&&readers!=PT_PAULA_SEGMENT_READERS_PENDING)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        if(ready)s->phase=PT_PAULA_SEQUENCE_NEXT;break;
    case PT_PAULA_SEQUENCE_KEY:
        if(s->events>=4||s->boundary!=s->config.readers.frame[s->events])return fail(s,PT_PAULA_SEQUENCE_FAILED);
        if(s->events){s->scheduled_result=pt_paula_segment_readers_key(s->pool,&s->actual);
            if(s->scheduled_result==PT_SCHEDULED_STALE)return PT_PAULA_SEQUENCE_WAIT_ACTIVE;
            if(s->scheduled_result!=PT_SCHEDULED_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
            if(!s->original_seen){memcpy(&s->original,&s->actual,sizeof(s->original));s->original_seen=1;}
            else if(!same_key(&s->actual,&s->original))return fail(s,PT_PAULA_SEQUENCE_FAILED);}
        if(pt_paula_segment_boundary_admit(&s->scope,s->events,s->boundary,&s->plan,
            s->events?&s->original:NULL,s->events?&s->actual:NULL,&s->action)!=PT_PAULA_BOUNDARY_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->phase=PT_PAULA_SEQUENCE_BEGIN;break;
    case PT_PAULA_SEQUENCE_BEGIN:
        readers=pt_paula_segment_readers_begin(s->pool,s->boundary,&s->action,s->events?&s->original:NULL,&s->command);
        if(readers==PT_PAULA_SEGMENT_READERS_BUSY)return PT_PAULA_SEQUENCE_WAIT_PRESSURE;
        if(readers!=PT_PAULA_SEGMENT_READERS_OK||s->command!=s->events)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->phase=PT_PAULA_SEQUENCE_ENQUEUE;break;
    case PT_PAULA_SEQUENCE_ENQUEUE:
        s->scheduled_result=pt_paula_segment_readers_enqueue(s->pool,s->command,s->ticket+s->command);
        if(s->scheduled_result==PT_SCHEDULED_CAPACITY)return PT_PAULA_SEQUENCE_WAIT_PRESSURE;
        if(s->scheduled_result!=PT_SCHEDULED_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->phase=PT_PAULA_SEQUENCE_PUBLISH;return PT_PAULA_SEQUENCE_READY;
    case PT_PAULA_SEQUENCE_PUBLISH:return PT_PAULA_SEQUENCE_READY;
    case PT_PAULA_SEQUENCE_COMMIT:
        if(!s->audit&&!s->plan.count&&s->events!=0&&s->events!=4)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        if(s->interval.end&&(s->plan.count||s->events!=4))return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->render_result=pt_render_lookahead_commit(&s->lookahead);
        if(s->render_result!=PT_RENDER_OK)return fail(s,PT_PAULA_SEQUENCE_FAILED);
        s->frame=s->boundary;s->phase=s->interval.end?(s->audit?PT_PAULA_SEQUENCE_REWIND:PT_PAULA_SEQUENCE_END):PT_PAULA_SEQUENCE_NEXT;
        if(s->phase==PT_PAULA_SEQUENCE_END)return PT_PAULA_SEQUENCE_DONE;break;
    case PT_PAULA_SEQUENCE_END:return PT_PAULA_SEQUENCE_DONE;
    default:return fail(s,PT_PAULA_SEQUENCE_FAILED);
    }
    return PT_PAULA_SEQUENCE_PENDING;
}
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_step(struct pt_paula_segment_sequence *s,uint32_t revision,unsigned work)
{
    enum pt_paula_segment_sequence_result r;
    if(!work||work>256||!enter(s))return PT_PAULA_SEQUENCE_INVALID;
    if(s->failed||s->cancelled){s->busy=0;return s->result;}
    if(!current(s,revision)){s->busy=0;return fail(s,PT_PAULA_SEQUENCE_STALE);}
    r=step_inner(s,work);if(!current(s,revision)||s->failed){if(r!=PT_PAULA_SEQUENCE_STALE)r=fail(s,PT_PAULA_SEQUENCE_FAILED);}
    s->result=r;s->busy=0;return r;
}
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_get(struct pt_paula_segment_sequence *s,struct pt_paula_segment_sequence_status *out)
{
    struct pt_paula_segment_sequence_status status;
    if(!s||!object(out,sizeof(*out),ALIGNMENT(struct pt_paula_segment_sequence_status))||!output_apart(s,out,sizeof(*out))||!enter(s))return PT_PAULA_SEQUENCE_INVALID;
    memset(&status,0,sizeof(status));status.phase=s->phase;status.result=s->result;status.render_result=s->render_result;status.scheduled_result=s->scheduled_result;
    status.frame=s->frame;status.boundary=s->boundary;status.intervals=s->intervals;memcpy(status.ticket,s->ticket,sizeof(status.ticket));
    status.audit_events=s->audit_events;status.live_events=s->events;status.published_mask=s->published;status.detached_mask=s->detached;
    status.reader_retired=s->reader_retired;status.callback_faults=s->callback_faults;status.cancelled=s->cancelled;status.failed=s->failed;status.control_bytes=s->bytes;status.peak_control_bytes=s->peak;
    memcpy(out,&status,sizeof(status));s->busy=0;return status.result;
}
enum pt_scheduled_result pt_paula_segment_sequence_publish_next(struct pt_paula_segment_sequence *s,uint32_t revision)
{
    enum pt_scheduled_result r;
    if(!enter(s))return PT_SCHEDULED_INVALID;
    if(s->failed||s->cancelled||s->phase!=PT_PAULA_SEQUENCE_PUBLISH){s->busy=0;return PT_SCHEDULED_STALE;}
    if(!current(s,revision)){fail(s,PT_PAULA_SEQUENCE_STALE);s->busy=0;return PT_SCHEDULED_STALE;}
    r=pt_paula_segment_readers_publish(s->pool,s->command);
    /* Check actual callback/current faults before deliberate local failure. */
    if(!current(s,revision)||s->failed)r=PT_SCHEDULED_BACKEND;
    s->scheduled_result=r;
    if(r==PT_SCHEDULED_OK){s->published|=1U<<s->command;++s->events;s->phase=PT_PAULA_SEQUENCE_COMMIT;s->result=PT_PAULA_SEQUENCE_PENDING;}
    else if(r==PT_SCHEDULED_PENDING)s->result=PT_PAULA_SEQUENCE_WAIT_PRESSURE;
    else {if(r==PT_SCHEDULED_BACKEND)s->published|=1U<<s->command;fail(s,PT_PAULA_SEQUENCE_FAILED);}
    s->busy=0;return r;
}
enum pt_scheduled_result pt_paula_segment_sequence_service_command(struct pt_paula_segment_sequence *s,unsigned index,unsigned cancel)
{
    struct pt_readers_command_receipt receipt;enum pt_scheduled_result r;
    if(index>=4||cancel>1||!enter(s))return PT_SCHEDULED_INVALID;
    if(!s->pool||!s->ticket[index]||(s->detached&(1U<<index))){s->busy=0;return PT_SCHEDULED_INVALID;}
    memset(&receipt,0,sizeof(receipt));r=pt_paula_segment_readers_poll_command(s->pool,index,(int)cancel,&receipt);
    s->scheduled_result=r;
    if(r==PT_SCHEDULED_OK){
        s->detached|=1U<<index;
        if(!index&&!cancel&&!s->failed&&!s->cancelled){
            enum pt_scheduled_result key_result=pt_paula_segment_readers_key(s->pool,&s->actual);
            if(key_result==PT_SCHEDULED_OK){
                if(!s->original_seen){memcpy(&s->original,&s->actual,sizeof(s->original));s->original_seen=1;}
                else if(!same_key(&s->original,&s->actual))fail(s,PT_PAULA_SEQUENCE_FAILED);
            }else if(key_result!=PT_SCHEDULED_STALE)fail(s,PT_PAULA_SEQUENCE_FAILED);
        }
    }
    if(cancel){s->cancelled=1;s->result=PT_PAULA_SEQUENCE_FAILED;}
    if(r!=PT_SCHEDULED_OK&&r!=PT_SCHEDULED_PENDING&&r!=PT_SCHEDULED_STALE)fail(s,PT_PAULA_SEQUENCE_FAILED);
    s->busy=0;return r;
}
enum pt_scheduled_result pt_paula_segment_sequence_service_reader(struct pt_paula_segment_sequence *s,unsigned cancel)
{
    struct pt_readers_reader_receipt receipt;enum pt_scheduled_result r;
    if(cancel>1||!enter(s))return PT_SCHEDULED_INVALID;
    if(!s->pool||!s->ticket[0]||s->reader_retired){s->busy=0;return PT_SCHEDULED_INVALID;}
    memset(&receipt,0,sizeof(receipt));r=pt_paula_segment_readers_poll_reader(s->pool,(int)cancel,&receipt);
    if(r==PT_SCHEDULED_OK&&receipt.state==PT_READERS_RETIRED)s->reader_retired=1;
    if(cancel){s->cancelled=1;s->result=PT_PAULA_SEQUENCE_FAILED;}
    if(r!=PT_SCHEDULED_OK&&r!=PT_SCHEDULED_PENDING)fail(s,PT_PAULA_SEQUENCE_FAILED);
    s->busy=0;return r;
}
/* Genuine local cancellation returns no backend envelope or activation proof.
 * Validate only the exact original command/reader identities from that real
 * queue operation; never infer detach from future accepted counts. */
static int local_cancelled(const struct pt_paula_segment_sequence *s,unsigned index,
    const struct pt_readers_command_receipt *receipt)
{
    int8_t map[PT_CHANNEL_LIMIT];
    const struct pt_readers_action_receipt *a=receipt->action;
    const struct pt_readers_key *k=&a->key;
    if(pt_channels_paula_map(&s->header.channels,NULL,map)!=PT_CHANNEL_OK||
       s->config.readers.track>=PT_CHANNEL_LIMIT||map[s->config.readers.track]<0)return 0;
    if(receipt->domain!=PT_READERS_COMMAND_DOMAIN||receipt->origin!=PT_READERS_LOCAL_UNSUBMITTED||
       !receipt->queue||receipt->session!=s->config.readers.session||
       receipt->generation!=s->config.readers.grid.generation||receipt->ticket!=s->ticket[index]||
       receipt->owner!=index+1U||receipt->count!=1||receipt->event||receipt->context||receipt->context_bytes||
       a->command!=PT_READERS_CANCELLED_BEFORE||a->reader!=PT_READERS_STATE_UNKNOWN||
       a->adoption!=PT_READERS_UNADOPTED||a->observed||a->issued||
       k->queue!=receipt->queue||k->session!=receipt->session||k->generation!=receipt->generation||
       k->trigger!=s->ticket[0]||k->owner!=5||!k->serial||k->action||k->slot!=(unsigned)map[s->config.readers.track])return 0;
    /* Continuing commands must use the full key previously exposed by the
     * genuine positive ACTIVE getter. A never-published first trigger has no
     * ACTIVE key and the local receipt grants none. */
    return !index||(s->original_seen&&same_key(k,&s->original));
}
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_cancel(struct pt_paula_segment_sequence *s)
{
    unsigned i,faults_before,failed_before;struct pt_readers_command_receipt receipt;
    enum pt_scheduled_result result;
    if(!enter(s))return PT_PAULA_SEQUENCE_INVALID;
    faults_before=s->callback_faults;failed_before=s->failed;
    s->cancelled=1;s->phase=PT_PAULA_SEQUENCE_CANCELLED;s->result=PT_PAULA_SEQUENCE_FAILED;pt_render_lookahead_cancel(&s->lookahead);
    if(s->pool){
        /* Collect actual local proofs BEFORE whole stop consumes the holders.
         * Published or uncertain submits retain their external proof duties. */
        for(i=0;i<4;++i)if(s->ticket[i]&&!(s->published&(1U<<i))&&!(s->detached&(1U<<i))){
            memset(&receipt,0,sizeof(receipt));
            result=pt_paula_segment_readers_poll_command(s->pool,i,1,&receipt);
            if(result!=PT_SCHEDULED_OK||!local_cancelled(s,i,&receipt)||
               s->callback_faults!=faults_before||s->failed!=failed_before){
                s->scheduled_result=PT_SCHEDULED_BACKEND;s->failed=1;s->busy=0;return s->result;}
            s->detached|=1U<<i;
        }
        s->scheduled_result=pt_paula_segment_readers_stop(s->pool);
        if(s->scheduled_result!=PT_SCHEDULED_OK&&s->scheduled_result!=PT_SCHEDULED_PENDING)s->failed=1;
    }
    s->busy=0;return s->result;
}
int pt_paula_segment_sequence_close(struct pt_paula_segment_sequence **owner)
{
    struct pt_paula_segment_sequence *s;unsigned i,faults_before;
    if(!object(owner,sizeof(*owner),ALIGNMENT(struct pt_paula_segment_sequence *)))return 0;
    s=*owner;if(!s)return 1;
    if(!output_apart(s,owner,sizeof(*owner))||!enter(s))return 0;
    faults_before=s->callback_faults;s->cancelled=1;pt_render_lookahead_cancel(&s->lookahead);
    if(s->pool){if(!pt_paula_segment_readers_close(s->pool)){s->busy=0;return 0;}s->pool=NULL;}
    if(s->setup){if(pt_render_sequence_setup_cancel(&s->setup)!=PT_RENDER_SETUP_READY){s->busy=0;return 0;}}
    if(s->sequence){struct pt_render_sequence *sequence=s->sequence;s->sequence=NULL;pt_render_sequence_close(sequence);}
    for(i=0;i<ALLOCATIONS;++i)if(s->allocation[i].address){s->busy=0;return 0;}
    if(s->unowned_return){s->busy=0;return 0;}
    if(*owner!=s){s->failed=1;s->busy=0;return 0;}
    /* A new release callback fault keeps the external control diagnosable.
     * Consumed child slots/bytes remain consumed; no history is resurrected. */
    if(faults_before!=s->callback_faults){fail(s,PT_PAULA_SEQUENCE_FAILED);s->busy=0;return 0;}
    s->busy=0;*owner=NULL;return 1;
}
