#include "prepared_paula_lifetime.h"
#include <stdint.h>
#include <string.h>

#define ALIGNMENT(T) offsetof(struct {char byte;T value;},value)
enum {EMPTY=1,QUEUED,STOPPED,CLOSED,HELD};
enum {OPEN_CALL=1,ENQUEUE_CALL,STOP_CALL,DRAIN_CALL,CLOSE_CALL};
struct command_control {struct pt_prepared_paula_lifetime *owner;};
struct reader_control {struct pt_prepared_paula_lifetime *owner;};
struct pt_prepared_paula_lifetime {
    struct pt_prepared_paula_lifetime *self;
    size_t capacity;
    struct pt_prepared_paula_sources *sources;
    struct pt_prepared_paula_lifetime_config config;
    const struct pt_prepared_paula_lifetime_config *config_original;
    struct pt_readers_backend backend;
    const struct pt_readers_backend *backend_original;
    struct pt_readers_output *queue;
    void *allocation,*unrecognized;size_t allocation_bytes,used;
    struct pt_prepared_paula_lifetime_data data;
    struct command_control command;
    struct reader_control reader;
    uint64_t ticket;
    unsigned state,busy,outer,failed,uncertain,attempted,transferred;
    unsigned stop_attempted,stop_returned,drained;
    unsigned command_terminal,command_consumed,reader_terminal,reader_consumed;
    unsigned release_consumed;
};
/* The witness is established from our actual original queue call. Incoming
 * callback context is compared by address BEFORE any incoming dereference. */
static struct pt_prepared_paula_lifetime *callback_witness;
/* Independent original OPEN/CLOSE allocator witness. Never discover an owner
 * by dereferencing an incoming allocator context. */
static struct pt_prepared_paula_lifetime *allocator_witness;

static int object(const void *p,size_t n,size_t a)
{uintptr_t v=(uintptr_t)p;return p&&n&&a&&v%a==0&&n<=UINTPTR_MAX-v;}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return (!n||object(a,n,1))&&(!m||object(b,m,1))&&(!n||!m||x+n<=y||y+m<=x);}
static int valid(const struct pt_prepared_paula_lifetime *l)
{return object(l,sizeof(*l),ALIGNMENT(struct pt_prepared_paula_lifetime))&&l->self==l&&l->capacity>=sizeof(*l)&&object(l,l->capacity,ALIGNMENT(struct pt_prepared_paula_lifetime))&&l->state>=EMPTY&&l->state<=HELD&&l->config.context_count<=PT_PREPARED_PAULA_LIFETIME_CONTEXTS&&l->data.count<=PT_PREPARED_PAULA_LIFETIME_DATA_SPANS;}
static void fault(struct pt_prepared_paula_lifetime *l)
{l->failed=1;l->uncertain=1;}
static int enter(struct pt_prepared_paula_lifetime *l,unsigned phase)
{if(l->busy){fault(l);return 0;}l->busy=1;l->outer=phase;return 1;}
static void leave(struct pt_prepared_paula_lifetime *l)
{l->outer=0;l->busy=0;}
static int contexts_clear(const struct pt_prepared_paula_lifetime *l,const void *p,size_t n)
{
    unsigned i;
    for(i=0;i<l->config.context_count;++i)
        if(!apart(p,n,l->config.context[i].base,l->config.context[i].bytes))return 0;
    return 1;
}
static int covered_context(const struct pt_prepared_paula_context_extent *a,
    unsigned count,const void *p,size_t n)
{
    uintptr_t x=(uintptr_t)p;unsigned i;
    if(!p)return !n;
    if(!n||!object(p,n,1))return 0;
    for(i=0;i<count;++i){uintptr_t b=(uintptr_t)a[i].base;
        if(object(a[i].base,a[i].bytes,1)&&x>=b&&x-b<=a[i].bytes&&n<=a[i].bytes-(x-b))return 1;
    }
    return 0;
}
int pt_prepared_paula_lifetime_output_disjoint(
    const struct pt_prepared_paula_lifetime *l,const void *p,size_t n)
{
    unsigned i;
    if(!valid(l)||!object(p,n,1)||!apart(p,n,l,l->capacity)||
       !apart(p,n,l->allocation,l->allocation_bytes)||!contexts_clear(l,p,n)||
       !apart(p,n,l->backend_original,l->backend_original?sizeof(*l->backend_original):0))return 0;
    for(i=0;i<l->data.count;++i)
        if(!apart(p,n,l->data.span[i].data,l->data.span[i].bytes))return 0;
    return 1;
}
int pt_prepared_paula_lifetime_source_phase(const struct pt_prepared_paula_lifetime *l,
    const struct pt_prepared_paula_sources *p,unsigned phase)
{
    if(!valid(l)||l->sources!=p||!l->busy||(callback_witness&&callback_witness!=l))return 0;
    if(phase==1)return l->outer==ENQUEUE_CALL&&!l->transferred&&l->state==EMPTY;
    return phase==2&&!callback_witness&&l->outer==DRAIN_CALL&&l->transferred&&l->state==STOPPED&&
        l->stop_returned&&l->command_consumed&&l->reader_consumed&&!l->uncertain;
}
static int current_source_phase(struct pt_prepared_paula_lifetime *l)
{
    /* Keep the original authenticated witness through every metadata check. */
    return callback_witness==l&&pt_prepared_paula_sources_private_current(l->sources,l);
}
static struct pt_prepared_paula_lifetime *callback_owner(void *context,unsigned kind,uint64_t token,unsigned phase)
{
    struct pt_prepared_paula_lifetime *l=callback_witness;
    if(!l)return NULL;
    if(!valid(l)||!l->busy||l->outer!=phase||!l->queue||
       context!=(kind==1?(void *)&l->command:(void *)&l->reader)||
       token!=(uint64_t)kind||l->command.owner!=l||l->reader.owner!=l){fault(l);return NULL;}
    return l;
}
static int command_current(void *context,uint64_t token,uint64_t generation)
{
    struct pt_prepared_paula_lifetime *l=callback_owner(context,1,token,ENQUEUE_CALL);
    return l&&!l->failed&&generation==l->config.grid.generation&&current_source_phase(l);
}
static int reader_current(void *context,uint64_t token,uint64_t generation)
{
    struct pt_prepared_paula_lifetime *l=callback_owner(context,2,token,ENQUEUE_CALL);
    return l&&!l->failed&&generation==l->config.grid.generation&&current_source_phase(l);
}
static void command_terminal(void *context,uint64_t token,int good)
{
    struct pt_prepared_paula_lifetime *l=callback_owner(context,1,token,STOP_CALL);
    if(!l)return;
    if(!l->transferred||l->command_terminal||l->command_consumed||good!=1){fault(l);return;}
    l->command_terminal=1;
}
static void command_release(void *context,uint64_t token)
{
    struct pt_prepared_paula_lifetime *l=callback_owner(context,1,token,STOP_CALL);
    if(!l)return;
    if(!l->transferred||!l->command_terminal||l->command_consumed){fault(l);return;}
    l->command_consumed=1;
}
static void reader_terminal(void *context,uint64_t token,int good)
{
    struct pt_prepared_paula_lifetime *l=callback_owner(context,2,token,STOP_CALL);
    if(!l)return;
    if(!l->transferred||!l->command_consumed||l->reader_terminal||l->reader_consumed||good!=1){fault(l);return;}
    l->reader_terminal=1;
}
static void reader_release(void *context,uint64_t token)
{
    struct pt_prepared_paula_lifetime *l=callback_owner(context,2,token,STOP_CALL);
    if(!l)return;
    if(!l->transferred||!l->command_consumed||!l->reader_terminal||l->reader_consumed){fault(l);return;}
    l->reader_consumed=1;
}
static struct pt_prepared_paula_lifetime *allocator_owner(void *context,unsigned allocation)
{
    struct pt_prepared_paula_lifetime *l=allocator_witness;
    if(!l)return NULL;
    if(context!=(void *)l){if(valid(l))fault(l);return NULL;}
    if(!valid(l))return NULL;
    if(!l->busy||(allocation?l->outer!=OPEN_CALL:
       (l->outer!=OPEN_CALL&&l->outer!=CLOSE_CALL))){fault(l);return NULL;}
    return l;
}
static void *queue_allocate(void *context,size_t bytes)
{
    struct pt_prepared_paula_lifetime *l=allocator_owner(context,1);void *p;
    if(!l||l->allocation||l->unrecognized||
       l->used>l->config.control_budget||bytes>l->config.control_budget-l->used)return NULL;
    p=l->config.allocator.allocate(l->config.allocator.context,bytes);
    if(!p)return NULL;
    /* Original source may have been cancelled by that callback. Recheck its
     * genuine CURRENT guard; no pre-claim resource-preservation promise. */
    if(!object(p,bytes,ALIGNMENT(struct pt_readers_backend))||
       !apart(p,bytes,l,l->capacity)||!contexts_clear(l,p,bytes)||
       !apart(p,bytes,l->config_original,sizeof(*l->config_original))||
       !apart(p,bytes,l->backend_original,sizeof(*l->backend_original))||
       !pt_prepared_paula_sources_private_output_disjoint(l->sources,p,bytes)){
        l->unrecognized=p;fault(l);return NULL; /* Known alias: never free it. */
    }
    l->allocation=p;l->allocation_bytes=bytes;l->used+=bytes;
    if(l->failed){
        l->allocation=NULL;l->allocation_bytes=0;l->used=sizeof(*l);
        l->config.allocator.release(l->config.allocator.context,p);return NULL;
    }
    return p;
}
static void queue_release(void *context,void *p)
{
    struct pt_prepared_paula_lifetime *l=allocator_owner(context,0);
    if(!l)return;
    if(p!=l->allocation||!l->allocation_bytes){fault(l);return;}
    /* Raw pt_readers_close may free before an outer allocator fault. Consume
     * our queue pointer BEFORE that callback; never restore or retry it. */
    l->allocation=NULL;l->allocation_bytes=0;l->queue=NULL;l->used=sizeof(*l);
    l->release_consumed=1;
    l->config.allocator.release(l->config.allocator.context,p);
}
size_t pt_prepared_paula_lifetime_size(void)
{return sizeof(struct pt_prepared_paula_lifetime);}
size_t pt_prepared_paula_lifetime_alignment(void)
{return ALIGNMENT(struct pt_prepared_paula_lifetime);}

enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_init(
    struct pt_prepared_paula_lifetime *l,size_t capacity,
    struct pt_prepared_paula_sources *p,const struct pt_prepared_paula_lifetime_config *config,
    const struct pt_readers_backend *backend)
{
    struct pt_prepared_paula_lifetime_config cfg;struct pt_readers_backend be;
    struct pt_allocator wrapped;struct pt_readers_output *queue=NULL;
    const unsigned char *zero=(const unsigned char *)l;size_t i;
    enum pt_scheduled_result result;
    if(capacity<sizeof(*l)||!object(l,capacity,ALIGNMENT(struct pt_prepared_paula_lifetime))||
       !object(config,sizeof(*config),ALIGNMENT(struct pt_prepared_paula_lifetime_config))||
       !object(backend,sizeof(*backend),ALIGNMENT(struct pt_readers_backend))||
       !apart(l,capacity,config,sizeof(*config))||!apart(l,capacity,backend,sizeof(*backend))||
       !pt_prepared_paula_sources_private_output_disjoint(p,l,capacity))return PT_PREPARED_PAULA_SOURCES_ALIAS;
    /* Active/reentered init refuses before copying/resetting any controls. */
    for(i=0;i<sizeof(*l);++i)if(zero[i])return PT_PREPARED_PAULA_SOURCES_BUSY;
    memcpy(&cfg,config,sizeof(cfg));memcpy(&be,backend,sizeof(be));
    if(!cfg.allocator.allocate||!cfg.allocator.release||!cfg.session||
       !cfg.grid.generation||cfg.frame==UINT64_MAX||cfg.slot>=4||!cfg.period||
       cfg.volume>64||cfg.context_count>PT_PREPARED_PAULA_LIFETIME_CONTEXTS||
       cfg.control_budget<sizeof(*l))return PT_PREPARED_PAULA_SOURCES_INVALID;
    for(i=0;i<cfg.context_count;++i)
        if(!object(cfg.context[i].base,cfg.context[i].bytes,1)||
           !apart(l,capacity,cfg.context[i].base,cfg.context[i].bytes))return PT_PREPARED_PAULA_SOURCES_ALIAS;
    if((cfg.allocator.context&&!covered_context(cfg.context,cfg.context_count,cfg.allocator.context,1))||
       !covered_context(cfg.context,cfg.context_count,be.context,be.context_bytes)||
       !pt_prepared_paula_sources_private_contexts(p,cfg.context,cfg.context_count))return PT_PREPARED_PAULA_SOURCES_INVALID;
    /* A nested constructor never clobbers a different active witness. */
    if(allocator_witness)return PT_PREPARED_PAULA_SOURCES_BUSY;
    l->self=l;l->capacity=capacity;l->sources=p;l->config=cfg;l->backend=be;
    l->config_original=config;l->backend_original=backend;l->state=EMPTY;l->used=sizeof(*l);
    l->command.owner=l;l->reader.owner=l;l->busy=1;l->outer=OPEN_CALL;
    wrapped=(struct pt_allocator){l,queue_allocate,queue_release};
    allocator_witness=l;
    result=pt_readers_open(&wrapped,&l->config.grid,l->config.session,&l->backend,1,1,&queue);
    if(allocator_witness!=l)fault(l);
    else allocator_witness=NULL;
    l->config_original=NULL; /* Temporary constructor input may now expire. */
    /* Callback-visible queue pointer is installed only after actual open. */
    if(result==PT_SCHEDULED_OK)l->queue=queue;
    leave(l);
    if(l->uncertain||l->unrecognized){l->state=HELD;return PT_PREPARED_PAULA_SOURCES_FAULT;}
    if(result!=PT_SCHEDULED_OK)return result==PT_SCHEDULED_CAPACITY?
        PT_PREPARED_PAULA_SOURCES_CAPACITY:PT_PREPARED_PAULA_SOURCES_INVALID;
    return PT_PREPARED_PAULA_SOURCES_OK;
}

enum pt_scheduled_result pt_prepared_paula_lifetime_lower_enqueue(
    struct pt_prepared_paula_lifetime *l,struct pt_prepared_paula_sources *p,
    const struct pt_prepared_paula_lifetime_data *data)
{
    struct pt_scheduled_batch batch;struct pt_readers_control command;
    struct pt_readers_owner reader;enum pt_scheduled_result result;unsigned i;
    if(!pt_prepared_paula_lifetime_source_phase(l,p,1)||!l->queue||l->failed||
       !object(data,sizeof(*data),ALIGNMENT(struct pt_prepared_paula_lifetime_data))||
       !data->count||data->count>PT_PREPARED_PAULA_LIFETIME_DATA_SPANS||
       !object(data->first,data->first_bytes,2)||data->first_bytes%2||
       data->first_bytes/2>UINT16_MAX||!data->first_bytes)return PT_SCHEDULED_INVALID;
    for(i=0;i<data->count;++i)
        if(!object(data->span[i].data,data->span[i].bytes,1)||
           !apart(data->span[i].data,data->span[i].bytes,l,l->capacity)||
           !apart(data->span[i].data,data->span[i].bytes,l->backend.context,l->backend.context_bytes))return PT_SCHEDULED_INVALID;
    memcpy(&l->data,data,sizeof(l->data));memset(&batch,0,sizeof(batch));
    batch.generation=l->config.grid.generation;batch.frame=l->config.frame;batch.count=1;
    batch.action[0]=(struct pt_scheduled_action){PT_SCHEDULED_TRIGGER,l->config.slot,
        l->data.first,(uint16_t)(l->data.first_bytes/2),l->config.period,l->config.volume};
    command=(struct pt_readers_control){&l->command,sizeof(l->command),1,command_current,command_release,command_terminal};
    reader=(struct pt_readers_owner){{&l->reader,sizeof(l->reader),2,reader_current,reader_release,reader_terminal},l->data.span,l->data.count};
    if(callback_witness){fault(l);return PT_SCHEDULED_BACKEND;}
    callback_witness=l;
    result=pt_readers_enqueue(l->queue,&batch,NULL,&command,&reader,&l->ticket);
    if(callback_witness!=l)fault(l);
    callback_witness=NULL;
    /* Retain actual lower transfer before any classification at the caller. */
    if(result==PT_SCHEDULED_OK){l->transferred=1;l->state=QUEUED;}
    return result;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_enqueue_unpublished(
    struct pt_prepared_paula_lifetime *l)
{
    enum pt_prepared_paula_sources_result result;
    if(!valid(l))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(!enter(l,ENQUEUE_CALL))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(l->state!=EMPTY||l->attempted||l->failed||!l->queue){leave(l);return PT_PREPARED_PAULA_SOURCES_INVALID;}
    l->attempted=1;
    result=pt_prepared_paula_sources_private_enqueue(l->sources,l);
    leave(l);
    return l->uncertain?PT_PREPARED_PAULA_SOURCES_FAULT:result;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_stop_local(
    struct pt_prepared_paula_lifetime *l)
{
    enum pt_scheduled_result result;
    if(!valid(l))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(!enter(l,STOP_CALL))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(l->state!=QUEUED||!l->transferred||l->stop_attempted||!l->queue){leave(l);return PT_PREPARED_PAULA_SOURCES_INVALID;}
    if(callback_witness){fault(l);leave(l);return PT_PREPARED_PAULA_SOURCES_FAULT;}
    l->stop_attempted=1;callback_witness=l;
    result=pt_readers_stop(l->queue);
    if(callback_witness!=l)fault(l);
    callback_witness=NULL;
    if(result!=PT_SCHEDULED_OK||!l->command_consumed||!l->reader_consumed||
       pt_readers_commands_held(l->queue)||pt_readers_readers_held(l->queue))fault(l);
    /* This publication occurs only after the actual stop/callback stack ended.
     * A callback attempting drain before this point sees busy, never authority. */
    if(!l->uncertain){l->state=STOPPED;l->stop_returned=1;}
    leave(l);
    return l->uncertain?PT_PREPARED_PAULA_SOURCES_FAULT:PT_PREPARED_PAULA_SOURCES_OK;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_drain_step(
    struct pt_prepared_paula_lifetime *l)
{
    enum pt_prepared_paula_sources_result result;
    if(!valid(l))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(l->drained)return PT_PREPARED_PAULA_SOURCES_CLOSED;
    if(!enter(l,DRAIN_CALL))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(l->state!=STOPPED||!l->stop_returned||l->uncertain){leave(l);return PT_PREPARED_PAULA_SOURCES_BUSY;}
    result=pt_prepared_paula_sources_private_drain(l->sources,l);
    if(result==PT_PREPARED_PAULA_SOURCES_CLOSED)l->drained=1;
    if(result==PT_PREPARED_PAULA_SOURCES_FAULT)fault(l);
    leave(l);return result;
}
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_close(
    struct pt_prepared_paula_lifetime *l)
{
    struct pt_readers_output *queue;int result;
    if(!valid(l))return PT_PREPARED_PAULA_SOURCES_INVALID;
    if(l->state==CLOSED)return PT_PREPARED_PAULA_SOURCES_CLOSED;
    if(!enter(l,CLOSE_CALL))return PT_PREPARED_PAULA_SOURCES_FAULT;
    if(l->unrecognized||l->uncertain||(l->transferred&&!l->drained)){
        leave(l);return PT_PREPARED_PAULA_SOURCES_FAULT;
    }
    if(l->queue){
        if(allocator_witness){
            /* Forbidden cross-owner callback work cannot steal its witness. */
            if(valid(allocator_witness))fault(allocator_witness);
            fault(l);leave(l);return PT_PREPARED_PAULA_SOURCES_FAULT;
        }
        queue=l->queue;allocator_witness=l;
        result=pt_readers_close(queue);
        if(allocator_witness!=l)fault(l);
        else allocator_witness=NULL;
        /* queue_release already consumed queue/allocation before user release.
         * A failed outer callback cannot resurrect the now-freed raw pointer. */
        if(!result||l->queue||l->allocation){fault(l);leave(l);return PT_PREPARED_PAULA_SOURCES_FAULT;}
    }
    if(l->uncertain){leave(l);return PT_PREPARED_PAULA_SOURCES_FAULT;}
    l->sources=NULL;l->backend_original=NULL;memset(&l->backend,0,sizeof(l->backend));
    memset(&l->config,0,sizeof(l->config));memset(&l->data,0,sizeof(l->data));
    l->state=CLOSED;leave(l);return PT_PREPARED_PAULA_SOURCES_CLOSED;
}
