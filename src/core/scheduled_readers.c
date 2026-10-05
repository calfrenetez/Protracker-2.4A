#include "scheduled_readers.h"
#include "document.h"
#include <string.h>
struct reader_entry {
    struct pt_readers_domain domain;struct pt_readers_owner owner;
    struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];
    uint64_t frame,first,last,observed,issued;
    unsigned held,submitted,references,adopted,timed,closed,superseded,cancel_requested,retired,valid;
    enum pt_readers_state state;
};
struct command_entry {
    struct pt_readers_event event;struct pt_readers_control owner;
    struct pt_readers_command_receipt last;unsigned held,published,seen;
    unsigned reader[PT_READERS_ACTIONS];
};
struct pt_readers_output {
    struct pt_allocator allocator;struct pt_scheduled_grid grid;
    struct pt_readers_backend backend;struct pt_elapsed_clock clock;
    uint64_t session,tickets,serial,last_frame,last_ticks;
    unsigned commands,readers,command_count,reader_count,ordered,clock_seen,closing,failed,busy;
    struct command_entry command[PT_READERS_COMMANDS];
    struct reader_entry reader[PT_READERS_PERSISTENT];
};
static int span(const void *p,size_t n)
{return !n||(p&&(uintptr_t)p<=UINTPTR_MAX-(n-1));}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int zero_key(const struct pt_readers_key *k)
{return !k->queue&&!k->session&&!k->generation&&!k->trigger&&!k->owner&&!k->serial&&!k->action&&!k->slot;}
static int equal_key(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static struct command_entry *command(struct pt_readers_output *q,uint64_t t)
{unsigned i;for(i=0;i<q->commands;++i)if(q->command[i].held&&q->command[i].event.scheduled.ticket==t)return q->command+i;return NULL;}
static struct reader_entry *reader(struct pt_readers_output *q,uint64_t t,unsigned a)
{unsigned i;for(i=0;i<q->readers;++i)if(q->reader[i].held&&q->reader[i].domain.key.trigger==t&&q->reader[i].domain.key.action==a)return q->reader+i;return NULL;}
static struct reader_entry *key_reader(struct pt_readers_output *q,const struct pt_readers_key *k)
{struct reader_entry *r=reader(q,k->trigger,k->action);return r&&equal_key(&r->domain.key,k)?r:NULL;}
static int reentry(struct pt_readers_output *q)
{if(q->busy){q->failed=1;return 1;}return 0;}
static int output_apart(const struct pt_readers_output *q,const void *p,size_t n)
{
    unsigned i,j;
    if(!n||!apart(p,n,q,sizeof(*q))||!apart(p,n,q->backend.context,q->backend.context_bytes))return 0;
    for(i=0;i<q->commands;++i)if(q->command[i].held&&!apart(p,n,q->command[i].owner.context,q->command[i].owner.context_bytes))return 0;
    for(i=0;i<q->readers;++i)if(q->reader[i].held){
        const struct reader_entry *r=q->reader+i;
        if(!apart(p,n,r->owner.control.context,r->owner.control.context_bytes))return 0;
        for(j=0;j<r->owner.count;++j)if(!apart(p,n,r->spans[j].data,r->spans[j].bytes))return 0;
    }
    return 1;
}
static int control_valid(const struct pt_readers_control *o)
{return o&&span(o,sizeof(*o))&&o->context_bytes&&span(o->context,o->context_bytes)&&o->token&&o->current&&o->release;}
static int zero_owner(const struct pt_readers_owner *o)
{return !o->control.context&&!o->control.context_bytes&&!o->control.token&&!o->control.current&&!o->control.release&&!o->control.terminal&&!o->spans&&!o->count;}
static int current(struct pt_readers_output *q,const struct pt_readers_control *o)
{int good;q->busy=1;good=o->current(o->context,o->token,q->grid.generation);q->busy=0;return good==1&&!q->failed;}
static int active(struct pt_readers_output *q,const struct reader_entry *r,uint64_t frame,int admission)
{
    unsigned i,j;
    if(!r||!r->held||!r->submitted||!r->adopted||!r->timed||r->retired||r->superseded||r->cancel_requested||r->state!=PT_READERS_ACTIVE||r->frame>=frame||(admission&&r->closed))return 0;
    for(i=0;i<q->commands;++i)if(q->command[i].held&&q->command[i].event.scheduled.batch.frame>r->frame&&q->command[i].event.scheduled.batch.frame<frame)
        for(j=0;j<q->command[i].event.scheduled.batch.count;++j){
            const struct pt_scheduled_action *a=q->command[i].event.scheduled.batch.action+j;
            if(a->slot==r->domain.key.slot&&(a->kind==PT_SCHEDULED_TRIGGER||a->kind==PT_SCHEDULED_STOP))return 0;
        }
    return 1;
}
static int capable(const struct pt_readers_backend *b,unsigned c,unsigned r)
{return b&&span(b,sizeof(*b))&&c&&c<=PT_READERS_COMMANDS&&r&&r<=PT_READERS_PERSISTENT&&b->caps.flags==PT_SCHEDULED_REQUIRED&&b->caps.maximum_batches>=c&&b->caps.maximum_batches<=PT_READERS_COMMANDS&&b->caps.maximum_actions&&b->caps.maximum_actions<=PT_READERS_ACTIONS&&b->maximum_readers>=r&&b->maximum_readers<=PT_READERS_PERSISTENT&&b->version==PT_READERS_VERSION&&b->reference_flags==PT_READERS_REQUIRED&&b->read_clock&&b->submit&&b->poll_command&&b->cancel_command&&b->poll_reader&&b->cancel_reader;}
enum pt_scheduled_result pt_readers_open(const struct pt_allocator *a,const struct pt_scheduled_grid *g,uint64_t s,const struct pt_readers_backend *b,unsigned c,unsigned r,struct pt_readers_output **out)
{
    struct pt_readers_output *q;struct pt_elapsed_clock clock;
    if(!a||!span(a,sizeof(*a))||!a->allocate||!a->release||!g||!span(g,sizeof(*g))||!out||!s||!g->generation||g->frequency<g->rate||pt_elapsed_clock_init(&clock,g->frequency,g->rate,g->epoch,0)!=PT_ELAPSED_OK)return PT_SCHEDULED_INVALID;
    if(!capable(b,c,r))return PT_SCHEDULED_UNSUPPORTED;
    if(!b->context_bytes||!span(b->context,b->context_bytes)||!apart(out,sizeof(*out),a,sizeof(*a))||!apart(out,sizeof(*out),g,sizeof(*g))||!apart(out,sizeof(*out),b,sizeof(*b))||!apart(out,sizeof(*out),b->context,b->context_bytes))return PT_SCHEDULED_INVALID;
    q=a->allocate(a->context,sizeof(*q));if(!q)return PT_SCHEDULED_CAPACITY;
    if(!apart(q,sizeof(*q),out,sizeof(*out))||!apart(q,sizeof(*q),a,sizeof(*a))||!apart(q,sizeof(*q),g,sizeof(*g))||!apart(q,sizeof(*q),b,sizeof(*b))||!apart(q,sizeof(*q),b->context,b->context_bytes)){a->release(a->context,q);return PT_SCHEDULED_INVALID;}
    memset(q,0,sizeof(*q));q->allocator=*a;q->grid=*g;q->clock=clock;q->backend=*b;q->session=s;q->commands=c;q->readers=r;*out=q;return PT_SCHEDULED_OK;
}
static int covered(const struct pt_readers_owner *o,const struct pt_scheduled_action *a)
{
    unsigned i;size_t bytes=(size_t)a->words*2;uintptr_t x=(uintptr_t)a->data;
    if(!a->words||!a->period||(x&1)||!span(a->data,bytes)||!apart(a->data,bytes,o->control.context,o->control.context_bytes))return 0;
    for(i=0;i<o->count;++i){uintptr_t y=(uintptr_t)o->spans[i].data;if(x>=y&&x-y<=o->spans[i].bytes&&bytes<=o->spans[i].bytes-(x-y))return 1;}
    return 0;
}
static int inputs(struct pt_readers_output *q,const struct pt_scheduled_batch *b,const struct pt_readers_key *keys,const struct pt_readers_control *c,const struct pt_readers_owner *owners,uint64_t *out,unsigned *new_readers)
{
    unsigned i,j,k,slots=0;
    if(!b||!c||!out||!output_apart(q,b,sizeof(*b))||!output_apart(q,c,sizeof(*c))||!control_valid(c)||!output_apart(q,c->context,c->context_bytes)||!output_apart(q,out,sizeof(*out))||!apart(out,sizeof(*out),b,sizeof(*b))||!apart(out,sizeof(*out),c,sizeof(*c))||!apart(out,sizeof(*out),c->context,c->context_bytes)||!b->count||b->count>q->backend.caps.maximum_actions||b->generation!=q->grid.generation)return 0;
    if(keys&&(!output_apart(q,keys,b->count*sizeof(*keys))||!apart(out,sizeof(*out),keys,b->count*sizeof(*keys))))return 0;
    if(owners&&(!output_apart(q,owners,b->count*sizeof(*owners))||!apart(out,sizeof(*out),owners,b->count*sizeof(*owners))))return 0;
    *new_readers=0;
    for(i=0;i<b->count;++i){
        const struct pt_scheduled_action *a=b->action+i;
        if(a->slot>=4||(slots&(1U<<a->slot))||a->volume>64)return 0;
        slots|=1U<<a->slot;
        if(a->kind==PT_SCHEDULED_TRIGGER){
            const struct pt_readers_owner *o;if(!owners||(keys&&!zero_key(keys+i)))return 0;o=owners+i;
            if(!control_valid(&o->control)||!o->count||o->count>PT_SCHEDULED_SPANS||!o->spans||!output_apart(q,o->control.context,o->control.context_bytes)||!output_apart(q,o->spans,o->count*sizeof(*o->spans))||!apart(out,sizeof(*out),o->control.context,o->control.context_bytes)||!apart(out,sizeof(*out),o->spans,o->count*sizeof(*o->spans))||!apart(c->context,c->context_bytes,o->control.context,o->control.context_bytes))return 0;
            for(j=0;j<o->count;++j){
                if(!span(o->spans[j].data,o->spans[j].bytes)||!apart(out,sizeof(*out),o->spans[j].data,o->spans[j].bytes)||!apart(q,sizeof(*q),o->spans[j].data,o->spans[j].bytes)||!apart(q->backend.context,q->backend.context_bytes,o->spans[j].data,o->spans[j].bytes)||!apart(c->context,c->context_bytes,o->spans[j].data,o->spans[j].bytes)||!apart(o->control.context,o->control.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
                for(k=0;k<q->commands;++k)if(q->command[k].held&&!apart(q->command[k].owner.context,q->command[k].owner.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
                for(k=0;k<q->readers;++k)if(q->reader[k].held&&!apart(q->reader[k].owner.control.context,q->reader[k].owner.control.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
                for(k=0;k<i;++k)if(b->action[k].kind==PT_SCHEDULED_TRIGGER&&!apart(owners[k].control.context,owners[k].control.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
            }
            for(j=0;j<i;++j)if(b->action[j].kind==PT_SCHEDULED_TRIGGER){
                if(!apart(o->control.context,o->control.context_bytes,owners[j].control.context,owners[j].control.context_bytes))return 0;
                for(k=0;k<owners[j].count;++k)if(!apart(o->control.context,o->control.context_bytes,owners[j].spans[k].data,owners[j].spans[k].bytes))return 0;
            }
            if(!covered(o,a))return 0;
            ++*new_readers;
        }else if(a->kind==PT_SCHEDULED_CONTROL||a->kind==PT_SCHEDULED_STOP){
            struct reader_entry *r;if(!keys||a->data||a->words||(owners&&!zero_owner(owners+i))||keys[i].slot!=a->slot||!(r=key_reader(q,keys+i))||!active(q,r,b->frame,1))return 0;
            if(a->kind==PT_SCHEDULED_CONTROL){if(!a->period)return 0;}else if(a->period||a->volume)return 0;
        }else return 0;
    }
    return 1;
}
enum pt_scheduled_result pt_readers_enqueue(struct pt_readers_output *q,const struct pt_scheduled_batch *b,const struct pt_readers_key *keys,const struct pt_readers_control *control,const struct pt_readers_owner *owners,uint64_t *out)
{
    struct {
        struct pt_scheduled_batch batch;struct pt_readers_key keys[PT_READERS_ACTIONS];
        struct pt_readers_control control;struct pt_readers_owner owner[PT_READERS_ACTIONS];
        struct pt_scheduled_span spans[PT_READERS_ACTIONS][PT_SCHEDULED_SPANS];
    } saved;
    unsigned i,j,n,indices[PT_READERS_ACTIONS],used=0;uint64_t first,last,ticket;struct command_entry *e;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(q->closing||q->failed||!inputs(q,b,keys,control,owners,out,&n))return PT_SCHEDULED_INVALID;
    /* Copy all declaration metadata before ANY caller callback. Holders may
     * contain their own descriptor declarations; transfer uses this bounded
     * snapshot instead of reading mutable holder extents after current(). */
    saved.batch=*b;saved.control=*control;
    if(keys)memcpy(saved.keys,keys,b->count*sizeof(*keys));
    if(owners){memcpy(saved.owner,owners,b->count*sizeof(*owners));
        for(i=0;i<b->count;++i)if(b->action[i].kind==PT_SCHEDULED_TRIGGER){
            memcpy(saved.spans[i],owners[i].spans,owners[i].count*sizeof(*owners[i].spans));saved.owner[i].spans=saved.spans[i];}}
    b=&saved.batch;control=&saved.control;if(keys)keys=saved.keys;if(owners)owners=saved.owner;
    if(q->ordered&&b->frame<=q->last_frame)return PT_SCHEDULED_INVALID;
    if(b->frame==UINT64_MAX||pt_elapsed_clock_deadline(&q->clock,b->frame,&first)!=PT_ELAPSED_OK||pt_elapsed_clock_deadline(&q->clock,b->frame+1,&last)!=PT_ELAPSED_OK||first>=last)return PT_SCHEDULED_CLOCK;
    if(q->command_count==q->commands||n>q->readers-q->reader_count||q->tickets==UINT64_MAX||n>UINT64_MAX-q->serial)return PT_SCHEDULED_CAPACITY;
    for(i=0;i<b->count;++i){
        if(b->action[i].kind==PT_SCHEDULED_TRIGGER){
            if(!current(q,&owners[i].control))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
            for(j=0;j<q->readers;++j)if(!q->reader[j].held&&!(used&(1U<<j)))break;
            indices[i]=j;used|=1U<<j;
        }else{
            struct reader_entry *r=key_reader(q,keys+i);if(!current(q,&r->owner.control))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
            indices[i]=(unsigned)(r-q->reader);
        }
    }
    if(!current(q,control))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
    for(i=0;i<q->commands;++i)if(!q->command[i].held)break;
    e=q->command+i;memset(e,0,sizeof(*e));e->held=1;e->owner=*control;ticket=++q->tickets;
    e->event.queue=q;e->event.session=q->session;e->event.command_owner=control->token;
    e->event.binding=(struct pt_readers_binding){control->context,control->context_bytes};
    e->event.scheduled=(struct pt_scheduled_event){ticket,first,last,*b};
    for(i=0;i<b->count;++i){
        struct reader_entry *r=q->reader+indices[i];e->reader[i]=indices[i];
        if(b->action[i].kind==PT_SCHEDULED_TRIGGER){
            memset(r,0,sizeof(*r));r->held=1;r->owner=owners[i];memcpy(r->spans,owners[i].spans,owners[i].count*sizeof(*r->spans));r->owner.spans=r->spans;
            r->domain=(struct pt_readers_domain){{q,q->session,q->grid.generation,ticket,owners[i].control.token,++q->serial,i,b->action[i].slot},r->spans,owners[i].count,{owners[i].control.context,owners[i].control.context_bytes}};
            r->frame=b->frame;r->first=first;r->last=last;r->state=PT_READERS_RESERVED;r->valid=1;++q->reader_count;
        }else if(b->action[i].kind==PT_SCHEDULED_STOP)r->closed=1;
        ++r->references;e->event.reader[i]=&r->domain;
    }
    ++q->command_count;q->ordered=1;q->last_frame=b->frame;*out=ticket;return PT_SCHEDULED_OK;
}
static int targets_current(struct pt_readers_output *q,const struct command_entry *e)
{
    unsigned i;
    for(i=0;i<e->event.scheduled.batch.count;++i){struct reader_entry *r=q->reader+e->reader[i];
        if(!r->held||r->retired||!current(q,&r->owner.control))return 0;
        if(e->event.scheduled.batch.action[i].kind!=PT_SCHEDULED_TRIGGER&&!active(q,r,e->event.scheduled.batch.frame,0))return 0;
    }
    return current(q,&e->owner);
}
enum pt_scheduled_result pt_readers_publish(struct pt_readers_output *q,uint64_t t)
{
    struct command_entry *e;uint64_t now;uint32_t frequency;int result;unsigned i;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(q->closing||q->failed||!(e=command(q,t))||e->published)return PT_SCHEDULED_INVALID;
    for(i=0;i<q->commands;++i)if(q->command[i].held&&!q->command[i].published&&q->command[i].event.scheduled.batch.frame<e->event.scheduled.batch.frame)return PT_SCHEDULED_INVALID;
    if(!targets_current(q,e))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
    q->busy=1;result=q->backend.read_clock(q->backend.context,&now,&frequency);q->busy=0;
    if(q->failed)return PT_SCHEDULED_BACKEND;
    if(result!=1||frequency!=q->grid.frequency||now<q->grid.epoch||(q->clock_seen&&now<q->last_ticks)){q->failed=1;return PT_SCHEDULED_CLOCK;}
    if(now>=e->event.scheduled.first){q->failed=1;return PT_SCHEDULED_LATE;}
    q->busy=1;result=q->backend.submit(q->backend.context,&e->event);q->busy=0;
    if(result==0&&!q->failed)return PT_SCHEDULED_PENDING;
    e->published=1;for(i=0;i<e->event.scheduled.batch.count;++i)if(e->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER)q->reader[e->reader[i]].submitted=1;
    q->last_ticks=now;q->clock_seen=1;if(result==1&&!q->failed)return PT_SCHEDULED_OK;
    q->failed=1;return PT_SCHEDULED_BACKEND;
}
static void reader_release(struct pt_readers_output *q,struct reader_entry *r)
{
    struct pt_readers_control owner=r->owner.control;int valid=r->valid;
    if(!r->held||r->references||!r->retired)return;
    memset(r,0,sizeof(*r));--q->reader_count;q->busy=1;if(owner.terminal)owner.terminal(owner.context,owner.token,valid);owner.release(owner.context,owner.token);q->busy=0;
}
static void command_release(struct pt_readers_output *q,struct command_entry *e,int valid,int local)
{
    struct pt_readers_control owner=e->owner;unsigned indices[PT_READERS_ACTIONS],i,n=e->event.scheduled.batch.count;
    for(i=0;i<n;++i){struct reader_entry *r=q->reader+e->reader[i];indices[i]=e->reader[i];--r->references;
        if(local&&e->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER){r->retired=1;r->closed=1;r->state=PT_READERS_RETIRED;}}
    memset(e,0,sizeof(*e));--q->command_count;q->busy=1;if(owner.terminal)owner.terminal(owner.context,owner.token,valid);owner.release(owner.context,owner.token);q->busy=0;
    for(i=0;i<n;++i)reader_release(q,q->reader+indices[i]);
}
static int issued(enum pt_readers_command c)
{return c==PT_READERS_ISSUED||c==PT_READERS_CANCELLED_AFTER;}
static int command_envelope(const struct pt_readers_output *q,const struct command_entry *e,const struct pt_readers_command_receipt *r)
{return r->domain==PT_READERS_COMMAND_DOMAIN&&r->origin==PT_READERS_BACKEND_ACTUAL&&r->queue==q&&r->session==q->session&&r->generation==q->grid.generation&&r->ticket==e->event.scheduled.ticket&&r->owner==e->owner.token&&r->count==e->event.scheduled.batch.count&&r->event==&e->event&&r->context==e->owner.context&&r->context_bytes==e->owner.context_bytes;}
static int reader_state_valid(const struct reader_entry *r,enum pt_readers_state state,enum pt_readers_adoption adoption)
{
    if((unsigned)state>PT_READERS_RETIRED||(unsigned)adoption>PT_READERS_ADOPTED)return 0;
    if(r->retired&&state!=PT_READERS_RETIRED)return 0;
    if(r->adopted&&adoption!=PT_READERS_ADOPTED)return 0;
    if(r->adopted&&state<r->state)return 0;
    if(adoption==PT_READERS_ADOPTED&&state<PT_READERS_ACTIVE)return 0;
    if(adoption==PT_READERS_UNADOPTED&&state!=PT_READERS_NONE&&state!=PT_READERS_RESERVED&&state!=PT_READERS_RETIRED)return 0;
    return 1;
}
static int command_reader_state_valid(const struct reader_entry *r,
    const struct pt_readers_action_receipt *a,enum pt_scheduled_kind kind)
{
    /* Independent positive reader retirement does not rewrite a cancelled,
     * never-issued TRIGGER command's NONE+UNADOPTED snapshot. It also cannot
     * detach that command, revive the reader, or create adoption/timing. */
    if(kind==PT_SCHEDULED_TRIGGER&&a->command==PT_READERS_CANCELLED_BEFORE&&
       a->reader==PT_READERS_NONE&&a->adoption==PT_READERS_UNADOPTED&&
       !a->observed&&!a->issued&&r->retired&&r->state==PT_READERS_RETIRED&&
       r->valid&&!r->adopted&&!r->timed)return 1;
    return reader_state_valid(r,a->reader,a->adoption);
}
static int command_valid(const struct pt_readers_output *q,const struct command_entry *e,const struct pt_readers_command_receipt *receipt,int detached)
{
    unsigned i;
    for(i=0;i<receipt->count;++i){
        const struct pt_readers_action_receipt *a=receipt->action+i,*old=e->last.action+i;const struct reader_entry *r=q->reader+e->reader[i];enum pt_scheduled_kind kind=e->event.scheduled.batch.action[i].kind;
        if((unsigned)a->command>PT_READERS_CANCELLED_AFTER||a->command!=receipt->action[0].command||!equal_key(&a->key,&r->domain.key)||!command_reader_state_valid(r,a,kind))return 0;
        if(issued(a->command)){
            if(a->observed<e->event.scheduled.first||a->observed>a->issued||a->issued>=e->event.scheduled.last)return 0;
            if(kind==PT_SCHEDULED_STOP&&(a->reader!=PT_READERS_DRAINING&&a->reader!=PT_READERS_RETIRED))return 0;
            if(kind!=PT_SCHEDULED_TRIGGER&&a->adoption!=PT_READERS_ADOPTED)return 0;
            if(kind==PT_SCHEDULED_TRIGGER&&r->timed&&(r->observed!=a->observed||r->issued!=a->issued))return 0;
        }else{
            if(a->observed||a->issued)return 0;
            if(kind==PT_SCHEDULED_TRIGGER&&(a->reader!=PT_READERS_NONE||a->adoption!=PT_READERS_UNADOPTED))return 0;
            if(kind!=PT_SCHEDULED_TRIGGER&&a->adoption!=PT_READERS_ADOPTED)return 0;
        }
        if(detached&&a->command==PT_READERS_WAITING)return 0;
        if(e->seen){
            if(old->command==PT_READERS_CANCELLED_BEFORE&&a->command!=old->command)return 0;
            if(issued(old->command)&&(!issued(a->command)||old->observed!=a->observed||old->issued!=a->issued))return 0;
            if(old->command==PT_READERS_CANCELLED_AFTER&&a->command!=old->command)return 0;
        }
    }
    return 1;
}
static void advance(struct pt_readers_output *q,const struct command_entry *e,const struct pt_readers_command_receipt *receipt)
{
    unsigned i,j;
    for(i=0;i<receipt->count;++i){struct reader_entry *r=q->reader+e->reader[i];const struct pt_readers_action_receipt *a=receipt->action+i;
        if(e->event.scheduled.batch.action[i].kind==PT_SCHEDULED_TRIGGER&&issued(a->command)){
            r->timed=1;r->observed=a->observed;r->issued=a->issued;
            for(j=0;j<q->readers;++j)if(q->reader[j].held&&q->reader[j].frame<r->frame&&q->reader[j].domain.key.slot==r->domain.key.slot){q->reader[j].closed=1;q->reader[j].superseded=1;}
        }
        if(a->adoption==PT_READERS_ADOPTED)r->adopted=1;
        if(a->reader>r->state)r->state=a->reader;
        if(a->reader>=PT_READERS_STOP_PENDING)r->closed=1;
    }
}
static enum pt_scheduled_result command_check(struct pt_readers_output *q,uint64_t t,struct pt_readers_command_receipt *out,int cancel)
{
    struct command_entry *e;struct pt_readers_command_receipt receipt;enum pt_readers_reply reply;int good;unsigned i;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(!out||!output_apart(q,out,sizeof(*out))||!(e=command(q,t)))return PT_SCHEDULED_INVALID;
    if(!e->published){
        if(!cancel)return PT_SCHEDULED_INVALID;
        memset(&receipt,0,sizeof(receipt));receipt.domain=PT_READERS_COMMAND_DOMAIN;receipt.origin=PT_READERS_LOCAL_UNSUBMITTED;receipt.queue=q;receipt.session=q->session;receipt.generation=q->grid.generation;receipt.ticket=t;receipt.owner=e->owner.token;receipt.count=e->event.scheduled.batch.count;
        for(i=0;i<receipt.count;++i){receipt.action[i].command=PT_READERS_CANCELLED_BEFORE;receipt.action[i].reader=PT_READERS_STATE_UNKNOWN;receipt.action[i].key=q->reader[e->reader[i]].domain.key;}
        /* Local proof has no backend provenance and never exposes ACTIVE. */
        command_release(q,e,1,1);*out=receipt;return PT_SCHEDULED_OK;
    }
    memset(&receipt,0,sizeof(receipt));q->busy=1;reply=cancel?q->backend.cancel_command(q->backend.context,t,&receipt):q->backend.poll_command(q->backend.context,t,&receipt);q->busy=0;
    if(reply==PT_READERS_PENDING&&!q->failed)return PT_SCHEDULED_PENDING;
    if((reply!=PT_READERS_OBSERVATION&&reply!=PT_READERS_COMMAND_DETACHED)||!command_envelope(q,e,&receipt)){q->failed=1;return PT_SCHEDULED_BACKEND;}
    good=command_valid(q,e,&receipt,reply==PT_READERS_COMMAND_DETACHED);if(!good)q->failed=1;
    if(good)advance(q,e,&receipt);
    if(reply==PT_READERS_COMMAND_DETACHED)command_release(q,e,good,0);
    else if(good){e->last=receipt;e->seen=1;}
    if(!good||q->failed)return PT_SCHEDULED_BACKEND;
    *out=receipt;return reply==PT_READERS_COMMAND_DETACHED?PT_SCHEDULED_OK:PT_SCHEDULED_PENDING;
}
enum pt_scheduled_result pt_readers_poll_command(struct pt_readers_output *q,uint64_t t,struct pt_readers_command_receipt *o)
{return command_check(q,t,o,0);}
enum pt_scheduled_result pt_readers_cancel_command(struct pt_readers_output *q,uint64_t t,struct pt_readers_command_receipt *o)
{return command_check(q,t,o,1);}
static int reader_envelope(const struct reader_entry *r,const struct pt_readers_reader_receipt *a)
{return a->domain==PT_READERS_READER_DOMAIN&&equal_key(&a->key,&r->domain.key)&&a->reference==&r->domain&&a->context==r->owner.control.context&&a->context_bytes==r->owner.control.context_bytes;}
static int reader_valid(const struct reader_entry *r,const struct pt_readers_reader_receipt *a,int retired)
{
    if(!reader_state_valid(r,a->state,a->adoption)||(retired&&a->state!=PT_READERS_RETIRED))return 0;
    if(a->adoption==PT_READERS_ADOPTED){
        if(a->observed<r->first||a->observed>a->issued||a->issued>=r->last||(r->timed&&(a->observed!=r->observed||a->issued!=r->issued)))return 0;
    }else if(a->observed||a->issued)return 0;
    return 1;
}
static enum pt_scheduled_result reader_check(struct pt_readers_output *q,uint64_t t,unsigned i,struct pt_readers_reader_receipt *out,int cancel)
{
    struct reader_entry *r;struct pt_readers_reader_receipt receipt;enum pt_readers_reply reply;int good;unsigned j;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(!out||!output_apart(q,out,sizeof(*out))||!(r=reader(q,t,i))||!r->submitted)return PT_SCHEDULED_INVALID;
    if(cancel){r->closed=1;r->cancel_requested=1;}
    memset(&receipt,0,sizeof(receipt));q->busy=1;reply=cancel?q->backend.cancel_reader(q->backend.context,&r->domain,&receipt):q->backend.poll_reader(q->backend.context,&r->domain,&receipt);q->busy=0;
    if(reply==PT_READERS_PENDING&&!q->failed)return PT_SCHEDULED_PENDING;
    if((reply!=PT_READERS_OBSERVATION&&reply!=PT_READERS_READER_RETIRED)||!reader_envelope(r,&receipt)){q->failed=1;return PT_SCHEDULED_BACKEND;}
    good=reader_valid(r,&receipt,reply==PT_READERS_READER_RETIRED);if(!good)q->failed=1;
    if(good){r->state=receipt.state;if(receipt.adoption==PT_READERS_ADOPTED){r->adopted=1;r->timed=1;r->observed=receipt.observed;r->issued=receipt.issued;for(j=0;j<q->readers;++j)if(q->reader[j].held&&q->reader[j].frame<r->frame&&q->reader[j].domain.key.slot==r->domain.key.slot){q->reader[j].closed=1;q->reader[j].superseded=1;}}if(receipt.state>=PT_READERS_STOP_PENDING)r->closed=1;}
    if(reply==PT_READERS_READER_RETIRED){r->retired=1;r->closed=1;r->valid=r->valid&&(unsigned)good;reader_release(q,r);}
    if(!good||q->failed)return PT_SCHEDULED_BACKEND;
    *out=receipt;return reply==PT_READERS_READER_RETIRED?PT_SCHEDULED_OK:PT_SCHEDULED_PENDING;
}
enum pt_scheduled_result pt_readers_poll_reader(struct pt_readers_output *q,uint64_t t,unsigned i,struct pt_readers_reader_receipt *o)
{return reader_check(q,t,i,o,0);}
enum pt_scheduled_result pt_readers_cancel_reader(struct pt_readers_output *q,uint64_t t,unsigned i,struct pt_readers_reader_receipt *o)
{return reader_check(q,t,i,o,1);}
enum pt_scheduled_result pt_readers_reader_key(struct pt_readers_output *q,uint64_t t,unsigned i,struct pt_readers_key *out)
{
    struct reader_entry *r;
    if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    if(!out||!output_apart(q,out,sizeof(*out))||q->closing||q->failed||!(r=reader(q,t,i)))return PT_SCHEDULED_INVALID;
    if(!active(q,r,UINT64_MAX,1)||!current(q,&r->owner.control))return q->failed?PT_SCHEDULED_BACKEND:PT_SCHEDULED_STALE;
    *out=r->domain.key;return PT_SCHEDULED_OK;
}
enum pt_scheduled_result pt_readers_stop(struct pt_readers_output *q)
{
    unsigned i;if(!q)return PT_SCHEDULED_INVALID;
    if(reentry(q))return PT_SCHEDULED_BACKEND;
    q->closing=1;for(i=0;i<q->commands;++i)if(q->command[i].held&&!q->command[i].published)command_release(q,q->command+i,1,1);
    return q->failed?PT_SCHEDULED_BACKEND:q->command_count||q->reader_count?PT_SCHEDULED_PENDING:PT_SCHEDULED_OK;
}
unsigned pt_readers_commands_held(const struct pt_readers_output *q){return q?q->command_count:0;}
unsigned pt_readers_readers_held(const struct pt_readers_output *q){return q?q->reader_count:0;}
int pt_readers_close(struct pt_readers_output *q)
{struct pt_allocator a;if(!q||reentry(q)||q->command_count||q->reader_count)return 0;a=q->allocator;q->busy=1;a.release(a.context,q);return 1;}
