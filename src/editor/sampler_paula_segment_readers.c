#include "sampler_paula_segment_readers.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "project_snapshot.h"
#include <stddef.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char c;T value;},value)
enum {PREPARING=1,READY,LIVE,RETIRED};
enum {PIN_BEGIN,CHIP_BEGIN,CHIP_STEP};
struct dependency {
    struct pt_sample_version *pin;struct pt_pcm pcm;struct pt_cache_lease lease;
    struct pt_paula_segment_source source;unsigned leased;
};
struct reader {
    struct pt_paula_segment_readers *pool;unsigned state,released,terminal_seen,terminal_valid,key_seen;
    uint64_t trigger;struct pt_readers_key key;
    struct dependency dependency[PT_PAULA_SEGMENT_DEPENDENCIES];
    struct pt_sampler_paula_job job;struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];unsigned span_count;
};
struct command {
    struct pt_paula_segment_readers *pool;unsigned index,state,terminal_seen,terminal_valid;
    uint64_t ticket;struct pt_readers_key key;struct pt_paula_segment_plan plan,sealed;
    struct pt_scheduled_batch batch;
};
struct event_anchor {struct pt_readers_event *address;struct pt_readers_event original;};
struct facade {
    struct pt_paula_segment_readers *pool;struct pt_allocator allocator;
    struct pt_paula_segment_backend backend,backend_seal;size_t allocated,budget;
    /* Separate, unborrowed custody. These are owned metadata and supplied
     * immutable descriptors, never snapshots of genuine queue/cache history. */
    struct reader reader_seal;struct command command_seal[PT_PAULA_SEGMENT_COMMANDS];
    struct event_anchor event[PT_PAULA_SEGMENT_COMMANDS];
    struct pt_readers_domain *domain;struct pt_readers_domain domain_seal;
    struct pt_scheduled_span *spans;unsigned span_count;
    struct pt_scheduled_span span_seal[PT_SCHEDULED_SPANS];
    unsigned custody_ready,callback_active;
};
struct pt_paula_segment_readers {
    struct pt_allocator allocator;struct pt_sampler_paula *bridge;struct pt_sampler *sampler;struct pt_project *project,header;
    struct pt_paula_segment_readers_config config;struct facade *facade;struct pt_readers_output *queue;
    struct reader reader;struct command command[PT_PAULA_SEGMENT_COMMANDS];
    unsigned generation,state,index,phase,next,busy,queue_call,failed,closing,slot;uint64_t cache_version;
};
/* Serialized HOST callbacks only. This unexposed short-lived witness is
 * established from the authenticated private facade, never a borrowed holder.
 * Every owner callback checks it before reading even the holder's first byte. */
static struct facade *callback_witness;
static int owner_callback_admitted(void)
{if(callback_witness){callback_witness->pool->failed=1;return 0;}return 1;}
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
/* Complete checked numeric table extent, independent of the narrower stored
 * sample-count type. The same proven byte extent guards every constructor use. */
static int table_extent(size_t count,size_t element,size_t *bytes)
{if(element&&count>SIZE_MAX/element)return 0;*bytes=count*element;return 1;}
static int object(const void *p,size_t n,size_t a)
{return p&&span(p,n)&&!((uintptr_t)p%a);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static void seal_reader(struct pt_paula_segment_readers *p)
{if(p->facade&&p->facade->custody_ready)p->facade->reader_seal=p->reader;}
static void seal_command(struct pt_paula_segment_readers *p,unsigned i)
{if(p->facade&&p->facade->custody_ready)p->facade->command_seal[i]=p->command[i];}
/* Restore only authentic fixed owned-holder metadata and immutable borrowed
 * descriptors. Saved destinations/extents live outside the borrowed holders;
 * damaged pointers/counts never select a read or a restoration destination.
 * Real queue state, receipts, references, retirement and resource counters are
 * neither inspected nor restored. Anchors end only in real owner release. */
static int custody_repair(struct pt_paula_segment_readers *p)
{
    struct facade *f=p->facade;unsigned i;int good=1;
    if(!f||!f->custody_ready)return 1;
    if(memcmp(&p->reader,&f->reader_seal,sizeof(p->reader))){p->reader=f->reader_seal;good=0;}
    for(i=0;i<PT_PAULA_SEGMENT_COMMANDS;++i){
        if(memcmp(p->command+i,f->command_seal+i,sizeof(p->command[i]))){p->command[i]=f->command_seal[i];good=0;}
        if(f->event[i].address&&memcmp(f->event[i].address,&f->event[i].original,sizeof(f->event[i].original))){
            memcpy(f->event[i].address,&f->event[i].original,sizeof(f->event[i].original));good=0;}
    }
    if(f->domain){
        if(memcmp(f->domain,&f->domain_seal,sizeof(f->domain_seal))){memcpy(f->domain,&f->domain_seal,sizeof(f->domain_seal));good=0;}
        if(memcmp(f->spans,f->span_seal,(size_t)f->span_count*sizeof(*f->spans))){
            memcpy(f->spans,f->span_seal,(size_t)f->span_count*sizeof(*f->spans));good=0;}
    }
    if(memcmp(&f->backend,&f->backend_seal,sizeof(f->backend))){f->backend=f->backend_seal;good=0;}
    if(!good)p->failed=1;return good;
}
static int callback_enter(struct facade *f,unsigned *failure)
{if(callback_witness||f->callback_active||!custody_repair(f->pool)){
    if(callback_witness)callback_witness->pool->failed=1;f->pool->failed=1;return 0;}
 *failure=f->pool->failed;f->callback_active=1;callback_witness=f;return 1;}
static int callback_leave(struct facade *f,unsigned failure)
{int good=custody_repair(f->pool);
 /* Keep the authenticated witness through descriptor/holder repair. Genuine
  * core owner callbacks resume only after the matching callback has returned. */
 if(callback_witness!=f||!f->callback_active){f->pool->failed=1;good=0;}
 if(f->pool->failed!=failure)good=0;
 if(callback_witness==f)callback_witness=NULL;f->callback_active=0;return good;}
static int owner_phase(struct pt_paula_segment_readers *p)
{if(!p||!p->busy||!p->queue_call||p->facade->callback_active){if(p)p->failed=1;return 0;}return 1;}
static int equal_key(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static int equal_pcm(const struct pt_pcm *a,const struct pt_pcm *b)
{return a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&a->rate==b->rate&&a->channels==b->channels&&a->bits==b->bits;}
static int fixed_current(struct pt_paula_segment_readers *p)
{
    struct pt_project header=p->header;
    if(p->bridge->sampler!=p->sampler||p->bridge->project!=p->project||p->sampler->generation!=p->generation||
       p->bridge->version!=p->cache_version||!pt_sampler_paula_prepared_current(p->bridge))return 0;
    header.channels.selected=p->project->channels.selected;
    return pt_project_snapshot_equal(p->project,&header);
}
static int dependencies_current(struct pt_paula_segment_readers *p)
{
    unsigned i;struct pt_pcm pcm;struct pt_sample_version *pin;const uint8_t *data;size_t bytes;
    if(p->failed||p->closing||!fixed_current(p))return 0;
    for(i=0;i<p->config.dependency_count;++i){struct dependency *d=p->reader.dependency+i;
        struct pt_sample_version *expected=d->pin?d->pin:p->config.dependency[i].expected;
        if(pt_sampler_pin_current(p->sampler,p->project,p->config.dependency[i].sample,p->generation,expected,&pcm,&pin)!=PT_EDIT_OK)return 0;
        pt_sampler_unpin(pin);
        if(d->pin&&!equal_pcm(&pcm,&d->pcm))return 0;
        if(d->leased&&(!pt_sampler_paula_prepared_location_validated(p->bridge,p->config.track,d->lease,&data,&bytes)||
           data!=d->source.cache_data||bytes!=d->source.cache_bytes))return 0;
    }
    return 1;
}
static int output_storage_apart(struct pt_paula_segment_readers *p,const void *out,size_t n)
{
    unsigned i;const struct reader *r=p->facade&&p->facade->custody_ready?&p->facade->reader_seal:&p->reader;
    if(!span(out,n)||!n||!apart(out,n,p,sizeof(*p))||!apart(out,n,p->facade,sizeof(*p->facade))||
       !apart(out,n,p->facade->backend.readers.context,p->facade->backend.readers.context_bytes)||
       !apart(out,n,p->bridge,sizeof(*p->bridge))||!apart(out,n,p->project,sizeof(*p->project))||
       !apart(out,n,p->header.samples,(size_t)p->header.sample_count*sizeof(*p->header.samples))||
       !pt_sampler_output_disjoint(p->sampler,out,n))return 0;
    for(i=0;i<p->config.dependency_count;++i){const struct dependency *d=r->dependency+i;
        if(!pt_sampler_version_output_disjoint(d->pin?d->pin:p->config.dependency[i].expected,out,n)||
           (d->leased&&!apart(out,n,d->source.cache_data,d->source.cache_bytes)))return 0;
    }
    return 1;
}
static int output_apart(struct pt_paula_segment_readers *p,const void *out,size_t n)
{
    unsigned i;if(!output_storage_apart(p,out,n))return 0;
    if(p->facade->custody_ready){struct facade *f=p->facade;
        for(i=0;i<PT_PAULA_SEGMENT_COMMANDS;++i)if(f->event[i].address&&!apart(out,n,f->event[i].address,sizeof(f->event[i].original)))return 0;
        if(f->domain&&(!apart(out,n,f->domain,sizeof(*f->domain))||!apart(out,n,f->spans,(size_t)f->span_count*sizeof(*f->spans))))return 0;
    }
    return 1;
}
static int enter(struct pt_paula_segment_readers *p)
{if(!p)return 0;if(p->busy){p->failed=1;return 0;}p->busy=1;custody_repair(p);return 1;}
static void resources(struct pt_paula_segment_readers *p)
{
    unsigned i;struct reader *r=&p->reader;if(r->released)return;
    pt_sampler_paula_job_cancel(&r->job);
    for(i=0;i<p->config.dependency_count;++i){struct dependency *d=r->dependency+i;
        if(d->leased){pt_sampler_paula_unpin(p->bridge,d->lease);d->leased=0;}
        pt_sampler_unpin(d->pin);d->pin=NULL;
    }
    r->released=1;
}
static int reader_current(void *v,uint64_t token,uint64_t generation)
{if(!owner_callback_admitted())return 0;struct reader *r=v;struct pt_paula_segment_readers *p=r->pool;return owner_phase(p)&&token==5&&generation==p->config.grid.generation&&!r->released&&
 (r->state==READY||r->state==LIVE)&&(!p->busy||p->queue_call)&&dependencies_current(p);}
static int command_current(void *v,uint64_t token,uint64_t generation)
{if(!owner_callback_admitted())return 0;struct command *c=v;struct pt_paula_segment_readers *p=c->pool;return owner_phase(p)&&token==c->index+1U&&generation==p->config.grid.generation&&
 (c->state==READY||c->state==LIVE)&&!memcmp(&c->plan,&c->sealed,sizeof(c->plan))&&(!p->busy||p->queue_call)&&dependencies_current(p);}
static void command_terminal(void *v,uint64_t token,int valid)
{if(!owner_callback_admitted())return;struct command *c=v;if(!owner_phase(c->pool))return;if(token!=c->index+1U||c->state!=LIVE||c->terminal_seen){c->pool->failed=1;return;}
 c->terminal_seen=1;c->terminal_valid=valid!=0;if(!valid)c->pool->failed=1;seal_command(c->pool,c->index);}
static void command_release(void *v,uint64_t token)
{if(!owner_callback_admitted())return;struct command *c=v;if(!owner_phase(c->pool))return;
 if(token!=c->index+1U||c->state!=LIVE||!c->terminal_seen){c->pool->failed=1;return;}
 /* The genuine core already consumed/zeroed this descriptor before its owner
  * callbacks. Clear before any future callback; never resurrect its storage. */
 c->pool->facade->event[c->index].address=NULL;c->state=RETIRED;seal_command(c->pool,c->index);}
static void reader_terminal(void *v,uint64_t token,int valid)
{if(!owner_callback_admitted())return;struct reader *r=v;if(!owner_phase(r->pool))return;if(token!=5||r->state!=LIVE||r->terminal_seen){r->pool->failed=1;return;}
 r->terminal_seen=1;r->terminal_valid=valid!=0;if(!valid)r->pool->failed=1;seal_reader(r->pool);}
static void reader_release(void *v,uint64_t token)
{if(!owner_callback_admitted())return;struct reader *r=v;struct pt_paula_segment_readers *p=r->pool;unsigned busy;
 if(!owner_phase(p))return;busy=p->busy;
 if(token!=5||r->state!=LIVE||!r->terminal_seen){p->failed=1;return;}
 /* Core release requires independent retirement AND zero command refs. End
  * descriptor custody before release-side allocator callbacks, never earlier. */
 p->facade->domain=NULL;p->facade->spans=NULL;p->facade->span_count=0;
 p->busy=1;resources(p);r->state=RETIRED;seal_reader(p);p->busy=busy;}
static int add_span(struct reader *r,const void *data,size_t n)
{if(!n)return 1;if(!span(data,n)||r->span_count==PT_SCHEDULED_SPANS)return 0;r->spans[r->span_count++]=(struct pt_scheduled_span){data,n};return 1;}
static int observation(struct pt_paula_segment_readers *p,unsigned index)
{
    struct dependency *d=p->reader.dependency+index;struct pt_paula_segment_source s;
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned i,n=0;const uint8_t *data;size_t bytes;
    if(!pt_sampler_paula_prepared_location(p->bridge,p->config.track,d->lease,&data,&bytes)||
       !pt_sampler_version_spans(d->pin,spans,PT_SAMPLER_VERSION_SPANS,&n))return 0;
    memset(&s,0,sizeof(s));s.descriptor=&p->project->samples[p->config.dependency[index].sample].pcm;s.master=d->pcm;
    s.version=d->pin;s.sampler_owner=p->sampler;s.project_identity=p->project;s.cache_owner=p->bridge;
    s.cache_version=p->cache_version;s.generation=p->generation;s.sample=p->config.dependency[index].sample;
    s.lease=d->lease;s.cache_data=data;s.cache_bytes=bytes;
    for(i=0;i<n;++i){s.guard[s.guard_count++]=(struct pt_paula_segment_span){spans[i].data,spans[i].bytes};
        if(!add_span(&p->reader,spans[i].data,spans[i].bytes))return 0;}
    s.guard[s.guard_count++]=(struct pt_paula_segment_span){p->bridge,sizeof(*p->bridge)};
    s.guard[s.guard_count++]=(struct pt_paula_segment_span){p->sampler,sizeof(*p->sampler)};
    s.guard[s.guard_count++]=(struct pt_paula_segment_span){p->project,sizeof(*p->project)};
    if(s.guard_count>PT_PAULA_SEGMENT_SOURCE_GUARDS||!add_span(&p->reader,data,bytes))return 0;
    d->source=s;return 1;
}
/* Admit only the exact genuine command and its one original retained domain.
 * Core supplies mutable allocated descriptor storage through a const interface.
 * Numeric extent checks precede typed reads; identity snapshots are recovery,
 * never activation/retirement permission. Original pointers are loaned intact. */
static int anchor_event(struct facade *f,const struct pt_readers_event *event)
{
    struct pt_paula_segment_readers *p=f->pool;struct command *c=NULL;
    const struct pt_readers_domain *d;unsigned i,j;size_t bytes;
    if(!object(event,sizeof(*event),ALIGNMENT(struct pt_readers_event)))return 0;
    for(i=0;i<PT_PAULA_SEGMENT_COMMANDS;++i)if(event->binding.context==p->command+i&&event->binding.context_bytes==sizeof(*c)){c=p->command+i;break;}
    if(!c||c->state!=LIVE||event->queue!=p->queue||event->session!=p->config.session||event->scheduled.ticket!=c->ticket||
       event->command_owner!=c->index+1U||event->scheduled.batch.count!=1||memcmp(&event->scheduled.batch,&c->batch,sizeof(c->batch)))return 0;
    d=event->reader[0];if(!object(d,sizeof(*d),ALIGNMENT(struct pt_readers_domain))||
       d->binding.context!=&p->reader||d->binding.context_bytes!=sizeof(p->reader)||
       d->key.queue!=p->queue||d->key.session!=p->config.session||d->key.generation!=p->config.grid.generation||
       d->key.trigger!=p->reader.trigger||d->key.owner!=5||d->key.action||d->key.slot!=p->slot||
       d->count!=p->reader.span_count||!d->count||d->count>PT_SCHEDULED_SPANS)return 0;
    bytes=(size_t)d->count*sizeof(*d->spans);
    if(!object(d->spans,bytes,ALIGNMENT(struct pt_scheduled_span))||!apart(event,sizeof(*event),d,sizeof(*d))||
       !apart(event,sizeof(*event),d->spans,bytes)||!apart(d,sizeof(*d),d->spans,bytes)||
       !output_storage_apart(p,event,sizeof(*event))||!output_storage_apart(p,d,sizeof(*d))||!output_storage_apart(p,d->spans,bytes))return 0;
    for(j=0;j<PT_PAULA_SEGMENT_COMMANDS;++j)if(f->event[j].address&&j!=i&&
       (!apart(event,sizeof(*event),f->event[j].address,sizeof(f->event[j].original))||
        !apart(d,sizeof(*d),f->event[j].address,sizeof(f->event[j].original))||
        !apart(d->spans,bytes,f->event[j].address,sizeof(f->event[j].original))))return 0;
    if(c->index&&!equal_key(&c->key,&d->key))return 0;
    for(j=0;j<d->count;++j)if(d->spans[j].data!=p->reader.spans[j].data||d->spans[j].bytes!=p->reader.spans[j].bytes)return 0;
    if(f->domain){if(d!=f->domain||memcmp(d,&f->domain_seal,sizeof(*d)))return 0;}
    else{f->domain=(struct pt_readers_domain *)d;f->domain_seal=*d;f->spans=(struct pt_scheduled_span *)d->spans;
        f->span_count=d->count;memcpy(f->span_seal,d->spans,bytes);}
    if(f->event[i].address){if(event!=f->event[i].address||memcmp(event,&f->event[i].original,sizeof(*event)))return 0;}
    else{f->event[i].address=(struct pt_readers_event *)event;f->event[i].original=*event;}
    return 1;
}
static int facade_clock(void *v,uint64_t *tick,uint32_t *frequency)
{
    struct facade *f=v;int raw;unsigned failure;
    if(!callback_enter(f,&failure))return -1;
    if(!dependencies_current(f->pool)){callback_leave(f,failure);return 0;}
    raw=f->backend.readers.read_clock(f->backend.readers.context,tick,frequency);
    if(!callback_leave(f,failure)||!dependencies_current(f->pool)){f->pool->failed=1;return -1;}return raw;
}
static int facade_submit(void *v,const struct pt_readers_event *event)
{
    struct facade *f=v;struct command *c;int raw;unsigned failure;
    if(!custody_repair(f->pool)||!dependencies_current(f->pool))return -1;
    if(!anchor_event(f,event)){f->pool->failed=1;return -1;}
    c=event->binding.context;
    if(!callback_enter(f,&failure))return -1;
    raw=f->backend.submit_plan(f->backend.readers.context,event,&c->plan);
    if(!callback_leave(f,failure)||!dependencies_current(f->pool)){f->pool->failed=1;return -1;}return raw;
}
static enum pt_readers_reply facade_poll_command(void *v,uint64_t t,struct pt_readers_command_receipt *out)
{struct facade *f=v;enum pt_readers_reply reply;unsigned failure;if(!callback_enter(f,&failure))return PT_READERS_UNCERTAIN;
 reply=f->backend.readers.poll_command(f->backend.readers.context,t,out);
 return callback_leave(f,failure)?reply:PT_READERS_UNCERTAIN;}
static enum pt_readers_reply facade_cancel_command(void *v,uint64_t t,struct pt_readers_command_receipt *out)
{struct facade *f=v;enum pt_readers_reply reply;unsigned failure;if(!callback_enter(f,&failure))return PT_READERS_UNCERTAIN;
 reply=f->backend.readers.cancel_command(f->backend.readers.context,t,out);
 return callback_leave(f,failure)?reply:PT_READERS_UNCERTAIN;}
static enum pt_readers_reply facade_poll_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct facade *f=v;enum pt_readers_reply reply;unsigned failure;if(!callback_enter(f,&failure))return PT_READERS_UNCERTAIN;
 if(d!=f->domain){f->pool->failed=1;callback_leave(f,failure);return PT_READERS_UNCERTAIN;}
 reply=f->backend.readers.poll_reader(f->backend.readers.context,d,out);
 return callback_leave(f,failure)?reply:PT_READERS_UNCERTAIN;}
static enum pt_readers_reply facade_cancel_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out)
{struct facade *f=v;enum pt_readers_reply reply;unsigned failure;if(!callback_enter(f,&failure))return PT_READERS_UNCERTAIN;
 if(d!=f->domain){f->pool->failed=1;callback_leave(f,failure);return PT_READERS_UNCERTAIN;}
 reply=f->backend.readers.cancel_reader(f->backend.readers.context,d,out);
 return callback_leave(f,failure)?reply:PT_READERS_UNCERTAIN;}
static void *queue_allocate(void *v,size_t n)
{
    struct facade *f=v;void *out;if(f->allocated>f->budget||n>f->budget-f->allocated)return NULL;
    out=f->allocator.allocate(f->allocator.context,n);if(!out)return NULL;
    if(!object(out,n,ALIGNMENT(uint64_t))||!output_apart(f->pool,out,n)){f->pool->failed=1;return NULL;}
    if(!dependencies_current(f->pool)){f->allocator.release(f->allocator.context,out);return NULL;}
    f->allocated+=n;return out;
}
static void queue_release(void *v,void *out)
{struct facade *f=v;f->allocator.release(f->allocator.context,out);}
static int open_fixed(const struct pt_allocator *a,struct pt_sampler_paula *b,
    const struct pt_paula_segment_readers_config *c,const struct pt_paula_segment_backend *backend,struct pt_paula_segment_readers **out)
{
    const void *p[5]={a,b,c,backend,out};size_t n[5]={sizeof(*a),sizeof(*b),sizeof(*c),sizeof(*backend),sizeof(*out)};unsigned i,j;
    if(!object(a,n[0],ALIGNMENT(struct pt_allocator))||!object(b,n[1],ALIGNMENT(struct pt_sampler_paula))||
       !object(c,n[2],ALIGNMENT(struct pt_paula_segment_readers_config))||!object(backend,n[3],ALIGNMENT(struct pt_paula_segment_backend))||
       !object(out,n[4],ALIGNMENT(struct pt_paula_segment_readers *)))return 0;
    for(i=0;i<5;++i)for(j=i+1;j<5;++j)if(!apart(p[i],n[i],p[j],n[j]))return 0;
    return 1;
}
/* The already-bound bridge supplies two additional fixed objects. Validate
 * their complete numeric extents and separation before following either, and
 * retain exact constructor identities across all allocator callbacks. */
static int open_borrowed(const struct pt_allocator *a,struct pt_sampler_paula *b,
    const struct pt_paula_segment_readers_config *c,const struct pt_paula_segment_backend *backend,
    struct pt_paula_segment_readers **out,struct pt_sampler *sampler,struct pt_project *project)
{
    const void *fixed[5]={a,b,c,backend,out};size_t sizes[5]={sizeof(*a),sizeof(*b),sizeof(*c),sizeof(*backend),sizeof(*out)};unsigned i;
    if(!object(sampler,sizeof(*sampler),ALIGNMENT(struct pt_sampler))||!object(project,sizeof(*project),ALIGNMENT(struct pt_project))||
       !apart(sampler,sizeof(*sampler),project,sizeof(*project)))return 0;
    for(i=0;i<5;++i)if(!apart(fixed[i],sizes[i],sampler,sizeof(*sampler))||!apart(fixed[i],sizes[i],project,sizeof(*project)))return 0;
    return 1;
}
static int constructor_unchanged(const struct pt_allocator *a,const struct pt_allocator *allocator,
    const struct pt_paula_segment_readers_config *config,const struct pt_paula_segment_readers_config *c,
    const struct pt_paula_segment_backend *backend,const struct pt_paula_segment_backend *be,
    struct pt_sampler_paula *b,const struct pt_sampler_paula *bridge,
    struct pt_sampler *sampler,const struct pt_sampler *sampler_header,struct pt_project *project,const struct pt_project *header)
{
    return !memcmp(a,allocator,sizeof(*a))&&!memcmp(config,c,sizeof(*config))&&!memcmp(backend,be,sizeof(*backend))&&
        b->sampler==sampler&&b->project==project&&!memcmp(b,bridge,sizeof(*b))&&
        !memcmp(sampler,sampler_header,sizeof(*sampler))&&pt_project_snapshot_equal(project,header);
}
enum pt_paula_segment_readers_result pt_paula_segment_readers_open(const struct pt_allocator *a,
    struct pt_sampler_paula *b,const struct pt_paula_segment_readers_config *config,
    const struct pt_paula_segment_backend *backend,struct pt_paula_segment_readers **out)
{
    struct pt_paula_segment_readers *p;struct facade *f;struct pt_allocator allocator,qa;
    struct pt_paula_segment_readers_config c;struct pt_paula_segment_backend be;struct pt_readers_backend rb;
    struct pt_project header,*project;struct pt_sampler *sampler,sampler_header;struct pt_sampler_paula bridge;
    struct pt_pcm pcm;struct pt_sample_version *pin;unsigned i,j,generation;uint64_t version;int8_t map[PT_CHANNEL_LIMIT];size_t table_bytes;
    if(!open_fixed(a,b,config,backend,out))return PT_PAULA_SEGMENT_READERS_INVALID;
    sampler=b->sampler;project=b->project;
    if(!open_borrowed(a,b,config,backend,out,sampler,project))return PT_PAULA_SEGMENT_READERS_INVALID;
    bridge=*b;sampler_header=*sampler;header=*project;
    if(header.sample_count>PT_PROJECT_SAMPLES||!table_extent(header.sample_count,sizeof(*header.samples),&table_bytes)||
       !object(header.samples,table_bytes,ALIGNMENT(struct pt_sample)))return PT_PAULA_SEGMENT_READERS_INVALID;
    {const void *fixed[5]={a,b,config,backend,out};size_t sizes[5]={sizeof(*a),sizeof(*b),sizeof(*config),sizeof(*backend),sizeof(*out)};
        for(i=0;i<5;++i)if(!apart(fixed[i],sizes[i],header.samples,table_bytes)||
            !pt_sampler_output_disjoint(sampler,fixed[i],sizes[i]))return PT_PAULA_SEGMENT_READERS_INVALID;}
    allocator=*a;c=*config;be=*backend;
    if(!allocator.allocate||!allocator.release||!pt_sampler_paula_prepared_current(b)||
       !c.dependency_count||c.dependency_count>PT_PAULA_SEGMENT_DEPENDENCIES||!c.grid.generation||!c.session||
       (c.grid.rate!=44100&&c.grid.rate!=48000)||!pt_paula_render_caps_valid(&c.caps)||c.track>=project->channels.count||
       pt_channels_paula_map(&project->channels,NULL,map)!=PT_CHANNEL_OK||map[c.track]<0||
       be.plan_version!=PT_PAULA_SEGMENT_BACKEND_VERSION||be.plan_flags!=PT_PAULA_SEGMENT_BACKEND_REQUIRED||!be.submit_plan||
       be.readers.submit||!be.readers.read_clock||!be.readers.poll_command||!be.readers.cancel_command||!be.readers.poll_reader||
       !be.readers.cancel_reader||!span(be.readers.context,be.readers.context_bytes)||!be.readers.context_bytes)
        return PT_PAULA_SEGMENT_READERS_INVALID;
    generation=sampler->generation;version=b->version;
    if(!apart(out,sizeof(*out),be.readers.context,be.readers.context_bytes)||!apart(out,sizeof(*out),sampler,sizeof(*sampler))||
       !apart(out,sizeof(*out),project,sizeof(*project))||!apart(out,sizeof(*out),header.samples,table_bytes)||
       !apart(a,sizeof(*a),be.readers.context,be.readers.context_bytes)||!apart(b,sizeof(*b),be.readers.context,be.readers.context_bytes)||
       !apart(config,sizeof(*config),be.readers.context,be.readers.context_bytes)||!apart(backend,sizeof(*backend),be.readers.context,be.readers.context_bytes)||
       !apart(sampler,sizeof(*sampler),be.readers.context,be.readers.context_bytes)||!apart(project,sizeof(*project),be.readers.context,be.readers.context_bytes)||
       !apart(header.samples,table_bytes,be.readers.context,be.readers.context_bytes)||
       !pt_sampler_output_disjoint(sampler,out,sizeof(*out)))return PT_PAULA_SEGMENT_READERS_INVALID;
    for(i=0;i<PT_PAULA_SEGMENT_COMMANDS;++i)if(c.frame[i]==UINT64_MAX||(i&&c.frame[i]<=c.frame[i-1]))return PT_PAULA_SEGMENT_READERS_INVALID;
    for(i=0;i<c.dependency_count;++i){if(c.dependency[i].sample>=header.sample_count||!c.dependency[i].expected||
        pt_sampler_pin_current(sampler,project,c.dependency[i].sample,generation,c.dependency[i].expected,&pcm,&pin)!=PT_EDIT_OK)return PT_PAULA_SEGMENT_READERS_STALE;
        pt_sampler_unpin(pin);if(pcm.channels!=1)return PT_PAULA_SEGMENT_READERS_INVALID;
        for(j=0;j<i;++j)if(c.dependency[j].sample==c.dependency[i].sample)return PT_PAULA_SEGMENT_READERS_INVALID;
    }
    if(c.control_budget<sizeof(*p)+sizeof(*f))return PT_PAULA_SEGMENT_READERS_CAPACITY;
    p=allocator.allocate(allocator.context,sizeof(*p));if(!p)return PT_PAULA_SEGMENT_READERS_CAPACITY;
    if(!object(p,sizeof(*p),ALIGNMENT(struct pt_paula_segment_readers))||!apart(p,sizeof(*p),out,sizeof(*out))||
       !apart(p,sizeof(*p),a,sizeof(*a))||!apart(p,sizeof(*p),config,sizeof(*config))||!apart(p,sizeof(*p),backend,sizeof(*backend))||
       !apart(p,sizeof(*p),b,sizeof(*b))||!apart(p,sizeof(*p),project,sizeof(*project))||
       !apart(p,sizeof(*p),header.samples,table_bytes)||!apart(p,sizeof(*p),be.readers.context,be.readers.context_bytes)||
       !apart(p,sizeof(*p),sampler,sizeof(*sampler))||!pt_sampler_output_disjoint(&sampler_header,p,sizeof(*p)))return PT_PAULA_SEGMENT_READERS_INVALID;
    if(!constructor_unchanged(a,&allocator,config,&c,backend,&be,b,&bridge,sampler,&sampler_header,project,&header)){
        allocator.release(allocator.context,p);return PT_PAULA_SEGMENT_READERS_STALE;
    }
    memset(p,0,sizeof(*p));p->allocator=allocator;p->bridge=b;p->sampler=sampler;p->project=project;p->header=header;
    p->config=c;p->generation=generation;p->cache_version=version;p->state=PREPARING;p->slot=(unsigned)map[c.track];p->reader.pool=p;
    f=allocator.allocate(allocator.context,sizeof(*f));if(!f){allocator.release(allocator.context,p);return PT_PAULA_SEGMENT_READERS_CAPACITY;}
    if(!object(f,sizeof(*f),ALIGNMENT(struct facade))||!apart(f,sizeof(*f),p,sizeof(*p))||!apart(f,sizeof(*f),out,sizeof(*out))||
       !apart(f,sizeof(*f),a,sizeof(*a))||!apart(f,sizeof(*f),config,sizeof(*config))||!apart(f,sizeof(*f),backend,sizeof(*backend))||
       !apart(f,sizeof(*f),b,sizeof(*b))||!apart(f,sizeof(*f),project,sizeof(*project))||
       !apart(f,sizeof(*f),header.samples,table_bytes)||!apart(f,sizeof(*f),be.readers.context,be.readers.context_bytes)||
       !apart(f,sizeof(*f),sampler,sizeof(*sampler))||!pt_sampler_output_disjoint(&sampler_header,f,sizeof(*f))){allocator.release(allocator.context,p);return PT_PAULA_SEGMENT_READERS_INVALID;}
    if(!constructor_unchanged(a,&allocator,config,&c,backend,&be,b,&bridge,sampler,&sampler_header,project,&header)){
        allocator.release(allocator.context,f);allocator.release(allocator.context,p);return PT_PAULA_SEGMENT_READERS_STALE;
    }
    memset(f,0,sizeof(*f));f->pool=p;f->allocator=allocator;f->backend=f->backend_seal=be;f->budget=c.control_budget-sizeof(*p)-sizeof(*f);p->facade=f;
    if(!dependencies_current(p)){allocator.release(allocator.context,f);allocator.release(allocator.context,p);return PT_PAULA_SEGMENT_READERS_STALE;}
    rb=be.readers;rb.context=f;rb.context_bytes=sizeof(*f);rb.read_clock=facade_clock;rb.submit=facade_submit;
    rb.poll_command=facade_poll_command;rb.cancel_command=facade_cancel_command;rb.poll_reader=facade_poll_reader;rb.cancel_reader=facade_cancel_reader;
    qa=(struct pt_allocator){f,queue_allocate,queue_release};
    if(pt_readers_open(&qa,&c.grid,c.session,&rb,4,1,&p->queue)!=PT_SCHEDULED_OK||!dependencies_current(p)){
        if(p->queue){pt_readers_stop(p->queue);pt_readers_close(p->queue);}
        allocator.release(allocator.context,f);allocator.release(allocator.context,p);return PT_PAULA_SEGMENT_READERS_CAPACITY;
    }
    if(!add_span(&p->reader,b,sizeof(*b))||!add_span(&p->reader,p->sampler,sizeof(*p->sampler))||
       !add_span(&p->reader,p->project,sizeof(*p->project))||!add_span(&p->reader,header.samples,table_bytes)||
       !constructor_unchanged(a,&allocator,config,&c,backend,&be,b,&bridge,sampler,&sampler_header,project,&header)){
        pt_readers_stop(p->queue);pt_readers_close(p->queue);allocator.release(allocator.context,f);allocator.release(allocator.context,p);
        return PT_PAULA_SEGMENT_READERS_STALE;
    }
    f->custody_ready=1;seal_reader(p);for(i=0;i<PT_PAULA_SEGMENT_COMMANDS;++i)seal_command(p,i);
    *out=p;return PT_PAULA_SEGMENT_READERS_OK;
}
enum pt_paula_segment_readers_result pt_paula_segment_readers_step(struct pt_paula_segment_readers *p,unsigned *ready)
{
    struct dependency *d;enum pt_cache_result cr;enum pt_paula_segment_readers_result result=PT_PAULA_SEGMENT_READERS_PENDING;
    if(!p||!object(ready,sizeof(*ready),ALIGNMENT(unsigned))||!output_apart(p,ready,sizeof(*ready))||!enter(p))return PT_PAULA_SEGMENT_READERS_INVALID;
    if(!dependencies_current(p)){result=PT_PAULA_SEGMENT_READERS_STALE;goto done;}
    if(p->state==READY){*ready=1;result=PT_PAULA_SEGMENT_READERS_OK;goto done;}
    if(p->state!=PREPARING){result=PT_PAULA_SEGMENT_READERS_BUSY;goto done;}
    d=p->reader.dependency+p->index;
    if(p->phase==PIN_BEGIN){
        if(pt_sampler_pin_current(p->sampler,p->project,p->config.dependency[p->index].sample,p->generation,
            p->config.dependency[p->index].expected,&d->pcm,&d->pin)!=PT_EDIT_OK){result=PT_PAULA_SEGMENT_READERS_STALE;goto done;}
        p->phase=CHIP_BEGIN;
    }else if(p->phase==CHIP_BEGIN){
        cr=pt_sampler_paula_job_begin(&p->reader.job,p->bridge,p->config.track,p->config.dependency[p->index].sample,0,d->pin,&d->lease);
        if(cr==PT_CACHE_PENDING)p->phase=CHIP_STEP;
        else if(cr==PT_CACHE_HIT)d->leased=1;
        else{result=cr==PT_CACHE_CAPACITY?PT_PAULA_SEGMENT_READERS_CAPACITY:PT_PAULA_SEGMENT_READERS_STALE;goto done;}
    }else{
        cr=pt_sampler_paula_job_step(&p->reader.job,&d->lease);
        if(cr==PT_CACHE_LOAD)d->leased=1;else if(cr!=PT_CACHE_PENDING){result=PT_PAULA_SEGMENT_READERS_STALE;goto done;}
    }
    if(d->leased){if(!observation(p,p->index)){result=PT_PAULA_SEGMENT_READERS_STALE;goto done;}++p->index;p->phase=PIN_BEGIN;}
    if(p->index==p->config.dependency_count){if(!dependencies_current(p)){result=PT_PAULA_SEGMENT_READERS_STALE;goto done;}
        p->state=READY;p->reader.state=READY;result=PT_PAULA_SEGMENT_READERS_OK;}
    *ready=p->state==READY;
done: if(result!=PT_PAULA_SEGMENT_READERS_OK&&result!=PT_PAULA_SEGMENT_READERS_PENDING)p->failed=1;
    seal_reader(p);p->busy=0;return result;
}
static int source_index(struct pt_paula_segment_readers *p,const struct pt_pcm *descriptor)
{unsigned i;for(i=0;i<p->config.dependency_count;++i)if(p->reader.dependency[i].source.descriptor==descriptor)return (int)i;return -1;}
enum pt_paula_segment_readers_result pt_paula_segment_readers_begin(struct pt_paula_segment_readers *p,uint64_t frame,
    const struct pt_render_action *action,const struct pt_readers_key *key,unsigned *index)
{
    struct pt_render_action a;struct pt_readers_key k,actual;struct pt_paula_segment_plan plan;struct command *c;int current,next;
    enum pt_paula_segment_result result;enum pt_scheduled_result scheduled;
    if(!p||!object(action,sizeof(*action),ALIGNMENT(struct pt_render_action))||!object(index,sizeof(*index),ALIGNMENT(unsigned))||
       !output_apart(p,action,sizeof(*action))||!output_apart(p,index,sizeof(*index))||!apart(index,sizeof(*index),action,sizeof(*action))||
       (key&&(!object(key,sizeof(*key),ALIGNMENT(struct pt_readers_key))||!output_apart(p,key,sizeof(*key))||!apart(index,sizeof(*index),key,sizeof(*key))))||!enter(p))return PT_PAULA_SEGMENT_READERS_INVALID;
    if(p->state!=READY||p->next>=PT_PAULA_SEGMENT_COMMANDS||!dependencies_current(p)){p->busy=0;return PT_PAULA_SEGMENT_READERS_STALE;}
    a=*action;
    if(frame!=p->config.frame[p->next]||a.channel!=p->config.track||a.kind!=(p->next==0?PT_RENDER_SEGMENT:p->next==3?PT_RENDER_STOP:PT_RENDER_REPEAT)){
        p->busy=0;return PT_PAULA_SEGMENT_READERS_INVALID;}
    memset(&k,0,sizeof(k));
    if(p->next){if(!key){p->busy=0;return PT_PAULA_SEGMENT_READERS_INVALID;}k=*key;p->queue_call=1;
        scheduled=pt_readers_reader_key(p->queue,p->reader.trigger,0,&actual);p->queue_call=0;
        if(scheduled!=PT_SCHEDULED_OK||!p->reader.key_seen||!equal_key(&k,&actual)||!equal_key(&k,&p->reader.key)){
            p->busy=0;return PT_PAULA_SEGMENT_READERS_STALE;}
    }else if(key){p->busy=0;return PT_PAULA_SEGMENT_READERS_INVALID;}
    if(p->next==3)result=pt_paula_segment_plan_stop(&p->command[2].plan,frame,&plan);
    else{current=source_index(p,a.voice.pcm);next=source_index(p,p->next?a.voice.repeat_pcm:a.voice.pcm);
        if(current<0||next<0){p->busy=0;return PT_PAULA_SEGMENT_READERS_INVALID;}
        result=pt_paula_segment_plan_build(&a,&p->reader.dependency[current].source,&p->reader.dependency[next].source,
            frame,p->config.grid.rate,p->slot,&p->config.caps,&plan);
    }
    if(result!=PT_PAULA_SEGMENT_OK){p->busy=0;return PT_PAULA_SEGMENT_READERS_INVALID;}
    c=p->command+p->next;if(c->state){p->busy=0;return PT_PAULA_SEGMENT_READERS_BUSY;}
    memset(c,0,sizeof(*c));c->pool=p;c->index=p->next;c->state=READY;c->key=k;c->plan=c->sealed=plan;
    c->batch.generation=p->config.grid.generation;c->batch.frame=frame;c->batch.count=1;
    c->batch.action[0]=(struct pt_scheduled_action){p->next==0?PT_SCHEDULED_TRIGGER:p->next==3?PT_SCHEDULED_STOP:PT_SCHEDULED_CONTROL,
        p->slot,p->next==0?plan.source[0].cache_data+plan.initial.byte_offset:NULL,p->next==0?plan.initial.words:0,plan.period,plan.volume};
    seal_command(p,p->next);*index=p->next;p->busy=0;return PT_PAULA_SEGMENT_READERS_OK;
}
enum pt_scheduled_result pt_paula_segment_readers_enqueue(struct pt_paula_segment_readers *p,unsigned index,uint64_t *out)
{
    struct command *c;struct pt_readers_control control;struct pt_readers_owner owner;enum pt_scheduled_result result;
    if(!p||index!=p->next||index>=PT_PAULA_SEGMENT_COMMANDS||!object(out,sizeof(*out),ALIGNMENT(uint64_t))||!output_apart(p,out,sizeof(*out))||!enter(p))return PT_SCHEDULED_INVALID;
    c=p->command+index;if(c->state!=READY||!dependencies_current(p)){p->busy=0;return PT_SCHEDULED_STALE;}
    control=(struct pt_readers_control){c,sizeof(*c),index+1U,command_current,command_release,command_terminal};
    owner=(struct pt_readers_owner){{&p->reader,sizeof(p->reader),5,reader_current,reader_release,reader_terminal},p->reader.spans,p->reader.span_count};
    p->queue_call=1;result=pt_readers_enqueue(p->queue,&c->batch,index?&c->key:NULL,&control,index?NULL:&owner,out);p->queue_call=0;
    if(result==PT_SCHEDULED_OK){c->ticket=*out;c->state=LIVE;++p->next;if(!index){p->reader.state=LIVE;p->reader.trigger=*out;}seal_command(p,index);seal_reader(p);}
    p->busy=0;return result;
}
enum pt_scheduled_result pt_paula_segment_readers_publish(struct pt_paula_segment_readers *p,unsigned index)
{enum pt_scheduled_result result;if(!p||index>=PT_PAULA_SEGMENT_COMMANDS||!enter(p))return PT_SCHEDULED_INVALID;
 if(p->command[index].state!=LIVE){p->busy=0;return PT_SCHEDULED_INVALID;}
 p->queue_call=1;result=pt_readers_publish(p->queue,p->command[index].ticket);p->queue_call=0;p->busy=0;return result;}
enum pt_scheduled_result pt_paula_segment_readers_poll_command(struct pt_paula_segment_readers *p,unsigned index,int cancel,struct pt_readers_command_receipt *out)
{enum pt_scheduled_result result;if(!p||index>=PT_PAULA_SEGMENT_COMMANDS||!object(out,sizeof(*out),ALIGNMENT(struct pt_readers_command_receipt))||!output_apart(p,out,sizeof(*out))||!enter(p))return PT_SCHEDULED_INVALID;
 p->queue_call=1;result=cancel?pt_readers_cancel_command(p->queue,p->command[index].ticket,out):pt_readers_poll_command(p->queue,p->command[index].ticket,out);p->queue_call=0;p->busy=0;return result;}
enum pt_scheduled_result pt_paula_segment_readers_poll_reader(struct pt_paula_segment_readers *p,int cancel,struct pt_readers_reader_receipt *out)
{enum pt_scheduled_result result;if(!p||!object(out,sizeof(*out),ALIGNMENT(struct pt_readers_reader_receipt))||!output_apart(p,out,sizeof(*out))||!enter(p))return PT_SCHEDULED_INVALID;
 p->queue_call=1;result=cancel?pt_readers_cancel_reader(p->queue,p->reader.trigger,0,out):pt_readers_poll_reader(p->queue,p->reader.trigger,0,out);p->queue_call=0;p->busy=0;return result;}
enum pt_scheduled_result pt_paula_segment_readers_key(struct pt_paula_segment_readers *p,struct pt_readers_key *out)
{enum pt_scheduled_result result;if(!p||!object(out,sizeof(*out),ALIGNMENT(struct pt_readers_key))||!output_apart(p,out,sizeof(*out))||!enter(p))return PT_SCHEDULED_INVALID;
 if(!dependencies_current(p)){p->busy=0;return PT_SCHEDULED_STALE;}p->queue_call=1;result=pt_readers_reader_key(p->queue,p->reader.trigger,0,out);p->queue_call=0;
 if(result==PT_SCHEDULED_OK){p->reader.key=*out;p->reader.key_seen=1;seal_reader(p);}p->busy=0;return result;}
enum pt_scheduled_result pt_paula_segment_readers_stop(struct pt_paula_segment_readers *p)
{enum pt_scheduled_result result;if(!enter(p))return PT_SCHEDULED_INVALID;p->closing=1;p->queue_call=1;result=pt_readers_stop(p->queue);p->queue_call=0;p->busy=0;return result;}
int pt_paula_segment_readers_close(struct pt_paula_segment_readers *p)
{
    struct pt_allocator a;struct facade *f;
    if(!enter(p))return 0;
    if(pt_readers_commands_held(p->queue)||pt_readers_readers_held(p->queue)){p->busy=0;return 0;}
    p->closing=1;resources(p);seal_reader(p);p->queue_call=1;pt_readers_stop(p->queue);p->queue_call=0;
    if(!pt_readers_close(p->queue)){p->busy=0;return 0;}
    a=p->allocator;f=p->facade;a.release(a.context,f);a.release(a.context,p);return 1;
}
