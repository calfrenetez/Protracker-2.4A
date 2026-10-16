#include "prepared_paula_sources.h"
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The prepared-source custody opt-in is HOST-only"
#endif
#define PT_PREPARED_PAULA_HOST_LIFETIME 1
#include "prepared_paula_lifetime.h"
#endif
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "project_snapshot.h"
#include "../core/sample_usage.h"
#include "../core/pcm_internal.h"
#include <stdint.h>
#include <string.h>

#define ALIGNMENT(T) offsetof(struct {char byte;T value;},value)
enum {PIN_BEGIN,PIN_STEP,CHIP_BEGIN,CHIP_STEP};
struct prepared_source {
    unsigned sample,leased,lease_release_attempted;
    struct pt_sample original;
    struct pt_sample_version *current,*pin;
    struct pt_pcm pcm;
    struct pt_cache_lease lease;
    const uint8_t *cache_data;
    size_t cache_bytes;
};
struct pt_prepared_paula_sources {
    struct pt_prepared_paula_sources *self;
    size_t capacity;
    struct pt_sampler_paula *bridge;
    struct pt_sampler *sampler;
    struct pt_project *project,header;
    struct pt_sampler sampler_header;
    void *chip_context;
    void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
    size_t chip_budget;
    uint64_t cache_version;
    unsigned generation,track,index,child_phase,busy,cancel_index;
    unsigned pin_cancel_attempted,chip_cancel_attempted;
#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
    struct pt_prepared_paula_lifetime *custodian;unsigned custody_stage;
#endif
    enum pt_prepared_paula_sources_phase phase;
    enum pt_prepared_paula_sources_result failure;
    struct prepared_source source[PT_PREPARED_PAULA_SOURCES_COUNT];
    struct pt_sampler_pin_job pin_job;
    struct pt_sampler_paula_job chip_job;
};

static int object(const void *p,size_t n,size_t alignment)
{
    uintptr_t a=(uintptr_t)p;
    return p&&n&&alignment&&a%alignment==0&&n<=UINTPTR_MAX-a;
}
static int apart(const void *a,size_t n,const void *b,size_t m)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return (!n||object(a,n,1))&&(!m||object(b,m,1))&&
        (!n||!m||x+n<=y||y+m<=x);
}
static int table(const void *p,size_t count,size_t width,size_t alignment)
{return width&&count<=SIZE_MAX/width&&(!count||object(p,count*width,alignment));}
static int pcm_equal(const struct pt_pcm *a,const struct pt_pcm *b)
{return a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&
    a->rate==b->rate&&a->channels==b->channels&&a->bits==b->bits;}
static int project_tables(const struct pt_project *p)
{
    size_t events;
    if(!p->sample_count||p->sample_count>PT_PROJECT_SAMPLES||
       !p->pattern_count||p->pattern_count>PT_PROJECT_PATTERNS||
       !p->order_count||p->order_count>PT_PROJECT_ORDERS||
       !p->channels.count||p->channels.count>PT_CHANNEL_LIMIT||
       p->extension_count>4090)return 0;
    events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    return table(p->samples,p->sample_count,sizeof(*p->samples),ALIGNMENT(struct pt_sample))&&
        table(p->events,events,sizeof(*p->events),ALIGNMENT(struct pt_event))&&
        table(p->orders,p->order_count,sizeof(*p->orders),ALIGNMENT(uint16_t))&&
        table(p->extensions,p->extension_count,sizeof(*p->extensions),ALIGNMENT(struct pt_extension));
}
/* Metadata interval guard only; this neither acquires nor directly operates
 * the embedded cache. Include unselected and retired allocations as well. */
static int cache_apart(const struct pt_sampler_paula *b,const void *out,size_t n)
{
    unsigned i;
    for(i=0;i<PT_CACHE_SLOTS;++i)
        if(!apart(out,n,b->cache.entry[i].data,b->cache.entry[i].bytes))return 0;
    return 1;
}
static int valid(const struct pt_prepared_paula_sources *p)
{
    return object(p,sizeof(*p),ALIGNMENT(struct pt_prepared_paula_sources))&&
        p->self==p&&p->capacity>=sizeof(*p)&&
        object(p,p->capacity,ALIGNMENT(struct pt_prepared_paula_sources))&&
        p->phase>=PT_PREPARED_PAULA_SOURCES_PREPARING&&
        p->phase<=PT_PREPARED_PAULA_SOURCES_HELD&&
        p->index<=PT_PREPARED_PAULA_SOURCES_COUNT;
}
static enum pt_prepared_paula_sources_result hold(struct pt_prepared_paula_sources *p)
{p->failure=PT_PREPARED_PAULA_SOURCES_FAULT;p->phase=PT_PREPARED_PAULA_SOURCES_HELD;return p->failure;}
static enum pt_prepared_paula_sources_result fail(struct pt_prepared_paula_sources *p,
    enum pt_prepared_paula_sources_result result)
{if(p->failure==PT_PREPARED_PAULA_SOURCES_OK)p->failure=result;return p->failure;}
static int enter(struct pt_prepared_paula_sources *p)
{if(p->busy){hold(p);return 0;}p->busy=1;return 1;}
/* Only the original pin-job pending-head/count may change in the sampler
 * header. No master is allocated/promoted and no allocator is replaced here. */
static int sampler_fixed(const struct pt_prepared_paula_sources *p)
{
    const unsigned char *a=(const unsigned char *)p->sampler;
    const unsigned char *b=(const unsigned char *)&p->sampler_header;
    size_t start=offsetof(struct pt_sampler,pending_head);
    size_t end=offsetof(struct pt_sampler,pending_count)+sizeof(p->sampler->pending_count);
    return start<=end&&end<=sizeof(*p->sampler)&&
        !memcmp(a,b,start)&&!memcmp(a+end,b+end,sizeof(*p->sampler)-end);
}
static int bindings_current(const struct pt_prepared_paula_sources *p)
{
    return p->bridge->sampler==p->sampler&&p->bridge->project==p->project&&
        !memcmp(&p->sampler->allocator,&p->sampler_header.allocator,
                sizeof(p->sampler->allocator))&&
        p->bridge->cache.context==p->chip_context&&
        p->bridge->cache.allocate==p->chip_allocate&&
        p->bridge->cache.release==p->chip_release&&
        p->bridge->cache.budget==p->chip_budget&&!p->sampler->lifetime_fault;
}
static int fixed_current(const struct pt_prepared_paula_sources *p)
{
    struct pt_project header;unsigned i;
    if(!bindings_current(p)||!sampler_fixed(p)||
       p->sampler->generation!=p->generation||p->bridge->version!=p->cache_version||
       !pt_sampler_paula_prepared_current(p->bridge))return 0;
    memcpy(&header,&p->header,sizeof(header));header.channels.selected=p->project->channels.selected;
    if(!pt_project_snapshot_equal(p->project,&header))return 0;
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){const struct prepared_source *s=p->source+i;
        if(p->sampler->current[s->sample]!=s->current||
           memcmp(p->project->samples+s->sample,&s->original,sizeof(s->original))||
           (s->pin&&(s->pin!=s->current||!pcm_equal(&s->pcm,&s->original.pcm))))return 0;
    }
    return 1;
}
static int pending_current(const struct pt_prepared_paula_sources *p)
{
    const struct pt_sampler_pending *n=&p->pin_job.reservation;
    if(!n->owner)return !p->pin_job.owner&&!p->pin_job.value&&
        p->sampler->pending_head==p->sampler_header.pending_head&&
        p->sampler->pending_count==p->sampler_header.pending_count;
    if(p->index>=PT_PREPARED_PAULA_SOURCES_COUNT||
       p->sampler_header.pending_count==SIZE_MAX||
       p->sampler->pending_count!=p->sampler_header.pending_count+1||
       p->sampler->pending_head!=n||n->previous||
       n->next!=p->sampler_header.pending_head||
       (n->next&&n->next->previous!=n))return 0;
    return pt_sampler_pin_job_inventory_current(&p->pin_job,p->sampler,p->project,
        p->generation,p->source[p->index].sample)&&p->pin_job.value==p->pin_job.previous&&
        p->pin_job.previous==p->source[p->index].current;
}
static int held_current(struct pt_prepared_paula_sources *p,unsigned capture)
{
    unsigned i;const uint8_t *data;size_t bytes;
    if(!fixed_current(p)||!pending_current(p))return 0;
    /* No callback/retain/release intervenes between prepared_current above and
     * these exact representation-key/serial/pin/data observations. */
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){struct prepared_source *s=p->source+i;
        if(!s->leased)continue;
        data=NULL;bytes=0;
        if(!s->pin||!pt_sampler_paula_prepared_source_location_validated(p->bridge,
           p->track,s->sample,0,s->lease,&data,&bytes))return 0;
        if(!object(data,bytes,2)||!apart(data,bytes,p,p->capacity)||
           !apart(data,bytes,p->bridge,sizeof(*p->bridge))||
           !pt_sample_usage_output_disjoint(p->project,data,bytes)||
           !pt_sampler_output_disjoint(p->sampler,data,bytes))return 0;
        if(s->cache_data){if(s->cache_data!=data||s->cache_bytes!=bytes)return 0;}
        else if(capture){s->cache_data=data;s->cache_bytes=bytes;}
        else return 0;
    }
    return 1;
}
/* Before the first copy, guard the genuine unpublished child's interval. No
 * pointer escapes, no valid bit is changed, and no borrowed declaration acts
 * as a cache/pin provenance certificate. A known alias is retained, not freed. */
static int pending_chip_clear(const struct pt_prepared_paula_sources *p)
{
    const struct pt_sampler_paula_job *j=&p->chip_job;
    const struct pt_cache_entry *e;unsigned i;
    if(!j->owner)return 1;
    if(j->owner!=p->bridge||j->sampler!=p->sampler||j->project!=p->project||
       j->upload.cache!=&p->bridge->cache||j->upload.lease.slot>=PT_CACHE_SLOTS)return 0;
    e=p->bridge->cache.entry+j->upload.lease.slot;
    if(!e->pins||e->serial!=j->upload.lease.serial||e->valid||
       e->bytes!=j->upload.bytes||!object(e->data,e->bytes,2)||
       !apart(e->data,e->bytes,p,p->capacity)||
       !apart(e->data,e->bytes,p->bridge,sizeof(*p->bridge))||
       !pt_sample_usage_output_disjoint(p->project,e->data,e->bytes)||
       !pt_sampler_output_disjoint(p->sampler,e->data,e->bytes))return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(i!=j->upload.lease.slot&&
       !apart(e->data,e->bytes,p->bridge->cache.entry[i].data,p->bridge->cache.entry[i].bytes))return 0;
    return 1;
}
#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
static int custody_output_clear(const struct pt_prepared_paula_sources *p,const void *out,size_t n)
{return !p->custodian||pt_prepared_paula_lifetime_output_disjoint(p->custodian,out,n);}
#endif
static int output_clear(struct pt_prepared_paula_sources *p,const void *out,size_t n)
{
    unsigned i;
    if(!object(out,n,ALIGNMENT(struct pt_prepared_paula_sources_status))||
       !apart(out,n,p,p->capacity)
#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
       ||!custody_output_clear(p,out,n)
#endif
       )return 0;
    if(p->phase==PT_PREPARED_PAULA_SOURCES_FINISHED)return 1;
    if(!apart(out,n,p->bridge,sizeof(*p->bridge))||
       !apart(out,n,p->sampler,sizeof(*p->sampler))||
       !apart(out,n,p->project,sizeof(*p->project))||
       !pt_sampler_output_disjoint(p->sampler,out,n)||
       !pt_sampler_lifetime_output_disjoint(p->sampler,out,n)||
       !pt_sample_usage_output_disjoint(p->project,out,n)||
       !cache_apart(p->bridge,out,n))return 0;
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i)
        if(!pt_sampler_version_output_disjoint(p->source[i].pin,out,n))return 0;
    return 1;
}

size_t pt_prepared_paula_sources_size(void)
{return sizeof(struct pt_prepared_paula_sources);}
size_t pt_prepared_paula_sources_alignment(void)
{return ALIGNMENT(struct pt_prepared_paula_sources);}

enum pt_prepared_paula_sources_result pt_prepared_paula_sources_init(
    struct pt_prepared_paula_sources *p,size_t capacity,struct pt_sampler_paula *b,
    unsigned track,const unsigned sample[PT_PREPARED_PAULA_SOURCES_COUNT])
{
    struct pt_sampler *s;struct pt_project *project;unsigned i,j;
    const unsigned char *bytes=(const unsigned char *)p;
    if(capacity<sizeof(*p)||!object(p,capacity,ALIGNMENT(struct pt_prepared_paula_sources))||
       !object(b,sizeof(*b),ALIGNMENT(struct pt_sampler_paula))||
       !object(sample,PT_PREPARED_PAULA_SOURCES_COUNT*sizeof(*sample),ALIGNMENT(unsigned)))
        return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(!apart(p,capacity,b,sizeof(*b))||
       !apart(p,capacity,sample,PT_PREPARED_PAULA_SOURCES_COUNT*sizeof(*sample)))
        return PT_PREPARED_PAULA_SOURCES_ALIAS;
    s=b->sampler;project=b->project;
    if(!object(s,sizeof(*s),ALIGNMENT(struct pt_sampler))||
       !object(project,sizeof(*project),ALIGNMENT(struct pt_project))||
       !apart(s,sizeof(*s),project,sizeof(*project))||!project_tables(project))
        return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(!apart(p,capacity,s,sizeof(*s))||!apart(p,capacity,project,sizeof(*project))||
       !pt_sampler_output_disjoint(s,p,capacity)||
       !pt_sampler_lifetime_output_disjoint(s,p,capacity)||
       !pt_sample_usage_output_disjoint(project,p,capacity)||!cache_apart(b,p,capacity))
        return PT_PREPARED_PAULA_SOURCES_ALIAS;
    if(!s->allocator.allocate||!s->allocator.release||s->lifetime_fault||
       !b->cache.allocate||!b->cache.release||!pt_sampler_paula_prepared_current(b)||
       track>=project->channels.count||project->channels.track[track].route!=PT_PAULA)
        return PT_PREPARED_PAULA_SOURCES_INVALID;
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){const struct pt_pcm *pcm;
        if(sample[i]>=project->sample_count||!s->current[sample[i]])
            return PT_PREPARED_PAULA_SOURCES_STALE;
        for(j=0;j<i;++j)if(sample[i]==sample[j])return PT_PREPARED_PAULA_SOURCES_INVALID;
        pcm=&project->samples[sample[i]].pcm;
        if(!pcm->frames||pcm->channels!=1||pt_pcm_shape(pcm)!=PT_PCM_OK||
           (pcm->bits!=8&&pcm->bits!=16&&pcm->bits!=24))return PT_PREPARED_PAULA_SOURCES_INVALID;
    }
    for(i=0;i<sizeof(*p);++i)if(bytes[i])return PT_PREPARED_PAULA_SOURCES_INVALID;
    p->self=p;p->capacity=capacity;p->bridge=b;p->sampler=s;p->project=project;
    memcpy(&p->header,project,sizeof(p->header));memcpy(&p->sampler_header,s,sizeof(*s));
    p->chip_context=b->cache.context;p->chip_allocate=b->cache.allocate;
    p->chip_release=b->cache.release;p->chip_budget=b->cache.budget;
    p->cache_version=b->version;p->generation=s->generation;p->track=track;
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){struct prepared_source *d=p->source+i;
        d->sample=sample[i];d->current=s->current[sample[i]];
        memcpy(&d->original,project->samples+sample[i],sizeof(d->original));
    }
    p->phase=PT_PREPARED_PAULA_SOURCES_PREPARING;
    return PT_PREPARED_PAULA_SOURCES_OK;
}

enum pt_prepared_paula_sources_result pt_prepared_paula_sources_step(
    struct pt_prepared_paula_sources *p)
{
    struct prepared_source *s;struct pt_pcm pcm;struct pt_sample_version *pin=NULL;
    struct pt_cache_lease lease={0,0};unsigned ready=0;
    enum pt_edit_result edit;enum pt_cache_result cache;
    enum pt_prepared_paula_sources_result result=PT_PREPARED_PAULA_SOURCES_PENDING;
    if(!valid(p))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(!enter(p))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD){result=PT_PREPARED_PAULA_SOURCES_FAULT;goto done;}
    if(p->failure!=PT_PREPARED_PAULA_SOURCES_OK){result=p->failure;goto done;}
    if(p->phase!=PT_PREPARED_PAULA_SOURCES_PREPARING&&
       p->phase!=PT_PREPARED_PAULA_SOURCES_PREPARED){result=PT_PREPARED_PAULA_SOURCES_INVALID;goto done;}
    if(!held_current(p,0)){result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);goto done;}
    if(!pending_chip_clear(p)){result=hold(p);goto done;}
    if(p->phase==PT_PREPARED_PAULA_SOURCES_PREPARED){result=PT_PREPARED_PAULA_SOURCES_READY;goto done;}
    if(p->index>=PT_PREPARED_PAULA_SOURCES_COUNT){result=hold(p);goto done;}
    s=p->source+p->index;
    if(p->child_phase==PIN_BEGIN){
        edit=pt_sampler_pin_job_begin(&p->pin_job,p->sampler,p->project,s->sample,p->generation);
        if(edit==PT_EDIT_OK)p->child_phase=PIN_STEP;
        else result=fail(p,edit==PT_EDIT_CAPACITY?PT_PREPARED_PAULA_SOURCES_CAPACITY:PT_PREPARED_PAULA_SOURCES_STALE);
    }else if(p->child_phase==PIN_STEP){
        memset(&pcm,0,sizeof(pcm));
        edit=pt_sampler_pin_job_step(&p->pin_job,PT_SAMPLER_PIN_CHUNK,&pcm,&pin,&ready);
        if(edit==PT_EDIT_OK&&ready){
            /* Record actual transfer even if a forbidden callback already
             * latched HELD. Never erase this genuine acquired reference. */
            s->pin=pin;memcpy(&s->pcm,&pcm,sizeof(pcm));p->child_phase=CHIP_BEGIN;
        }else if(edit!=PT_EDIT_OK||!ready)result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);
    }else if(p->child_phase==CHIP_BEGIN){
        cache=pt_sampler_paula_job_begin(&p->chip_job,p->bridge,p->track,s->sample,0,s->pin,&lease);
        if(cache==PT_CACHE_HIT){s->lease=lease;s->leased=1;}
        else if(cache==PT_CACHE_PENDING)p->child_phase=CHIP_STEP;
        else result=fail(p,cache==PT_CACHE_CAPACITY||cache==PT_CACHE_BUSY?
            PT_PREPARED_PAULA_SOURCES_CAPACITY:PT_PREPARED_PAULA_SOURCES_STALE);
    }else if(p->child_phase==CHIP_STEP){
        cache=pt_sampler_paula_job_step(&p->chip_job,&lease);
        if(cache==PT_CACHE_LOAD){s->lease=lease;s->leased=1;}
        else if(cache!=PT_CACHE_PENDING)result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);
    }else result=hold(p);
    if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD){result=PT_PREPARED_PAULA_SOURCES_FAULT;goto done;}
    if(!held_current(p,1)){result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);goto done;}
    if(!pending_chip_clear(p)){result=hold(p);goto done;}
    if(p->failure!=PT_PREPARED_PAULA_SOURCES_OK){result=p->failure;goto done;}
    if(s->leased){++p->index;p->child_phase=PIN_BEGIN;}
    if(p->index==PT_PREPARED_PAULA_SOURCES_COUNT){
        if(!held_current(p,0)){result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);goto done;}
        p->phase=PT_PREPARED_PAULA_SOURCES_PREPARED;result=PT_PREPARED_PAULA_SOURCES_READY;
    }
done:p->busy=0;return result;
}

enum pt_prepared_paula_sources_result pt_prepared_paula_sources_get(
    struct pt_prepared_paula_sources *p,struct pt_prepared_paula_sources_status *out)
{
    struct pt_prepared_paula_sources_status status;unsigned i;
    enum pt_prepared_paula_sources_result result=PT_PREPARED_PAULA_SOURCES_OK;
    if(!valid(p))return PT_PREPARED_PAULA_SOURCES_INVALID;
    /* Refuse the entire owner-capacity output alias before enter or a stale/
     * reentry failure latch can write any byte of the refused destination. */
    if(!object(out,sizeof(*out),ALIGNMENT(struct pt_prepared_paula_sources_status))||
       !apart(out,sizeof(*out),p,p->capacity)
#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
       ||!custody_output_clear(p,out,sizeof(*out))
#endif
       )return PT_PREPARED_PAULA_SOURCES_ALIAS;
    if(!enter(p))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD){result=PT_PREPARED_PAULA_SOURCES_FAULT;goto done;}
    if(p->phase!=PT_PREPARED_PAULA_SOURCES_FINISHED&&!fixed_current(p)){
        result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);goto done;
    }
    if(p->phase==PT_PREPARED_PAULA_SOURCES_PREPARED&&!held_current(p,0)){
        result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);goto done;
    }
    if(!output_clear(p,out,sizeof(*out))){result=PT_PREPARED_PAULA_SOURCES_ALIAS;goto done;}
    memset(&status,0,sizeof(status));status.phase=p->phase;status.failure=p->failure;
    status.completed=p->index;status.master_job=p->pin_job.reservation.owner!=NULL;
    status.chip_job=p->chip_job.owner!=NULL;
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){
        status.master_pins+=p->source[i].pin!=NULL;status.chip_leases+=p->source[i].leased!=0;
    }
    memcpy(out,&status,sizeof(status));
done:p->busy=0;return result;
}

#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
static enum pt_prepared_paula_sources_result cancel_owned_step(
#else
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_cancel_step(
#endif
    struct pt_prepared_paula_sources *p)
{
    struct prepared_source *s;struct pt_sample_version *pin;unsigned i;
    enum pt_prepared_paula_sources_result result=PT_PREPARED_PAULA_SOURCES_PENDING;
    if(!valid(p))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(!enter(p))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD){result=PT_PREPARED_PAULA_SOURCES_FAULT;goto done;}
    if(p->phase==PT_PREPARED_PAULA_SOURCES_FINISHED){result=PT_PREPARED_PAULA_SOURCES_CLOSED;goto done;}
    p->phase=PT_PREPARED_PAULA_SOURCES_CANCELLING;
    /* Cancellation deliberately does not follow old sample tables or require
     * current generation/route/descriptor observations. Release bindings and
     * original owner contexts still have to be the captured genuine ones. */
    if(!bindings_current(p)){result=hold(p);goto done;}
    if(p->chip_job.owner){
        if(p->chip_cancel_attempted){result=hold(p);goto done;}
        p->chip_cancel_attempted=1;pt_sampler_paula_job_cancel(&p->chip_job);
        if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD||p->chip_job.owner||!bindings_current(p))result=hold(p);
        goto done;
    }
    if(p->pin_job.reservation.owner||p->pin_job.value){
        if(p->pin_cancel_attempted){result=hold(p);goto done;}
        p->pin_cancel_attempted=1;pt_sampler_pin_job_cancel(&p->pin_job);
        if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD||p->pin_job.reservation.owner||
           p->pin_job.value||!bindings_current(p))result=hold(p);
        goto done;
    }
    for(i=p->cancel_index;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){
        s=p->source+i;p->cancel_index=i;
        if(s->leased){
            int released;
            if(s->lease_release_attempted){result=hold(p);goto done;}
            s->lease_release_attempted=1;
            released=pt_sampler_paula_unpin(p->bridge,s->lease);
            if(released){s->leased=0;memset(&s->lease,0,sizeof(s->lease));s->cache_data=NULL;s->cache_bytes=0;}
            if(!released||p->phase==PT_PREPARED_PAULA_SOURCES_HELD||!bindings_current(p))result=hold(p);
            goto done;
        }
        if(s->pin){
            /* Consume our exact reference before release-side callbacks. No
             * later failure may resurrect an already-consumed child pointer. */
            pin=s->pin;s->pin=NULL;memset(&s->pcm,0,sizeof(s->pcm));
            pt_sampler_unpin(pin);
            if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD||!bindings_current(p))result=hold(p);
            goto done;
        }
        p->cancel_index=i+1;
    }
    /* All genuine children ended. Remove borrowed pointer snapshots; keep the
     * original self/capacity and terminal failure so storage cannot be reused. */
    {size_t capacity=p->capacity;enum pt_prepared_paula_sources_result failure=p->failure;
        memset(p,0,sizeof(*p));p->self=p;p->capacity=capacity;
        p->phase=PT_PREPARED_PAULA_SOURCES_FINISHED;p->failure=failure;}
    result=PT_PREPARED_PAULA_SOURCES_CLOSED;
done:p->busy=0;return result;
}

#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
/* Public cancellation never acts on a privately reserved/transferred domain.
 * This early refusal performs no enter/failure write or child operation. */
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_cancel_step(
    struct pt_prepared_paula_sources *p)
{
    if(!valid(p))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(p->custodian)return PT_PREPARED_PAULA_SOURCES_BUSY;
    return cancel_owned_step(p);
}
#endif

#if defined(PT_PREPARED_PAULA_HOST_LIFETIME)
int pt_prepared_paula_sources_private_output_disjoint(
    struct pt_prepared_paula_sources *p,const void *out,size_t n)
{return valid(p)&&output_clear(p,out,n);}
static int context_covered(const struct pt_prepared_paula_context_extent *a,
    unsigned count,const void *context)
{
    unsigned i;uintptr_t x=(uintptr_t)context;
    if(!context)return 1;
    for(i=0;i<count;++i){uintptr_t b=(uintptr_t)a[i].base;
        if(object(a[i].base,a[i].bytes,1)&&x>=b&&x-b<a[i].bytes)return 1;
    }
    return 0;
}
int pt_prepared_paula_sources_private_contexts(struct pt_prepared_paula_sources *p,
    const struct pt_prepared_paula_context_extent *a,unsigned count)
{
    unsigned i;
    if(!valid(p)||p->phase==PT_PREPARED_PAULA_SOURCES_FINISHED||
       count>PT_PREPARED_PAULA_LIFETIME_CONTEXTS||
       (count&&!object(a,count*sizeof(*a),ALIGNMENT(struct pt_prepared_paula_context_extent))))return 0;
    for(i=0;i<count;++i)if(!object(a[i].base,a[i].bytes,1)||
       !apart(a[i].base,a[i].bytes,p,p->capacity))return 0;
    return context_covered(a,count,p->sampler_header.allocator.context)&&
        context_covered(a,count,p->chip_context);
}
int pt_prepared_paula_sources_private_current(struct pt_prepared_paula_sources *p,
    struct pt_prepared_paula_lifetime *l)
{
    return valid(p)&&p->busy&&p->custodian==l&&p->custody_stage==1&&
        pt_prepared_paula_lifetime_source_phase(l,p,1)&&
        p->phase==PT_PREPARED_PAULA_SOURCES_PREPARED&&
        p->failure==PT_PREPARED_PAULA_SOURCES_OK&&held_current(p,0);
}
static int data_add(struct pt_prepared_paula_lifetime_data *d,const void *p,size_t n)
{
    if(!n)return 1;
    if(!object(p,n,1)||d->count==PT_PREPARED_PAULA_LIFETIME_DATA_SPANS)return 0;
    d->span[d->count++]=(struct pt_scheduled_span){p,n};return 1;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_private_enqueue(
    struct pt_prepared_paula_sources *p,struct pt_prepared_paula_lifetime *l)
{
    struct pt_prepared_paula_lifetime_data data;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned i,j,n;
    enum pt_scheduled_result lower;enum pt_prepared_paula_sources_result result;
    if(!valid(p)||!pt_prepared_paula_lifetime_source_phase(l,p,1))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(p->custodian)return PT_PREPARED_PAULA_SOURCES_BUSY;
    if(!enter(p))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(p->phase!=PT_PREPARED_PAULA_SOURCES_PREPARED||
       p->failure!=PT_PREPARED_PAULA_SOURCES_OK||p->pin_job.reservation.owner||
       p->chip_job.owner||!held_current(p,0)){
        result=PT_PREPARED_PAULA_SOURCES_STALE;goto done;
    }
    /* Reversible PRIVATE reservation. No caller gets a borrow or pointer. */
    p->custodian=l;p->custody_stage=1;memset(&data,0,sizeof(data));
    for(i=0;i<PT_PREPARED_PAULA_SOURCES_COUNT;++i){struct prepared_source *s=p->source+i;
        n=0;
        if(!s->pin||!s->leased||!pt_sampler_version_spans(s->pin,spans,
           PT_SAMPLER_VERSION_SPANS,&n)){result=PT_PREPARED_PAULA_SOURCES_STALE;goto abort;}
        for(j=0;j<n;++j)if(!data_add(&data,spans[j].data,spans[j].bytes)){
            result=PT_PREPARED_PAULA_SOURCES_CAPACITY;goto abort;}
        if(!data_add(&data,s->cache_data,s->cache_bytes)){
            result=PT_PREPARED_PAULA_SOURCES_CAPACITY;goto abort;}
    }
    data.first=p->source[0].cache_data;data.first_bytes=p->source[0].cache_bytes;
    lower=pt_prepared_paula_lifetime_lower_enqueue(l,p,&data);
    /* Actual lower OK is irreversible even if an outer callback faulted us.
     * No further callback is made between this actual return and the commit. */
    if(lower==PT_SCHEDULED_OK)p->custody_stage=2;
    else {p->custodian=NULL;p->custody_stage=0;}
    if(p->phase==PT_PREPARED_PAULA_SOURCES_HELD)result=PT_PREPARED_PAULA_SOURCES_FAULT;
    else if(lower!=PT_SCHEDULED_OK)result=lower==PT_SCHEDULED_CAPACITY?
        PT_PREPARED_PAULA_SOURCES_CAPACITY:PT_PREPARED_PAULA_SOURCES_STALE;
    else if(!held_current(p,0))result=fail(p,PT_PREPARED_PAULA_SOURCES_STALE);
    else result=PT_PREPARED_PAULA_SOURCES_OK;
    goto done;
abort:p->custodian=NULL;p->custody_stage=0;
done:p->busy=0;return result;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_private_drain(
    struct pt_prepared_paula_sources *p,struct pt_prepared_paula_lifetime *l)
{
    if(!valid(p)||p->custodian!=l||p->custody_stage!=2||
       !pt_prepared_paula_lifetime_source_phase(l,p,2))return PT_PREPARED_PAULA_SOURCES_INVALID;
    /* Callback consumption and completed outer stop return are checked by the
     * original adapter. This calls only the captured original resource ledger. */
    return cancel_owned_step(p);
}

#endif /* HOST-only additive friends; native owner has no adapter references. */
