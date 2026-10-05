#include "readers_activation.h"
#include "document.h"
#include <string.h>
struct activation_reader {
    const struct pt_readers_domain *reference;
    struct pt_readers_binding binding;
    struct pt_readers_key key;
    const struct pt_scheduled_span *span_vector;size_t span_vector_bytes;
    struct pt_scheduled_span spans[PT_SCHEDULED_SPANS];unsigned count;
    uint64_t frame,first,last,observed,issued;
    unsigned held,adopted,closed,cancelled,port_refs;
    enum pt_readers_state state;
};
struct activation_command {
    const struct pt_readers_event *event;
    struct pt_readers_binding binding;uint64_t owner;
    struct pt_readers_activation_packet packet;
    uint64_t observed,issued;
    unsigned held,unknown,port_refs;
    enum pt_readers_command command;
    enum pt_readers_state state[PT_READERS_ACTIONS];
    unsigned adopted[PT_READERS_ACTIONS];
};
struct pt_readers_activation {
    struct pt_allocator allocator;struct pt_scheduled_grid grid;
    struct pt_readers_activation_port port;const struct pt_readers_output *queue;
    uint64_t session,last_frame,last_ticks;
    unsigned ordered,clock_seen,busy,failed;unsigned *release_latch;
    struct pt_readers_key slot[PT_READERS_ACTIONS];unsigned active_mask;
    struct activation_command command[PT_READERS_ACTIVATION_COMMANDS],staged;
    struct activation_reader reader[PT_READERS_ACTIVATION_READERS];
    struct activation_reader staged_reader[PT_READERS_ACTIONS];
};
static int extent(const void *p,size_t n)
{return !n||(p&&(uintptr_t)p<=UINTPTR_MAX-(n-1));}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return extent(p,n)&&extent(q,m)&&(!n||!m||(a<=b?n<=b-a:m<=a-b));}
static int key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static int key_zero(const struct pt_readers_key *a)
{return !a->queue&&!a->session&&!a->generation&&!a->trigger&&!a->owner&&!a->serial&&!a->action&&!a->slot;}
static int enter(struct pt_readers_activation *b)
{if(b->busy){b->failed=1;if(b->release_latch)*b->release_latch=1;return 0;}b->busy=1;return 1;}
/* Task-side full-span guards only. Never called by fire. */
static int output_apart(const struct pt_readers_activation *b,const void *p,size_t n)
{
    unsigned i,j;
    if(!n||!apart(p,n,b,sizeof(*b))||!apart(p,n,b->port.context,b->port.context_bytes))return 0;
    for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(b->command[i].held){
        const struct activation_command *c=b->command+i;
        if(!apart(p,n,c->event,sizeof(*c->event))||!apart(p,n,c->binding.context,c->binding.context_bytes))return 0;
    }
    for(i=0;i<PT_READERS_ACTIVATION_READERS;++i)if(b->reader[i].held){
        const struct activation_reader *r=b->reader+i;
        if(!apart(p,n,r->reference,sizeof(*r->reference))||!apart(p,n,r->binding.context,r->binding.context_bytes)||
           !apart(p,n,r->span_vector,r->span_vector_bytes))return 0;
        for(j=0;j<r->count;++j)if(!apart(p,n,r->spans[j].data,r->spans[j].bytes))return 0;
    }
    return 1;
}
static struct activation_reader *find_reader(struct pt_readers_activation *b,const struct pt_readers_key *key)
{unsigned i;for(i=0;i<PT_READERS_ACTIVATION_READERS;++i)if(b->reader[i].held&&key_equal(&b->reader[i].key,key))return b->reader+i;return NULL;}
static struct activation_command *find_command(struct pt_readers_activation *b,uint64_t ticket)
{unsigned i;for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(b->command[i].held&&b->command[i].packet.ticket==ticket)return b->command+i;return NULL;}
static int clock_read(struct pt_readers_activation *b,uint64_t *ticks)
{
    uint64_t now=0;uint32_t frequency=0;
    int result=b->port.read_clock(b->port.context,&now,&frequency);
    if(result!=1||b->failed||frequency!=b->grid.frequency||now<b->grid.epoch||(b->clock_seen&&now<b->last_ticks))return 0;
    b->last_ticks=now;b->clock_seen=1;*ticks=now;return 1;
}
static int backend_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_readers_activation *b=context;uint64_t now;
    if(!b||!ticks||!frequency||!output_apart(b,ticks,sizeof(*ticks))||!output_apart(b,frequency,sizeof(*frequency))||!apart(ticks,sizeof(*ticks),frequency,sizeof(*frequency))||!enter(b))return 0;
    if(!clock_read(b,&now)){b->failed=1;b->busy=0;return 0;}
    *ticks=now;*frequency=b->grid.frequency;b->busy=0;return 1;
}
static int current_packet(const struct pt_readers_activation *b,const struct pt_readers_activation_packet *p)
{
    unsigned i;
    if(p->session!=b->session||p->generation!=b->grid.generation||p->expected_mask!=b->active_mask)return 0;
    for(i=0;i<PT_READERS_ACTIONS;++i)if(!key_equal(p->expected+i,b->slot+i))return 0;
    for(i=0;i<p->count;++i){
        unsigned slot=p->action[i].slot;
        if(p->action[i].kind!=PT_SCHEDULED_TRIGGER&&
           (!(b->active_mask&(1U<<slot))||!key_equal(p->key+i,b->slot+slot)))return 0;
    }
    return 1;
}
static int original_window(const struct pt_readers_activation *b,const struct pt_readers_event *e)
{
    struct pt_elapsed_clock c;uint64_t first,last;
    return pt_elapsed_clock_init(&c,b->grid.frequency,b->grid.rate,b->grid.epoch,0)==PT_ELAPSED_OK&&
        pt_elapsed_clock_deadline(&c,e->scheduled.batch.frame,&first)==PT_ELAPSED_OK&&
        e->scheduled.batch.frame!=UINT64_MAX&&
        pt_elapsed_clock_deadline(&c,e->scheduled.batch.frame+1,&last)==PT_ELAPSED_OK&&
        first==e->scheduled.first&&last==e->scheduled.last&&first<last;
}
static int prepare(struct pt_readers_activation *b,const struct pt_readers_event *e,unsigned *new_count)
{
    struct activation_command *c=&b->staged;unsigned i,j,mask=0;
    if(!e||!apart(e,sizeof(*e),b,sizeof(*b))||!apart(e,sizeof(*e),b->port.context,b->port.context_bytes)||
       !e->queue||(b->queue&&e->queue!=b->queue)||e->session!=b->session||!e->command_owner||!e->binding.context_bytes||
       !extent(e->binding.context,e->binding.context_bytes)||
       !apart(e->binding.context,e->binding.context_bytes,b,sizeof(*b))||
       !apart(e->binding.context,e->binding.context_bytes,b->port.context,b->port.context_bytes)||
       e->scheduled.batch.generation!=b->grid.generation||!e->scheduled.ticket||
       !e->scheduled.batch.count||e->scheduled.batch.count>PT_READERS_ACTIONS||
       (b->ordered&&e->scheduled.batch.frame<=b->last_frame)||!original_window(b,e))return 0;
    memset(c,0,sizeof(*c));memset(b->staged_reader,0,sizeof(b->staged_reader));*new_count=0;
    c->event=e;c->binding=e->binding;c->owner=e->command_owner;
    c->packet.session=e->session;c->packet.generation=e->scheduled.batch.generation;
    c->packet.ticket=e->scheduled.ticket;c->packet.frame=e->scheduled.batch.frame;
    c->packet.first=e->scheduled.first;c->packet.last=e->scheduled.last;c->packet.count=e->scheduled.batch.count;
    c->packet.expected_mask=b->active_mask;memcpy(c->packet.expected,b->slot,sizeof(b->slot));
    for(i=0;i<c->packet.count;++i){
        const struct pt_scheduled_action *a=e->scheduled.batch.action+i;
        const struct pt_readers_domain *d=e->reader[i];struct activation_reader *r;
        if(a->slot>=PT_READERS_ACTIONS||(mask&(1U<<a->slot))||a->volume>64||
           !d||!apart(d,sizeof(*d),b,sizeof(*b))||!apart(d,sizeof(*d),b->port.context,b->port.context_bytes)||
           !d->binding.context_bytes||!extent(d->binding.context,d->binding.context_bytes)||
           !apart(d->binding.context,d->binding.context_bytes,b,sizeof(*b))||
           !apart(d->binding.context,d->binding.context_bytes,b->port.context,b->port.context_bytes)||
           d->key.queue!=e->queue||d->key.session!=b->session||d->key.generation!=b->grid.generation||
           d->key.slot!=a->slot||!d->key.owner||!d->key.serial||d->key.action>=PT_READERS_ACTIONS)return 0;
        mask|=1U<<a->slot;c->packet.action[i]=*a;c->packet.key[i]=d->key;
        if(a->kind==PT_SCHEDULED_TRIGGER){
            uintptr_t p=(uintptr_t)a->data;size_t bytes=(size_t)a->words*2;unsigned covered=0;
            if(!a->words||!a->period||(p&1)||!extent(a->data,bytes)||d->key.trigger!=e->scheduled.ticket||d->key.action!=i||
               find_reader(b,&d->key)||!d->count||d->count>PT_SCHEDULED_SPANS||!d->spans||
               !apart(d->spans,d->count*sizeof(*d->spans),b,sizeof(*b))||
               !apart(d->spans,d->count*sizeof(*d->spans),b->port.context,b->port.context_bytes))return 0;
            r=b->staged_reader+(*new_count)++;r->reference=d;r->binding=d->binding;r->key=d->key;
            r->span_vector=d->spans;r->span_vector_bytes=d->count*sizeof(*d->spans);
            r->count=d->count;r->frame=c->packet.frame;r->first=c->packet.first;r->last=c->packet.last;
            for(j=0;j<r->count;++j){
                struct pt_scheduled_span s=d->spans[j];uintptr_t q=(uintptr_t)s.data;
                if(!extent(s.data,s.bytes)||!apart(s.data,s.bytes,b,sizeof(*b))||
                   !apart(s.data,s.bytes,b->port.context,b->port.context_bytes)||
                   !apart(s.data,s.bytes,d->binding.context,d->binding.context_bytes))return 0;
                r->spans[j]=s;
                if(p>=q&&p-q<=s.bytes&&bytes<=s.bytes-(p-q))covered=1;
            }
            if(!covered)return 0;
            r->held=1;r->state=PT_READERS_RESERVED;c->state[i]=PT_READERS_NONE;
        }else if(a->kind==PT_SCHEDULED_CONTROL||a->kind==PT_SCHEDULED_STOP){
            r=find_reader(b,&d->key);
            if(a->data||a->words||!r||r->reference!=d||r->binding.context!=d->binding.context||
               r->binding.context_bytes!=d->binding.context_bytes||!r->adopted||r->closed||r->state!=PT_READERS_ACTIVE||
               (a->kind==PT_SCHEDULED_CONTROL?!a->period:a->period||a->volume)||
               !(b->active_mask&(1U<<a->slot))||!key_equal(b->slot+a->slot,&d->key))return 0;
            c->state[i]=r->state;c->adopted[i]=1;
        }else return 0;
    }
    c->held=1;c->command=PT_READERS_WAITING;return 1;
}
static int backend_submit(void *context,const struct pt_readers_event *e)
{
    struct pt_readers_activation *b=context;unsigned i,j,index=PT_READERS_ACTIVATION_COMMANDS,n=0,free_count=0;
    unsigned assigned[PT_READERS_ACTIONS];struct activation_command *c;uint64_t now;int result;
    if(!b||!enter(b))return -1;
    if(b->failed){b->busy=0;return -1;}
    for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(!b->command[i].held){index=i;break;}
    for(i=0;i<PT_READERS_ACTIVATION_READERS;++i)free_count+=!b->reader[i].held;
    if(index==PT_READERS_ACTIVATION_COMMANDS||!prepare(b,e,&n)||n>free_count){memset(&b->staged,0,sizeof(b->staged));memset(b->staged_reader,0,sizeof(b->staged_reader));b->busy=0;return 0;}
    /* Full actual registry snapshots cannot predict even a disjoint pending
     * trigger/STOP. Explicit retry after actual commit keeps the same frame.
     * Control-only batches may coexist because they preserve all current keys. */
    for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(b->command[i].held&&b->command[i].command==PT_READERS_WAITING)
        for(j=0;j<b->command[i].packet.count;++j)if(b->command[i].packet.action[j].kind!=PT_SCHEDULED_CONTROL){
            memset(&b->staged,0,sizeof(b->staged));memset(b->staged_reader,0,sizeof(b->staged_reader));b->busy=0;return 0;
        }
    c=b->command+index;*c=b->staged;
    for(i=0,j=0;i<PT_READERS_ACTIVATION_READERS&&j<n;++i)if(!b->reader[i].held){assigned[j]=i;b->reader[i]=b->staged_reader[j++];}
    memset(&b->staged,0,sizeof(b->staged));memset(b->staged_reader,0,sizeof(b->staged_reader));
    if(!clock_read(b,&now)||now>=c->packet.first){b->queue=e->queue;c->unknown=1;c->command=PT_READERS_UNKNOWN;b->failed=1;b->busy=0;return -1;}
    c->port_refs=1;for(i=0;i<n;++i)b->reader[assigned[i]].port_refs=1;
    result=b->port.publish(b->port.context,b,&c->packet);
    if(result==0&&!b->failed){for(i=0;i<n;++i)memset(b->reader+assigned[i],0,sizeof(b->reader[i]));memset(c,0,sizeof(*c));b->busy=0;return 0;}
    b->queue=e->queue;b->ordered=1;b->last_frame=c->packet.frame;
    if(result!=1||b->failed){c->unknown=1;b->failed=1;result=-1;}
    for(i=0;i<c->packet.count;++i)if(c->packet.action[i].kind==PT_SCHEDULED_STOP){struct activation_reader *r=find_reader(b,c->packet.key+i);if(r)r->closed=1;}
    memset(&b->staged,0,sizeof(b->staged));memset(b->staged_reader,0,sizeof(b->staged_reader));b->busy=0;return result;
}
static void reader_closed(struct pt_readers_activation *b,struct activation_reader *r)
{
    unsigned i,j;r->closed=1;r->state=PT_READERS_DRAINING;
    for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(b->command[i].held)
        for(j=0;j<b->command[i].packet.count;++j)if(key_equal(b->command[i].packet.key+j,&r->key)&&b->command[i].command!=PT_READERS_WAITING)b->command[i].state[j]=PT_READERS_DRAINING;
}
enum pt_readers_activation_result pt_readers_activation_fire(struct pt_readers_activation *b,uint64_t ticket)
{
    struct activation_command *c;struct pt_readers_activation_actual actual;uint64_t observed,issued;
    unsigned i,j;int result;
    if(!b||!enter(b))return PT_READERS_ACTIVATION_INVALID;
    c=find_command(b,ticket);
    if(!c||c->command!=PT_READERS_WAITING){b->busy=0;return PT_READERS_ACTIVATION_INVALID;}
    if(b->failed||!clock_read(b,&observed)||!current_packet(b,&c->packet)){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
    if(observed<c->packet.first){b->busy=0;return PT_READERS_ACTIVATION_EARLY;}
    if(observed>=c->packet.last){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
    for(i=0;i<c->packet.count;++i){struct activation_reader *r=find_reader(b,c->packet.key+i);if(!r||r->cancelled){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}}
    memset(&actual,0,sizeof(actual));result=b->port.commit(b->port.context,&c->packet,&actual);
    if(result!=1||!clock_read(b,&issued)||issued>=c->packet.last){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
    if(actual.active_mask&~15U||actual.adopted_mask&~15U){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
    for(i=0;i<c->packet.count;++i){
        unsigned slot=c->packet.action[i].slot,bit=1U<<slot;
        if(c->packet.action[i].kind==PT_SCHEDULED_STOP){if((actual.active_mask&bit)||!key_zero(actual.slot+slot))break;}
        else if(!(actual.active_mask&bit)||!(actual.adopted_mask&bit)||!key_equal(actual.slot+slot,c->packet.key+i))break;
    }
    if(i!=c->packet.count){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
    for(i=0;i<PT_READERS_ACTIONS;++i){
        unsigned touched=0;for(j=0;j<c->packet.count;++j)touched|=c->packet.action[j].slot==i;
        if(!touched&&(((actual.active_mask^c->packet.expected_mask)&(1U<<i))||!key_equal(actual.slot+i,c->packet.expected+i)))break;
    }
    if(i!=PT_READERS_ACTIONS||actual.adopted_mask!=actual.active_mask){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
    for(i=0;i<c->packet.count;++i){
        struct activation_reader *r=find_reader(b,c->packet.key+i);unsigned slot=c->packet.action[i].slot;
        if(!r){c->unknown=1;b->failed=1;b->busy=0;return PT_READERS_ACTIVATION_FAILED;}
        if(c->packet.action[i].kind==PT_SCHEDULED_TRIGGER){
            for(j=0;j<PT_READERS_ACTIVATION_READERS;++j)if(b->reader[j].held&&b->reader[j].key.slot==slot&&!key_equal(&b->reader[j].key,&r->key))reader_closed(b,b->reader+j);
            r->adopted=1;r->state=PT_READERS_ACTIVE;r->observed=observed;r->issued=issued;
        }else if(c->packet.action[i].kind==PT_SCHEDULED_STOP)reader_closed(b,r);
        c->state[i]=r->state;c->adopted[i]=r->adopted;
        /* Retain only reader-owned geometry; completed command snapshots must
         * not keep a sample address capable of reviving a retired reader. */
        c->packet.action[i].data=NULL;c->packet.action[i].words=0;
    }
    b->active_mask=actual.active_mask;memcpy(b->slot,actual.slot,sizeof(b->slot));
    c->observed=observed;c->issued=issued;c->command=PT_READERS_ISSUED;b->busy=0;return PT_READERS_ACTIVATION_COMMITTED;
}
static enum pt_readers_reply command_probe(void *context,uint64_t ticket,struct pt_readers_command_receipt *out,unsigned cancel)
{
    struct pt_readers_activation *b=context;struct activation_command *c;struct pt_readers_command_receipt receipt;unsigned i;int result;
    if(!b||!out||!output_apart(b,out,sizeof(*out))||!enter(b))return PT_READERS_UNCERTAIN;
    c=find_command(b,ticket);if(!c){b->busy=0;return PT_READERS_UNCERTAIN;}
    result=c->port_refs?b->port.command_quiet(b->port.context,ticket,cancel):1;
    if(result!=0&&result!=1){b->failed=1;b->busy=0;return PT_READERS_UNCERTAIN;}
    if(b->failed&&result==0){b->busy=0;return PT_READERS_UNCERTAIN;}
    if(b->failed)c->unknown=1;
    if(result==1&&c->command==PT_READERS_WAITING)c->command=PT_READERS_CANCELLED_BEFORE;
    else if(result==1&&cancel&&c->command==PT_READERS_ISSUED)c->command=PT_READERS_CANCELLED_AFTER;
    memset(&receipt,0,sizeof(receipt));receipt.domain=PT_READERS_COMMAND_DOMAIN;receipt.origin=PT_READERS_BACKEND_ACTUAL;
    receipt.queue=c->packet.key[0].queue;receipt.session=b->session;receipt.generation=b->grid.generation;
    receipt.ticket=ticket;receipt.owner=c->owner;receipt.count=c->packet.count;
    receipt.event=c->event;receipt.context=c->binding.context;receipt.context_bytes=c->binding.context_bytes;
    for(i=0;i<receipt.count;++i){
        struct pt_readers_action_receipt *a=receipt.action+i;
        a->key=c->packet.key[i];a->command=c->unknown?PT_READERS_UNKNOWN:c->command;
        a->reader=c->state[i];a->adoption=c->adopted[i]?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;
        if(c->command==PT_READERS_ISSUED||c->command==PT_READERS_CANCELLED_AFTER){a->observed=c->observed;a->issued=c->issued;}
    }
    if(result==1)memset(c,0,sizeof(*c));
    *out=receipt;b->busy=0;
    return result==1?PT_READERS_COMMAND_DETACHED:PT_READERS_OBSERVATION;
}
static enum pt_readers_reply poll_command(void *b,uint64_t t,struct pt_readers_command_receipt *o)
{return command_probe(b,t,o,0);}
static enum pt_readers_reply cancel_command(void *b,uint64_t t,struct pt_readers_command_receipt *o)
{return command_probe(b,t,o,1);}
static enum pt_readers_reply reader_probe(void *context,const struct pt_readers_domain *reference,struct pt_readers_reader_receipt *out,unsigned cancel)
{
    struct pt_readers_activation *b=context;struct activation_reader *r=NULL;struct pt_readers_reader_receipt receipt;
    unsigned i,j;int result;
    if(!b||!reference||!out||!output_apart(b,out,sizeof(*out))||!enter(b))return PT_READERS_UNCERTAIN;
    for(i=0;i<PT_READERS_ACTIVATION_READERS;++i)if(b->reader[i].held&&b->reader[i].reference==reference){r=b->reader+i;break;}
    if(!r||!key_equal(&reference->key,&r->key)||reference->binding.context!=r->binding.context||reference->binding.context_bytes!=r->binding.context_bytes){b->failed=1;b->busy=0;return PT_READERS_UNCERTAIN;}
    if(cancel){r->closed=1;r->cancelled=1;}
    result=r->port_refs?b->port.reader_quiet(b->port.context,&r->key,cancel):1;
    if(result!=0&&result!=1){b->failed=1;b->busy=0;return PT_READERS_UNCERTAIN;}
    if(b->failed&&result==0){b->busy=0;return PT_READERS_UNCERTAIN;}
    if(result==1){
        /* A copied pending activation still owns an independent sample reference.
         * Cancellation is whole-batch; the port must independently stop callbacks. */
        for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(b->command[i].held)
            for(j=0;j<b->command[i].packet.count;++j)if(key_equal(b->command[i].packet.key+j,&r->key)){
                if(b->command[i].command==PT_READERS_WAITING){
                    if(!cancel){b->busy=0;return PT_READERS_PENDING;}
                    b->command[i].command=PT_READERS_CANCELLED_BEFORE;
                    {unsigned k;for(k=0;k<b->command[i].packet.count;++k){b->command[i].packet.action[k].data=NULL;b->command[i].packet.action[k].words=0;}}
                }
                /* A never-issued TRIGGER command has no reader-adoption fact.
                 * Its independent reader receipt below can prove retirement
                 * while this historical command snapshot remains NONE. */
                if(b->command[i].command==PT_READERS_CANCELLED_BEFORE&&
                   b->command[i].packet.action[j].kind==PT_SCHEDULED_TRIGGER&&
                   !b->command[i].adopted[j])b->command[i].state[j]=PT_READERS_NONE;
                else b->command[i].state[j]=PT_READERS_RETIRED;
            }
        if((b->active_mask&(1U<<r->key.slot))&&key_equal(b->slot+r->key.slot,&r->key)){
            b->active_mask&=~(1U<<r->key.slot);memset(b->slot+r->key.slot,0,sizeof(*b->slot));
        }
    }
    memset(&receipt,0,sizeof(receipt));receipt.domain=PT_READERS_READER_DOMAIN;receipt.key=r->key;
    receipt.reference=r->reference;receipt.context=r->binding.context;receipt.context_bytes=r->binding.context_bytes;
    receipt.state=result==1?PT_READERS_RETIRED:r->state;receipt.adoption=r->adopted?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;
    receipt.observed=r->observed;receipt.issued=r->issued;
    if(b->failed&&result==1)receipt.state=PT_READERS_STATE_UNKNOWN;
    if(result==1)memset(r,0,sizeof(*r));
    *out=receipt;b->busy=0;
    return result==1?PT_READERS_READER_RETIRED:PT_READERS_OBSERVATION;
}
static enum pt_readers_reply poll_reader(void *b,const struct pt_readers_domain *r,struct pt_readers_reader_receipt *o)
{return reader_probe(b,r,o,0);}
static enum pt_readers_reply cancel_reader(void *b,const struct pt_readers_domain *r,struct pt_readers_reader_receipt *o)
{return reader_probe(b,r,o,1);}
enum pt_scheduled_result pt_readers_activation_open(const struct pt_allocator *a,const struct pt_scheduled_grid *g,uint64_t session,const struct pt_readers_activation_port *p,struct pt_readers_activation **out)
{
    struct pt_readers_activation *b;struct pt_allocator allocator;struct pt_readers_activation_port port;struct pt_scheduled_grid grid;struct pt_elapsed_clock clock;
    if(!a||!g||!p||!out||!a->allocate||!a->release||!session||!g->generation||g->frequency<g->rate||
       pt_elapsed_clock_init(&clock,g->frequency,g->rate,g->epoch,0)!=PT_ELAPSED_OK||
       p->version!=PT_READERS_ACTIVATION_PORT_VERSION||p->flags!=PT_READERS_ACTIVATION_PORT_REQUIRED||
       !p->context_bytes||!extent(p->context,p->context_bytes)||!p->read_clock||!p->publish||!p->commit||!p->command_quiet||!p->reader_quiet||
       !apart(out,sizeof(*out),a,sizeof(*a))||!apart(out,sizeof(*out),g,sizeof(*g))||!apart(out,sizeof(*out),p,sizeof(*p))||!apart(out,sizeof(*out),p->context,p->context_bytes))return PT_SCHEDULED_INVALID;
    allocator=*a;grid=*g;port=*p;b=allocator.allocate(allocator.context,sizeof(*b));if(!b)return PT_SCHEDULED_CAPACITY;
    if(!apart(b,sizeof(*b),out,sizeof(*out))||!apart(b,sizeof(*b),a,sizeof(*a))||!apart(b,sizeof(*b),g,sizeof(*g))||!apart(b,sizeof(*b),p,sizeof(*p))||!apart(b,sizeof(*b),port.context,port.context_bytes))return PT_SCHEDULED_INVALID;
    memset(b,0,sizeof(*b));b->allocator=allocator;b->grid=grid;b->port=port;b->session=session;*out=b;return PT_SCHEDULED_OK;
}
enum pt_scheduled_result pt_readers_activation_api(struct pt_readers_activation *b,struct pt_readers_backend *out)
{
    struct pt_readers_backend api;
    if(!b||!out||!output_apart(b,out,sizeof(*out))||!enter(b))return PT_SCHEDULED_INVALID;
    if(b->failed){b->busy=0;return PT_SCHEDULED_BACKEND;}
    api=(struct pt_readers_backend){b,sizeof(*b),{PT_SCHEDULED_REQUIRED,2,4},PT_READERS_VERSION,PT_READERS_REQUIRED,8,backend_clock,backend_submit,poll_command,cancel_command,poll_reader,cancel_reader};
    *out=api;b->busy=0;return PT_SCHEDULED_OK;
}
int pt_readers_activation_close(struct pt_readers_activation **out)
{
    struct pt_readers_activation *b;struct pt_allocator a;unsigned i,reentry=0;
    if(!out||!(b=*out)||!output_apart(b,out,sizeof(*out))||!enter(b))return 0;
    for(i=0;i<PT_READERS_ACTIVATION_COMMANDS;++i)if(b->command[i].held){b->busy=0;return 0;}
    for(i=0;i<PT_READERS_ACTIVATION_READERS;++i)if(b->reader[i].held){b->busy=0;return 0;}
    a=b->allocator;b->release_latch=&reentry;*out=NULL;a.release(a.context,b);
    return !reentry&&!*out;
}
