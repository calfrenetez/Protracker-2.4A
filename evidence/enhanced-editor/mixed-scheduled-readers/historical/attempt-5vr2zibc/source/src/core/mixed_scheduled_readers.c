#include "mixed_scheduled_readers.h"
#include "document.h"
#include <string.h>
#include <stddef.h>
struct reader_entry {
    struct pt_mixed_readers_domain domain;struct pt_mixed_readers_owner owner;
    struct pt_mixed_readers_span spans[PT_MIXED_READERS_SPANS];
    uint64_t frame,first,last,observed,issued;
    unsigned held,submitted,references,adopted,timed,closed,superseded,cancel_requested,retired,valid;
    enum pt_mixed_readers_state state;
};
struct command_entry {
    struct pt_mixed_readers_event event;struct pt_mixed_readers_control owner;
    struct pt_mixed_readers_command_receipt last;unsigned held,published,seen;
    unsigned reader[PT_MIXED_READERS_ACTIONS];
};
struct admission {
    struct pt_mixed_readers_inputs input;
    struct pt_mixed_readers_span spans[PT_MIXED_READERS_ACTIONS][PT_MIXED_READERS_SPANS];
};
struct construction {unsigned busy,failed;struct pt_mixed_readers_config config;};
struct pt_mixed_readers_output {
    struct pt_allocator allocator;struct pt_mixed_readers_grid grid;
    struct pt_mixed_readers_backend backend;struct pt_elapsed_clock clock;
    uint64_t session,tickets,serial,last_frame,last_ticks;
    unsigned commands,readers,command_count,reader_count,ordered,clock_seen,closing,failed,busy,stale;
    struct command_entry command[PT_MIXED_READERS_COMMANDS];
    struct reader_entry reader[PT_MIXED_READERS_PERSISTENT];
    struct pt_mixed_readers_span allocator_context;struct admission scratch;int *release_fault;
};
static int span(const void *p,size_t n)
{return !n||(p&&(uintptr_t)p<=UINTPTR_MAX-(n-1));}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,n)&&span(b,m)&&(!n||!m||(x<=y?n<=y-x:m<=x-y));}
static int zero_key(const struct pt_mixed_readers_key *k)
{return !k->queue&&!k->session&&!k->generation&&!k->trigger&&!k->owner&&!k->serial&&!k->action&&!k->route&&!k->slot;}
static int equal_key(const struct pt_mixed_readers_key *a,const struct pt_mixed_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->route==b->route&&a->slot==b->slot;}
static struct command_entry *command(struct pt_mixed_readers_output *q,uint64_t t)
{unsigned i;for(i=0;i<q->commands;++i)if(q->command[i].held&&q->command[i].event.ticket==t)return q->command+i;return NULL;}
static struct reader_entry *reader(struct pt_mixed_readers_output *q,uint64_t t,unsigned a)
{unsigned i;for(i=0;i<q->readers;++i)if(q->reader[i].held&&q->reader[i].domain.key.trigger==t&&q->reader[i].domain.key.action==a)return q->reader+i;return NULL;}
static struct reader_entry *key_reader(struct pt_mixed_readers_output *q,const struct pt_mixed_readers_key *k)
{struct reader_entry *r=reader(q,k->trigger,k->action);return r&&equal_key(&r->domain.key,k)?r:NULL;}
static int reentry(struct pt_mixed_readers_output *q)
{if(q->busy){q->failed=1;if(q->release_fault)*q->release_fault=1;return 1;}return 0;}
static int output_apart(const struct pt_mixed_readers_output *q,const void *p,size_t n)
{
    unsigned i,j;
    if(!n||!apart(p,n,q,sizeof(*q))||!apart(p,n,q->allocator_context.data,q->allocator_context.bytes)||!apart(p,n,q->backend.context,q->backend.context_bytes))return 0;
    for(i=0;i<q->commands;++i)if(q->command[i].held&&!apart(p,n,q->command[i].owner.context,q->command[i].owner.context_bytes))return 0;
    for(i=0;i<q->readers;++i)if(q->reader[i].held){
        const struct reader_entry *r=q->reader+i;
        if(!apart(p,n,r->owner.control.context,r->owner.control.context_bytes))return 0;
        for(j=0;j<r->owner.count;++j)if(!apart(p,n,r->spans[j].data,r->spans[j].bytes))return 0;
    }
    return 1;
}
static int control_valid(const struct pt_mixed_readers_control *o)
{return o&&span(o,sizeof(*o))&&o->context_bytes&&span(o->context,o->context_bytes)&&o->token&&o->current&&o->release;}
static int zero_card(const struct pt_mixed_readers_card *c)
{return !c->reservation&&!c->cache&&!c->version&&!c->serial&&!c->cache_slot&&!c->bits&&!c->little_endian&&!c->source_channel&&!c->address&&!c->logical_bytes&&!c->full_capacity;}
static int zero_owner(const struct pt_mixed_readers_owner *o)
{return !o->control.context&&!o->control.context_bytes&&!o->control.token&&!o->control.current&&!o->control.release&&!o->control.terminal&&!o->spans&&!o->count&&zero_card(&o->card);}
static int current(struct pt_mixed_readers_output *q,const struct pt_mixed_readers_control *o)
{int good;q->busy=1;good=o->current(o->context,o->token,q->grid.generation);q->busy=0;
 if(good!=1){q->failed=q->stale=1;}return good==1&&!q->failed;}
static int active(struct pt_mixed_readers_output *q,const struct reader_entry *r,uint64_t frame,int admission)
{
    unsigned i,j;
    if(!r||!r->held||!r->submitted||!r->adopted||!r->timed||r->retired||r->superseded||r->cancel_requested||r->state!=PT_MIXED_READER_ACTIVE||r->frame>=frame||(admission&&r->closed))return 0;
    for(i=0;i<q->commands;++i)if(q->command[i].held&&q->command[i].event.batch.frame>r->frame&&q->command[i].event.batch.frame<frame)
        for(j=0;j<q->command[i].event.batch.count;++j){
            const struct pt_mixed_readers_action *a=q->command[i].event.batch.action+j;
            if(a->route==r->domain.key.route&&a->slot==r->domain.key.slot&&(a->kind==PT_MIXED_READERS_TRIGGER||a->kind==PT_MIXED_READERS_STOP))return 0;
        }
    return 1;
}
static size_t queue_alignment(void)
{struct aligned {char byte;struct pt_mixed_readers_output value;};return offsetof(struct aligned,value);}
size_t pt_mixed_readers_workspace_size(void){return sizeof(struct construction);}
size_t pt_mixed_readers_workspace_alignment(void)
{struct aligned {char byte;struct construction value;};return offsetof(struct aligned,value);}
size_t pt_mixed_readers_control_size(void){return sizeof(struct pt_mixed_readers_output);}
static int capable(const struct pt_mixed_readers_backend *b)
{return b->version==PT_MIXED_READERS_VERSION&&b->flags==PT_MIXED_READERS_REQUIRED&&b->read_clock&&b->submit&&b->command&&b->reader;}
enum pt_mixed_readers_result pt_mixed_readers_open(const struct pt_mixed_readers_config *c,
    void *workspace,size_t capacity,struct pt_mixed_readers_output **out)
{
    struct construction *w=workspace;struct pt_mixed_readers_output *q;struct pt_elapsed_clock clock;
    struct pt_allocator allocator;int fresh;
    if(!c||!span(c,sizeof(*c))||!workspace||capacity<sizeof(*w)||
       !span(workspace,capacity)||(uintptr_t)workspace%pt_mixed_readers_workspace_alignment()||
       !out||!span(out,sizeof(*out))||!apart(workspace,capacity,c,sizeof(*c))||
       !apart(workspace,capacity,out,sizeof(*out))||!apart(out,sizeof(*out),c,sizeof(*c)))return PT_MIXED_READERS_INVALID;
    if(!c->allocator.allocate||!c->allocator.release||!c->allocator_context.bytes||
       !span(c->allocator_context.data,c->allocator_context.bytes)||!c->allocator.context||
       (uintptr_t)c->allocator.context<(uintptr_t)c->allocator_context.data||
       (uintptr_t)c->allocator.context-(uintptr_t)c->allocator_context.data>=c->allocator_context.bytes||
       !c->backend.context_bytes||!span(c->backend.context,c->backend.context_bytes)||
       !apart(workspace,capacity,c->allocator_context.data,c->allocator_context.bytes)||
       !apart(workspace,capacity,c->backend.context,c->backend.context_bytes)||
       !apart(out,sizeof(*out),c->allocator_context.data,c->allocator_context.bytes)||
       !apart(out,sizeof(*out),c->backend.context,c->backend.context_bytes)||
       !apart(c,sizeof(*c),c->allocator_context.data,c->allocator_context.bytes)||
       !apart(c,sizeof(*c),c->backend.context,c->backend.context_bytes)||
       !c->session||!c->grid.generation||c->grid.frequency<c->grid.rate||
       pt_elapsed_clock_init(&clock,c->grid.frequency,c->grid.rate,c->grid.epoch,0)!=PT_ELAPSED_OK)return PT_MIXED_READERS_INVALID;
    if(w->busy){w->failed=1;return PT_MIXED_READERS_BACKEND;}
    if(!capable(&c->backend))return PT_MIXED_READERS_UNSUPPORTED;
    if(c->control_budget<sizeof(*q))return PT_MIXED_READERS_CAPACITY;
    memset(w,0,sizeof(*w));memcpy(&w->config,c,sizeof(*c));w->busy=1;allocator=c->allocator;
    q=allocator.allocate(allocator.context,sizeof(*q));
    if(!q){int bad=w->failed||memcmp(c,&w->config,sizeof(*c));memset(w,0,sizeof(*w));return bad?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_CAPACITY;}
    fresh=apart(q,sizeof(*q),c,sizeof(*c))&&apart(q,sizeof(*q),workspace,capacity)&&
        apart(q,sizeof(*q),out,sizeof(*out))&&
        apart(q,sizeof(*q),w->config.allocator_context.data,w->config.allocator_context.bytes)&&
        apart(q,sizeof(*q),w->config.backend.context,w->config.backend.context_bytes);
    if(!fresh){memset(w,0,sizeof(*w));return PT_MIXED_READERS_INVALID;}
    if(w->failed||memcmp(c,&w->config,sizeof(*c))||(uintptr_t)q%queue_alignment()){
        allocator.release(allocator.context,q);memset(w,0,sizeof(*w));return PT_MIXED_READERS_BACKEND;}
    memset(q,0,sizeof(*q));q->allocator=allocator;q->allocator_context=c->allocator_context;
    q->grid=c->grid;q->clock=clock;q->backend=c->backend;q->session=c->session;
    q->commands=PT_MIXED_READERS_COMMANDS;q->readers=PT_MIXED_READERS_PERSISTENT;
    *out=q;memset(w,0,sizeof(*w));return PT_MIXED_READERS_OK;
}
static int card_valid(const struct pt_mixed_readers_card *c,const struct pt_amigus_voice_plan *p)
{
    uint32_t end;
    if(!c->reservation||!c->cache||!c->version||!c->serial||c->cache_slot>=32||
       (c->bits!=8&&c->bits!=16)||c->little_endian>1||c->source_channel>1||
       (c->bits==16&&(c->logical_bytes&1))||
       (c->address&3)||!c->logical_bytes||c->logical_bytes>c->full_capacity||
       (c->full_capacity&3)||c->address>=PT_AMIGUS_RAM_ADDRESS_SPACE||
       c->full_capacity>PT_AMIGUS_RAM_ADDRESS_SPACE-c->address)return 0;
    end=c->address+c->logical_bytes;
    return !(p->control&~0x800FU)&&(p->control&0x8000)&&
        !!(p->control&1)==(c->bits==16)&&!!(p->control&8)==(c->bits==16&&c->little_endian)&&
        p->rate&&p->rate<=0x40000000UL&&!((p->start|p->loop|p->end_exclusive)&1)&&
        p->start>=c->address&&p->start<p->end_exclusive&&p->end_exclusive<=end&&
        p->end_exclusive<PT_AMIGUS_RAM_ADDRESS_SPACE&&p->loop>=c->address&&
        p->loop<p->end_exclusive&&((p->control&2)||p->loop==c->address);
}
static int covered(const struct pt_mixed_readers_owner *o,const struct pt_mixed_readers_action *a)
{
    unsigned i;size_t bytes;uintptr_t x;
    if(a->route==PT_MIXED_READERS_AMIGUS)return card_valid(&o->card,&a->geometry.amigus);
    if(!zero_card(&o->card))return 0;
    bytes=(size_t)a->geometry.paula.words*2;x=(uintptr_t)a->geometry.paula.data;
    if(!a->geometry.paula.words||!a->geometry.paula.period||(x&1)||!span(a->geometry.paula.data,bytes)||!apart(a->geometry.paula.data,bytes,o->control.context,o->control.context_bytes))return 0;
    for(i=0;i<o->count;++i){uintptr_t y=(uintptr_t)o->spans[i].data;if(x>=y&&x-y<=o->spans[i].bytes&&bytes<=o->spans[i].bytes-(x-y))return 1;}
    return 0;
}
static int inputs(struct pt_mixed_readers_output *q,const struct pt_mixed_readers_inputs *input,uint64_t *out,unsigned *new_readers)
{
    unsigned i,j,k,slot,slots=0;
    const struct pt_mixed_readers_batch *b;const struct pt_mixed_readers_key *keys;
    const struct pt_mixed_readers_control *c;const struct pt_mixed_readers_owner *owners;
    if(!input||!output_apart(q,input,sizeof(*input))||!out||!output_apart(q,out,sizeof(*out))||!apart(input,sizeof(*input),out,sizeof(*out)))return 0;
    b=&input->batch;keys=input->target;c=&input->command;owners=input->reader;
    if(!b||!c||!out||!output_apart(q,b,sizeof(*b))||!output_apart(q,c,sizeof(*c))||!control_valid(c)||!output_apart(q,c->context,c->context_bytes)||!output_apart(q,out,sizeof(*out))||!apart(out,sizeof(*out),b,sizeof(*b))||!apart(out,sizeof(*out),c,sizeof(*c))||!apart(out,sizeof(*out),c->context,c->context_bytes)||!b->count||b->count>PT_MIXED_READERS_ACTIONS||b->generation!=q->grid.generation)return 0;
    if(keys&&(!output_apart(q,keys,b->count*sizeof(*keys))||!apart(out,sizeof(*out),keys,b->count*sizeof(*keys))))return 0;
    if(owners&&(!output_apart(q,owners,b->count*sizeof(*owners))||!apart(out,sizeof(*out),owners,b->count*sizeof(*owners))))return 0;
    *new_readers=0;
    for(i=0;i<b->count;++i){
        const struct pt_mixed_readers_action *a=b->action+i;
        if(a->route==PT_MIXED_READERS_PAULA){if(a->slot>=4||a->geometry.paula.volume>64)return 0;slot=a->slot;}
        else if(a->route==PT_MIXED_READERS_AMIGUS){if(a->slot>=16)return 0;slot=4+a->slot;}
        else return 0;
        if(slots&(1UL<<slot))return 0;
        slots|=1UL<<slot;
        if(a->kind==PT_MIXED_READERS_TRIGGER){
            const struct pt_mixed_readers_owner *o;if(!owners||(keys&&!zero_key(keys+i)))return 0;o=owners+i;
            if(!control_valid(&o->control)||!o->count||o->count>PT_MIXED_READERS_SPANS||!o->spans||!output_apart(q,o->control.context,o->control.context_bytes)||!output_apart(q,o->spans,o->count*sizeof(*o->spans))||!apart(out,sizeof(*out),o->control.context,o->control.context_bytes)||!apart(out,sizeof(*out),o->spans,o->count*sizeof(*o->spans))||!apart(c->context,c->context_bytes,o->control.context,o->control.context_bytes))return 0;
            for(j=0;j<o->count;++j){
                if(!span(o->spans[j].data,o->spans[j].bytes)||!apart(out,sizeof(*out),o->spans[j].data,o->spans[j].bytes)||!apart(q,sizeof(*q),o->spans[j].data,o->spans[j].bytes)||!apart(q->backend.context,q->backend.context_bytes,o->spans[j].data,o->spans[j].bytes)||!apart(q->allocator_context.data,q->allocator_context.bytes,o->spans[j].data,o->spans[j].bytes)||!apart(c->context,c->context_bytes,o->spans[j].data,o->spans[j].bytes)||!apart(o->control.context,o->control.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
                for(k=0;k<q->commands;++k)if(q->command[k].held&&!apart(q->command[k].owner.context,q->command[k].owner.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
                for(k=0;k<q->readers;++k)if(q->reader[k].held&&!apart(q->reader[k].owner.control.context,q->reader[k].owner.control.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
                for(k=0;k<i;++k)if(b->action[k].kind==PT_MIXED_READERS_TRIGGER&&!apart(owners[k].control.context,owners[k].control.context_bytes,o->spans[j].data,o->spans[j].bytes))return 0;
            }
            for(j=0;j<i;++j)if(b->action[j].kind==PT_MIXED_READERS_TRIGGER){
                if(!apart(o->control.context,o->control.context_bytes,owners[j].control.context,owners[j].control.context_bytes))return 0;
                for(k=0;k<owners[j].count;++k)if(!apart(o->control.context,o->control.context_bytes,owners[j].spans[k].data,owners[j].spans[k].bytes))return 0;
            }
            if(!covered(o,a))return 0;
            ++*new_readers;
        }else if(a->kind==PT_MIXED_READERS_CONTROL||a->kind==PT_MIXED_READERS_STOP){
            struct reader_entry *r;
            if(!zero_owner(owners+i)||keys[i].route!=a->route||keys[i].slot!=a->slot||
               !(r=key_reader(q,keys+i))||!active(q,r,b->frame,1))return 0;
            if(a->route==PT_MIXED_READERS_PAULA){
                if(a->geometry.paula.data||a->geometry.paula.words)return 0;
                if(a->kind==PT_MIXED_READERS_CONTROL){if(!a->geometry.paula.period)return 0;}
                else if(a->geometry.paula.period||a->geometry.paula.volume)return 0;
            }else{
                const struct pt_amigus_voice_plan *g=&a->geometry.amigus;
                if(g->start||g->loop||g->end_exclusive||g->control)return 0;
                if(a->kind==PT_MIXED_READERS_CONTROL){if(!g->rate||g->rate>0x40000000UL)return 0;}
                else if(g->rate||g->left||g->right)return 0;
            }
        }else return 0;
    }
    return 1;
}
enum pt_mixed_readers_result pt_mixed_readers_enqueue(struct pt_mixed_readers_output *q,
    const struct pt_mixed_readers_inputs *input,uint64_t *out)
{
    const struct pt_mixed_readers_batch *b;const struct pt_mixed_readers_key *keys;
    const struct pt_mixed_readers_control *control;const struct pt_mixed_readers_owner *owners;
    unsigned i,j,n,indices[PT_MIXED_READERS_ACTIONS],used=0;uint64_t first,last,ticket;struct command_entry *e;
    if(!q)return PT_MIXED_READERS_INVALID;
    if(!input||!output_apart(q,input,sizeof(*input))||!out||!output_apart(q,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
    if(reentry(q))return PT_MIXED_READERS_BACKEND;
    if(q->closing||q->failed||!inputs(q,input,out,&n))return PT_MIXED_READERS_INVALID;
    q->scratch.input=*input;
    for(i=0;i<input->batch.count;++i)if(input->batch.action[i].kind==PT_MIXED_READERS_TRIGGER){
        memcpy(q->scratch.spans[i],input->reader[i].spans,input->reader[i].count*sizeof(*input->reader[i].spans));
        q->scratch.input.reader[i].spans=q->scratch.spans[i];
    }
    b=&q->scratch.input.batch;keys=q->scratch.input.target;control=&q->scratch.input.command;owners=q->scratch.input.reader;
    if(q->ordered&&b->frame<=q->last_frame)return PT_MIXED_READERS_INVALID;
    if(b->frame==UINT64_MAX||pt_elapsed_clock_deadline(&q->clock,b->frame,&first)!=PT_ELAPSED_OK||pt_elapsed_clock_deadline(&q->clock,b->frame+1,&last)!=PT_ELAPSED_OK||first>=last)return PT_MIXED_READERS_CLOCK;
    if(q->command_count==q->commands||n>q->readers-q->reader_count||q->tickets==UINT64_MAX||n>UINT64_MAX-q->serial)return PT_MIXED_READERS_CAPACITY;
    for(i=0;i<b->count;++i){
        if(b->action[i].kind==PT_MIXED_READERS_TRIGGER){
            if(!current(q,&owners[i].control))return q->stale?PT_MIXED_READERS_STALE:PT_MIXED_READERS_BACKEND;
            for(j=0;j<q->readers;++j)if(!q->reader[j].held&&!(used&(1U<<j)))break;
            indices[i]=j;used|=1U<<j;
        }else{
            struct reader_entry *r=key_reader(q,keys+i);if(!current(q,&r->owner.control))return q->stale?PT_MIXED_READERS_STALE:PT_MIXED_READERS_BACKEND;
            indices[i]=(unsigned)(r-q->reader);
        }
    }
    if(!current(q,control))return q->stale?PT_MIXED_READERS_STALE:PT_MIXED_READERS_BACKEND;
    for(i=0;i<q->commands;++i)if(!q->command[i].held)break;
    e=q->command+i;memset(e,0,sizeof(*e));e->held=1;e->owner=*control;ticket=++q->tickets;
    e->event.queue=q;e->event.session=q->session;e->event.command_owner=control->token;
    e->event.binding=(struct pt_mixed_readers_binding){control->context,control->context_bytes};
    e->event.ticket=ticket;e->event.first=first;e->event.last=last;e->event.batch=*b;
    for(i=0;i<b->count;++i){
        struct reader_entry *r=q->reader+indices[i];e->reader[i]=indices[i];
        if(b->action[i].kind==PT_MIXED_READERS_TRIGGER){
            memset(r,0,sizeof(*r));r->held=1;r->owner=owners[i];memcpy(r->spans,owners[i].spans,owners[i].count*sizeof(*r->spans));r->owner.spans=r->spans;
            r->domain.key=(struct pt_mixed_readers_key){q,q->session,q->grid.generation,ticket,owners[i].control.token,++q->serial,i,b->action[i].route,b->action[i].slot};
            r->domain.spans=r->spans;r->domain.count=owners[i].count;r->domain.card=owners[i].card;
            r->domain.binding=(struct pt_mixed_readers_binding){owners[i].control.context,owners[i].control.context_bytes};
            r->frame=b->frame;r->first=first;r->last=last;r->state=PT_MIXED_READER_RESERVED;r->valid=1;++q->reader_count;
        }else if(b->action[i].kind==PT_MIXED_READERS_STOP)r->closed=1;
        ++r->references;e->event.reader[i]=&r->domain;
    }
    ++q->command_count;q->ordered=1;q->last_frame=b->frame;*out=ticket;return PT_MIXED_READERS_OK;
}
static int targets_current(struct pt_mixed_readers_output *q,const struct command_entry *e)
{
    unsigned i;
    for(i=0;i<e->event.batch.count;++i){struct reader_entry *r=q->reader+e->reader[i];
        if(!r->held||r->retired||!current(q,&r->owner.control))return 0;
        if(e->event.batch.action[i].kind!=PT_MIXED_READERS_TRIGGER&&!active(q,r,e->event.batch.frame,0))return 0;
    }
    return current(q,&e->owner);
}
enum pt_mixed_readers_result pt_mixed_readers_publish(struct pt_mixed_readers_output *q,uint64_t t)
{
    struct command_entry *e;uint64_t now;uint32_t frequency;int result;unsigned i;
    if(!q)return PT_MIXED_READERS_INVALID;
    if(reentry(q))return PT_MIXED_READERS_BACKEND;
    if(q->closing||q->failed||!(e=command(q,t))||e->published)return PT_MIXED_READERS_INVALID;
    for(i=0;i<q->commands;++i)if(q->command[i].held&&!q->command[i].published&&q->command[i].event.batch.frame<e->event.batch.frame)return PT_MIXED_READERS_INVALID;
    if(!targets_current(q,e))return q->stale?PT_MIXED_READERS_STALE:PT_MIXED_READERS_BACKEND;
    q->busy=1;result=q->backend.read_clock(q->backend.context,&now,&frequency);q->busy=0;
    if(q->failed)return PT_MIXED_READERS_BACKEND;
    if(result!=1||frequency!=q->grid.frequency||now<q->grid.epoch||(q->clock_seen&&now<q->last_ticks)){q->failed=1;return PT_MIXED_READERS_CLOCK;}
    q->last_ticks=now;q->clock_seen=1;
    if(now>=e->event.first){q->failed=1;return PT_MIXED_READERS_LATE;}
    if(!targets_current(q,e))return q->stale?PT_MIXED_READERS_STALE:PT_MIXED_READERS_BACKEND;
    q->busy=1;result=q->backend.submit(q->backend.context,&e->event);q->busy=0;
    if(result==0)return q->failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_PENDING;
    e->published=1;for(i=0;i<e->event.batch.count;++i)if(e->event.batch.action[i].kind==PT_MIXED_READERS_TRIGGER)q->reader[e->reader[i]].submitted=1;
    if(result==1&&!q->failed)return PT_MIXED_READERS_OK;
    q->failed=1;return PT_MIXED_READERS_BACKEND;
}
static void reader_release(struct pt_mixed_readers_output *q,struct reader_entry *r)
{
    struct pt_mixed_readers_control owner=r->owner.control;int valid=r->valid;
    if(!r->held||r->references||!r->retired)return;
    memset(r,0,sizeof(*r));--q->reader_count;q->busy=1;if(owner.terminal)owner.terminal(owner.context,owner.token,valid);owner.release(owner.context,owner.token);q->busy=0;
}
static void command_release(struct pt_mixed_readers_output *q,struct command_entry *e,int valid,int local)
{
    struct pt_mixed_readers_control owner=e->owner;unsigned indices[PT_MIXED_READERS_ACTIONS],i,n=e->event.batch.count;
    for(i=0;i<n;++i){struct reader_entry *r=q->reader+e->reader[i];indices[i]=e->reader[i];--r->references;
        if(local&&e->event.batch.action[i].kind==PT_MIXED_READERS_TRIGGER){r->retired=1;r->closed=1;r->state=PT_MIXED_READER_RETIRED;}}
    memset(e,0,sizeof(*e));--q->command_count;q->busy=1;if(owner.terminal)owner.terminal(owner.context,owner.token,valid);owner.release(owner.context,owner.token);q->busy=0;
    for(i=0;i<n;++i)reader_release(q,q->reader+indices[i]);
}
static int issued(enum pt_mixed_readers_command c)
{return c==PT_MIXED_COMMAND_ISSUED||c==PT_MIXED_COMMAND_CANCELLED_AFTER;}
static int command_envelope(const struct pt_mixed_readers_output *q,const struct command_entry *e,const struct pt_mixed_readers_command_receipt *r)
{return r->domain==PT_MIXED_COMMAND_DOMAIN&&r->queue==q&&r->session==q->session&&r->generation==q->grid.generation&&r->ticket==e->event.ticket&&r->owner==e->owner.token&&r->count==e->event.batch.count&&r->event==&e->event&&r->binding.context==e->owner.context&&r->binding.context_bytes==e->owner.context_bytes;}
static int reader_state_valid(const struct reader_entry *r,enum pt_mixed_readers_state state,enum pt_mixed_readers_adoption adoption)
{
    if((unsigned)state>PT_MIXED_READER_RETIRED||(unsigned)adoption>PT_MIXED_ADOPTED)return 0;
    if(r->retired&&state!=PT_MIXED_READER_RETIRED)return 0;
    if(r->adopted&&adoption!=PT_MIXED_ADOPTED)return 0;
    if(r->adopted&&state<r->state)return 0;
    if(adoption==PT_MIXED_ADOPTED&&state<PT_MIXED_READER_ACTIVE)return 0;
    if(adoption==PT_MIXED_UNADOPTED&&state!=PT_MIXED_READER_NONE&&state!=PT_MIXED_READER_RESERVED&&state!=PT_MIXED_READER_RETIRED)return 0;
    return 1;
}
static int command_reader_state_valid(const struct reader_entry *r,
    const struct pt_mixed_readers_action_receipt *a,enum pt_mixed_readers_kind kind)
{
    /* Independent positive reader retirement does not rewrite a cancelled,
     * never-issued TRIGGER command's NONE+UNADOPTED snapshot. It also cannot
     * detach that command, revive the reader, or create adoption/timing. */
    if(kind==PT_MIXED_READERS_TRIGGER&&a->command==PT_MIXED_COMMAND_CANCELLED_BEFORE&&
       a->reader==PT_MIXED_READER_NONE&&a->adoption==PT_MIXED_UNADOPTED&&
       !a->observed&&!a->issued&&r->retired&&r->state==PT_MIXED_READER_RETIRED&&
       r->valid&&!r->adopted&&!r->timed)return 1;
    return reader_state_valid(r,a->reader,a->adoption);
}
static int command_valid(const struct pt_mixed_readers_output *q,const struct command_entry *e,const struct pt_mixed_readers_command_receipt *receipt,int detached)
{
    unsigned i;
    for(i=0;i<receipt->count;++i){
        const struct pt_mixed_readers_action_receipt *a=receipt->action+i,*old=e->last.action+i;const struct reader_entry *r=q->reader+e->reader[i];enum pt_mixed_readers_kind kind=e->event.batch.action[i].kind;
        if((unsigned)a->command>PT_MIXED_COMMAND_CANCELLED_AFTER||a->command!=receipt->action[0].command||!equal_key(&a->key,&r->domain.key)||!command_reader_state_valid(r,a,kind))return 0;
        if(issued(a->command)){
            if(a->observed<e->event.first||a->observed>a->issued||a->issued>=e->event.last)return 0;
            if(kind==PT_MIXED_READERS_STOP&&(a->reader!=PT_MIXED_READER_DRAINING&&a->reader!=PT_MIXED_READER_RETIRED))return 0;
            if(kind!=PT_MIXED_READERS_TRIGGER&&a->adoption!=PT_MIXED_ADOPTED)return 0;
            if(kind==PT_MIXED_READERS_TRIGGER&&r->timed&&(r->observed!=a->observed||r->issued!=a->issued))return 0;
        }else{
            if(a->observed||a->issued)return 0;
            if(kind==PT_MIXED_READERS_TRIGGER&&(a->reader!=PT_MIXED_READER_NONE||a->adoption!=PT_MIXED_UNADOPTED))return 0;
            if(kind!=PT_MIXED_READERS_TRIGGER&&a->adoption!=PT_MIXED_ADOPTED)return 0;
        }
        if(detached&&a->command==PT_MIXED_COMMAND_WAITING)return 0;
        if(e->seen){
            if(old->command==PT_MIXED_COMMAND_CANCELLED_BEFORE&&a->command!=old->command)return 0;
            if(issued(old->command)&&(!issued(a->command)||old->observed!=a->observed||old->issued!=a->issued))return 0;
            if(old->command==PT_MIXED_COMMAND_CANCELLED_AFTER&&a->command!=old->command)return 0;
        }
    }
    return 1;
}
static void advance(struct pt_mixed_readers_output *q,const struct command_entry *e,const struct pt_mixed_readers_command_receipt *receipt)
{
    unsigned i,j;
    for(i=0;i<receipt->count;++i){struct reader_entry *r=q->reader+e->reader[i];const struct pt_mixed_readers_action_receipt *a=receipt->action+i;
        if(e->event.batch.action[i].kind==PT_MIXED_READERS_TRIGGER&&issued(a->command)){
            r->timed=1;r->observed=a->observed;r->issued=a->issued;
            for(j=0;j<q->readers;++j)if(q->reader[j].held&&q->reader[j].frame<r->frame&&q->reader[j].domain.key.route==r->domain.key.route&&q->reader[j].domain.key.slot==r->domain.key.slot){q->reader[j].closed=1;q->reader[j].superseded=1;}
        }
        if(a->adoption==PT_MIXED_ADOPTED)r->adopted=1;
        if(a->reader>r->state)r->state=a->reader;
        if(a->reader>=PT_MIXED_READER_STOP_PENDING)r->closed=1;
    }
}
static enum pt_mixed_readers_result command_check(struct pt_mixed_readers_output *q,uint64_t t,struct pt_mixed_readers_command_receipt *out,int cancel)
{
    struct command_entry *e;struct pt_mixed_readers_command_receipt receipt;enum pt_mixed_readers_reply reply;int good;
    if(!q)return PT_MIXED_READERS_INVALID;
    if(reentry(q))return PT_MIXED_READERS_BACKEND;
    if((out&&!output_apart(q,out,sizeof(*out)))||!(e=command(q,t)))return PT_MIXED_READERS_INVALID;
    if(!e->published)return PT_MIXED_READERS_INVALID;
    memset(&receipt,0,sizeof(receipt));q->busy=1;reply=q->backend.command(q->backend.context,t,(unsigned)cancel,&receipt);q->busy=0;
    if(reply==PT_MIXED_PENDING&&!q->failed)return PT_MIXED_READERS_PENDING;
    if((reply!=PT_MIXED_OBSERVATION&&reply!=PT_MIXED_COMMAND_DETACHED)||!command_envelope(q,e,&receipt)){q->failed=1;return PT_MIXED_READERS_BACKEND;}
    good=command_valid(q,e,&receipt,reply==PT_MIXED_COMMAND_DETACHED);if(!good)q->failed=1;
    if(good)advance(q,e,&receipt);
    if(reply==PT_MIXED_COMMAND_DETACHED)command_release(q,e,good,0);
    else if(good){e->last=receipt;e->seen=1;}
    if(!good||q->failed)return PT_MIXED_READERS_BACKEND;
    if(out)*out=receipt;
    return reply==PT_MIXED_COMMAND_DETACHED?PT_MIXED_READERS_OK:PT_MIXED_READERS_PENDING;
}
enum pt_mixed_readers_result pt_mixed_readers_service_command(struct pt_mixed_readers_output *q,uint64_t t,unsigned cancel,struct pt_mixed_readers_command_receipt *out)
{if(cancel>1)return PT_MIXED_READERS_INVALID;return command_check(q,t,out,(int)cancel);}
static int reader_envelope(const struct reader_entry *r,const struct pt_mixed_readers_reader_receipt *a)
{return a->domain==PT_MIXED_READER_DOMAIN&&equal_key(&a->key,&r->domain.key)&&a->reference==&r->domain&&a->binding.context==r->owner.control.context&&a->binding.context_bytes==r->owner.control.context_bytes;}
static int reader_valid(const struct reader_entry *r,const struct pt_mixed_readers_reader_receipt *a,int retired)
{
    if(!reader_state_valid(r,a->state,a->adoption)||(retired&&a->state!=PT_MIXED_READER_RETIRED))return 0;
    if(a->adoption==PT_MIXED_ADOPTED){
        if(a->observed<r->first||a->observed>a->issued||a->issued>=r->last||(r->timed&&(a->observed!=r->observed||a->issued!=r->issued)))return 0;
    }else if(a->observed||a->issued)return 0;
    return 1;
}
static enum pt_mixed_readers_result reader_check(struct pt_mixed_readers_output *q,uint64_t t,unsigned i,struct pt_mixed_readers_reader_receipt *out,int cancel)
{
    struct reader_entry *r;struct pt_mixed_readers_reader_receipt receipt;enum pt_mixed_readers_reply reply;int good;unsigned j;
    if(!q)return PT_MIXED_READERS_INVALID;
    if(reentry(q))return PT_MIXED_READERS_BACKEND;
    if((out&&!output_apart(q,out,sizeof(*out)))||!(r=reader(q,t,i))||!r->submitted)return PT_MIXED_READERS_INVALID;
    if(cancel){r->closed=1;r->cancel_requested=1;}
    memset(&receipt,0,sizeof(receipt));q->busy=1;reply=q->backend.reader(q->backend.context,&r->domain,(unsigned)cancel,&receipt);q->busy=0;
    if(reply==PT_MIXED_PENDING&&!q->failed)return PT_MIXED_READERS_PENDING;
    if((reply!=PT_MIXED_OBSERVATION&&reply!=PT_MIXED_READER_RETIRE_PROOF)||!reader_envelope(r,&receipt)){q->failed=1;return PT_MIXED_READERS_BACKEND;}
    good=reader_valid(r,&receipt,reply==PT_MIXED_READER_RETIRE_PROOF);if(!good)q->failed=1;
    if(good){r->state=receipt.state;if(receipt.adoption==PT_MIXED_ADOPTED){r->adopted=1;r->timed=1;r->observed=receipt.observed;r->issued=receipt.issued;for(j=0;j<q->readers;++j)if(q->reader[j].held&&q->reader[j].frame<r->frame&&q->reader[j].domain.key.route==r->domain.key.route&&q->reader[j].domain.key.slot==r->domain.key.slot){q->reader[j].closed=1;q->reader[j].superseded=1;}}if(receipt.state>=PT_MIXED_READER_STOP_PENDING)r->closed=1;}
    if(reply==PT_MIXED_READER_RETIRE_PROOF){r->retired=1;r->closed=1;r->valid=r->valid&&(unsigned)good;reader_release(q,r);}
    if(!good||q->failed)return PT_MIXED_READERS_BACKEND;
    if(out)*out=receipt;
    return reply==PT_MIXED_READER_RETIRE_PROOF?PT_MIXED_READERS_OK:PT_MIXED_READERS_PENDING;
}
enum pt_mixed_readers_result pt_mixed_readers_service_reader(struct pt_mixed_readers_output *q,uint64_t t,unsigned action,unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{if(cancel>1)return PT_MIXED_READERS_INVALID;return reader_check(q,t,action,out,(int)cancel);}
enum pt_mixed_readers_result pt_mixed_readers_reader_key(struct pt_mixed_readers_output *q,uint64_t t,unsigned i,struct pt_mixed_readers_key *out)
{
    struct reader_entry *r;
    if(!q)return PT_MIXED_READERS_INVALID;
    if(reentry(q))return PT_MIXED_READERS_BACKEND;
    if(!out||!output_apart(q,out,sizeof(*out))||q->closing||q->failed||!(r=reader(q,t,i)))return PT_MIXED_READERS_INVALID;
    if(!active(q,r,UINT64_MAX,1))return PT_MIXED_READERS_STALE;
    if(!current(q,&r->owner.control))return q->stale?PT_MIXED_READERS_STALE:PT_MIXED_READERS_BACKEND;
    *out=r->domain.key;return PT_MIXED_READERS_OK;
}
enum pt_mixed_readers_result pt_mixed_readers_stop(struct pt_mixed_readers_output *q)
{
    unsigned i;if(!q)return PT_MIXED_READERS_INVALID;
    if(reentry(q))return PT_MIXED_READERS_BACKEND;
    q->closing=1;for(i=0;i<q->commands;++i)if(q->command[i].held&&!q->command[i].published)command_release(q,q->command+i,1,1);
    return q->failed?PT_MIXED_READERS_BACKEND:q->command_count||q->reader_count?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_OK;
}
unsigned pt_mixed_readers_commands_held(const struct pt_mixed_readers_output *q){return q?q->command_count:0;}
unsigned pt_mixed_readers_readers_held(const struct pt_mixed_readers_output *q){return q?q->reader_count:0;}
int pt_mixed_readers_close(struct pt_mixed_readers_output **slot)
{
    struct pt_mixed_readers_output *q;struct pt_allocator a;int fault=0;
    if(!slot||!span(slot,sizeof(*slot)))return 0;
    q=*slot;if(!q)return 1;
    if(!output_apart(q,slot,sizeof(*slot)))return 0;
    if(reentry(q)||q->command_count||q->reader_count)return 0;
    a=q->allocator;q->busy=1;q->release_fault=&fault;*slot=NULL;
    a.release(a.context,q);return !fault&&!*slot;
}
