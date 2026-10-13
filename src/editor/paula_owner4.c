#include "paula_owner4.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include <stddef.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char c;T value;},value)
#define OWNER4_MAGIC 0x504f5734U
struct pt_paula_owner4 {
    uint32_t magic;struct pt_paula_owner4 *self;
    void *future_storage,*workspace;size_t future_bytes,capacity;
    struct pt_paula_segment_future *future;
    struct pt_paula_segment_sequence *producer;
    struct pt_sampler_paula *bridge;
    struct pt_sampler *original_sampler;struct pt_project *original_project;
    struct pt_allocator allocator,children;
    struct {void *address;size_t bytes;} allocation[16];
    const void *argument[4];size_t argument_bytes[4];
    void *unowned_return;size_t unowned_bytes;unsigned constructor_active;
    struct pt_paula_segment_sequence_config config;
    struct pt_paula_future_ops ops;
    struct pt_paula_segment_backend backend;
    uint32_t revision;uint64_t ticket[4];
    enum pt_paula_owner4_result result;
    enum pt_paula_owner4_retire_order order;
    unsigned busy,fault,cancelled,future_initialized,future_disposed,source_ready;
};
/* Serialized HOST witness only. It forbids facade reentry during genuine
 * callbacks; provider/allocator raw memory mutation is forbidden by the same
 * immutable borrowing contract, not made safe by a new snapshot certificate. */
static struct pt_paula_owner4 *active;
static int extent(const void *p,size_t n,size_t a)
{return p&&n&&a&&!((uintptr_t)p%a)&&n<=UINTPTR_MAX-(uintptr_t)p;}
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int product(size_t a,size_t b,size_t *out)
{if(b&&a>SIZE_MAX/b)return 0;*out=a*b;return 1;}
/* Same complete known-project span semantics as the genuine finite producer.
 * Valid immutable project metadata is a precondition; no PCM scans/promotion. */
static int project_apart(const struct pt_project *p,const void *out,size_t bytes)
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
static int known_apart(const struct pt_sampler_paula *b,const void *out,size_t bytes)
{
    unsigned i;
    if(!b||!b->sampler||!b->project)return 0;
    if(!apart(out,bytes,b,sizeof(*b))||!apart(out,bytes,b->sampler,sizeof(*b->sampler))||
       !project_apart(b->project,out,bytes)||!pt_sampler_output_disjoint(b->sampler,out,bytes))return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)
        if(!apart(out,bytes,b->cache.entry[i].data,b->cache.entry[i].bytes))return 0;
    return 1;
}
/* The genuine producer tracks its child allocations separately. This narrow
 * adapter additionally excludes the complete outer owner/future/workspace and
 * original constructor arguments, which its partial inner argument spans cannot
 * exclude. A foreign/alias return is retained as uncertainty, never freed. */
static void *owner_allocate(void *v,size_t bytes)
{
    struct pt_paula_owner4 *o=v;void *p;unsigned i,j;
    struct pt_sampler_paula bridge_before;struct pt_sampler sampler_before;
    struct pt_project project_before;struct pt_sampler *sampler;struct pt_project *project;
    for(i=0;i<16&&o->allocation[i].address;++i){}
    if(!bytes||i==16||o->fault)return NULL;
    sampler=o->bridge->sampler;project=o->bridge->project;
    if(!sampler||!project){o->fault=1;return NULL;}
    /* Capture the genuine metadata at THIS allocation entry. Legitimate cache
     * preparation between calls is not compared to a historical constructor
     * snapshot. Changed pointers are compared before following any new one. */
    memcpy(&bridge_before,o->bridge,sizeof(bridge_before));
    memcpy(&sampler_before,sampler,sizeof(sampler_before));memcpy(&project_before,project,sizeof(project_before));
    p=o->allocator.allocate(o->allocator.context,bytes);
    if(memcmp(o->bridge,&bridge_before,sizeof(bridge_before))||
       o->bridge->sampler!=sampler||o->bridge->project!=project||
       memcmp(sampler,&sampler_before,sizeof(sampler_before))||
       memcmp(project,&project_before,sizeof(project_before))){
        if(p){o->unowned_return=p;o->unowned_bytes=bytes;}o->fault=1;return NULL;
    }
    if(!p)return NULL;
    if(!extent(p,bytes,1)||!apart(p,bytes,o,sizeof(*o))||
       !apart(p,bytes,o->future_storage,o->future_bytes)||!apart(p,bytes,o->workspace,o->capacity)||
       !apart(p,bytes,o->ops.context,o->ops.context_bytes)||!known_apart(o->bridge,p,bytes))goto unknown;
    for(j=0;j<16;++j)if(!apart(p,bytes,o->allocation[j].address,o->allocation[j].bytes))goto unknown;
    if(o->constructor_active)for(j=0;j<4;++j)
        if(!apart(p,bytes,o->argument[j],o->argument_bytes[j]))goto unknown;
    o->allocation[i].address=p;o->allocation[i].bytes=bytes;return p;
unknown:
    o->unowned_return=p;o->unowned_bytes=bytes;o->fault=1;return NULL;
}
static void owner_release(void *v,void *p)
{
    struct pt_paula_owner4 *o=v;unsigned i,check=0;struct pt_sampler_paula bridge_before;
    struct pt_sampler sampler_before;struct pt_project project_before;
    struct pt_sampler *sampler=NULL;struct pt_project *project=NULL;
    for(i=0;i<16&&o->allocation[i].address!=p;++i){}
    if(!p||i==16){o->fault=1;return;}
    if(!o->fault&&o->bridge->sampler&&o->bridge->project){
        sampler=o->bridge->sampler;project=o->bridge->project;
        memcpy(&bridge_before,o->bridge,sizeof(bridge_before));
        memcpy(&sampler_before,sampler,sizeof(sampler_before));memcpy(&project_before,project,sizeof(project_before));check=1;
    }
    /* Real parent already consumed its child slot before release. Consume only
     * this matching adapter record once; never resurrect either history. */
    o->allocation[i].address=NULL;o->allocation[i].bytes=0;
    o->allocator.release(o->allocator.context,p);
    if(check&&(memcmp(o->bridge,&bridge_before,sizeof(bridge_before))||
       o->bridge->sampler!=sampler||o->bridge->project!=project||
       memcmp(sampler,&sampler_before,sizeof(sampler_before))||
       memcmp(project,&project_before,sizeof(project_before))))o->fault=1;
}
static int identity(const struct pt_paula_owner4 *o)
{return extent(o,sizeof(*o),ALIGNMENT(struct pt_paula_owner4))&&o->magic==OWNER4_MAGIC&&o->self==o;}
static int enter(struct pt_paula_owner4 *o)
{
    if(active){active->fault=1;return 0;}
    if(!identity(o)||o->busy)return 0;
    o->busy=1;active=o;return 1;
}
static void leave(struct pt_paula_owner4 *o)
{active=NULL;o->busy=0;if(o->fault)o->result=PT_OWNER4_HOLD;}
static enum pt_paula_owner4_result hold(struct pt_paula_owner4 *o)
{o->result=PT_OWNER4_HOLD;return o->result;}
static int scheduled_clean(enum pt_scheduled_result r)
{return r==PT_SCHEDULED_OK||r==PT_SCHEDULED_PENDING;}
static int observations(struct pt_paula_owner4 *o,struct pt_paula_segment_sequence_status *s,struct pt_paula_future_status *f)
{
    enum pt_paula_segment_sequence_result r;unsigned i;
    if(!o->producer||!o->future_initialized||o->future_disposed)return 0;
    memset(s,0,sizeof(*s));r=pt_paula_segment_sequence_get(o->producer,s);
    if(r==PT_PAULA_SEQUENCE_INVALID||r!=s->result||s->callback_faults)return 0;
    memset(f,0,sizeof(*f));if(!pt_paula_segment_future_get(o->future,f)||f->failed)return 0;
    if(o->cancelled){
        for(i=0;i<4;++i)if(s->ticket[i]!=o->ticket[i])return 0;
        if(!s->cancelled||!scheduled_clean(s->scheduled_result))return 0;
    }
    return 1;
}
size_t pt_paula_owner4_size(void){return sizeof(struct pt_paula_owner4);}
size_t pt_paula_owner4_alignment(void){return ALIGNMENT(struct pt_paula_owner4);}
enum pt_paula_owner4_result pt_paula_owner4_construct(
    void *storage,size_t bytes,void *future,size_t future_bytes,void *workspace,size_t capacity,
    const struct pt_allocator *allocator,struct pt_sampler_paula *bridge,
    const struct pt_paula_segment_sequence_config *config,const struct pt_paula_future_ops *ops,
    uint32_t revision,enum pt_paula_owner4_retire_order order,struct pt_paula_owner4 **out)
{
    struct pt_paula_owner4 *o=storage;struct pt_paula_segment_sequence *producer=NULL;
    enum pt_paula_segment_sequence_result r;size_t i,j;
    const void *fixed[8]={storage,future,workspace,allocator,config,ops,out,bridge};
    size_t sizes[8]={bytes,future_bytes,capacity,sizeof(*allocator),sizeof(*config),sizeof(*ops),sizeof(*out),sizeof(*bridge)};
    if(active){active->fault=1;return PT_OWNER4_REFUSED;}
    if(bytes!=sizeof(*o)||!extent(storage,bytes,ALIGNMENT(struct pt_paula_owner4))||
       future_bytes!=pt_paula_segment_future_size()||!extent(future,future_bytes,pt_paula_segment_future_alignment())||
       capacity<pt_paula_segment_sequence_control_size()||!extent(workspace,capacity,pt_paula_segment_sequence_control_alignment())||
       !extent(allocator,sizeof(*allocator),ALIGNMENT(struct pt_allocator))||
       !extent(bridge,sizeof(*bridge),ALIGNMENT(struct pt_sampler_paula))||
       !extent(config,sizeof(*config),ALIGNMENT(struct pt_paula_segment_sequence_config))||
       !extent(ops,sizeof(*ops),ALIGNMENT(struct pt_paula_future_ops))||
       !extent(out,sizeof(*out),ALIGNMENT(struct pt_paula_owner4 *))||
       !revision||(order!=PT_OWNER4_COMMAND_FIRST&&order!=PT_OWNER4_READER_FIRST))return PT_OWNER4_REFUSED;
    for(i=0;i<8;++i)for(j=0;j<i;++j)if(!apart(fixed[i],sizes[i],fixed[j],sizes[j]))return PT_OWNER4_REFUSED;
    if(!bridge->project||!bridge->sampler||!pt_sampler_paula_prepared_current(bridge)||
       !allocator->allocate||!allocator->release||!extent(ops->context,ops->context_bytes,1)||
       !known_apart(bridge,ops->context,ops->context_bytes))return PT_OWNER4_REFUSED;
    /* Bridge is itself live source; the other complete controls/input/output
     * objects must be apart from all known current/project/cache capacities. */
    for(i=0;i<7;++i)if(!known_apart(bridge,fixed[i],sizes[i])||
       !apart(fixed[i],sizes[i],ops->context,ops->context_bytes))return PT_OWNER4_REFUSED;
    if(!apart(bridge,sizeof(*bridge),ops->context,ops->context_bytes)||*out)return PT_OWNER4_REFUSED;
    for(i=0;i<bytes;++i)if(((const unsigned char *)storage)[i])return PT_OWNER4_REFUSED;
    for(i=0;i<future_bytes;++i)if(((const unsigned char *)future)[i])return PT_OWNER4_REFUSED;
    /* All subsequent failures retain this genuine outer recovery anchor.
     * A NULL producer returned by failed begin is never synthesized from workspace. */
    o->magic=OWNER4_MAGIC;o->self=o;o->future_storage=future;o->future_bytes=future_bytes;
    o->workspace=workspace;o->capacity=capacity;o->future=future;o->bridge=bridge;
    o->original_sampler=bridge->sampler;o->original_project=bridge->project;
    memcpy(&o->allocator,allocator,sizeof(o->allocator));memcpy(&o->config,config,sizeof(o->config));
    memcpy(&o->ops,ops,sizeof(o->ops));o->revision=revision;o->order=order;o->result=PT_OWNER4_HOLD;
    o->children=(struct pt_allocator){o,owner_allocate,owner_release};o->constructor_active=1;
    o->argument[0]=allocator;o->argument_bytes[0]=sizeof(*allocator);
    o->argument[1]=config;o->argument_bytes[1]=sizeof(*config);
    o->argument[2]=ops;o->argument_bytes[2]=sizeof(*ops);
    o->argument[3]=out;o->argument_bytes[3]=sizeof(*out);
    *out=o;
    if(!enter(o))return PT_OWNER4_HOLD;
    if(!pt_paula_segment_future_init(future,future_bytes,&o->ops,&o->config.readers.grid,o->config.readers.frame,&o->backend)){
        leave(o);return PT_OWNER4_HOLD;}
    o->future_initialized=1;
    r=pt_paula_segment_sequence_begin(&o->children,bridge,&o->config,&o->backend,revision,workspace,capacity,&producer);
    o->producer=producer;o->constructor_active=0;
    memset(o->argument,0,sizeof(o->argument));memset(o->argument_bytes,0,sizeof(o->argument_bytes));
    if(r==PT_PAULA_SEQUENCE_PENDING&&producer&&!o->fault)o->result=PT_OWNER4_OPEN;
    leave(o);return o->result;
}
enum pt_paula_segment_sequence_result pt_paula_owner4_step(struct pt_paula_owner4 *o,uint32_t revision,unsigned work)
{
    enum pt_paula_segment_sequence_result r=PT_PAULA_SEQUENCE_INVALID;
    if(!enter(o))return r;
    if(o->result==PT_OWNER4_OPEN){
        r=pt_paula_segment_sequence_step(o->producer,revision,work);
        if(r==PT_PAULA_SEQUENCE_FAILED||r==PT_PAULA_SEQUENCE_STALE||r==PT_PAULA_SEQUENCE_INVALID)hold(o);
    }
    leave(o);if(o->fault)r=PT_PAULA_SEQUENCE_FAILED;return r;
}
enum pt_scheduled_result pt_paula_owner4_publish(struct pt_paula_owner4 *o,uint32_t revision)
{
    enum pt_scheduled_result r=PT_SCHEDULED_INVALID;
    if(!enter(o))return r;
    if(o->result==PT_OWNER4_OPEN){r=pt_paula_segment_sequence_publish_next(o->producer,revision);if(!scheduled_clean(r))hold(o);}
    leave(o);if(o->fault)r=PT_SCHEDULED_BACKEND;return r;
}
enum pt_paula_future_result pt_paula_owner4_deliver(struct pt_paula_owner4 *o,uint64_t ticket)
{
    enum pt_paula_future_result r=PT_PAULA_FUTURE_STALE;
    if(!enter(o))return PT_PAULA_FUTURE_INVALID;
    if(o->result==PT_OWNER4_OPEN){
        r=pt_paula_segment_future_deliver(o->future,ticket);
        if(r==PT_PAULA_FUTURE_UNKNOWN||r==PT_PAULA_FUTURE_LATE||r==PT_PAULA_FUTURE_INVALID)hold(o);
    }
    leave(o);if(o->fault)r=PT_PAULA_FUTURE_UNKNOWN;return r;
}
enum pt_scheduled_result pt_paula_owner4_poll_command(struct pt_paula_owner4 *o,unsigned index)
{
    enum pt_scheduled_result r=PT_SCHEDULED_INVALID;
    if(!enter(o))return r;
    if(o->result==PT_OWNER4_OPEN&&index<4){
        r=pt_paula_segment_sequence_service_command(o->producer,index,0);if(!scheduled_clean(r))hold(o);}
    leave(o);if(o->fault)r=PT_SCHEDULED_BACKEND;return r;
}
enum pt_paula_owner4_result pt_paula_owner4_request_stop(struct pt_paula_owner4 *o)
{
    struct pt_paula_segment_sequence_status s;struct pt_paula_future_status f;enum pt_paula_future_result r;
    if(!enter(o))return PT_OWNER4_REFUSED;
    if(o->result==PT_OWNER4_OPEN){
        o->result=PT_OWNER4_STOPPING;
        /* Local admission is closed before any cancellation callback. */
        (void)pt_paula_segment_sequence_cancel(o->producer);
        memset(&s,0,sizeof(s));(void)pt_paula_segment_sequence_get(o->producer,&s);
        memcpy(o->ticket,s.ticket,sizeof(o->ticket));o->cancelled=1;
        if(s.callback_faults||!s.cancelled||!scheduled_clean(s.scheduled_result))hold(o);
        if(o->result==PT_OWNER4_STOPPING){
            r=pt_paula_segment_future_stop(o->future);
            if(r!=PT_PAULA_FUTURE_OK&&r!=PT_PAULA_FUTURE_PENDING)hold(o);
            if(!observations(o,&s,&f))hold(o);
            if(r==PT_PAULA_FUTURE_OK&&o->result==PT_OWNER4_STOPPING)o->source_ready=1;
        }
    }
    leave(o);return o->result;
}
enum pt_paula_owner4_result pt_paula_owner4_advance_stop(struct pt_paula_owner4 *o)
{
    struct pt_paula_segment_sequence_status s;struct pt_paula_future_status f;
    unsigned i,command=4,reader_needed;enum pt_scheduled_result r;enum pt_paula_future_result fr;
    if(!enter(o))return PT_OWNER4_REFUSED;
    if(o->result!=PT_OWNER4_STOPPING){leave(o);return o->result;}
    if(o->future_disposed){
        /* Genuine close may consume children then fail. Never repeat future
         * disposal, fabricate NULL, rollback or automatically retry that fault. */
        if(!pt_paula_segment_sequence_close(&o->producer)||o->producer)hold(o);
        else {
            for(i=0;i<16;++i)if(o->allocation[i].address)break;
            if(i!=16||o->unowned_return)hold(o);else o->result=PT_OWNER4_CLOSED;
        }
        leave(o);return o->result;
    }
    if(!observations(o,&s,&f)){hold(o);leave(o);return o->result;}
    for(i=0;i<4;++i)if(s.ticket[i]&&!(s.detached_mask&(1U<<i))){command=i;break;}
    reader_needed=f.accepted&&!s.reader_retired;
    if(reader_needed&&(o->order==PT_OWNER4_READER_FIRST||command==4)){
        r=pt_paula_segment_sequence_service_reader(o->producer,1);
        if(!scheduled_clean(r))hold(o);
    }else if(command<4){
        r=pt_paula_segment_sequence_service_command(o->producer,command,1);
        if(!scheduled_clean(r))hold(o);
    }else if(!o->source_ready){
        fr=pt_paula_segment_future_stop(o->future);
        if(fr!=PT_PAULA_FUTURE_OK&&fr!=PT_PAULA_FUTURE_PENDING)hold(o);
        else if(fr==PT_PAULA_FUTURE_OK)o->source_ready=1;
    }else{
        /* Getter counts alone never grant: real dispose checks source quiet,
         * reader/command obligations and known final-restoration state again. */
        if(f.detached!=f.accepted||(f.accepted&&!f.reader_retired)||!f.source_quiet||
           !pt_paula_segment_future_dispose(o->future))hold(o);
        else o->future_disposed=1;
    }
    leave(o);return o->result;
}
enum pt_paula_owner4_result pt_paula_owner4_status(const struct pt_paula_owner4 *o)
{
    if(active){active->fault=1;return PT_OWNER4_REFUSED;}
    if(!identity(o)||o->busy)return PT_OWNER4_REFUSED;
    return o->result;
}
