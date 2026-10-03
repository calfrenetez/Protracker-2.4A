#include "scheduled_lineage.h"
#include "document.h"
#include <string.h>
struct lineage_entry {
    struct pt_lineage_event event;struct pt_lineage_owner owner;
    struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];
    struct pt_lineage_receipt last;unsigned state,seen,blocked,superseded;
};
struct pt_lineage_output {
    struct pt_allocator allocator;struct pt_scheduled_grid grid;
    struct pt_lineage_backend backend;struct pt_elapsed_clock clock;
    uint64_t session,tickets,readers,last_frame,last_ticks;
    unsigned capacity,held,ordered,clock_seen,closing,failed,busy;
    struct lineage_entry entry[PT_SCHEDULED_BATCHES];
};
enum {LOCAL=1,PUBLISHED};
static int span(const void *p,size_t n)
{return !n || (p && (uintptr_t)p<=UINTPTR_MAX-(n-1));}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int key_zero(const struct pt_lineage_key *k)
{return !k->queue&&!k->session&&!k->generation&&!k->ticket&&!k->owner&&!k->serial&&!k->action&&!k->slot;}
static int key_equal(const struct pt_lineage_key *a,const struct pt_lineage_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->ticket==b->ticket&&
 a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static struct lineage_entry *find(struct pt_lineage_output *q,uint64_t t)
{unsigned i;for(i=0;i<q->capacity;++i)if(q->entry[i].state&&q->entry[i].event.scheduled.ticket==t)return q->entry+i;return NULL;}
static int reentry(struct pt_lineage_output *q)
{if(q->busy){q->failed=1;return 1;}return 0;}
static int output_apart(const struct pt_lineage_output *q,const void *out,size_t bytes)
{
    unsigned i,j;
    if(!bytes||!apart(out,bytes,q,sizeof(*q))||!apart(out,bytes,q->backend.context,q->backend.context_bytes))return 0;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state)
        for(j=0;j<q->entry[i].owner.held.count;++j)
            if(!apart(out,bytes,q->entry[i].spans[j].data,q->entry[i].spans[j].bytes))return 0;
    return 1;
}
static int capable(const struct pt_lineage_backend *b,unsigned n)
{return b&&span(b,sizeof(*b))&&n&&n<=PT_SCHEDULED_BATCHES&&b->caps.flags==PT_SCHEDULED_REQUIRED&&
 b->caps.maximum_batches>=n&&b->caps.maximum_batches<=PT_SCHEDULED_BATCHES&&
 b->caps.maximum_actions&&b->caps.maximum_actions<=PT_LINEAGE_ACTIONS&&b->version==PT_LINEAGE_VERSION&&
 b->lineage_flags==PT_LINEAGE_CONDITIONAL&&b->read_clock&&b->submit&&b->poll&&b->cancel;}
enum pt_scheduled_result pt_lineage_open(const struct pt_allocator *a,
    const struct pt_scheduled_grid *g,uint64_t session,const struct pt_lineage_backend *b,
    unsigned n,struct pt_lineage_output **out)
{
    struct pt_lineage_output *q;struct pt_elapsed_clock c;
    if(!a||!span(a,sizeof(*a))||!a->allocate||!a->release||!g||!out||!session||!span(a,sizeof(*a))||!span(g,sizeof(*g))||!g->generation||g->frequency<g->rate||
       pt_elapsed_clock_init(&c,g->frequency,g->rate,g->epoch,0)!=PT_ELAPSED_OK)return PT_SCHEDULED_INVALID;
    if(!capable(b,n))return PT_SCHEDULED_UNSUPPORTED;
    if(!b->context_bytes||!span(b->context,b->context_bytes)||!apart(out,sizeof(*out),a,sizeof(*a))||
       !apart(out,sizeof(*out),g,sizeof(*g))||!apart(out,sizeof(*out),b,sizeof(*b))||
       !apart(out,sizeof(*out),b->context,b->context_bytes))return PT_SCHEDULED_INVALID;
    q=a->allocate(a->context,sizeof(*q));if(!q)return PT_SCHEDULED_CAPACITY;
    if(!apart(q,sizeof(*q),out,sizeof(*out))||!apart(q,sizeof(*q),a,sizeof(*a))||
       !apart(q,sizeof(*q),g,sizeof(*g))||!apart(q,sizeof(*q),b,sizeof(*b))||
       !apart(q,sizeof(*q),b->context,b->context_bytes)){
        a->release(a->context,q);return PT_SCHEDULED_INVALID;
    }
    memset(q,0,sizeof(*q));q->allocator=*a;q->grid=*g;q->session=session;q->backend=*b;q->clock=c;q->capacity=n;
    *out=q;return PT_SCHEDULED_OK;
}
static int active(struct pt_lineage_output *q,const struct pt_lineage_key *k,uint64_t frame,int admission)
{
    struct lineage_entry *e;unsigned i,j;
    if(k->queue!=q||k->session!=q->session||k->generation!=q->grid.generation||!(e=find(q,k->ticket))||
       e->state!=PUBLISHED||!e->seen||k->action>=e->event.scheduled.batch.count||
       e->event.scheduled.batch.action[k->action].kind!=PT_SCHEDULED_TRIGGER||
       e->event.scheduled.batch.frame>=frame||(e->superseded&(1U<<k->action))||(admission&&(e->blocked&(1U<<k->action))))return 0;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state&&q->entry[i].event.scheduled.batch.frame<frame&&
       q->entry[i].event.scheduled.batch.frame>e->event.scheduled.batch.frame)
        for(j=0;j<q->entry[i].event.scheduled.batch.count;++j){
            const struct pt_scheduled_action *a=q->entry[i].event.scheduled.batch.action+j;
            if(a->slot==k->slot&&(a->kind==PT_SCHEDULED_TRIGGER||
               (a->kind==PT_SCHEDULED_STOP&&key_equal(q->entry[i].event.key+j,k))))return 0;
        }
    return e->last.action[k->action].command==PT_LINEAGE_ISSUED&&
        e->last.action[k->action].reader==PT_LINEAGE_ACTIVE&&key_equal(k,e->event.key+k->action);
}
static int targets_current(struct pt_lineage_output *q,const struct pt_lineage_event *event)
{
    unsigned i;int result;
    for(i=0;i<event->scheduled.batch.count;++i)if(event->scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){
        struct lineage_entry *source;
        if(!active(q,event->key+i,event->scheduled.batch.frame,0))return 0;
        source=find(q,event->key[i].ticket);q->busy=1;
        result=source->owner.held.current(source->owner.held.context,source->owner.held.token,q->grid.generation);
        q->busy=0;if(result!=1||q->failed)return 0;
    }
    return 1;
}
static int inputs(struct pt_lineage_output *q,const struct pt_scheduled_batch *b,
    const struct pt_lineage_key *target,const struct pt_lineage_owner *o,const uint64_t *out,unsigned *triggers)
{
    unsigned i,j,slots=0;size_t bytes;
    if(!b||!o||!out||!output_apart(q,b,sizeof(*b))||!output_apart(q,o,sizeof(*o))||!o->held.token||!o->held.current||!o->held.release||!o->held.spans||!o->held.count||
       o->held.count>PT_SCHEDULED_SPANS||!b->count||b->count>q->backend.caps.maximum_actions||
       b->generation!=q->grid.generation||!output_apart(q,b,sizeof(*b))||!output_apart(q,o,sizeof(*o))||
       !output_apart(q,o->held.spans,o->held.count*sizeof(*o->held.spans))||
       !output_apart(q,out,sizeof(*out))||!apart(out,sizeof(*out),b,sizeof(*b))||
       !apart(out,sizeof(*out),o,sizeof(*o))||!apart(out,sizeof(*out),o->held.spans,o->held.count*sizeof(*o->held.spans)))return 0;
    if(target&&(!output_apart(q,target,b->count*sizeof(*target))||!apart(out,sizeof(*out),target,b->count*sizeof(*target))))return 0;
    for(i=0;i<o->held.count;++i)if(!span(o->held.spans[i].data,o->held.spans[i].bytes)||
       !apart(out,sizeof(*out),o->held.spans[i].data,o->held.spans[i].bytes)||
       !apart(q,sizeof(*q),o->held.spans[i].data,o->held.spans[i].bytes))return 0;
    *triggers=0;
    for(i=0;i<b->count;++i){const struct pt_scheduled_action *a=b->action+i;
        if(a->slot>=4||(slots&(1U<<a->slot))||a->volume>64)return 0;
        slots|=1U<<a->slot;
        if(a->kind==PT_SCHEDULED_TRIGGER){
            bytes=(size_t)a->words*2;
            if(!a->words||!a->period||((uintptr_t)a->data&1)||!span(a->data,bytes)||(target&&!key_zero(target+i)))return 0;
            for(j=0;j<o->held.count;++j){uintptr_t x=(uintptr_t)a->data,y=(uintptr_t)o->held.spans[j].data;
                if(x>=y&&x-y<=o->held.spans[j].bytes&&bytes<=o->held.spans[j].bytes-(x-y))break;}
            if(j==o->held.count)return 0;
            ++*triggers;
        }else if(a->kind==PT_SCHEDULED_CONTROL||a->kind==PT_SCHEDULED_STOP){
            if(!target||a->data||a->words||target[i].slot!=a->slot||!active(q,target+i,b->frame,1))return 0;
            if(a->kind==PT_SCHEDULED_CONTROL){if(!a->period)return 0;}
            else if(a->period||a->volume)return 0;
        }else return 0;
    }
    return 1;
}
enum pt_scheduled_result pt_lineage_enqueue(struct pt_lineage_output *q,
    const struct pt_scheduled_batch *b,const struct pt_lineage_key *target,
    const struct pt_lineage_owner *o,uint64_t *out)
{
    struct lineage_entry *e;struct pt_lineage_event candidate;uint64_t first,last;unsigned i,n;int good;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(q->closing||q->failed||!inputs(q,b,target,o,out,&n))return PT_SCHEDULED_INVALID;
    if(q->ordered&&b->frame<=q->last_frame)return PT_SCHEDULED_INVALID;
    if(b->frame==UINT64_MAX||pt_elapsed_clock_deadline(&q->clock,b->frame,&first)!=PT_ELAPSED_OK||
       pt_elapsed_clock_deadline(&q->clock,b->frame+1,&last)!=PT_ELAPSED_OK||first>=last)return PT_SCHEDULED_CLOCK;
    if(q->held==q->capacity||q->tickets==UINT64_MAX||n>UINT64_MAX-q->readers)return PT_SCHEDULED_CAPACITY;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state&&q->entry[i].owner.held.context==o->held.context&&
       q->entry[i].owner.held.token==o->held.token)return PT_SCHEDULED_INVALID;
    memset(&candidate,0,sizeof(candidate));candidate.scheduled.batch=*b;
    for(i=0;i<b->count;++i)if(b->action[i].kind!=PT_SCHEDULED_TRIGGER)candidate.key[i]=target[i];
    if(!targets_current(q,&candidate))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
    q->busy=1;good=o->held.current(o->held.context,o->held.token,b->generation)==1;q->busy=0;
    if(q->failed)return PT_SCHEDULED_BACKEND;
    if(!good)return PT_SCHEDULED_STALE;
    for(i=0;i<q->capacity;++i)if(!q->entry[i].state)break;
    e=q->entry+i;memset(e,0,sizeof(*e));e->event.queue=q;e->event.session=q->session;e->event.owner=o->held.token;
    e->event.scheduled.batch=*b;e->event.scheduled.first=first;e->event.scheduled.last=last;e->event.scheduled.ticket=++q->tickets;
    e->owner=*o;memcpy(e->spans,o->held.spans,o->held.count*sizeof(*o->held.spans));e->owner.held.spans=e->spans;
    for(i=0;i<b->count;++i)if(b->action[i].kind==PT_SCHEDULED_TRIGGER)
        e->event.key[i]=(struct pt_lineage_key){q,q->session,q->grid.generation,e->event.scheduled.ticket,o->held.token,++q->readers,i,b->action[i].slot};
    else {struct lineage_entry *source;e->event.key[i]=target[i];
        if(b->action[i].kind==PT_SCHEDULED_STOP){source=find(q,target[i].ticket);source->blocked|=1U<<target[i].action;}}
    e->state=LOCAL;++q->held;q->last_frame=b->frame;q->ordered=1;*out=e->event.scheduled.ticket;return PT_SCHEDULED_OK;
}
enum pt_scheduled_result pt_lineage_publish(struct pt_lineage_output *q,uint64_t ticket)
{
    struct lineage_entry *e;uint64_t now;uint32_t frequency;int result;unsigned i;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(q->closing||q->failed||!(e=find(q,ticket))||e->state!=LOCAL)return PT_SCHEDULED_INVALID;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state==LOCAL&&q->entry[i].event.scheduled.batch.frame<e->event.scheduled.batch.frame)return PT_SCHEDULED_INVALID;
    /* Blocking prevents new admission; earlier admitted controls and the STOP
     * itself still validate the exact source without mutating its block mask. */
    if(!targets_current(q,&e->event))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
    q->busy=1;result=e->owner.held.current(e->owner.held.context,e->owner.held.token,q->grid.generation);q->busy=0;
    if(q->failed)return PT_SCHEDULED_BACKEND;
    if(result!=1)return PT_SCHEDULED_STALE;
    q->busy=1;result=q->backend.read_clock(q->backend.context,&now,&frequency);q->busy=0;
    if(q->failed)return PT_SCHEDULED_BACKEND;
    if(result!=1||frequency!=q->grid.frequency||now<q->grid.epoch||(q->clock_seen&&now<q->last_ticks)){q->failed=1;return PT_SCHEDULED_CLOCK;}
    if(now>=e->event.scheduled.first){q->failed=1;return PT_SCHEDULED_LATE;}
    q->busy=1;result=q->backend.submit(q->backend.context,&e->event);q->busy=0;
    if(result==0&&!q->failed)return PT_SCHEDULED_PENDING;
    e->state=PUBLISHED;q->last_ticks=now;q->clock_seen=1;
    if(result==1&&!q->failed)return PT_SCHEDULED_OK;
    q->failed=1;return PT_SCHEDULED_BACKEND;
}
static int envelope(struct pt_lineage_output *q,const struct lineage_entry *e,const struct pt_lineage_receipt *r)
{return r->provenance==PT_LINEAGE_BACKEND_ACTUAL&&r->queue==q&&r->session==q->session&&r->generation==q->grid.generation&&r->ticket==e->event.scheduled.ticket&&
 r->owner==e->event.owner&&r->count==e->event.scheduled.batch.count;}
static int issued(enum pt_lineage_command c)
{return c==PT_LINEAGE_ISSUED||c==PT_LINEAGE_CANCELLED_AFTER;}
static int receipt_valid(struct pt_lineage_output *q,const struct lineage_entry *e,const struct pt_lineage_receipt *r,int retired)
{
    unsigned i;
    for(i=0;i<r->count;++i){const struct pt_lineage_action_receipt *a=r->action+i;
        const struct pt_scheduled_action *action=e->event.scheduled.batch.action+i;
        const struct pt_lineage_action_receipt *old=e->last.action+i;
        if(a->command!=r->action[0].command)return 0; /* no partial command batch */
        if((unsigned)a->command>PT_LINEAGE_UNKNOWN||(unsigned)a->reader>PT_LINEAGE_READER_UNKNOWN||a->command==PT_LINEAGE_UNKNOWN||
           a->command==PT_LINEAGE_FAILED||a->reader==PT_LINEAGE_READER_UNKNOWN)return 0;
        if(issued(a->command)){
            if(a->observed<e->event.scheduled.first||a->observed>a->issued||a->issued>=e->event.scheduled.last||
               !key_equal(&a->key,e->event.key+i))return 0;
            if(action->kind==PT_SCHEDULED_TRIGGER){if(a->reader==PT_LINEAGE_NONE||
                (a->command==PT_LINEAGE_CANCELLED_AFTER&&a->reader==PT_LINEAGE_ACTIVE))return 0;}
            else if(action->kind==PT_SCHEDULED_STOP){if(a->reader!=PT_LINEAGE_DRAINING&&a->reader!=PT_LINEAGE_RETIRED)return 0;}
            else if(a->reader==PT_LINEAGE_NONE)return 0;
        }else{
            if(a->observed||a->issued)return 0;
            if(action->kind==PT_SCHEDULED_TRIGGER){if(a->reader!=PT_LINEAGE_NONE||!key_zero(&a->key))return 0;}
            else if(!key_equal(&a->key,e->event.key+i)||a->reader==PT_LINEAGE_NONE)return 0;
        }
        if(action->kind!=PT_SCHEDULED_TRIGGER&&a->reader!=PT_LINEAGE_RETIRED){
            struct lineage_entry *source=find(q,a->key.ticket);const struct pt_lineage_action_receipt *original;
            if(!source||!source->seen||a->key.action>=source->last.count)return 0;
            original=source->last.action+a->key.action;
            if(original->reader==PT_LINEAGE_RETIRED||
               (original->reader==PT_LINEAGE_DRAINING&&a->reader!=PT_LINEAGE_DRAINING)||
               (original->reader==PT_LINEAGE_STOP_PENDING&&a->reader==PT_LINEAGE_ACTIVE))return 0;
        }
        if(retired&&(a->command==PT_LINEAGE_WAITING||
           (a->reader!=PT_LINEAGE_NONE&&a->reader!=PT_LINEAGE_RETIRED)))return 0;
        if(e->seen){
            if(old->command==PT_LINEAGE_CANCELLED_BEFORE&&a->command!=PT_LINEAGE_CANCELLED_BEFORE)return 0;
            if(issued(old->command)&&(!issued(a->command)||old->observed!=a->observed||old->issued!=a->issued))return 0;
            if(old->command==PT_LINEAGE_CANCELLED_AFTER&&a->command!=PT_LINEAGE_CANCELLED_AFTER)return 0;
            if(old->reader==PT_LINEAGE_RETIRED&&a->reader!=PT_LINEAGE_RETIRED)return 0;
            if(action->kind==PT_SCHEDULED_TRIGGER&&old->reader==PT_LINEAGE_STOP_PENDING&&a->reader==PT_LINEAGE_ACTIVE)return 0;
            if(old->reader==PT_LINEAGE_DRAINING&&a->reader!=PT_LINEAGE_DRAINING&&a->reader!=PT_LINEAGE_RETIRED)return 0;
        }
    }
    return 1;
}
static void release(struct pt_lineage_output *q,struct lineage_entry *e,const struct pt_lineage_receipt *r,int good)
{
    q->busy=1;if(e->owner.terminal)e->owner.terminal(e->owner.held.context,e->owner.held.token,r,good);
    e->owner.held.release(e->owner.held.context,e->owner.held.token);q->busy=0;
    memset(e,0,sizeof(*e));--q->held;
}
static void local_receipt(struct pt_lineage_output *q,const struct lineage_entry *e,struct pt_lineage_receipt *r)
{
    unsigned i;memset(r,0,sizeof(*r));r->provenance=PT_LINEAGE_LOCAL_UNSUBMITTED;r->queue=q;r->session=q->session;r->generation=q->grid.generation;
    r->ticket=e->event.scheduled.ticket;r->owner=e->event.owner;r->count=e->event.scheduled.batch.count;
    for(i=0;i<r->count;++i){r->action[i].command=PT_LINEAGE_CANCELLED_BEFORE;
        if(e->event.scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){r->action[i].key=e->event.key[i];r->action[i].reader=PT_LINEAGE_READER_UNKNOWN;}}
}
static void advance_sources(struct pt_lineage_output *q,const struct lineage_entry *e,const struct pt_lineage_receipt *r)
{
    unsigned i,j,k;
    for(i=0;i<r->count;++i)if(e->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER&&issued(r->action[i].command)){
        /* Positive issue is permanent replacement history, even if its ticket
         * retires before the old source is polled. No other reader retires. */
        for(j=0;j<q->capacity;++j)if(q->entry[j].state&&
           q->entry[j].event.scheduled.batch.frame<e->event.scheduled.batch.frame)
            for(k=0;k<q->entry[j].event.scheduled.batch.count;++k)
                if(q->entry[j].event.scheduled.batch.action[k].kind==PT_SCHEDULED_TRIGGER&&
                   q->entry[j].event.scheduled.batch.action[k].slot==e->event.scheduled.batch.action[i].slot)
                    q->entry[j].superseded|=1U<<k;
    }
    for(i=0;i<r->count;++i)if(e->event.scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER){
        const struct pt_lineage_action_receipt *a=r->action+i;struct lineage_entry *source=find(q,a->key.ticket);
        if(source&&source->seen&&a->key.action<source->last.count&&key_equal(&a->key,source->event.key+a->key.action)){
            struct pt_lineage_action_receipt *original=source->last.action+a->key.action;
            if(a->reader==PT_LINEAGE_RETIRED||
               (a->reader==PT_LINEAGE_DRAINING&&original->reader!=PT_LINEAGE_RETIRED)||
               (a->reader==PT_LINEAGE_STOP_PENDING&&original->reader==PT_LINEAGE_ACTIVE))original->reader=a->reader;
        }
    }
}
static enum pt_scheduled_result inspect(struct pt_lineage_output *q,struct lineage_entry *e,int cancelling,struct pt_lineage_receipt *out)
{
    struct pt_lineage_receipt r;enum pt_lineage_reply reply;int good;
    memset(&r,0,sizeof(r));q->busy=1;reply=cancelling?q->backend.cancel(q->backend.context,e->event.scheduled.ticket,&r):
        q->backend.poll(q->backend.context,e->event.scheduled.ticket,&r);q->busy=0;
    if(reply==PT_LINEAGE_PENDING&&!q->failed)return PT_SCHEDULED_PENDING;
    if(reply!=PT_LINEAGE_OBSERVATION&&reply!=PT_LINEAGE_ALL_RETIRED){q->failed=1;return PT_SCHEDULED_BACKEND;}
    if(!envelope(q,e,&r)){q->failed=1;return PT_SCHEDULED_BACKEND;}
    good=receipt_valid(q,e,&r,reply==PT_LINEAGE_ALL_RETIRED);
    if(!good)q->failed=1;
    if(good)advance_sources(q,e,&r);
    if(reply==PT_LINEAGE_ALL_RETIRED){release(q,e,&r,good);if(!good||q->failed)return PT_SCHEDULED_BACKEND;}
    else {if(!good||q->failed)return PT_SCHEDULED_BACKEND;e->last=r;e->seen=1;}
    if(out)*out=r;
    return reply==PT_LINEAGE_ALL_RETIRED?PT_SCHEDULED_OK:PT_SCHEDULED_PENDING;
}
static enum pt_scheduled_result check(struct pt_lineage_output *q,uint64_t ticket,struct pt_lineage_receipt *out,int cancelling)
{
    struct lineage_entry *e;struct pt_lineage_receipt r;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(!out||!output_apart(q,out,sizeof(*out))||!(e=find(q,ticket)))return PT_SCHEDULED_INVALID;
    if(e->state==LOCAL){if(!cancelling)return PT_SCHEDULED_INVALID;local_receipt(q,e,&r);release(q,e,&r,1);*out=r;return PT_SCHEDULED_OK;}
    return inspect(q,e,cancelling,out);
}
enum pt_scheduled_result pt_lineage_poll(struct pt_lineage_output *q,uint64_t t,struct pt_lineage_receipt *out)
{return check(q,t,out,0);}
enum pt_scheduled_result pt_lineage_cancel(struct pt_lineage_output *q,uint64_t t,struct pt_lineage_receipt *out)
{return check(q,t,out,1);}
enum pt_scheduled_result pt_lineage_reader_key(struct pt_lineage_output *q,uint64_t t,unsigned i,struct pt_lineage_key *out)
{
    struct lineage_entry *e;int current;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(!out||!output_apart(q,out,sizeof(*out))||q->closing||q->failed||!(e=find(q,t))||
       i>=e->event.scheduled.batch.count||e->event.scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER||
       e->event.key[i].ticket!=t||e->event.key[i].action!=i)return PT_SCHEDULED_INVALID;
    if(!active(q,e->event.key+i,UINT64_MAX,1))return PT_SCHEDULED_STALE;
    q->busy=1;current=e->owner.held.current(e->owner.held.context,e->owner.held.token,q->grid.generation)==1;q->busy=0;
    if(q->failed)return PT_SCHEDULED_BACKEND;
    if(!current)return PT_SCHEDULED_STALE;
    *out=e->event.key[i];return PT_SCHEDULED_OK;
}
enum pt_scheduled_result pt_lineage_stop(struct pt_lineage_output *q)
{
    unsigned i;int failed=0;enum pt_scheduled_result result;struct pt_lineage_receipt r;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    q->closing=1;
    for(i=0;i<q->capacity;++i)if(q->entry[i].state){
        if(q->entry[i].state==LOCAL){local_receipt(q,q->entry+i,&r);release(q,q->entry+i,&r,1);}
        else {result=inspect(q,q->entry+i,1,NULL);if(result==PT_SCHEDULED_BACKEND)failed=1;}
    }
    return failed||q->failed?PT_SCHEDULED_BACKEND:q->held?PT_SCHEDULED_PENDING:PT_SCHEDULED_OK;
}
unsigned pt_lineage_held(const struct pt_lineage_output *q)
{return q?q->held:0;}
int pt_lineage_close(struct pt_lineage_output *q)
{
    struct pt_allocator a;
    if(!q)return 0;
    if(reentry(q)||q->held)return 0;
    a=q->allocator;a.release(a.context,q);return 1;
}
