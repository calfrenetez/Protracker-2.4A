#include "prepared_paula_mutation_barrier.h"
#include "editor.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "project_snapshot.h"
#include "../core/sample_usage.h"
#include "../core/pcm_internal.h"
#include <stdint.h>
#include <string.h>

#define ALIGNMENT(T) offsetof(struct {char byte;T value;},value)
enum {
    SOURCE_ATTEMPT=1U,SOURCE_OPEN=2U,SOURCE_READY=4U,
    QUEUE_ATTEMPT=8U,QUEUE_OPEN=16U,ENQUEUE_ATTEMPT=32U,TRANSFERRED=64U,
    STOP_ATTEMPT=128U,STOPPED=256U,DRAINED=512U,
    QUEUE_CLOSED=1024U,SOURCE_CLOSED=2048U,
    BRIDGE_ATTEMPT=4096U,BRIDGE_CLOSED=8192U,DETACHED=16384U
};
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
enum {
    PUBLICATION_ATTEMPT=32768U,PUBLICATION_STOP_ATTEMPT=65536U,
    PUBLICATION_STOP_RETURNED=131072U
};
#endif
struct pt_prepared_paula_mutation_barrier {
    struct pt_prepared_paula_mutation_barrier *self;size_t capacity;
    struct pt_prepared_paula_mutation_config config;
    struct pt_project *project,project_header;
    struct pt_sampler sampler_header;
    void *chip_context;void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);size_t chip_budget;
    uint64_t bridge_version;uint32_t revision;
    unsigned flags,busy,cancelled,uncertain;
    enum pt_prepared_paula_sources_result failure;
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
    enum pt_scheduled_result publication_failure;
#endif
};

static int object(const void *p,size_t n,size_t a)
{uintptr_t v=(uintptr_t)p;return p&&n&&a&&v%a==0&&n<=UINTPTR_MAX-v;}
static int apart(const void *p,size_t n,const void *q,size_t m)
{
    uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
    return (!n||object(p,n,1))&&(!m||object(q,m,1))&&
        (!n||!m||a+n<=b||b+m<=a);
}
static int array(const void *p,size_t n,size_t width,size_t a)
{return width&&n<=SIZE_MAX/width&&(!n||object(p,n*width,a));}
static int zero_prefix(const void *p,size_t bytes)
{const unsigned char *v=(const unsigned char *)p;size_t i;for(i=0;i<bytes;++i)if(v[i])return 0;return 1;}
static int valid(const struct pt_prepared_paula_mutation_barrier *b)
{
    return object(b,sizeof(*b),ALIGNMENT(struct pt_prepared_paula_mutation_barrier))&&
        b->self==b&&b->capacity>=sizeof(*b)&&
        object(b,b->capacity,ALIGNMENT(struct pt_prepared_paula_mutation_barrier));
}
static void classify(struct pt_prepared_paula_mutation_barrier *b,
    enum pt_prepared_paula_sources_result r)
{
    if(b->failure==PT_PREPARED_PAULA_SOURCES_OK&&r!=PT_PREPARED_PAULA_SOURCES_OK&&
       r!=PT_PREPARED_PAULA_SOURCES_PENDING&&r!=PT_PREPARED_PAULA_SOURCES_READY&&
       r!=PT_PREPARED_PAULA_SOURCES_CLOSED)b->failure=r;
}
static enum pt_prepared_paula_mutation_result hold(struct pt_prepared_paula_mutation_barrier *b)
{b->uncertain=1;b->cancelled=1;classify(b,PT_PREPARED_PAULA_SOURCES_FAULT);return PT_PREPARED_PAULA_MUTATION_HELD;}

/* Only interval topology, never PCM/marker values or an input validator. The
 * caller already established the complete immutable project and real masters. */
static int project_tables(const struct pt_project *p)
{
    size_t events;unsigned i;
    if(!object(p,sizeof(*p),ALIGNMENT(struct pt_project))||!p->channels.count||
       p->channels.count>PT_CHANNEL_LIMIT||!p->sample_count||p->sample_count>PT_PROJECT_SAMPLES||
       !p->pattern_count||p->pattern_count>PT_PROJECT_PATTERNS||!p->order_count||
       p->order_count>PT_PROJECT_ORDERS||p->extension_count>4090)return 0;
    events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    if(!array(p->samples,p->sample_count,sizeof(*p->samples),ALIGNMENT(struct pt_sample))||
       !array(p->events,events,sizeof(*p->events),ALIGNMENT(struct pt_event))||
       !array(p->orders,p->order_count,sizeof(*p->orders),ALIGNMENT(uint16_t))||
       !array(p->extensions,p->extension_count,sizeof(*p->extensions),ALIGNMENT(struct pt_extension)))return 0;
    for(i=0;i<p->sample_count;++i){const struct pt_sample *s=p->samples+i;
        if(!array(s->pcm.data,s->pcm.capacity,sizeof(*s->pcm.data),ALIGNMENT(int32_t))||
           !array(s->slices,s->slice_count,sizeof(*s->slices),ALIGNMENT(uint32_t)))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(!array(p->extensions[i].data,p->extensions[i].length,1,1))return 0;
    return 1;
}
static int cache_apart(const struct pt_sampler_paula *p,const void *out,size_t n)
{
    unsigned i;
    for(i=0;i<PT_CACHE_SLOTS;++i)
        if(!apart(out,n,p->cache.entry[i].data,p->cache.entry[i].bytes))return 0;
    return 1;
}
static int cache_empty(const struct pt_sampler_paula *p)
{
    unsigned i;if(p->cache.bytes)return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i){const struct pt_cache_entry *e=p->cache.entry+i;
        if(e->data||e->bytes||e->pins||e->valid)return 0;
    }
    return 1;
}
static int workflows_clear(const struct pt_editor *e)
{
    const struct pt_editor_workflow *w=&e->workflow;
    return !e->playback.active&&!w->transaction&&!w->busy&&!w->pending_apply&&
        !w->scanning&&!w->resolving&&!w->wave_building&&!w->open_resource&&
        !w->recording_busy&&!w->recording_context;
}
static int ready_hook(void *);
static int attached(const struct pt_prepared_paula_mutation_barrier *b)
{
    const struct pt_editor *e=b->config.editor;
    return object(e,sizeof(*e),ALIGNMENT(struct pt_editor))&&!e->before_change&&
        e->change_ready==ready_hook&&e->before_change_context==b;
}
static int sampler_fixed(const struct pt_prepared_paula_mutation_barrier *b)
{
    const unsigned char *a=(const unsigned char *)&b->config.editor->sampler;
    const unsigned char *s=(const unsigned char *)&b->sampler_header;
    size_t first=offsetof(struct pt_sampler,pending_head);
    size_t end=offsetof(struct pt_sampler,pending_count)+sizeof(b->sampler_header.pending_count);
    /* Only the genuine source pin job changes its original pending list. */
    return first<=end&&end<=sizeof(struct pt_sampler)&&
        !memcmp(a,s,first)&&!memcmp(a+end,s+end,sizeof(struct pt_sampler)-end);
}
static int bindings(const struct pt_prepared_paula_mutation_barrier *b)
{
    struct pt_project header;
    struct pt_editor *e=b->config.editor;struct pt_sampler_paula *p=b->config.bridge;
    if(!attached(b)||e->project!=b->project||e->history.revision!=b->revision||
       !workflows_clear(e)||!sampler_fixed(b))return 0;
    if(!object(b->project,sizeof(*b->project),ALIGNMENT(struct pt_project)))return 0;
    memcpy(&header,&b->project_header,sizeof(header));header.channels.selected=b->project->channels.selected;
    if(!pt_project_snapshot_equal(b->project,&header)||!project_tables(b->project))return 0;
    /* Actual bridge close clears its pointers. Never follow those after its
     * positive result was recorded, including when an outer callback faulted. */
    if(b->flags&BRIDGE_CLOSED)return 1;
    return object(p,sizeof(*p),ALIGNMENT(struct pt_sampler_paula))&&
        p->sampler==&e->sampler&&p->project==b->project&&
        p->version==b->bridge_version&&p->cache.context==b->chip_context&&
        p->cache.allocate==b->chip_allocate&&p->cache.release==b->chip_release&&
        p->cache.budget==b->chip_budget&&pt_sampler_paula_prepared_current(p);
}
static int release_bindings(const struct pt_prepared_paula_mutation_barrier *b)
{
    const struct pt_editor *e=b->config.editor;const struct pt_sampler_paula *p=b->config.bridge;
    if(!attached(b)||e->project!=b->project||e->history.revision!=b->revision||
       !workflows_clear(e)||e->sampler.lifetime_fault||
       memcmp(&e->sampler.allocator,&b->sampler_header.allocator,sizeof(e->sampler.allocator)))return 0;
    if(b->flags&BRIDGE_CLOSED)return 1;
    /* Original captured cleanup does not follow former sample tables/current
     * owners or require old generation/version/route to remain current. */
    return object(p,sizeof(*p),ALIGNMENT(struct pt_sampler_paula))&&
        p->sampler==&e->sampler&&p->project==b->project&&
        p->cache.context==b->chip_context&&p->cache.allocate==b->chip_allocate&&
        p->cache.release==b->chip_release&&p->cache.budget==b->chip_budget;
}
static int contexts_apart(const struct pt_prepared_paula_lifetime_config *q,const void *out,size_t n)
{
    unsigned i;
    for(i=0;i<q->context_count;++i)
        if(!apart(out,n,q->context[i].base,q->context[i].bytes))return 0;
    return 1;
}
static int source_apart(const struct pt_editor *e,const struct pt_sampler_paula *p,
    const void *out,size_t n)
{
    return apart(out,n,e,sizeof(*e))&&apart(out,n,p,sizeof(*p))&&
        pt_sampler_output_disjoint(&e->sampler,out,n)&&
        pt_sampler_lifetime_output_disjoint(&e->sampler,out,n)&&
        pt_sample_usage_output_disjoint(e->project,out,n)&&cache_apart(p,out,n);
}
static int controls_clear(struct pt_prepared_paula_mutation_barrier *b,int disposing)
{
    const struct pt_prepared_paula_mutation_config *c=&b->config;
    const struct pt_sampler_paula *p=c->bridge;unsigned i;
    struct pt_prepared_paula_sources_status status;
    if(disposing?!release_bindings(b):!bindings(b))return 0;
    if(b->flags&BRIDGE_CLOSED)return 1;
    if(!cache_apart(p,b,b->capacity)||!cache_apart(p,c->lifetime,c->lifetime_capacity)||
       !cache_apart(p,c->editor,sizeof(*c->editor)))return 0;
    /* The source's own private pending node legitimately occupies its source
     * workspace. Its original guard owns that interval; do not mistake it for
     * an alias by walking the sampler pending list against its own control. */
    if(!cache_apart(p,c->sources,c->source_capacity))return 0;
    /* Context arenas can overlap other genuinely borrowed inputs, but a NEW
     * derived allocation cannot overwrite any complete callback/control context.
     * Include tails, not merely the context pointer's first byte, before another
     * chunk or disposal can write/free the returned interval. */
    for(i=0;i<c->queue.context_count;++i)
        if(!cache_apart(p,c->queue.context[i].base,c->queue.context[i].bytes))return 0;
    if(disposing)return 1;
    if(!source_apart(c->editor,p,b,b->capacity)||
       !source_apart(c->editor,p,c->lifetime,c->lifetime_capacity))return 0;
    if((b->flags&SOURCE_OPEN)&&!(b->flags&SOURCE_CLOSED)){
        /* Same original child constructed here; this is only fresh metadata
         * currentness, not discovery of binding, transfer or cleanup authority. */
        if(pt_prepared_paula_sources_get(c->sources,&status)!=PT_PREPARED_PAULA_SOURCES_OK||
           status.failure!=PT_PREPARED_PAULA_SOURCES_OK||
           (status.phase!=PT_PREPARED_PAULA_SOURCES_PREPARING&&
            status.phase!=PT_PREPARED_PAULA_SOURCES_PREPARED))return 0;
    }
    return 1;
}
static int ready_hook(void *context)
{
    struct pt_prepared_paula_mutation_barrier *b=context;
    if(!valid(b))return 0;
    b->cancelled=1;
    if(b->busy||b->uncertain||!attached(b)){hold(b);return 0;}
    /* Refusal only. In particular no cleanup on an editor/child callback stack. */
    return 0;
}
static enum pt_prepared_paula_mutation_result enter(struct pt_prepared_paula_mutation_barrier *b,int disposing)
{
    if(!valid(b))return PT_PREPARED_PAULA_MUTATION_INVALID;
    if(b->flags&DETACHED)return PT_PREPARED_PAULA_MUTATION_CLOSED;
    if(b->uncertain)return PT_PREPARED_PAULA_MUTATION_HELD;
    if(b->busy||!controls_clear(b,disposing))return hold(b);
    b->busy=1;return PT_PREPARED_PAULA_MUTATION_OK;
}
static enum pt_prepared_paula_mutation_result finish(struct pt_prepared_paula_mutation_barrier *b,
    enum pt_prepared_paula_mutation_result result)
{
    /* Resource milestones are recorded by the caller BEFORE this check. */
    if(b->uncertain||!controls_clear(b,b->cancelled||(b->flags&BRIDGE_CLOSED)))result=hold(b);
    b->busy=0;return result;
}
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
/* Called only after an actual original lifetime outer call returned, while its
 * complete storage remains live. A scheduled failure is not a release fact. */
static void publication_observe(struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_scheduled_result result=pt_prepared_paula_lifetime_failure(b->config.lifetime);
    if(b->publication_failure==PT_SCHEDULED_OK&&
       result!=PT_SCHEDULED_OK&&result!=PT_SCHEDULED_PENDING)b->publication_failure=result;
}
static int publication_postcheck(struct pt_prepared_paula_mutation_barrier *b)
{
    /* Publication currentness belongs to the genuine stage2 friend. A stale
     * source may still require captured retirement, never a former-table walk. */
    int ok=!b->uncertain&&controls_clear(b,1);
    if(!ok)hold(b);
    b->busy=0;return ok;
}
static enum pt_scheduled_result publication_finish(struct pt_prepared_paula_mutation_barrier *b,
    enum pt_scheduled_result result)
{
    publication_observe(b);
    return publication_postcheck(b)?result:PT_SCHEDULED_BACKEND;
}
#endif
size_t pt_prepared_paula_mutation_barrier_size(void)
{return sizeof(struct pt_prepared_paula_mutation_barrier);}
size_t pt_prepared_paula_mutation_barrier_alignment(void)
{return ALIGNMENT(struct pt_prepared_paula_mutation_barrier);}

enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_begin(
    struct pt_prepared_paula_mutation_barrier *b,size_t capacity,
    const struct pt_prepared_paula_mutation_config *input)
{
    struct pt_prepared_paula_mutation_config c;struct pt_editor *e;
    struct pt_sampler_paula *p;size_t sn=pt_prepared_paula_sources_size();
    size_t ln=pt_prepared_paula_lifetime_size();unsigned i,j;
    if(capacity<sizeof(*b)||!object(b,capacity,ALIGNMENT(struct pt_prepared_paula_mutation_barrier))||
       !object(input,sizeof(*input),ALIGNMENT(struct pt_prepared_paula_mutation_config)))return PT_PREPARED_PAULA_MUTATION_INVALID;
    if(!apart(b,capacity,input,sizeof(*input)))return PT_PREPARED_PAULA_MUTATION_ALIAS;
    if(!zero_prefix(b,sizeof(*b)))return PT_PREPARED_PAULA_MUTATION_BUSY;
    memcpy(&c,input,sizeof(c));e=c.editor;p=c.bridge;
    if(!object(e,sizeof(*e),ALIGNMENT(struct pt_editor))||
       !object(p,sizeof(*p),ALIGNMENT(struct pt_sampler_paula))||
       c.source_capacity<sn||!object(c.sources,c.source_capacity,pt_prepared_paula_sources_alignment())||
       c.lifetime_capacity<ln||!object(c.lifetime,c.lifetime_capacity,pt_prepared_paula_lifetime_alignment()))return PT_PREPARED_PAULA_MUTATION_INVALID;
    if(!apart(b,capacity,e,sizeof(*e))||!apart(b,capacity,p,sizeof(*p))||
       !apart(b,capacity,c.sources,c.source_capacity)||!apart(b,capacity,c.lifetime,c.lifetime_capacity)||
       !apart(c.sources,c.source_capacity,c.lifetime,c.lifetime_capacity)||
       !apart(c.sources,c.source_capacity,input,sizeof(*input))||
       !apart(c.lifetime,c.lifetime_capacity,input,sizeof(*input))||
       !apart(e,sizeof(*e),p,sizeof(*p)))return PT_PREPARED_PAULA_MUTATION_ALIAS;
    if(e->before_change||e->change_ready||e->before_change_context||!workflows_clear(e)||
       e->sampler.pending_head||e->sampler.pending_count)return PT_PREPARED_PAULA_MUTATION_BUSY;
    if(!project_tables(e->project)||p->sampler!=&e->sampler||p->project!=e->project||
       !pt_sampler_paula_prepared_current(p)||!cache_empty(p)||
       !e->sampler.allocator.allocate||!e->sampler.allocator.release||e->sampler.lifetime_fault||
       !p->cache.allocate||!p->cache.release||c.track>=e->project->channels.count||
       e->project->channels.track[c.track].route!=PT_PAULA)return PT_PREPARED_PAULA_MUTATION_INVALID;
    if(!c.queue.allocator.allocate||!c.queue.allocator.release||!c.queue.session||
       !c.queue.grid.generation||c.queue.frame==UINT64_MAX||c.queue.slot>=4||
       !c.queue.period||c.queue.volume>64||c.queue.control_budget<ln||
       c.queue.context_count>PT_PREPARED_PAULA_LIFETIME_CONTEXTS-2)return PT_PREPARED_PAULA_MUTATION_INVALID;
    for(i=0;i<c.queue.context_count;++i)
        if(!object(c.queue.context[i].base,c.queue.context[i].bytes,1))return PT_PREPARED_PAULA_MUTATION_INVALID;
    if(!source_apart(e,p,b,capacity)||!source_apart(e,p,c.sources,c.source_capacity)||
       !source_apart(e,p,c.lifetime,c.lifetime_capacity)||!source_apart(e,p,input,sizeof(*input))||
       !contexts_apart(&c.queue,b,capacity)||!contexts_apart(&c.queue,c.sources,c.source_capacity)||
       !contexts_apart(&c.queue,c.lifetime,c.lifetime_capacity))return PT_PREPARED_PAULA_MUTATION_ALIAS;
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){const struct pt_pcm *pcm;
        if(c.sample[i]>=e->project->sample_count||!e->sampler.current[c.sample[i]])return PT_PREPARED_PAULA_MUTATION_INVALID;
        for(j=0;j<i;++j)if(c.sample[j]==c.sample[i])return PT_PREPARED_PAULA_MUTATION_INVALID;
        pcm=&e->project->samples[c.sample[i]].pcm;
        if(!pcm->frames||pcm->channels!=1||pt_pcm_shape(pcm)!=PT_PCM_OK||
           (pcm->bits!=8&&pcm->bits!=16&&pcm->bits!=24))return PT_PREPARED_PAULA_MUTATION_INVALID;
    }
    if(!zero_prefix(c.sources,sn)||!zero_prefix(c.lifetime,ln))return PT_PREPARED_PAULA_MUTATION_BUSY;
    c.queue.context[c.queue.context_count++]=(struct pt_prepared_paula_context_extent){b,capacity};
    c.queue.context[c.queue.context_count++]=(struct pt_prepared_paula_context_extent){e,sizeof(*e)};
    b->self=b;b->capacity=capacity;memcpy(&b->config,&c,sizeof(c));
    b->project=e->project;memcpy(&b->project_header,e->project,sizeof(b->project_header));
    memcpy(&b->sampler_header,&e->sampler,sizeof(b->sampler_header));
    b->chip_context=p->cache.context;b->chip_allocate=p->cache.allocate;
    b->chip_release=p->cache.release;b->chip_budget=p->cache.budget;
    b->bridge_version=p->version;b->revision=e->history.revision;
    /* Pure setter, after exact no-hook admission; no allocator/callback can run
     * between the check and this installation on the original serialized TASK. */
    if(!pt_editor_change_barrier(e,ready_hook,b)){memset(b,0,sizeof(*b));return PT_PREPARED_PAULA_MUTATION_BUSY;}
    if(!attached(b))return hold(b);
    return PT_PREPARED_PAULA_MUTATION_OK;
}

enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_step(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_prepared_paula_mutation_result r;
    enum pt_prepared_paula_sources_result child;unsigned f;
    if(!valid(b))return PT_PREPARED_PAULA_MUTATION_INVALID;
    r=enter(b,b->cancelled||(b->flags&BRIDGE_CLOSED)
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
        ||(b->flags&PUBLICATION_ATTEMPT)
#endif
    );
    if(r!=PT_PREPARED_PAULA_MUTATION_OK)return r;
    f=b->flags;
    if(f&BRIDGE_CLOSED)return finish(b,PT_PREPARED_PAULA_MUTATION_CLEANED);
    if(b->cancelled){
        if((f&QUEUE_OPEN)&&!(f&QUEUE_CLOSED)){
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
            if(f&PUBLICATION_ATTEMPT){
                if(!(f&PUBLICATION_STOP_ATTEMPT)){
                    enum pt_scheduled_result stopped;
                    b->flags|=PUBLICATION_STOP_ATTEMPT;
                    stopped=pt_prepared_paula_lifetime_stop_publication(b->config.lifetime);
                    /* Only outer-return provenance, not admission-closed,
                     * domain-consumed, reader-ended or quiet authority. */
                    b->flags|=PUBLICATION_STOP_RETURNED;
                    publication_observe(b);
                    if(stopped!=PT_SCHEDULED_OK&&stopped!=PT_SCHEDULED_PENDING&&
                       stopped!=PT_SCHEDULED_BACKEND){r=hold(b);goto done;}
                }else if(!(f&PUBLICATION_STOP_RETURNED)){
                    r=hold(b);goto done;
                }else if(!(f&DRAINED)){
                    child=pt_prepared_paula_lifetime_drain_step(b->config.lifetime);
                    publication_observe(b);
                    if(child==PT_PREPARED_PAULA_SOURCES_BUSY){r=PT_PREPARED_PAULA_MUTATION_BUSY;goto done;}
                    if(child==PT_PREPARED_PAULA_SOURCES_CLOSED)b->flags|=STOPPED|DRAINED|SOURCE_CLOSED;
                    else if(child==PT_PREPARED_PAULA_SOURCES_PENDING)b->flags|=STOPPED;
                    else {classify(b,child);r=hold(b);goto done;}
                }else{
                    child=pt_prepared_paula_lifetime_close(b->config.lifetime);
                    publication_observe(b);
                    if(child==PT_PREPARED_PAULA_SOURCES_CLOSED)b->flags|=QUEUE_CLOSED;
                    else {classify(b,child);r=hold(b);goto done;}
                }
                r=PT_PREPARED_PAULA_MUTATION_PENDING;goto done;
            }
#endif
            if(f&TRANSFERRED){
                if(!(f&STOPPED)){
                    if(f&STOP_ATTEMPT){r=hold(b);goto done;}
                    b->flags|=STOP_ATTEMPT;
                    child=pt_prepared_paula_lifetime_stop_local(b->config.lifetime);
                    if(child==PT_PREPARED_PAULA_SOURCES_OK)b->flags|=STOPPED;
                    else {classify(b,child);r=hold(b);goto done;}
                }else if(!(f&DRAINED)){
                    child=pt_prepared_paula_lifetime_drain_step(b->config.lifetime);
                    if(child==PT_PREPARED_PAULA_SOURCES_CLOSED)b->flags|=DRAINED|SOURCE_CLOSED;
                    else if(child!=PT_PREPARED_PAULA_SOURCES_PENDING){classify(b,child);r=hold(b);goto done;}
                }else{
                    child=pt_prepared_paula_lifetime_close(b->config.lifetime);
                    if(child==PT_PREPARED_PAULA_SOURCES_CLOSED)b->flags|=QUEUE_CLOSED;
                    else {classify(b,child);r=hold(b);goto done;}
                }
            }else{
                child=pt_prepared_paula_lifetime_close(b->config.lifetime);
                if(child==PT_PREPARED_PAULA_SOURCES_CLOSED)b->flags|=QUEUE_CLOSED;
                else {classify(b,child);r=hold(b);goto done;}
            }
        }else if((f&SOURCE_OPEN)&&!(f&SOURCE_CLOSED)){
            child=pt_prepared_paula_sources_cancel_step(b->config.sources);
            if(child==PT_PREPARED_PAULA_SOURCES_CLOSED)b->flags|=SOURCE_CLOSED;
            else if(child!=PT_PREPARED_PAULA_SOURCES_PENDING){classify(b,child);r=hold(b);goto done;}
        }else{
            if(f&BRIDGE_ATTEMPT){r=hold(b);goto done;}
            b->flags|=BRIDGE_ATTEMPT;
            if(pt_sampler_paula_close(b->config.bridge))b->flags|=BRIDGE_CLOSED;
            else {r=hold(b);goto done;}
            r=PT_PREPARED_PAULA_MUTATION_CLEANED;goto done;
        }
        r=PT_PREPARED_PAULA_MUTATION_PENDING;goto done;
    }
    if(!(f&SOURCE_OPEN)){
        if(f&SOURCE_ATTEMPT){r=hold(b);goto done;}
        b->flags|=SOURCE_ATTEMPT;
        child=pt_prepared_paula_sources_init(b->config.sources,b->config.source_capacity,
            b->config.bridge,b->config.track,b->config.sample);
        if(child==PT_PREPARED_PAULA_SOURCES_OK)b->flags|=SOURCE_OPEN;
        else if(child==PT_PREPARED_PAULA_SOURCES_INVALID||child==PT_PREPARED_PAULA_SOURCES_ALIAS||
                child==PT_PREPARED_PAULA_SOURCES_STALE){
            /* That actual constructor has no child/callback/resource action
             * on refusal. Do not call/cancel an uninitialized source object. */
            classify(b,child);b->cancelled=1;
        }else {classify(b,child);r=hold(b);goto done;}
    }else if(!(f&SOURCE_READY)){
        child=pt_prepared_paula_sources_step(b->config.sources);
        if(child==PT_PREPARED_PAULA_SOURCES_READY)b->flags|=SOURCE_READY;
        else if(child==PT_PREPARED_PAULA_SOURCES_CAPACITY||child==PT_PREPARED_PAULA_SOURCES_STALE){
            classify(b,child);b->cancelled=1;
        }else if(child!=PT_PREPARED_PAULA_SOURCES_OK&&child!=PT_PREPARED_PAULA_SOURCES_PENDING){
            classify(b,child);r=hold(b);goto done;
        }
    }else if(!(f&QUEUE_OPEN)){
        if(f&QUEUE_ATTEMPT){r=hold(b);goto done;}
        b->flags|=QUEUE_ATTEMPT;
        child=pt_prepared_paula_lifetime_init(b->config.lifetime,b->config.lifetime_capacity,
            b->config.sources,&b->config.queue,&b->config.backend);
        if(child==PT_PREPARED_PAULA_SOURCES_OK)b->flags|=QUEUE_OPEN;
        else {classify(b,child);r=hold(b);goto done;}
    }else{
        r=f&TRANSFERRED?PT_PREPARED_PAULA_MUTATION_QUEUED:PT_PREPARED_PAULA_MUTATION_READY;goto done;
    }
    r=(b->flags&QUEUE_OPEN)?PT_PREPARED_PAULA_MUTATION_READY:PT_PREPARED_PAULA_MUTATION_PENDING;
done:
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
    if(b->flags&PUBLICATION_ATTEMPT)return publication_postcheck(b)?r:PT_PREPARED_PAULA_MUTATION_HELD;
#endif
    return finish(b,r);
}

enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_enqueue_once(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_prepared_paula_mutation_result r;
    enum pt_prepared_paula_sources_result child;
    if(!valid(b))return PT_PREPARED_PAULA_MUTATION_INVALID;
    if(b->flags&DETACHED)return PT_PREPARED_PAULA_MUTATION_CLOSED;
    if(b->uncertain)return PT_PREPARED_PAULA_MUTATION_HELD;
    if(b->busy)return hold(b);
    /* Closed admission needs no former source/current-table walk. In
     * particular never query a completed child to decide whether to replay. */
    if(b->cancelled||!(b->flags&QUEUE_OPEN)||(b->flags&(ENQUEUE_ATTEMPT|QUEUE_CLOSED|BRIDGE_CLOSED)))
        return PT_PREPARED_PAULA_MUTATION_BUSY;
    r=enter(b,0);
    if(r!=PT_PREPARED_PAULA_MUTATION_OK)return r;
    b->flags|=ENQUEUE_ATTEMPT;
    child=pt_prepared_paula_lifetime_enqueue_unpublished(b->config.lifetime);
    if(child==PT_PREPARED_PAULA_SOURCES_OK){b->flags|=TRANSFERRED;r=PT_PREPARED_PAULA_MUTATION_QUEUED;}
    else {classify(b,child);r=hold(b);}
    return finish(b,r);
}
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_cancel(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_prepared_paula_mutation_result r=enter(b,1);
    if(r!=PT_PREPARED_PAULA_MUTATION_OK)return r;
    b->cancelled=1;
    return finish(b,b->flags&BRIDGE_CLOSED?PT_PREPARED_PAULA_MUTATION_CLEANED:PT_PREPARED_PAULA_MUTATION_PENDING);
}
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
enum pt_scheduled_result pt_prepared_paula_mutation_barrier_publish_once(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_scheduled_result result;
    if(!valid(b)||(b->flags&DETACHED))return PT_SCHEDULED_INVALID;
    if(b->uncertain)return PT_SCHEDULED_BACKEND;
    if(b->busy){hold(b);return PT_SCHEDULED_BACKEND;}
    if(b->cancelled||!(b->flags&QUEUE_OPEN)||!(b->flags&TRANSFERRED)||
       (b->flags&(PUBLICATION_ATTEMPT|STOP_ATTEMPT|QUEUE_CLOSED|BRIDGE_CLOSED)))return PT_SCHEDULED_INVALID;
    if(enter(b,1)!=PT_PREPARED_PAULA_MUTATION_OK)return PT_SCHEDULED_BACKEND;
    /* Irrevocable before the real call, including failed publication/reentry.
     * No result can route this original transfer back through stop_local. */
    b->flags|=PUBLICATION_ATTEMPT;
    result=pt_prepared_paula_lifetime_publish_once(b->config.lifetime);
    if(result!=PT_SCHEDULED_OK)b->cancelled=1;
    if(result==PT_SCHEDULED_INVALID)hold(b);
    return publication_finish(b,result);
}
enum pt_scheduled_result pt_prepared_paula_mutation_barrier_cancel_command_once(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_scheduled_result result;
    if(!valid(b)||(b->flags&DETACHED))return PT_SCHEDULED_INVALID;
    if(b->uncertain)return PT_SCHEDULED_BACKEND;
    if(b->busy){hold(b);return PT_SCHEDULED_BACKEND;}
    if(!(b->flags&PUBLICATION_ATTEMPT)||!(b->flags&PUBLICATION_STOP_RETURNED)||
       !b->cancelled||!(b->flags&QUEUE_OPEN)||!(b->flags&TRANSFERRED)||
       (b->flags&(STOPPED|DRAINED|QUEUE_CLOSED|BRIDGE_CLOSED)))return PT_SCHEDULED_INVALID;
    if(enter(b,1)!=PT_PREPARED_PAULA_MUTATION_OK)return PT_SCHEDULED_BACKEND;
    result=pt_prepared_paula_lifetime_cancel_command_once(b->config.lifetime);
    return publication_finish(b,result);
}
enum pt_scheduled_result pt_prepared_paula_mutation_barrier_cancel_reader_once(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_scheduled_result result;
    if(!valid(b)||(b->flags&DETACHED))return PT_SCHEDULED_INVALID;
    if(b->uncertain)return PT_SCHEDULED_BACKEND;
    if(b->busy){hold(b);return PT_SCHEDULED_BACKEND;}
    if(!(b->flags&PUBLICATION_ATTEMPT)||!(b->flags&PUBLICATION_STOP_RETURNED)||
       !b->cancelled||!(b->flags&QUEUE_OPEN)||!(b->flags&TRANSFERRED)||
       (b->flags&(STOPPED|DRAINED|QUEUE_CLOSED|BRIDGE_CLOSED)))return PT_SCHEDULED_INVALID;
    if(enter(b,1)!=PT_PREPARED_PAULA_MUTATION_OK)return PT_SCHEDULED_BACKEND;
    result=pt_prepared_paula_lifetime_cancel_reader_once(b->config.lifetime);
    return publication_finish(b,result);
}
enum pt_scheduled_result pt_prepared_paula_mutation_barrier_publication_failure(
    const struct pt_prepared_paula_mutation_barrier *b)
{
    if(!valid(b))return PT_SCHEDULED_INVALID;
    return b->publication_failure;
}
#endif
enum pt_prepared_paula_mutation_phase pt_prepared_paula_mutation_barrier_state(
    const struct pt_prepared_paula_mutation_barrier *b)
{
    if(!valid(b))return (enum pt_prepared_paula_mutation_phase)0;
    if(b->uncertain)return PT_PREPARED_PAULA_MUTATION_UNCERTAIN;
    if(b->flags&DETACHED)return PT_PREPARED_PAULA_MUTATION_FINISHED;
    if(b->flags&BRIDGE_CLOSED)return PT_PREPARED_PAULA_MUTATION_DETACHABLE;
    if(b->flags&STOPPED)return PT_PREPARED_PAULA_MUTATION_STOPPED;
    if(b->cancelled)return PT_PREPARED_PAULA_MUTATION_DISPOSING;
    if(b->flags&TRANSFERRED)return PT_PREPARED_PAULA_MUTATION_RETAINED;
    if(b->flags&QUEUE_OPEN)return PT_PREPARED_PAULA_MUTATION_EMPTY;
    if(b->flags&SOURCE_OPEN)return PT_PREPARED_PAULA_MUTATION_PREPARING;
    return PT_PREPARED_PAULA_MUTATION_ATTACHED;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_mutation_barrier_failure(
    const struct pt_prepared_paula_mutation_barrier *b)
{return valid(b)?b->failure:PT_PREPARED_PAULA_SOURCES_INVALID;}
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_detach(
    struct pt_prepared_paula_mutation_barrier *b)
{
    enum pt_prepared_paula_mutation_result r=enter(b,1);struct pt_editor *e;
    if(r!=PT_PREPARED_PAULA_MUTATION_OK)return r;
    if(!(b->flags&BRIDGE_CLOSED))return finish(b,PT_PREPARED_PAULA_MUTATION_BUSY);
    e=b->config.editor;
    /* No child/callback between exact authentication and the original setter. */
    if(!attached(b)||!pt_editor_change_barrier(e,NULL,NULL)||
       e->before_change||e->change_ready||e->before_change_context){r=hold(b);b->busy=0;return r;}
    b->flags|=DETACHED;
    memset(&b->config,0,sizeof(b->config));b->project=NULL;
    memset(&b->project_header,0,sizeof(b->project_header));
    memset(&b->sampler_header,0,sizeof(b->sampler_header));
    b->chip_context=NULL;b->chip_allocate=NULL;b->chip_release=NULL;
    b->chip_budget=0;b->bridge_version=0;b->busy=0;
    return PT_PREPARED_PAULA_MUTATION_CLOSED;
}
