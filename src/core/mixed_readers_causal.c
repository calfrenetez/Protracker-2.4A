#include "mixed_readers_causal.h"
#include "mixed_readers_causal_queue_internal.h"
#include "mixed_readers_causal_factory_internal.h"
#include "mixed_readers_causal_control_internal.h"
#include "mixed_readers_causal_stop_internal.h"
#include <stddef.h>
#include <string.h>
struct activation_reader {
    struct pt_mixed_causal_reader_identity identity;
    const struct pt_mixed_readers_span *vector;size_t vector_bytes;
    struct pt_mixed_readers_span spans[PT_MIXED_READERS_SPANS];unsigned count;
    struct pt_mixed_readers_card card;
    uint64_t frame,first,last,observed,issued;
    unsigned held,adopted,closed,cancelled,port_refs;
    int quiet_outcome;
    enum pt_mixed_readers_state state;
};
struct activation_command {
    struct pt_mixed_causal_command_identity identity;
    struct pt_mixed_causal_packet packet;
    uint64_t observed,issued;
    unsigned held,unknown,port_refs;
    int quiet_outcome,publication_outcome;
    enum pt_mixed_readers_command command;
    enum pt_mixed_readers_state state[PT_MIXED_READERS_ACTIONS];
    unsigned adopted[PT_MIXED_READERS_ACTIONS];
};
struct constructor {
    unsigned busy,failed;struct pt_mixed_causal_config config;
};
struct causal_pair {
    struct pt_mixed_causal_command_identity predecessor;
    struct pt_mixed_causal_actual predicted,completed_post;
    struct pt_mixed_causal_publication publication;
    uint64_t first,successor,serial,observed,issued,publishing;
    uint64_t completed_first_tick,completed_last_tick;
    unsigned admitted,published,completed,suppressed,commit_called;
    int commit_outcome;
};
struct pt_mixed_causal_owner {
    struct pt_allocator allocator;struct pt_mixed_readers_span allocator_context;
    struct pt_mixed_readers_grid grid;struct pt_mixed_causal_port port;
    struct pt_mixed_causal_registration registration;
    struct pt_mixed_readers_output *queue;
    uint64_t last_frame,last_ticks;
    unsigned ordered,clock_seen,busy,task_busy,failed,faults;
    unsigned source_closed,source_attempted,source_uncertain,construction_alias;
    unsigned *release_latch;
    const struct pt_mixed_causal_config *original_config;
    void *construction_workspace;size_t construction_capacity;
    struct pt_mixed_causal_owner **construction_output;
    struct constructor *construction;
    struct pt_mixed_readers_key slot[PT_MIXED_CAUSAL_SLOTS];unsigned mask;
    struct activation_command command[PT_MIXED_CAUSAL_COMMANDS],staged;
    struct activation_reader reader[PT_MIXED_CAUSAL_READERS];
    struct activation_reader staged_reader[PT_MIXED_READERS_ACTIONS];
    struct causal_pair causal;
    struct pt_mixed_causal_control_port control_port;
    struct pt_mixed_causal_control_publication control_publication;
    unsigned control_bound;
    struct pt_mixed_causal_stop_port stop_port;
    struct pt_mixed_causal_stop_publication stop_publication;
    unsigned stop_bound;
};
static int extent(const void *p,size_t n)
{return !n||(p&&(uintptr_t)p<=UINTPTR_MAX-(n-1));}
static int addresses_apart(uintptr_t a,size_t n,uintptr_t b,size_t m)
{return (!n||(a&&a<=UINTPTR_MAX-(n-1)))&&(!m||(b&&b<=UINTPTR_MAX-(m-1)))&&
    (!n||!m||(a<=b?n<=b-a:m<=a-b));}
static int apart(const void *p,size_t n,const void *q,size_t m)
{return addresses_apart((uintptr_t)p,n,(uintptr_t)q,m);}
static size_t alignment(void)
{struct aligned {char byte;struct constructor value;};return offsetof(struct aligned,value);}
static size_t owner_alignment(void)
{struct aligned {char byte;struct pt_mixed_causal_owner value;};return offsetof(struct aligned,value);}
static size_t queue_offset(void)
{size_t n=sizeof(struct constructor),a=pt_mixed_readers_workspace_alignment();return n+(a-n%a)%a;}
size_t pt_mixed_causal_workspace_size(void)
{return queue_offset()+pt_mixed_readers_workspace_size();}
size_t pt_mixed_causal_workspace_alignment(void)
{size_t a=alignment(),b=pt_mixed_readers_workspace_alignment();return a>b?a:b;}
size_t pt_mixed_causal_control_size(void){return sizeof(struct pt_mixed_causal_owner);}
static int key_equal(const struct pt_mixed_readers_key *a,const struct pt_mixed_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->route==b->route&&a->slot==b->slot;}
static int key_zero(const struct pt_mixed_readers_key *k)
{return !k->queue&&!k->session&&!k->generation&&!k->trigger&&!k->owner&&!k->serial&&!k->action&&!k->route&&!k->slot;}
static int slot_index(unsigned route,unsigned slot,unsigned *index)
{if(route==PT_MIXED_READERS_PAULA&&slot<4){*index=slot;return 1;}
 if(route==PT_MIXED_READERS_AMIGUS&&slot<16){*index=4+slot;return 1;}return 0;}
static void fault(struct pt_mixed_causal_owner *b)
{b->failed=1;b->causal.suppressed=1;++b->faults;if(b->release_latch)*b->release_latch=1;}
void pt_mixed_causal_fail_closed(struct pt_mixed_causal_owner *b)
{if(b)fault(b);}
static int enter(struct pt_mixed_causal_owner *b)
{if(b->busy){fault(b);return 0;}b->busy=1;return 1;}
static int task_enter(struct pt_mixed_causal_owner *b)
{if(b->busy||b->task_busy){fault(b);return 0;}b->task_busy=1;return 1;}
/* Task-only guards. Fire must never call this or inspect span metadata. */
static int output_apart(const struct pt_mixed_causal_owner *b,const void *p,size_t n)
{
    unsigned i,j;
    if(!n||!apart(p,n,b,sizeof(*b))||!apart(p,n,b->port.context,b->port.context_bytes)||
       !apart(p,n,b->allocator_context.data,b->allocator_context.bytes)||
       (b->queue&&!pt_mixed_readers_output_disjoint(b->queue,p,n)))return 0;
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held){
        const struct pt_mixed_causal_command_identity *d=&b->command[i].identity;
        if(!apart(p,n,d->event,sizeof(*d->event))||!apart(p,n,d->binding.context,d->binding.context_bytes))return 0;
    }
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held){
        const struct activation_reader *r=b->reader+i;
        if(!apart(p,n,r->identity.reference,sizeof(*r->identity.reference))||
           !apart(p,n,r->identity.binding.context,r->identity.binding.context_bytes)||
           !apart(p,n,r->vector,r->vector_bytes))return 0;
        for(j=0;j<r->count;++j)if(!apart(p,n,r->spans[j].data,r->spans[j].bytes))return 0;
    }
    return 1;
}
static int source_apart(const struct pt_mixed_causal_owner *b,const void *p,size_t n)
{return n&&apart(p,n,b,sizeof(*b))&&apart(p,n,b->port.context,b->port.context_bytes)&&
    apart(p,n,b->allocator_context.data,b->allocator_context.bytes);}
/* Address-only scratch query: never read uninitialized automatic contents. */
static int output_address_apart(const struct pt_mixed_causal_owner *b,uintptr_t address,size_t n)
{return output_apart(b,(const void *)address,n);}
static struct activation_command *find_command(struct pt_mixed_causal_owner *b,uint64_t ticket)
{unsigned i;for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held&&b->command[i].packet.ticket==ticket)return b->command+i;return NULL;}
static struct activation_reader *find_reader(struct pt_mixed_causal_owner *b,const struct pt_mixed_readers_key *key)
{unsigned i;for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held&&key_equal(&b->reader[i].identity.key,key))return b->reader+i;return NULL;}
static int clock_read(struct pt_mixed_causal_owner *b,uint64_t *out)
{
    uint64_t ticks=0;uint32_t frequency=0;int r=b->port.read_clock(b->port.context,&ticks,&frequency);
    if(r!=1||b->failed||frequency!=b->grid.frequency||ticks<b->grid.epoch||(b->clock_seen&&ticks<b->last_ticks))return 0;
    b->clock_seen=1;b->last_ticks=ticks;*out=ticks;return 1;
}
static int backend_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_mixed_causal_owner *b=context;uint64_t now;
    if(!b||!ticks||!frequency||!output_apart(b,ticks,sizeof(*ticks))||!output_apart(b,frequency,sizeof(*frequency))||
       !apart(ticks,sizeof(*ticks),frequency,sizeof(*frequency))||!enter(b))return 0;
    if(!clock_read(b,&now)){b->failed=1;b->causal.suppressed=1;b->busy=0;return 0;}
    *ticks=now;*frequency=b->grid.frequency;b->busy=0;return 1;
}
static int original_window(const struct pt_mixed_causal_owner *b,const struct pt_mixed_readers_event *e)
{
    struct pt_elapsed_clock clock;uint64_t first,last;
    return pt_elapsed_clock_init(&clock,b->grid.frequency,b->grid.rate,b->grid.epoch,0)==PT_ELAPSED_OK&&
        e->batch.frame!=UINT64_MAX&&pt_elapsed_clock_deadline(&clock,e->batch.frame,&first)==PT_ELAPSED_OK&&
        pt_elapsed_clock_deadline(&clock,e->batch.frame+1,&last)==PT_ELAPSED_OK&&first==e->first&&last==e->last&&first<last;
}
static int card_valid(const struct pt_mixed_readers_card *c,const struct pt_amigus_voice_plan *p)
{
    uint32_t end;
    if(!c->reservation||!c->cache||!c->version||!c->serial||c->cache_slot>=32||(c->bits!=8&&c->bits!=16)||
       c->little_endian>1||c->source_channel>1||(c->bits==16&&(c->logical_bytes&1))||(c->address&3)||
       !c->logical_bytes||c->logical_bytes>c->full_capacity||(c->full_capacity&3)||
       c->address>=PT_AMIGUS_RAM_ADDRESS_SPACE||c->full_capacity>PT_AMIGUS_RAM_ADDRESS_SPACE-c->address)return 0;
    end=c->address+c->logical_bytes;
    return !(p->control&~0x800FU)&&(p->control&0x8000)&&!!(p->control&1)==(c->bits==16)&&
        !!(p->control&8)==(c->bits==16&&c->little_endian)&&p->rate&&p->rate<=0x40000000UL&&
        !((p->start|p->loop|p->end_exclusive)&1)&&p->start>=c->address&&p->start<p->end_exclusive&&
        p->end_exclusive<=end&&p->end_exclusive<PT_AMIGUS_RAM_ADDRESS_SPACE&&p->loop>=c->address&&
        p->loop<p->end_exclusive&&((p->control&2)||p->loop==c->address);
}
static int zero_card(const struct pt_mixed_readers_card *c)
{return !c->reservation&&!c->cache&&!c->version&&!c->serial&&!c->cache_slot&&!c->bits&&!c->little_endian&&!c->source_channel&&!c->address&&!c->logical_bytes&&!c->full_capacity;}
static int control_geometry(const struct pt_mixed_readers_action *a)
{
    if(a->route==PT_MIXED_READERS_PAULA)return !a->geometry.paula.data&&!a->geometry.paula.words&&
        (a->kind==PT_MIXED_READERS_CONTROL?(a->geometry.paula.period&&a->geometry.paula.volume<=64):
         a->kind==PT_MIXED_READERS_STOP&&!a->geometry.paula.period&&!a->geometry.paula.volume);
    return !a->geometry.amigus.start&&!a->geometry.amigus.loop&&!a->geometry.amigus.end_exclusive&&!a->geometry.amigus.control&&
        (a->kind==PT_MIXED_READERS_CONTROL?(a->geometry.amigus.rate&&a->geometry.amigus.rate<=0x40000000UL):
         a->kind==PT_MIXED_READERS_STOP&&!a->geometry.amigus.rate&&!a->geometry.amigus.left&&!a->geometry.amigus.right);
}
static int prepare(struct pt_mixed_causal_owner *b,const struct pt_mixed_readers_event *e,unsigned *new_count)
{
    struct activation_command *c=&b->staged;unsigned i,j,mask=0;
    if(!e||!source_apart(b,e,sizeof(*e))||e->queue!=b->queue||e->session!=b->registration.session||
       !e->ticket||!e->command_owner||!source_apart(b,e->binding.context,e->binding.context_bytes)||
       e->batch.generation!=b->grid.generation||!e->batch.count||e->batch.count>PT_MIXED_READERS_ACTIONS||
       (b->ordered&&e->batch.frame<=b->last_frame)||!original_window(b,e))return 0;
    memset(c,0,sizeof(*c));memset(b->staged_reader,0,sizeof(b->staged_reader));*new_count=0;
    c->identity=(struct pt_mixed_causal_command_identity){b->registration,e->ticket,e->command_owner,e,e->binding};
    c->packet.registration=b->registration;c->packet.ticket=e->ticket;c->packet.frame=e->batch.frame;
    c->packet.first=e->first;c->packet.last=e->last;c->packet.count=e->batch.count;c->packet.expected_mask=b->mask;
    memcpy(c->packet.expected,b->slot,sizeof(b->slot));
    for(i=0;i<e->batch.count;++i){
        const struct pt_mixed_readers_action *a=e->batch.action+i;
        const struct pt_mixed_readers_domain *d=e->reader[i];struct activation_reader *r;unsigned index,covered=0;
        if(!slot_index(a->route,a->slot,&index)||(mask&(1U<<index))||!d||!source_apart(b,d,sizeof(*d))||
           !source_apart(b,d->binding.context,d->binding.context_bytes)||d->key.queue!=e->queue||d->key.session!=e->session||
           d->key.generation!=b->grid.generation||d->key.route!=a->route||d->key.slot!=a->slot||!d->key.trigger||
           !d->key.owner||!d->key.serial||d->key.action>=PT_MIXED_READERS_ACTIONS)return 0;
        mask|=1U<<index;memcpy(c->packet.action+i,a,sizeof(*a));c->packet.key[i]=d->key;
        if(a->kind==PT_MIXED_READERS_TRIGGER){
            size_t bytes=(size_t)a->geometry.paula.words*2;uintptr_t p=(uintptr_t)a->geometry.paula.data;
            if(d->key.trigger!=e->ticket||d->key.action!=i||find_reader(b,&d->key)||!d->count||d->count>PT_MIXED_READERS_SPANS||
               !source_apart(b,d->spans,d->count*sizeof(*d->spans)))return 0;
            r=b->staged_reader+(*new_count)++;r->identity=(struct pt_mixed_causal_reader_identity){b->registration,d->key,d,d->binding};
            r->vector=d->spans;r->vector_bytes=d->count*sizeof(*d->spans);r->count=d->count;r->card=d->card;
            r->frame=e->batch.frame;r->first=e->first;r->last=e->last;
            for(j=0;j<d->count;++j){struct pt_mixed_readers_span s=d->spans[j];uintptr_t q=(uintptr_t)s.data;
                if(!source_apart(b,s.data,s.bytes)||!apart(s.data,s.bytes,d->binding.context,d->binding.context_bytes))return 0;
                r->spans[j]=s;if(p>=q&&p-q<=s.bytes&&bytes<=s.bytes-(p-q))covered=1;
            }
            if(a->route==PT_MIXED_READERS_PAULA){
                if(!zero_card(&d->card)||!a->geometry.paula.words||!a->geometry.paula.period||a->geometry.paula.volume>64||
                   (p&1)||!extent(a->geometry.paula.data,bytes)||!covered)return 0;
            }else if(!card_valid(&d->card,&a->geometry.amigus))return 0;
            c->packet.card[i]=d->card;r->held=1;r->state=PT_MIXED_READER_RESERVED;c->state[i]=PT_MIXED_READER_NONE;
        }else{
            r=find_reader(b,&d->key);
            if(!control_geometry(a)||!r||r->identity.reference!=d||
               r->identity.binding.context!=d->binding.context||r->identity.binding.context_bytes!=d->binding.context_bytes||
               !r->adopted||r->closed||r->state!=PT_MIXED_READER_ACTIVE||!(b->mask&(1U<<index))||
               !key_equal(b->slot+index,&d->key))return 0;
            c->state[i]=r->state;c->adopted[i]=1;
        }
    }
    c->held=1;c->command=PT_MIXED_COMMAND_WAITING;return 1;
}
static void staged_clear(struct pt_mixed_causal_owner *b)
{memset(&b->staged,0,sizeof(b->staged));memset(b->staged_reader,0,sizeof(b->staged_reader));}
/* Complete task-side source extent admission precedes writable busy/fault
 * state. In particular a malformed/aliased recursive candidate is not allowed
 * to write through a purported backend control that names its source bytes. */
static int candidate_guard(const struct pt_mixed_causal_owner *b,const struct pt_mixed_readers_event *e)
{
    unsigned i,j;
    if(!e||!source_apart(b,e,sizeof(*e))||!source_apart(b,e->binding.context,e->binding.context_bytes)||
       !e->batch.count||e->batch.count>PT_MIXED_READERS_ACTIONS)return 0;
    for(i=0;i<e->batch.count;++i){const struct pt_mixed_readers_domain *d=e->reader[i];
        if(!d||!source_apart(b,d,sizeof(*d))||!source_apart(b,d->binding.context,d->binding.context_bytes)||
           !d->count||d->count>PT_MIXED_READERS_SPANS||!source_apart(b,d->spans,d->count*sizeof(*d->spans)))return 0;
        for(j=0;j<d->count;++j)if(!source_apart(b,d->spans[j].data,d->spans[j].bytes))return 0;
    }
    return 1;
}
/* Actual retained metadata gate, before second admission/publication. A genuine
 * queue ACTIVE getter is independently required for every requested target. */
static int control_first_current(const struct pt_mixed_causal_owner *b)
{
    const struct causal_pair *p=&b->causal;unsigned i;
    if(!b->control_bound||b->failed||b->source_closed||b->source_attempted||b->source_uncertain||
       !p->first||!p->completed||p->suppressed||!p->commit_called||p->commit_outcome!=1||
       p->predecessor.ticket!=p->first||p->predecessor.registration.owner!=b||
       p->predecessor.registration.queue!=b->registration.queue||
       p->predecessor.registration.session!=b->registration.session||
       p->predecessor.registration.generation!=b->registration.generation||
       p->completed_first_tick>=p->completed_last_tick||p->observed<p->completed_first_tick||
       p->issued<p->observed||p->issued>=p->completed_last_tick||
       p->completed_post.active_mask!=b->mask||p->completed_post.adopted_mask!=b->mask||
       p->predicted.active_mask!=b->mask||p->predicted.adopted_mask)return 0;
    for(i=0;i<20;++i){struct activation_reader *r;
        if(!key_equal(p->completed_post.slot+i,b->slot+i)||
           !key_equal(p->predicted.slot+i,b->slot+i))return 0;
        if(!(b->mask&(1U<<i))){if(!key_zero(b->slot+i))return 0;continue;}
        r=find_reader((struct pt_mixed_causal_owner *)(void *)b,b->slot+i);
        if(!r||!r->held||!r->adopted||r->closed||r->cancelled||r->state!=PT_MIXED_READER_ACTIVE||
           r->identity.key.trigger!=p->first||r->observed!=p->observed||r->issued!=p->issued)return 0;
    }
    return 1;
}
static int stop_first_current(const struct pt_mixed_causal_owner *b)
{
    const struct causal_pair *p=&b->causal;unsigned i;
    if(!b->stop_bound||b->failed||b->source_closed||b->source_attempted||b->source_uncertain||
       !p->first||!p->completed||p->suppressed||!p->commit_called||p->commit_outcome!=1||
       p->predecessor.ticket!=p->first||p->predecessor.registration.owner!=b||
       p->predecessor.registration.queue!=b->registration.queue||
       p->predecessor.registration.session!=b->registration.session||
       p->predecessor.registration.generation!=b->registration.generation||
       p->completed_first_tick>=p->completed_last_tick||p->observed<p->completed_first_tick||
       p->issued<p->observed||p->issued>=p->completed_last_tick||
       p->completed_post.active_mask!=b->mask||p->completed_post.adopted_mask!=b->mask||
       p->predicted.active_mask!=b->mask||p->predicted.adopted_mask)return 0;
    for(i=0;i<20;++i){struct activation_reader *r;
        if(!key_equal(p->completed_post.slot+i,b->slot+i)||
           !key_equal(p->predicted.slot+i,b->slot+i))return 0;
        if(!(b->mask&(1U<<i))){if(!key_zero(b->slot+i))return 0;continue;}
        r=find_reader((struct pt_mixed_causal_owner *)(void *)b,b->slot+i);
        if(!r||!r->held||!r->adopted||r->closed||r->cancelled||r->state!=PT_MIXED_READER_ACTIVE||
           r->identity.key.trigger!=p->first||r->observed!=p->observed||r->issued!=p->issued)return 0;
    }
    return 1;
}
static void control_first_prediction(struct pt_mixed_causal_owner *b,const struct activation_command *c)
{
    unsigned i,index=0;
    memset(&b->causal.predicted,0,sizeof(b->causal.predicted));
    b->causal.predicted.active_mask=c->packet.expected_mask;
    memcpy(b->causal.predicted.slot,c->packet.expected,sizeof(c->packet.expected));
    for(i=0;i<c->packet.count;++i){
        slot_index(c->packet.action[i].route,c->packet.action[i].slot,&index);
        b->causal.predicted.active_mask|=1U<<index;
        b->causal.predicted.slot[index]=c->packet.key[i];
    }
    /* adopted_mask deliberately stays zero; this is not actual adoption. */
}
static int backend_submit(void *context,const struct pt_mixed_readers_event *e)
{
    struct pt_mixed_causal_owner *b=context;struct activation_command *c;
    unsigned i,j,index=PT_MIXED_CAUSAL_COMMANDS,n=0,free_count=0;
    unsigned assigned[PT_MIXED_READERS_ACTIONS];uint64_t now,after;int result;
    struct pt_mixed_causal_command_identity identity;
    if(!b)return -1;
    if(!candidate_guard(b,e))return 0;
    if(!enter(b))return -1;
    if(b->failed||b->source_closed||b->source_uncertain){b->busy=0;return -1;}
    if(e->ticket!=b->causal.first&&
       (e->ticket!=b->causal.successor||b->causal.publishing!=e->ticket||
        !b->causal.admitted||b->causal.suppressed)){b->busy=0;return 0;}
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(!b->command[i].held){index=i;break;}
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)free_count+=!b->reader[i].held;
    if(index==PT_MIXED_CAUSAL_COMMANDS||!prepare(b,e,&n)||n>free_count){staged_clear(b);b->busy=0;return 0;}
    for(i=0;i<b->staged.packet.count;++i)
        if(b->staged.packet.action[i].kind!=(b->control_bound&&e->ticket==b->causal.successor?
            PT_MIXED_READERS_CONTROL:b->stop_bound&&e->ticket==b->causal.successor?
            PT_MIXED_READERS_STOP:PT_MIXED_READERS_TRIGGER)){
            staged_clear(b);b->busy=0;return 0;}
    if(b->control_bound&&e->ticket==b->causal.successor&&
       (n||!control_first_current(b))){staged_clear(b);b->busy=0;return 0;}
    if(b->stop_bound&&e->ticket==b->causal.successor&&
       (n||!stop_first_current(b))){staged_clear(b);b->busy=0;return 0;}
    if(e->ticket==b->causal.successor&&!b->control_bound&&!b->stop_bound){
        b->staged.packet.expected_mask=b->causal.predicted.active_mask;
        memcpy(b->staged.packet.expected,b->causal.predicted.slot,sizeof(b->staged.packet.expected));
    }
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(e->ticket!=b->causal.successor&&b->command[i].held&&b->command[i].command==PT_MIXED_COMMAND_WAITING)
        for(j=0;j<b->command[i].packet.count;++j)if(b->command[i].packet.action[j].kind!=PT_MIXED_READERS_CONTROL){staged_clear(b);b->busy=0;return 0;}
    c=b->command+index;*c=b->staged;
    for(i=0,j=0;i<PT_MIXED_CAUSAL_READERS&&j<n;++i)if(!b->reader[i].held){assigned[j]=i;b->reader[i]=b->staged_reader[j++];}
    staged_clear(b);
    if(!clock_read(b,&now)||now>=c->packet.first){c->unknown=1;c->command=PT_MIXED_COMMAND_UNKNOWN;b->failed=1;b->causal.suppressed=1;b->busy=0;return -1;}
    c->port_refs=1;for(i=0;i<n;++i)b->reader[assigned[i]].port_refs=1;
    /* Staging is already transferred and cleared; busy excludes reuse. */
    memcpy(&b->staged.packet,&c->packet,sizeof(c->packet));memcpy(&identity,&c->identity,sizeof(identity));
    if(e->ticket==b->causal.successor&&b->control_bound){
        struct pt_mixed_causal_control_publication *p=&b->control_publication;
        memset(p,0,sizeof(*p));memcpy(&p->predecessor,&b->causal.predecessor,sizeof(p->predecessor));
        memcpy(&p->successor,&c->identity,sizeof(p->successor));
        p->first_tick=b->causal.completed_first_tick;p->last_tick=b->causal.completed_last_tick;
        p->observed=b->causal.observed;p->issued=b->causal.issued;p->serial=b->causal.serial;
        memcpy(&p->first_post,&b->causal.completed_post,sizeof(p->first_post));
        memcpy(&p->packet,&c->packet,sizeof(p->packet));
        result=b->control_port.publish_control(b->port.context,b,p);
        if(memcmp(&p->predecessor,&b->causal.predecessor,sizeof(p->predecessor))||
           memcmp(&p->successor,&c->identity,sizeof(p->successor))||
           p->first_tick!=b->causal.completed_first_tick||p->last_tick!=b->causal.completed_last_tick||
           p->observed!=b->causal.observed||p->issued!=b->causal.issued||p->serial!=b->causal.serial||
           memcmp(&p->first_post,&b->causal.completed_post,sizeof(p->first_post))||
           memcmp(&p->packet,&b->staged.packet,sizeof(p->packet)))fault(b);
        memset(p,0,sizeof(*p));
    }else if(e->ticket==b->causal.successor&&b->stop_bound){
        struct pt_mixed_causal_stop_publication *p=&b->stop_publication;
        memset(p,0,sizeof(*p));memcpy(&p->predecessor,&b->causal.predecessor,sizeof(p->predecessor));
        memcpy(&p->successor,&c->identity,sizeof(p->successor));
        p->first_tick=b->causal.completed_first_tick;p->last_tick=b->causal.completed_last_tick;
        p->observed=b->causal.observed;p->issued=b->causal.issued;p->serial=b->causal.serial;
        memcpy(&p->first_post,&b->causal.completed_post,sizeof(p->first_post));
        memcpy(&p->packet,&c->packet,sizeof(p->packet));
        result=b->stop_port.publish_stop(b->port.context,b,p);
        if(memcmp(&p->predecessor,&b->causal.predecessor,sizeof(p->predecessor))||
           memcmp(&p->successor,&c->identity,sizeof(p->successor))||
           p->first_tick!=b->causal.completed_first_tick||p->last_tick!=b->causal.completed_last_tick||
           p->observed!=b->causal.observed||p->issued!=b->causal.issued||p->serial!=b->causal.serial||
           memcmp(&p->first_post,&b->causal.completed_post,sizeof(p->first_post))||
           memcmp(&p->packet,&b->staged.packet,sizeof(p->packet)))fault(b);
        memset(p,0,sizeof(*p));
    }else if(e->ticket==b->causal.successor){
        struct activation_command *first=find_command(b,b->causal.first);
        struct pt_mixed_causal_publication *p=&b->causal.publication;
        if(!first||first->command!=PT_MIXED_COMMAND_WAITING||first->unknown){fault(b);result=-1;}
        else{
            memset(p,0,sizeof(*p));memcpy(&p->predecessor,&first->identity,sizeof(p->predecessor));
            memcpy(&p->successor_identity,&c->identity,sizeof(p->successor_identity));
            memcpy(&p->first,&first->packet,sizeof(p->first));memcpy(&p->successor,&c->packet,sizeof(p->successor));p->serial=b->causal.serial;
            result=b->port.publish_successor(b->port.context,b,p);
            if(memcmp(&p->predecessor,&first->identity,sizeof(p->predecessor))||
               memcmp(&p->first,&first->packet,sizeof(p->first))||memcmp(&p->successor,&c->packet,sizeof(p->successor))||
               memcmp(&p->successor_identity,&identity,sizeof(identity))||
               p->serial!=b->causal.serial)fault(b);
            memset(p,0,sizeof(*p));
        }
    }else{
        if(b->control_bound||b->stop_bound)control_first_prediction(b,c);
        result=b->port.publish(b->port.context,b,&c->identity,&c->packet);
    }
    if(memcmp(&identity,&c->identity,sizeof(identity))){memcpy(&c->identity,&identity,sizeof(identity));fault(b);}
    if(memcmp(&b->staged.packet,&c->packet,sizeof(c->packet))){memcpy(&c->packet,&b->staged.packet,sizeof(c->packet));fault(b);}
    memset(&b->staged.packet,0,sizeof(b->staged.packet));c->publication_outcome=result;
    if(result==0&&!b->failed){for(i=0;i<n;++i)memset(b->reader+assigned[i],0,sizeof(b->reader[i]));memset(c,0,sizeof(*c));b->busy=0;return 0;}
    b->ordered=1;b->last_frame=c->packet.frame;
    if((b->control_bound||b->stop_bound)&&e->ticket==b->causal.first)
        memcpy(&b->causal.predecessor,&c->identity,sizeof(b->causal.predecessor));
    /* Accepted/unknown publication may already own a future callback. A late
     * post-read can only retain and classify unknown, never become refusal. */
    if(!clock_read(b,&after)||after>=c->packet.first||result!=1||b->failed){c->unknown=1;b->failed=1;b->causal.suppressed=1;result=-1;}
    if(e->ticket==b->causal.successor){b->causal.published=1;if(result!=1)b->causal.suppressed=1;}
    for(i=0;i<c->packet.count;++i)if(c->packet.action[i].kind==PT_MIXED_READERS_STOP){struct activation_reader *r=find_reader(b,c->packet.key+i);if(r)r->closed=1;}
    b->busy=0;return result;
}
static int current_packet(const struct pt_mixed_causal_owner *b,const struct pt_mixed_causal_packet *p)
{
    unsigned i,index;
    if(p->registration.owner!=b||p->registration.queue!=b->registration.queue||p->registration.session!=b->registration.session||
       p->registration.generation!=b->grid.generation||p->expected_mask!=b->mask)return 0;
    for(i=0;i<PT_MIXED_CAUSAL_SLOTS;++i)if(!key_equal(p->expected+i,b->slot+i))return 0;
    for(i=0;i<p->count;++i)if(p->action[i].kind!=PT_MIXED_READERS_TRIGGER&&
       (!slot_index(p->action[i].route,p->action[i].slot,&index)||!(b->mask&(1U<<index))||!key_equal(p->key+i,b->slot+index)))return 0;
    return 1;
}
static void geometry_drop(struct activation_command *c)
{unsigned i;for(i=0;i<PT_MIXED_READERS_ACTIONS;++i){memset(&c->packet.action[i].geometry,0,sizeof(c->packet.action[i].geometry));memset(c->packet.card+i,0,sizeof(c->packet.card[i]));}}
static void reader_closed(struct pt_mixed_causal_owner *b,struct activation_reader *r)
{
    unsigned i,j;r->closed=1;r->state=PT_MIXED_READER_DRAINING;
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held&&b->command[i].command!=PT_MIXED_COMMAND_WAITING)
        for(j=0;j<b->command[i].packet.count;++j)if(key_equal(b->command[i].packet.key+j,&r->identity.key))b->command[i].state[j]=PT_MIXED_READER_DRAINING;
}
static int causal_current(const struct pt_mixed_causal_owner *b)
{
    unsigned i;const struct causal_pair *p=&b->causal;
    if(!p->completed||p->suppressed||!p->published||p->completed_post.active_mask!=p->predicted.active_mask)return 0;
    for(i=0;i<20;++i){const struct activation_reader *r;
        if(!key_equal(p->completed_post.slot+i,p->predicted.slot+i))return 0;
        if(!(p->predicted.active_mask&(1U<<i)))continue;
        /* Only copied retained owner metadata is traversed at fire. Neither
         * historical command controls nor queue/domain/holder spans are read. */
        r=find_reader((struct pt_mixed_causal_owner *)(void *)b,p->predicted.slot+i);
        if(!r||!r->adopted||r->cancelled||r->state!=PT_MIXED_READER_ACTIVE)return 0;
    }
    return 1;
}
enum pt_mixed_causal_result pt_mixed_causal_fire(struct pt_mixed_causal_owner *b,uint64_t ticket)
{
    struct activation_command *c;struct pt_mixed_causal_actual actual;
    uint64_t observed,issued;unsigned i,j,index;int result;
    if(!b)return PT_MIXED_CAUSAL_INVALID;
    if(b->task_busy){fault(b);return PT_MIXED_CAUSAL_INVALID;}
    if(!enter(b))return PT_MIXED_CAUSAL_INVALID;
    c=find_command(b,ticket);
    if(!c||c->command!=PT_MIXED_COMMAND_WAITING){b->busy=0;return PT_MIXED_CAUSAL_INVALID;}
    if(b->failed||b->source_closed||b->source_uncertain||!clock_read(b,&observed))goto failed;
    if(observed<c->packet.first){
        if(ticket==b->causal.successor){
            if(!b->causal.admitted||!b->causal.published||b->causal.suppressed)goto failed;
            if(b->causal.completed){if(!causal_current(b)||!current_packet(b,&c->packet))goto failed;}
            else{
                struct activation_command *first=find_command(b,b->causal.first);
                /* Before actual first completion there is no actual predicted
                 * registry. Check the retained original copied identity and
                 * all twenty original keys, then preserve this early invocation.
                 * No queue/event/domain/holder traversal or adoption occurs. */
                if(!first||first->command!=PT_MIXED_COMMAND_WAITING||first->unknown||
                   first->publication_outcome!=1||!first->port_refs||
                   memcmp(&first->identity,&b->causal.predecessor,sizeof(first->identity))||
                   !current_packet(b,&first->packet))goto failed;
            }
        }else if(ticket!=b->causal.first||!current_packet(b,&c->packet))goto failed;
        b->busy=0;return PT_MIXED_CAUSAL_EARLY;
    }
    if(observed>=c->packet.last||!current_packet(b,&c->packet))goto failed;
    if(ticket==b->causal.first){
        if(b->control_bound||b->stop_bound){if(b->causal.admitted||b->causal.published||b->causal.suppressed)goto failed;}
        else if(!b->causal.admitted||!b->causal.published||b->causal.suppressed)goto failed;
    }
    else if(ticket!=b->causal.successor||!causal_current(b))goto failed;
    for(i=0;i<c->packet.count;++i){struct activation_reader *r=find_reader(b,c->packet.key+i);if(!r||r->cancelled)goto failed;}
    /* Keep actual automatic; only the complete packet backup uses scratch. */
    memset(&actual,0,sizeof(actual));memcpy(&b->staged.packet,&c->packet,sizeof(c->packet));result=b->port.commit(b->port.context,&c->packet,&actual);
    if(ticket==b->causal.first){b->causal.commit_called=1;b->causal.commit_outcome=result;b->causal.observed=observed;}
    if(memcmp(&b->staged.packet,&c->packet,sizeof(c->packet))){memcpy(&c->packet,&b->staged.packet,sizeof(c->packet));fault(b);}
    memset(&b->staged.packet,0,sizeof(b->staged.packet));
    if(result!=1||!clock_read(b,&issued)||issued>=c->packet.last||actual.active_mask&~0xFFFFFU||
       actual.adopted_mask!=actual.active_mask)goto failed;
    for(i=0;i<c->packet.count;++i){unsigned bit;
        if(!slot_index(c->packet.action[i].route,c->packet.action[i].slot,&index))goto failed;
        bit=1U<<index;
        if(c->packet.action[i].kind==PT_MIXED_READERS_STOP){if((actual.active_mask&bit)||!key_zero(actual.slot+index))goto failed;}
        else if(!(actual.active_mask&bit)||!key_equal(actual.slot+index,c->packet.key+i))goto failed;
    }
    for(i=0;i<PT_MIXED_CAUSAL_SLOTS;++i){unsigned touched=0;
        for(j=0;j<c->packet.count;++j){slot_index(c->packet.action[j].route,c->packet.action[j].slot,&index);touched|=index==i;}
        if(!touched&&(((actual.active_mask^c->packet.expected_mask)&(1U<<i))||!key_equal(actual.slot+i,c->packet.expected+i)))goto failed;
    }
    if(ticket==b->causal.first){
        if(actual.active_mask!=b->causal.predicted.active_mask)goto failed;
        for(i=0;i<20;++i)if(!key_equal(actual.slot+i,b->causal.predicted.slot+i))goto failed;
    }
    for(i=0;i<c->packet.count;++i){struct activation_reader *r=find_reader(b,c->packet.key+i);
        const struct pt_mixed_readers_action *a=c->packet.action+i;
        if(a->kind==PT_MIXED_READERS_TRIGGER){
            for(j=0;j<PT_MIXED_CAUSAL_READERS;++j)if(b->reader[j].held&&b->reader[j].frame<r->frame&&b->reader[j].identity.key.route==a->route&&
                b->reader[j].identity.key.slot==a->slot&&!key_equal(&b->reader[j].identity.key,&r->identity.key))reader_closed(b,b->reader+j);
            r->adopted=1;r->state=PT_MIXED_READER_ACTIVE;r->observed=observed;r->issued=issued;
        }else if(a->kind==PT_MIXED_READERS_STOP)reader_closed(b,r);
        c->state[i]=r->state;c->adopted[i]=r->adopted;
    }
    b->mask=actual.active_mask;memcpy(b->slot,actual.slot,sizeof(b->slot));geometry_drop(c);
    if(ticket==b->causal.first){memcpy(&b->causal.completed_post,&actual,sizeof(actual));
        b->causal.completed_first_tick=c->packet.first;b->causal.completed_last_tick=c->packet.last;
        b->causal.issued=issued;b->causal.completed=1;}
    c->observed=observed;c->issued=issued;c->command=PT_MIXED_COMMAND_ISSUED;b->busy=0;return PT_MIXED_CAUSAL_COMMITTED;
failed:
    c->unknown=1;b->causal.suppressed=1;b->failed=1;b->busy=0;return PT_MIXED_CAUSAL_FAILED;
}
static enum pt_mixed_readers_reply backend_command(void *context,uint64_t ticket,unsigned cancel,struct pt_mixed_readers_command_receipt *out)
{
    struct pt_mixed_causal_owner *b=context;struct activation_command *c;struct pt_mixed_readers_command_receipt receipt;
    struct pt_mixed_causal_command_identity identity;
    unsigned i,before;int result;
    if(!b||!out||cancel>1||!output_apart(b,out,sizeof(*out))||!enter(b))return PT_MIXED_UNCERTAIN;
    c=find_command(b,ticket);if(!c){b->busy=0;return PT_MIXED_UNCERTAIN;}
    before=b->faults;memcpy(&identity,&c->identity,sizeof(identity));
    result=c->port_refs?b->port.command_quiet(b->port.context,&c->identity,cancel):1;c->quiet_outcome=result;
    if(memcmp(&identity,&c->identity,sizeof(identity))){memcpy(&c->identity,&identity,sizeof(identity));fault(b);}
    if(b->faults!=before){b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(result!=0&&result!=1){b->failed=1;b->causal.suppressed=1;b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(b->failed&&result==0){b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(b->failed)c->unknown=1;
    if(result==1&&c->command==PT_MIXED_COMMAND_WAITING){c->command=PT_MIXED_COMMAND_CANCELLED_BEFORE;geometry_drop(c);}
    else if(result==1&&cancel&&c->command==PT_MIXED_COMMAND_ISSUED)c->command=PT_MIXED_COMMAND_CANCELLED_AFTER;
    memset(&receipt,0,sizeof(receipt));receipt.domain=PT_MIXED_COMMAND_DOMAIN;
    receipt.queue=c->identity.registration.queue;receipt.session=c->identity.registration.session;receipt.generation=c->identity.registration.generation;
    receipt.ticket=ticket;receipt.owner=c->identity.owner;receipt.count=c->packet.count;receipt.event=c->identity.event;receipt.binding=c->identity.binding;
    for(i=0;i<receipt.count;++i){struct pt_mixed_readers_action_receipt *a=receipt.action+i;
        a->key=c->packet.key[i];a->command=c->unknown?PT_MIXED_COMMAND_UNKNOWN:c->command;a->reader=c->state[i];
        a->adoption=c->adopted[i]?PT_MIXED_ADOPTED:PT_MIXED_UNADOPTED;
        if(c->command==PT_MIXED_COMMAND_ISSUED||c->command==PT_MIXED_COMMAND_CANCELLED_AFTER){a->observed=c->observed;a->issued=c->issued;}
    }
    if(result==1&&ticket==b->causal.first&&!b->causal.completed)b->causal.suppressed=1;
    if(result==1)memset(c,0,sizeof(*c));
    *out=receipt;b->busy=0;
    return result==1?PT_MIXED_COMMAND_DETACHED:PT_MIXED_OBSERVATION;
}
static enum pt_mixed_readers_reply backend_reader(void *context,const struct pt_mixed_readers_domain *reference,unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{
    struct pt_mixed_causal_owner *b=context;struct activation_reader *r=NULL;struct pt_mixed_readers_reader_receipt receipt;
    struct pt_mixed_causal_reader_identity identity;
    unsigned i,j,index,before;int result;
    if(!b||!reference||!source_apart(b,reference,sizeof(*reference))||
       !source_apart(b,reference->binding.context,reference->binding.context_bytes)||
       !out||cancel>1||!output_apart(b,out,sizeof(*out))||!enter(b))return PT_MIXED_UNCERTAIN;
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held&&b->reader[i].identity.reference==reference){r=b->reader+i;break;}
    if(!r||!key_equal(&reference->key,&r->identity.key)||reference->binding.context!=r->identity.binding.context||
       reference->binding.context_bytes!=r->identity.binding.context_bytes){b->failed=1;b->causal.suppressed=1;b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(cancel){r->closed=1;r->cancelled=1;}
    before=b->faults;memcpy(&identity,&r->identity,sizeof(identity));
    result=r->port_refs?b->port.reader_quiet(b->port.context,&r->identity,cancel):1;r->quiet_outcome=result;
    if(memcmp(&identity,&r->identity,sizeof(identity))){memcpy(&r->identity,&identity,sizeof(identity));fault(b);}
    if(b->faults!=before){b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(result!=0&&result!=1){b->failed=1;b->causal.suppressed=1;b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(b->failed&&result==0){b->busy=0;return PT_MIXED_UNCERTAIN;}
    if(result==1){
        /* Port R=1 independently proves every expected-only dependent ticket
         * quiet. Classification does not consume their separate C records. */
        for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held){
            struct activation_command *c=b->command+i;unsigned dependent=0;
            for(j=0;j<20;++j)dependent|=key_equal(c->packet.expected+j,&r->identity.key);
            for(j=0;j<c->packet.count;++j)dependent|=key_equal(c->packet.key+j,&r->identity.key);
            if(dependent&&c->command==PT_MIXED_COMMAND_WAITING){
                if(!cancel){b->busy=0;return PT_MIXED_REPLY_PENDING;}
                c->command=PT_MIXED_COMMAND_CANCELLED_BEFORE;geometry_drop(c);
                if(c->packet.ticket==b->causal.first||c->packet.ticket==b->causal.successor)b->causal.suppressed=1;
            }
        }
        for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held)
            for(j=0;j<b->command[i].packet.count;++j)if(key_equal(b->command[i].packet.key+j,&r->identity.key)){
                struct activation_command *c=b->command+i;
                if(c->command==PT_MIXED_COMMAND_WAITING){if(!cancel){b->busy=0;return PT_MIXED_REPLY_PENDING;}c->command=PT_MIXED_COMMAND_CANCELLED_BEFORE;geometry_drop(c);}
                if(c->command==PT_MIXED_COMMAND_CANCELLED_BEFORE&&c->packet.action[j].kind==PT_MIXED_READERS_TRIGGER&&!c->adopted[j])c->state[j]=PT_MIXED_READER_NONE;
                else c->state[j]=PT_MIXED_READER_RETIRED;
            }
        slot_index(r->identity.key.route,r->identity.key.slot,&index);
        if((b->mask&(1U<<index))&&key_equal(b->slot+index,&r->identity.key)){b->mask&=~(1U<<index);memset(b->slot+index,0,sizeof(b->slot[index]));}
    }
    memset(&receipt,0,sizeof(receipt));receipt.domain=PT_MIXED_READER_DOMAIN;receipt.key=r->identity.key;
    receipt.reference=r->identity.reference;receipt.binding=r->identity.binding;receipt.state=result==1?PT_MIXED_READER_RETIRED:r->state;
    receipt.adoption=r->adopted?PT_MIXED_ADOPTED:PT_MIXED_UNADOPTED;receipt.observed=r->observed;receipt.issued=r->issued;
    if(b->failed&&result==1)receipt.state=PT_MIXED_READER_UNKNOWN;
    if(result==1)memset(r,0,sizeof(*r));
    *out=receipt;b->busy=0;
    return result==1?PT_MIXED_READER_RETIRE_PROOF:PT_MIXED_OBSERVATION;
}
static void *queue_allocate(void *context,size_t bytes)
{
    struct pt_mixed_causal_owner *b=context;void *p=b->allocator.allocate(b->allocator.context,bytes);
    if(!p)return NULL;
    if(!source_apart(b,p,bytes)||!apart(p,bytes,b->original_config,sizeof(*b->original_config))||
       !apart(p,bytes,b->construction_workspace,b->construction_capacity)||
       !apart(p,bytes,b->construction_output,sizeof(*b->construction_output))){b->construction_alias=1;return NULL;}
    return p;
}
static void queue_release(void *context,void *p)
{
    struct pt_mixed_causal_owner *b=context;
    /* Core invokes this only AFTER consuming its actual close slot. Consume
     * our registered borrow before external release can free/reenter it. */
    if(b->queue==p)b->queue=NULL;
    b->allocator.release(b->allocator.context,p);
}
enum pt_mixed_readers_result pt_mixed_causal_open(const struct pt_mixed_causal_config *c,void *workspace,size_t capacity,struct pt_mixed_causal_owner **out)
{
    struct constructor *w=workspace;struct pt_mixed_causal_owner *b;
    struct pt_mixed_readers_config qconfig;struct pt_mixed_readers_output *queue=NULL;
    struct pt_allocator a;struct pt_elapsed_clock clock;enum pt_mixed_readers_result result;
    if(!c||!extent(c,sizeof(*c))||!workspace||capacity<pt_mixed_causal_workspace_size()||!extent(workspace,capacity)||
       (uintptr_t)workspace%pt_mixed_causal_workspace_alignment()||!out||!extent(out,sizeof(*out))||
       !apart(workspace,capacity,c,sizeof(*c))||!apart(workspace,capacity,out,sizeof(*out))||!apart(out,sizeof(*out),c,sizeof(*c)))return PT_MIXED_READERS_INVALID;
    if(!c->allocator.allocate||!c->allocator.release||!c->allocator.context||!c->allocator_context.bytes||
       !extent(c->allocator_context.data,c->allocator_context.bytes)||(uintptr_t)c->allocator.context<(uintptr_t)c->allocator_context.data||
       (uintptr_t)c->allocator.context-(uintptr_t)c->allocator_context.data>=c->allocator_context.bytes||
       !c->port.context_bytes||!extent(c->port.context,c->port.context_bytes)||!c->session||!c->grid.generation||
       c->grid.frequency<c->grid.rate||pt_elapsed_clock_init(&clock,c->grid.frequency,c->grid.rate,c->grid.epoch,0)!=PT_ELAPSED_OK||
       !apart(workspace,capacity,c->allocator_context.data,c->allocator_context.bytes)||!apart(workspace,capacity,c->port.context,c->port.context_bytes)||
       !apart(out,sizeof(*out),c->allocator_context.data,c->allocator_context.bytes)||!apart(out,sizeof(*out),c->port.context,c->port.context_bytes)||
       !apart(c,sizeof(*c),c->allocator_context.data,c->allocator_context.bytes)||!apart(c,sizeof(*c),c->port.context,c->port.context_bytes)||
       !addresses_apart((uintptr_t)&qconfig,sizeof(qconfig),(uintptr_t)c->allocator_context.data,c->allocator_context.bytes)||
       !addresses_apart((uintptr_t)&qconfig,sizeof(qconfig),(uintptr_t)c->port.context,c->port.context_bytes)||
       !apart(&queue,sizeof(queue),c->allocator_context.data,c->allocator_context.bytes)||!apart(&queue,sizeof(queue),c->port.context,c->port.context_bytes))return PT_MIXED_READERS_INVALID;
    if(c->port.version!=PT_MIXED_CAUSAL_PORT_VERSION||c->port.flags!=PT_MIXED_CAUSAL_PORT_REQUIRED||
       !c->port.read_clock||!c->port.publish||!c->port.commit||!c->port.command_quiet||!c->port.reader_quiet||
       !c->port.source_close||!c->port.source_quiet||!c->port.publish_successor)return PT_MIXED_READERS_UNSUPPORTED;
    if(c->control_budget<sizeof(*b)||c->queue_budget<pt_mixed_readers_control_size())return PT_MIXED_READERS_CAPACITY;
    if(w->busy){w->failed=1;return PT_MIXED_READERS_BACKEND;}
    memset(w,0,sizeof(*w));memcpy(&w->config,c,sizeof(*c));w->busy=1;a=w->config.allocator;
    b=a.allocate(a.context,sizeof(*b));
    if(!b){int bad=w->failed||memcmp(c,&w->config,sizeof(*c));memset(w,0,sizeof(*w));return bad?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_CAPACITY;}
    if(!apart(b,sizeof(*b),c,sizeof(*c))||!apart(b,sizeof(*b),workspace,capacity)||!apart(b,sizeof(*b),out,sizeof(*out))||
       !apart(b,sizeof(*b),w->config.allocator_context.data,w->config.allocator_context.bytes)||!apart(b,sizeof(*b),w->config.port.context,w->config.port.context_bytes)){
        memset(w,0,sizeof(*w));return PT_MIXED_READERS_INVALID;}
    if(w->failed||memcmp(c,&w->config,sizeof(*c))||(uintptr_t)b%owner_alignment()){
        a.release(a.context,b);memset(w,0,sizeof(*w));return PT_MIXED_READERS_BACKEND;}
    memset(b,0,sizeof(*b));b->allocator=a;b->allocator_context=w->config.allocator_context;b->grid=w->config.grid;b->port=w->config.port;
    b->registration=(struct pt_mixed_causal_registration){b,NULL,c->session,c->grid.generation};b->task_busy=1;
    b->original_config=c;b->construction_workspace=workspace;b->construction_capacity=capacity;b->construction_output=out;b->construction=w;
    memset(&qconfig,0,sizeof(qconfig));qconfig.allocator=(struct pt_allocator){b,queue_allocate,queue_release};qconfig.allocator_context=(struct pt_mixed_readers_span){b,sizeof(*b)};
    qconfig.grid=b->grid;qconfig.session=b->registration.session;qconfig.control_budget=w->config.queue_budget;
    qconfig.backend=(struct pt_mixed_readers_backend){b,sizeof(*b),PT_MIXED_READERS_VERSION,PT_MIXED_READERS_REQUIRED,backend_clock,backend_submit,backend_command,backend_reader};
    result=pt_mixed_readers_open(&qconfig,(uint8_t *)workspace+queue_offset(),capacity-queue_offset(),&queue);
    b->queue=queue;b->registration.queue=queue;
    if(result!=PT_MIXED_READERS_OK||w->failed||b->failed||memcmp(c,&w->config,sizeof(*c))){
        int alias=b->construction_alias,bad=w->failed||b->failed||memcmp(c,&w->config,sizeof(*c));
        if(queue){int consumed=pt_mixed_readers_close(&queue);(void)consumed;b->queue=queue;}
        if(queue){
            /* Refused cleanup is not permission to free a borrowed backend.
             * Publish a retained recovery owner; callers see BACKEND with this
             * explicit live handle and must preserve all contexts. */
            b->failed=1;b->causal.suppressed=1;b->original_config=NULL;b->construction_workspace=NULL;b->construction_capacity=0;
            b->construction_output=NULL;b->construction=NULL;b->task_busy=0;*out=b;
            memset(workspace,0,capacity);return PT_MIXED_READERS_BACKEND;
        }
        /* No port has seen this owner: constructor never publishes or reads a
         * clock. No source callback reference can have been acquired here. */
        a.release(a.context,b);memset(w,0,sizeof(*w));return bad?PT_MIXED_READERS_BACKEND:alias?PT_MIXED_READERS_INVALID:result;
    }
    b->original_config=NULL;b->construction_workspace=NULL;b->construction_capacity=0;b->construction_output=NULL;b->construction=NULL;b->task_busy=0;
    *out=b;memset(workspace,0,capacity);return PT_MIXED_READERS_OK;
}
enum pt_mixed_readers_result pt_mixed_causal_borrow_queue(struct pt_mixed_causal_owner *b,struct pt_mixed_readers_output **out)
{
    if(!b||!out||!output_apart(b,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue||b->failed){b->task_busy=0;return PT_MIXED_READERS_BACKEND;}
    *out=b->queue;b->task_busy=0;return PT_MIXED_READERS_OK;
}
static int input_guard(const struct pt_mixed_causal_owner *b,const struct pt_mixed_readers_inputs *input)
{
    unsigned i,j;
    if(!input||!source_apart(b,input,sizeof(*input))||input->batch.count>PT_MIXED_READERS_ACTIONS||
       !source_apart(b,input->command.context,input->command.context_bytes))return 0;
    for(i=0;i<input->batch.count;++i)if(input->batch.action[i].kind==PT_MIXED_READERS_TRIGGER){
        const struct pt_mixed_readers_owner *r=input->reader+i;
        if(!source_apart(b,r->control.context,r->control.context_bytes)||!r->count||r->count>PT_MIXED_READERS_SPANS||
           !source_apart(b,r->spans,r->count*sizeof(*r->spans)))return 0;
        for(j=0;j<r->count;++j)if(!source_apart(b,r->spans[j].data,r->spans[j].bytes))return 0;
    }
    return 1;
}
static int trigger_input_only(const struct pt_mixed_readers_inputs *input)
{
    unsigned i;struct pt_mixed_readers_action zero;
    if(!input||!input->batch.count||input->batch.count>16)return 0;
    memset(&zero,0,sizeof(zero));
    for(i=0;i<16;++i){
        if(!key_zero(input->target+i))return 0;
        if(i<input->batch.count){if(input->batch.action[i].kind!=PT_MIXED_READERS_TRIGGER)return 0;}
        else if(memcmp(input->batch.action+i,&zero,sizeof(zero)))return 0;
    }
    return 1;
}
static int causal_basis(struct pt_mixed_causal_owner *b,struct activation_command *first,
    struct pt_mixed_causal_queue_basis *basis,struct pt_mixed_causal_actual *predicted)
{
    unsigned i,index;struct activation_reader *r;
    if(!first||first->packet.ticket!=b->causal.first||first->command!=PT_MIXED_COMMAND_WAITING||
       first->unknown||first->publication_outcome!=1||!first->port_refs||!current_packet(b,&first->packet))return 0;
    memset(basis,0,sizeof(*basis));memset(predicted,0,sizeof(*predicted));
    basis->backend_owner=b;basis->event=first->identity.event;basis->command_binding=first->identity.binding;
    basis->session=b->registration.session;basis->generation=b->registration.generation;
    basis->ticket=first->packet.ticket;basis->command_owner=first->identity.owner;
    basis->frame=first->packet.frame;basis->first=first->packet.first;basis->last=first->packet.last;
    basis->count=first->packet.count;basis->expected_mask=first->packet.expected_mask;
    memcpy(basis->expected,first->packet.expected,sizeof(basis->expected));
    predicted->active_mask=first->packet.expected_mask;
    memcpy(predicted->slot,first->packet.expected,sizeof(predicted->slot));
    /* adopted_mask deliberately remains zero: these are copied intentions,
     * not actual ACTIVE/adoption state or a caller-facing receipt. */
    for(i=0;i<20;++i){
        if(!(b->mask&(1U<<i))){if(!key_zero(b->slot+i))return 0;continue;}
        r=find_reader(b,b->slot+i);
        if(!r||!r->adopted||r->cancelled||r->state!=PT_MIXED_READER_ACTIVE)return 0;
    }
    for(i=0;i<first->packet.count;++i){
        if(first->packet.action[i].kind!=PT_MIXED_READERS_TRIGGER||
           !slot_index(first->packet.action[i].route,first->packet.action[i].slot,&index))return 0;
        r=find_reader(b,first->packet.key+i);if(!r||r->cancelled||r->adopted||r->closed)return 0;
        basis->key[i]=r->identity.key;memcpy(basis->action+i,first->packet.action+i,sizeof(basis->action[i]));
        basis->reference[i]=r->identity.reference;basis->binding[i]=r->identity.binding;
        predicted->slot[index]=r->identity.key;predicted->active_mask|=1U<<index;
    }
    return 1;
}
enum pt_mixed_readers_result pt_mixed_causal_enqueue(struct pt_mixed_causal_owner *b,const struct pt_mixed_readers_inputs *input,uint64_t *out)
{
    enum pt_mixed_readers_result r;
    if(!b||!out||!output_apart(b,out,sizeof(*out))||!input_guard(b,input)||!trigger_input_only(input)||b->causal.first||
       !b->queue||!pt_mixed_readers_admission_valid(b->queue,input,out))return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue||b->failed){b->task_busy=0;return PT_MIXED_READERS_BACKEND;}
    r=pt_mixed_readers_enqueue(b->queue,input,out);b->task_busy=0;
    /* Actual core admission is the transfer authority even if a holder caused
     * an owner-only fault. Publish its ticket; never reinterpret an admitted
     * command as an ownership-preserving refusal. Later publish fails closed. */
    if(r==PT_MIXED_READERS_OK){b->causal.first=*out;return r;}
    return b->failed?PT_MIXED_READERS_BACKEND:r;
}
enum pt_mixed_readers_result pt_mixed_causal_publish(struct pt_mixed_causal_owner *b,uint64_t ticket)
{
    enum pt_mixed_readers_result r;if(!b)return PT_MIXED_READERS_INVALID;
    if(!ticket||ticket!=b->causal.first)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue){b->task_busy=0;return PT_MIXED_READERS_INVALID;}
    r=pt_mixed_readers_publish(b->queue,ticket);b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:r;
}
enum pt_mixed_readers_result pt_mixed_causal_control_bind(struct pt_mixed_causal_owner *b,
    const struct pt_mixed_causal_control_port *port)
{
    unsigned i;
    if(!b||!port||!output_apart(b,port,sizeof(*port))||
       port->context!=b->port.context||port->context_bytes!=b->port.context_bytes||
       port->version!=PT_MIXED_CAUSAL_CONTROL_VERSION||port->flags!=PT_MIXED_CAUSAL_CONTROL_REQUIRED||
       !port->publish_control||b->control_bound||b->stop_bound||!b->queue||b->causal.first||b->ordered||b->mask||
       b->failed||b->source_closed||b->source_attempted||b->source_uncertain||
       pt_mixed_readers_commands_held(b->queue)||pt_mixed_readers_readers_held(b->queue))return PT_MIXED_READERS_INVALID;
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held)return PT_MIXED_READERS_INVALID;
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    b->control_port=*port;b->control_bound=1;b->task_busy=0;return PT_MIXED_READERS_OK;
}
static int control_request_valid(const struct pt_mixed_causal_control_request *r)
{
    unsigned i,index,mask=0;
    if(!r->predecessor||!r->count||r->count>PT_MIXED_READERS_ACTIONS||!r->command.context||
       !r->command.context_bytes||!r->command.token||!r->command.current||!r->command.terminal||!r->command.release)return 0;
    for(i=0;i<PT_MIXED_READERS_ACTIONS;++i){const struct pt_mixed_causal_control_action *a=r->action+i;
        if(i>=r->count){if(a->route||a->slot||a->period||a->volume||a->rate||a->left||a->right)return 0;continue;}
        if(!slot_index(a->route,a->slot,&index)||(mask&(1U<<index)))return 0;
        mask|=1U<<index;
        if(a->route==PT_MIXED_READERS_PAULA){if(!a->period||a->volume>64||a->rate||a->left||a->right)return 0;}
        else if(!a->rate||a->rate>0x40000000UL||a->period||a->volume)return 0;
    }
    return 1;
}
enum pt_mixed_readers_result pt_mixed_causal_control_enqueue(struct pt_mixed_causal_owner *b,
    const struct pt_mixed_causal_control_request *request,uint64_t *out)
{
    struct pt_mixed_causal_control_request saved;
    struct pt_mixed_readers_inputs input;
    struct pt_mixed_readers_key key;
    enum pt_mixed_readers_result result;unsigned i,index;
    /* Guard the complete original and transformed capacities before ANY owner
     * write, copying or callback. In particular, a local lower-level input must
     * never hide caller output aliasing the original typed request/holder. */
    if(!b||!request||!out||!output_apart(b,request,sizeof(*request))||!output_apart(b,out,sizeof(*out))||
       !output_apart(b,request->command.context,request->command.context_bytes)||
       !apart(request,sizeof(*request),out,sizeof(*out))||
       !apart(request,sizeof(*request),request->command.context,request->command.context_bytes)||
       !apart(out,sizeof(*out),request->command.context,request->command.context_bytes)||
       !output_address_apart(b,(uintptr_t)&saved,sizeof(saved))||
       !output_address_apart(b,(uintptr_t)&input,sizeof(input))||
       !output_address_apart(b,(uintptr_t)&key,sizeof(key))||
       !apart(&saved,sizeof(saved),request,sizeof(*request))||!apart(&input,sizeof(input),request,sizeof(*request))||
       !apart(&key,sizeof(key),request,sizeof(*request))||!apart(&saved,sizeof(saved),out,sizeof(*out))||
       !apart(&input,sizeof(input),out,sizeof(*out))||!apart(&key,sizeof(key),out,sizeof(*out))||
       !apart(&saved,sizeof(saved),request->command.context,request->command.context_bytes)||
       !apart(&input,sizeof(input),request->command.context,request->command.context_bytes)||
       !apart(&key,sizeof(key),request->command.context,request->command.context_bytes)||
       !control_request_valid(request)||!b->queue||!control_first_current(b)||b->causal.admitted||b->causal.published||
       request->predecessor!=b->causal.first||request->frame<=b->last_frame)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    memcpy(&saved,request,sizeof(saved));memset(&input,0,sizeof(input));
    input.batch.generation=b->grid.generation;input.batch.frame=saved.frame;input.batch.count=saved.count;input.command=saved.command;
    for(i=0;i<saved.count;++i){const struct pt_mixed_causal_control_action *a=saved.action+i;
        struct pt_mixed_readers_action *d=input.batch.action+i;
        if(!slot_index(a->route,a->slot,&index)||!(b->mask&(1U<<index))){b->task_busy=0;return PT_MIXED_READERS_STALE;}
        key=b->causal.completed_post.slot[index];
        result=pt_mixed_readers_reader_key(b->queue,key.trigger,key.action,input.target+i);
        if(memcmp(request,&saved,sizeof(saved)))fault(b);
        if(result!=PT_MIXED_READERS_OK||!key_equal(&key,input.target+i)||!control_first_current(b)){
            b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:result==PT_MIXED_READERS_OK?PT_MIXED_READERS_STALE:result;
        }
        d->route=a->route;d->slot=a->slot;d->kind=PT_MIXED_READERS_CONTROL;
        if(a->route==PT_MIXED_READERS_PAULA){d->geometry.paula.period=a->period;d->geometry.paula.volume=a->volume;}
        else{d->geometry.amigus.rate=a->rate;d->geometry.amigus.left=a->left;d->geometry.amigus.right=a->right;}
    }
    /* Genuine enqueue alone transfers ownership. Use the ACTUAL caller slot. */
    result=pt_mixed_readers_enqueue(b->queue,&input,out);
    if(memcmp(request,&saved,sizeof(saved)))fault(b);
    if(result==PT_MIXED_READERS_OK){b->causal.successor=*out;b->causal.serial=1;b->causal.admitted=1;}
    b->task_busy=0;
    return result==PT_MIXED_READERS_OK?result:b->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_mixed_causal_control_publish(struct pt_mixed_causal_owner *b,uint64_t ticket)
{
    enum pt_mixed_readers_result result;
    if(!b||!b->queue||!control_first_current(b)||!b->causal.admitted||b->causal.published||
       !ticket||ticket!=b->causal.successor)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    b->causal.publishing=ticket;result=pt_mixed_readers_publish(b->queue,ticket);b->causal.publishing=0;
    b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_mixed_causal_stop_bind(struct pt_mixed_causal_owner *b,
    const struct pt_mixed_causal_stop_port *port)
{
    unsigned i;
    if(!b||!port||!output_apart(b,port,sizeof(*port))||
       port->context!=b->port.context||port->context_bytes!=b->port.context_bytes||
       port->version!=PT_MIXED_CAUSAL_STOP_VERSION||port->flags!=PT_MIXED_CAUSAL_STOP_REQUIRED||
       !port->publish_stop||b->control_bound||b->stop_bound||!b->queue||b->causal.first||b->ordered||b->mask||
       b->failed||b->source_closed||b->source_attempted||b->source_uncertain||
       pt_mixed_readers_commands_held(b->queue)||pt_mixed_readers_readers_held(b->queue))return PT_MIXED_READERS_INVALID;
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held)return PT_MIXED_READERS_INVALID;
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    b->stop_port=*port;b->stop_bound=1;b->task_busy=0;return PT_MIXED_READERS_OK;
}
static int stop_request_valid(const struct pt_mixed_causal_stop_request *r)
{
    unsigned i,index,mask=0;
    if(!r->predecessor||!r->count||r->count>PT_MIXED_READERS_ACTIONS||!r->command.context||
       !r->command.context_bytes||!r->command.token||!r->command.current||!r->command.terminal||!r->command.release)return 0;
    for(i=0;i<PT_MIXED_READERS_ACTIONS;++i){const struct pt_mixed_causal_stop_action *a=r->action+i;
        if(i>=r->count){if(a->route||a->slot)return 0;continue;}
        if(!slot_index(a->route,a->slot,&index)||(mask&(1U<<index)))return 0;
        mask|=1U<<index;
    }
    return 1;
}
enum pt_mixed_readers_result pt_mixed_causal_stop_enqueue(struct pt_mixed_causal_owner *b,
    const struct pt_mixed_causal_stop_request *request,uint64_t *out)
{
    struct pt_mixed_causal_stop_request saved;
    struct pt_mixed_readers_inputs input;
    struct pt_mixed_readers_key key;
    enum pt_mixed_readers_result result;unsigned i,index;
    /* Guard the complete original and transformed capacities before ANY owner
     * write, copying or callback. In particular, a local lower-level input must
     * never hide caller output aliasing the original typed request/holder. */
    if(!b||!request||!out||!output_apart(b,request,sizeof(*request))||!output_apart(b,out,sizeof(*out))||
       !output_apart(b,request->command.context,request->command.context_bytes)||
       !apart(request,sizeof(*request),out,sizeof(*out))||
       !apart(request,sizeof(*request),request->command.context,request->command.context_bytes)||
       !apart(out,sizeof(*out),request->command.context,request->command.context_bytes)||
       !output_address_apart(b,(uintptr_t)&saved,sizeof(saved))||
       !output_address_apart(b,(uintptr_t)&input,sizeof(input))||
       !output_address_apart(b,(uintptr_t)&key,sizeof(key))||
       !apart(&saved,sizeof(saved),request,sizeof(*request))||!apart(&input,sizeof(input),request,sizeof(*request))||
       !apart(&key,sizeof(key),request,sizeof(*request))||!apart(&saved,sizeof(saved),out,sizeof(*out))||
       !apart(&input,sizeof(input),out,sizeof(*out))||!apart(&key,sizeof(key),out,sizeof(*out))||
       !apart(&saved,sizeof(saved),request->command.context,request->command.context_bytes)||
       !apart(&input,sizeof(input),request->command.context,request->command.context_bytes)||
       !apart(&key,sizeof(key),request->command.context,request->command.context_bytes)||
       !stop_request_valid(request)||!b->queue||!stop_first_current(b)||b->causal.admitted||b->causal.published||
       request->predecessor!=b->causal.first||request->frame<=b->last_frame)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    memcpy(&saved,request,sizeof(saved));memset(&input,0,sizeof(input));
    input.batch.generation=b->grid.generation;input.batch.frame=saved.frame;input.batch.count=saved.count;input.command=saved.command;
    for(i=0;i<saved.count;++i){const struct pt_mixed_causal_stop_action *a=saved.action+i;
        struct pt_mixed_readers_action *d=input.batch.action+i;
        if(!slot_index(a->route,a->slot,&index)||!(b->mask&(1U<<index))){b->task_busy=0;return PT_MIXED_READERS_STALE;}
        key=b->causal.completed_post.slot[index];
        result=pt_mixed_readers_reader_key(b->queue,key.trigger,key.action,input.target+i);
        if(memcmp(request,&saved,sizeof(saved)))fault(b);
        if(result!=PT_MIXED_READERS_OK||!key_equal(&key,input.target+i)||!stop_first_current(b)){
            b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:result==PT_MIXED_READERS_OK?PT_MIXED_READERS_STALE:result;
        }
        d->route=a->route;d->slot=a->slot;d->kind=PT_MIXED_READERS_STOP;
        /* Entire transformed geometry and every new reader owner stay zero. */
    }
    /* Genuine enqueue alone transfers ownership. Use the ACTUAL caller slot. */
    result=pt_mixed_readers_enqueue(b->queue,&input,out);
    if(memcmp(request,&saved,sizeof(saved)))fault(b);
    if(result==PT_MIXED_READERS_OK){b->causal.successor=*out;b->causal.serial=1;b->causal.admitted=1;}
    b->task_busy=0;
    return result==PT_MIXED_READERS_OK?result:b->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_mixed_causal_stop_publish(struct pt_mixed_causal_owner *b,uint64_t ticket)
{
    enum pt_mixed_readers_result result;
    if(!b||!b->queue||!stop_first_current(b)||!b->causal.admitted||b->causal.published||
       !ticket||ticket!=b->causal.successor)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    b->causal.publishing=ticket;result=pt_mixed_readers_publish(b->queue,ticket);b->causal.publishing=0;
    b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_mixed_causal_enqueue_successor(
    struct pt_mixed_causal_owner *b,uint64_t predecessor,const struct pt_mixed_readers_inputs *input,uint64_t *out)
{
    struct activation_command *first;struct pt_mixed_causal_queue_basis basis;
    struct pt_mixed_causal_actual predicted;enum pt_mixed_readers_result result;
    if(!b||b->control_bound||b->stop_bound||!out||!output_apart(b,out,sizeof(*out))||!input_guard(b,input)||!trigger_input_only(input)||
       !b->queue||!predecessor||predecessor!=b->causal.first||b->causal.admitted||b->causal.suppressed||
       !pt_mixed_readers_admission_valid(b->queue,input,out))return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    first=find_command(b,predecessor);
    if(b->failed||!causal_basis(b,first,&basis,&predicted)){b->task_busy=0;return PT_MIXED_READERS_INVALID;}
    result=pt_mixed_readers_causal_validate(b->queue,&basis,0);
    if(result!=PT_MIXED_READERS_OK||b->failed){b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:result;}
    result=pt_mixed_readers_enqueue(b->queue,input,out);
    if(result==PT_MIXED_READERS_OK){
        b->causal.successor=*out;b->causal.serial=1;b->causal.admitted=1;memcpy(&b->causal.predicted,&predicted,sizeof(predicted));
        memcpy(&b->causal.predecessor,&first->identity,sizeof(b->causal.predecessor));
        if(b->failed)b->causal.suppressed=1;
    }
    b->task_busy=0;
    /* The actual genuine enqueue is ownership authority, even on owner fault.
     * Do not turn its real transfer into an ownership-preserving refusal. */
    return result==PT_MIXED_READERS_OK?result:b->failed?PT_MIXED_READERS_BACKEND:result;
}
enum pt_mixed_readers_result pt_mixed_causal_publish_successor(
    struct pt_mixed_causal_owner *b,uint64_t predecessor,uint64_t successor)
{
    struct activation_command *first;struct pt_mixed_causal_queue_basis basis;
    struct pt_mixed_causal_actual predicted;enum pt_mixed_readers_result result;
    if(!b||b->control_bound||b->stop_bound||!b->queue||!b->causal.admitted||b->causal.published||b->causal.suppressed||
       predecessor!=b->causal.first||successor!=b->causal.successor)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    first=find_command(b,predecessor);
    if(b->failed||!causal_basis(b,first,&basis,&predicted)||
       memcmp(&first->identity,&b->causal.predecessor,sizeof(first->identity))||
       memcmp(&predicted,&b->causal.predicted,sizeof(predicted))){b->task_busy=0;return PT_MIXED_READERS_INVALID;}
    result=pt_mixed_readers_causal_validate(b->queue,&basis,successor);
    if(result==PT_MIXED_READERS_OK&&!b->failed){
        b->causal.publishing=successor;result=pt_mixed_readers_publish(b->queue,successor);b->causal.publishing=0;
    }
    b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:result;
}
static int completion_identity_same(const struct pt_mixed_causal_command_identity *a,
    const struct pt_mixed_causal_command_identity *b)
{
    return a->registration.owner==b->registration.owner&&a->registration.queue==b->registration.queue&&
        a->registration.session==b->registration.session&&a->registration.generation==b->registration.generation&&
        a->ticket==b->ticket&&a->owner==b->owner&&a->event==b->event&&
        a->binding.context==b->binding.context&&a->binding.context_bytes==b->binding.context_bytes;
}
int pt_mixed_causal_first_completion_matches(struct pt_mixed_causal_owner *b,
    struct pt_mixed_causal_first_completion_match copied)
{
    const struct causal_pair *p;struct activation_command *successor;unsigned i;int matched;
    if(!b||b->control_bound||b->stop_bound)return 0;
    if(b->task_busy){fault(b);return 0;}
    if(!enter(b))return 0;
    p=&b->causal;successor=find_command(b,p->successor);
    matched=!b->failed&&!b->source_closed&&!b->source_attempted&&!b->source_uncertain&&
        copied.predecessor.registration.owner==b->registration.owner&&
        copied.predecessor.registration.queue==b->registration.queue&&
        copied.predecessor.registration.session==b->registration.session&&
        copied.predecessor.registration.generation==b->registration.generation&&
        completion_identity_same(&copied.predecessor,&p->predecessor)&&
        copied.predecessor.ticket==p->first&&copied.successor.ticket==p->successor&&
        copied.serial&&copied.serial==p->serial&&p->admitted&&p->published&&
        p->commit_called&&p->commit_outcome==1&&
        copied.first_tick==p->completed_first_tick&&copied.last_tick==p->completed_last_tick&&
        copied.first_tick<copied.last_tick&&copied.observed==p->observed&&copied.issued==p->issued&&
        copied.observed>=copied.first_tick&&copied.issued>=copied.observed&&copied.issued<copied.last_tick&&
        causal_current(b)&&successor&&successor->command==PT_MIXED_COMMAND_WAITING&&
        !successor->unknown&&successor->publication_outcome==1&&successor->port_refs&&
        completion_identity_same(&copied.successor,&successor->identity)&&current_packet(b,&successor->packet)&&
        copied.post.active_mask==b->mask&&copied.post.active_mask==p->completed_post.active_mask&&
        copied.post.adopted_mask==copied.post.active_mask&&copied.post.adopted_mask==p->completed_post.adopted_mask;
    for(i=0;matched&&i<PT_MIXED_CAUSAL_SLOTS;++i)
        if(!key_equal(copied.post.slot+i,b->slot+i)||!key_equal(copied.post.slot+i,p->completed_post.slot+i))matched=0;
    b->busy=0;return matched?1:0;
}
int pt_mixed_causal_diagnostic(struct pt_mixed_causal_owner *b,struct pt_mixed_causal_diagnostic *out)
{
    struct pt_mixed_causal_diagnostic value;
    if(!b||!out||!output_apart(b,out,sizeof(*out))||!task_enter(b))return 0;
    memset(&value,0,sizeof(value));value.first=b->causal.first;value.successor=b->causal.successor;value.serial=b->causal.serial;
    value.observed=b->causal.observed;value.issued=b->causal.issued;value.admitted=b->causal.admitted;
    value.published=b->causal.published;value.completed=b->causal.completed;value.suppressed=b->causal.suppressed;
    value.commit_called=b->causal.commit_called;value.commit_outcome=b->causal.commit_outcome;
    *out=value;b->task_busy=0;return 1;
}
enum pt_mixed_readers_result pt_mixed_causal_service_command(struct pt_mixed_causal_owner *b,uint64_t ticket,unsigned cancel,struct pt_mixed_readers_command_receipt *out)
{
    enum pt_mixed_readers_result r;
    if(!b||cancel>1||(out&&!output_apart(b,out,sizeof(*out))))return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue){b->task_busy=0;return PT_MIXED_READERS_INVALID;}
    r=pt_mixed_readers_service_command(b->queue,ticket,cancel,out);b->task_busy=0;return r;
}
enum pt_mixed_readers_result pt_mixed_causal_service_reader(struct pt_mixed_causal_owner *b,uint64_t ticket,unsigned action,unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{
    enum pt_mixed_readers_result r;
    if(!b||cancel>1||(out&&!output_apart(b,out,sizeof(*out))))return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue){b->task_busy=0;return PT_MIXED_READERS_INVALID;}
    r=pt_mixed_readers_service_reader(b->queue,ticket,action,cancel,out);b->task_busy=0;return r;
}
enum pt_mixed_readers_result pt_mixed_causal_reader_key(struct pt_mixed_causal_owner *b,uint64_t ticket,unsigned action,struct pt_mixed_readers_key *out)
{
    enum pt_mixed_readers_result r;
    if(!b||!out||!output_apart(b,out,sizeof(*out)))return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue||b->failed){b->task_busy=0;return PT_MIXED_READERS_BACKEND;}
    r=pt_mixed_readers_reader_key(b->queue,ticket,action,out);b->task_busy=0;return r;
}
enum pt_mixed_readers_result pt_mixed_causal_stop(struct pt_mixed_causal_owner *b)
{
    enum pt_mixed_readers_result r;if(!b)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue){b->task_busy=0;return PT_MIXED_READERS_INVALID;}
    r=pt_mixed_readers_stop(b->queue);b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:r;
}
int pt_mixed_causal_close(struct pt_mixed_causal_owner **out)
{
    struct pt_mixed_causal_owner *b;struct pt_mixed_readers_output *queue;struct pt_allocator allocator;
    struct pt_mixed_causal_registration registration;
    unsigned i,reentry=0,before;int result;
    if(!out||!extent(out,sizeof(*out)))return 0;
    b=*out;if(!b)return 1;
    if(!output_apart(b,out,sizeof(*out))||!task_enter(b))return 0;
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held){b->task_busy=0;return 0;}
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held){b->task_busy=0;return 0;}
    if(b->queue){
        queue=b->queue;result=pt_mixed_readers_close(&queue);
        if(!queue)b->queue=NULL; /* actual registration consumption, not counts */
        if(!result){b->task_busy=0;return 0;}
    }
    if(!b->source_closed){before=b->faults;memcpy(&registration,&b->registration,sizeof(registration));
        if(!b->source_attempted){b->source_attempted=1;result=b->port.source_close(b->port.context,&b->registration);}
        else result=b->port.source_quiet(b->port.context,&b->registration);
        if(memcmp(&registration,&b->registration,sizeof(registration))){memcpy(&b->registration,&registration,sizeof(registration));fault(b);}
        if(result!=1||b->faults!=before){if(result!=0||b->faults!=before)b->source_uncertain=1;b->task_busy=0;return 0;}
        b->source_closed=1;
    }
    allocator=b->allocator;b->release_latch=&reentry;*out=NULL;allocator.release(allocator.context,b);
    return !reentry&&!*out;
}

/* Additive private factory seam. Existing admission/fire/default paths above
 * are unchanged. Genuine caller owns the original live mutable owner. */
static int factory_empty(const struct pt_mixed_causal_owner *b,
    const struct pt_mixed_readers_output *queue,uint64_t session,uint64_t generation)
{
    unsigned i;
    if(b->failed||b->construction_alias||b->source_attempted||b->source_closed||
       b->queue!=queue||b->registration.owner!=b||b->registration.queue!=queue||
       b->registration.session!=session||b->registration.generation!=generation||
       b->causal.first||b->causal.successor||b->mask||
       pt_mixed_readers_commands_held(queue)||pt_mixed_readers_readers_held(queue))return 0;
    for(i=0;i<PT_MIXED_CAUSAL_COMMANDS;++i)if(b->command[i].held)return 0;
    for(i=0;i<PT_MIXED_CAUSAL_READERS;++i)if(b->reader[i].held)return 0;
    return 1;
}
int pt_mixed_causal_factory_original_empty(struct pt_mixed_causal_owner *b,
    struct pt_mixed_readers_output *queue,uint64_t session,uint64_t generation)
{
    int ok;
    if(!b||!queue||!session||!generation)return 0;
    if(!task_enter(b))return 0;
    ok=factory_empty(b,queue,session,generation);b->task_busy=0;return ok;
}
int pt_mixed_causal_factory_stop_original_empty(struct pt_mixed_causal_owner *b,
    struct pt_mixed_readers_output *queue,uint64_t session,uint64_t generation)
{
    int ok;
    if(!b||!queue||!session||!generation)return 0;
    if(!task_enter(b))return 0;
    ok=b->stop_bound&&!b->control_bound&&!b->ordered&&!b->source_uncertain&&
       b->stop_port.context==b->port.context&&b->stop_port.context_bytes==b->port.context_bytes&&
       b->stop_port.version==PT_MIXED_CAUSAL_STOP_VERSION&&
       b->stop_port.flags==PT_MIXED_CAUSAL_STOP_REQUIRED&&b->stop_port.publish_stop&&
       factory_empty(b,queue,session,generation);
    b->task_busy=0;return ok;
}
int pt_mixed_causal_factory_stop_first_current(struct pt_mixed_causal_owner *b,
    uint64_t predecessor)
{
    int ok;
    if(!b||!predecessor)return 0;
    if(!task_enter(b))return 0;
    ok=predecessor==b->causal.first&&stop_first_current(b)&&
       !b->causal.admitted&&!b->causal.published;
    b->task_busy=0;return ok;
}
struct pt_mixed_causal_factory_bind_result pt_mixed_causal_factory_bind_original(
    struct pt_mixed_causal_owner *b,
    int (*callback)(void *,const struct pt_mixed_causal_registration *),
    void *context,size_t context_bytes)
{
    struct pt_mixed_causal_factory_bind_result out={0,0,0};
    struct pt_mixed_causal_registration registration={0};unsigned before;
    if(!b||!callback||context!=b->port.context||context_bytes!=b->port.context_bytes||
       !output_apart(b,&registration,sizeof(registration)))return out;
    if(!task_enter(b))return out;
    if(!b->queue||!factory_empty(b,b->queue,b->registration.session,b->registration.generation)){
       b->task_busy=0;return out;}
    registration=b->registration;before=b->faults;
    out.raw=callback(context,&registration);out.called=1;
    if(registration.owner!=b->registration.owner||registration.queue!=b->registration.queue||
       registration.session!=b->registration.session||registration.generation!=b->registration.generation)
       fault(b);
    out.current=before==b->faults&&!b->failed;
    b->task_busy=0;return out;
}
enum pt_mixed_readers_result pt_mixed_causal_factory_reader_readiness(
    struct pt_mixed_causal_owner *b,uint64_t ticket,unsigned action)
{
    enum pt_mixed_readers_result r;
    if(!b||!ticket||action>=PT_MIXED_READERS_ACTIONS)return PT_MIXED_READERS_INVALID;
    if(!task_enter(b))return PT_MIXED_READERS_BACKEND;
    if(!b->queue||b->failed){b->task_busy=0;return PT_MIXED_READERS_BACKEND;}
    r=pt_mixed_readers_reader_readiness(b->queue,ticket,action);
    b->task_busy=0;return b->failed?PT_MIXED_READERS_BACKEND:r;
}
struct pt_mixed_reader_retirement pt_mixed_causal_factory_retire_original(
    struct pt_mixed_causal_owner *b,uint64_t ticket,unsigned action,unsigned cancel)
{
    struct pt_mixed_reader_retirement r={PT_MIXED_READERS_INVALID,0};
    if(!b||!ticket||action>=PT_MIXED_READERS_ACTIONS||cancel>1)return r;
    if(!task_enter(b)){r.result=PT_MIXED_READERS_BACKEND;return r;}
    if(!b->queue){b->task_busy=0;return r;}
    r=pt_mixed_readers_retire_original(b->queue,ticket,action,cancel);
    b->task_busy=0;return r;
}
