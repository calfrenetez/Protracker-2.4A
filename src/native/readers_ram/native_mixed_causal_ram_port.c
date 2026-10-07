/* Ordinary RAM adoption only. Numeric card facts never become CPU spans/MMIO.
 * Declarations and finite software work do not qualify a real native source. */
#include "native_mixed_causal_ram_port.h"
#include <string.h>

static int extent(const void *p,size_t n)
{return n&&p&&(uintptr_t)p<=UINTPTR_MAX-(n-1U);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return extent(p,n)&&extent(q,m)&&(a<=b?n<=b-a:m<=a-b);}
static int zero_bytes(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i){if(b[i])return 0;}return 1;}
static int registration_same(const struct pt_mixed_causal_registration *a,const struct pt_mixed_causal_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static int key_same(const struct pt_mixed_readers_key *a,const struct pt_mixed_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->route==b->route&&a->slot==b->slot;}
static int index_of(unsigned route,unsigned slot,unsigned *index)
{if(route==PT_MIXED_READERS_PAULA&&slot<4){*index=slot;return 1;}
 if(route==PT_MIXED_READERS_AMIGUS&&slot<16){*index=slot+4;return 1;}return 0;}
static int key_valid(const struct pt_private_mixed_causal_ram_port *p,const struct pt_mixed_readers_key *k)
{unsigned index;return k->queue==p->registration.queue&&k->session==p->session&&k->generation==p->generation&&k->trigger&&k->owner&&k->serial&&k->action<16&&index_of(k->route,k->slot,&index);}
static int card_same(const struct pt_mixed_readers_card *a,const struct pt_mixed_readers_card *b)
{return a->reservation==b->reservation&&a->cache==b->cache&&a->version==b->version&&a->serial==b->serial&&a->cache_slot==b->cache_slot&&a->bits==b->bits&&a->little_endian==b->little_endian&&a->source_channel==b->source_channel&&a->address==b->address&&a->logical_bytes==b->logical_bytes&&a->full_capacity==b->full_capacity;}
static int card_zero(const struct pt_mixed_readers_card *c)
{struct pt_mixed_readers_card zero;memset(&zero,0,sizeof(zero));return card_same(c,&zero);}
static int action_same(const struct pt_mixed_readers_action *a,const struct pt_mixed_readers_action *b)
{
    if(a->route!=b->route||a->slot!=b->slot||a->kind!=b->kind)return 0;
    if(a->route==PT_MIXED_READERS_PAULA)return a->geometry.paula.data==b->geometry.paula.data&&a->geometry.paula.words==b->geometry.paula.words&&a->geometry.paula.period==b->geometry.paula.period&&a->geometry.paula.volume==b->geometry.paula.volume;
    if(a->route==PT_MIXED_READERS_AMIGUS)return a->geometry.amigus.start==b->geometry.amigus.start&&a->geometry.amigus.loop==b->geometry.amigus.loop&&a->geometry.amigus.end_exclusive==b->geometry.amigus.end_exclusive&&a->geometry.amigus.rate==b->geometry.amigus.rate&&a->geometry.amigus.control==b->geometry.amigus.control&&a->geometry.amigus.left==b->geometry.amigus.left&&a->geometry.amigus.right==b->geometry.amigus.right;
    return !memcmp(&a->geometry,&b->geometry,sizeof(a->geometry));
}
static int packet_same(const struct pt_mixed_causal_packet *a,const struct pt_mixed_causal_packet *b)
{
    unsigned i;
    if(!registration_same(&a->registration,&b->registration)||a->ticket!=b->ticket||a->frame!=b->frame||a->first!=b->first||a->last!=b->last||a->count!=b->count||a->expected_mask!=b->expected_mask)return 0;
    for(i=0;i<20;++i)if(!key_same(a->expected+i,b->expected+i))return 0;
    for(i=0;i<16;++i)if(!key_same(a->key+i,b->key+i)||!action_same(a->action+i,b->action+i)||!card_same(a->card+i,b->card+i))return 0;
    return !memcmp(a,b,sizeof(*a)); /* Complete immutable copied bytes, padding too. */
}
static int current(const struct pt_private_mixed_causal_ram_port *p)
{return p&&p->self==p&&p->initialized&&p->bound;}
static void fault(struct pt_private_mixed_causal_ram_port *p)
{unsigned i;p->failed=1;++p->faults;for(i=0;i<2;++i)if(p->command[i].live)p->command[i].uncertain=1;}
static int enter(struct pt_private_mixed_causal_ram_port *p)
{if(!current(p))return 0;if(p->entry_busy||p->adapter_busy){fault(p);return 0;}p->entry_busy=1;return 1;}
static int expected_same(const struct pt_private_mixed_causal_ram_port *p,const struct pt_mixed_causal_packet *b)
{unsigned i;if(!registration_same(&p->registration,&b->registration)||b->expected_mask!=p->mask||(b->expected_mask&~0xFFFFFU))return 0;
 for(i=0;i<20;++i){if(!key_same(p->slot+i,b->expected+i))return 0;}return 1;}
static struct pt_private_mixed_causal_ram_command *command_find(struct pt_private_mixed_causal_ram_port *p,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(p->command[i].known&&p->command[i].identity.ticket==ticket)return p->command+i;return NULL;}
static struct pt_private_mixed_causal_ram_reader *reader_find(struct pt_private_mixed_causal_ram_port *p,const struct pt_mixed_readers_key *key)
{unsigned i;for(i=0;i<32;++i)if(p->reader[i].live&&key_same(&p->reader[i].key,key))return p->reader+i;return NULL;}
static void geometry_drop(struct pt_private_mixed_causal_ram_command *c)
{unsigned i;for(i=0;i<16;++i){memset(&c->packet.action[i].geometry,0,sizeof(c->packet.action[i].geometry));memset(c->packet.card+i,0,sizeof(c->packet.card[i]));}}
static int clock_locked(struct pt_private_mixed_causal_ram_port *p,uint64_t *ticks,uint32_t *frequency)
{
    uint64_t t=0;uint32_t f=0;unsigned before=p->faults;int raw;
    if(p->adapter_busy||(p->dispatching&&p->trace_count>=p->maximum_reads)){if(p->adapter_busy)fault(p);return 0;}
    p->adapter_busy=1;raw=p->adapter.clock(p->adapter.context,&t,&f);p->adapter_busy=0;p->clock_outcome=raw;
    if(raw!=1||p->faults!=before)return 0;
    if(p->dispatching)p->trace[p->trace_count++]=(struct pt_private_mixed_causal_ram_trace){t,f};
    if(f!=p->frequency||(p->clock_seen&&t<p->last_clock))return 0;
    p->clock_seen=1;p->last_clock=t;*ticks=t;*frequency=f;return 1;
}
static int read_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_private_mixed_causal_ram_port *p=context;int result;
    if(!current(p)||p->source_attempted||p->source_closed||!ticks||!frequency)return 0;
    if(p->dispatching&&p->fire_running&&!p->adapter_busy)return clock_locked(p,ticks,frequency);
    if(!enter(p))return 0;
    result=clock_locked(p,ticks,frequency);p->entry_busy=0;return result;
}
static int ticket_quiet(struct pt_private_mixed_causal_ram_port *p,uint64_t ticket,unsigned cancel,int *outcome)
{
    struct pt_mixed_causal_registration identity;unsigned before=p->faults;int raw;
    memcpy(&identity,&p->registration,sizeof(identity));p->adapter_busy=1;
    raw=p->adapter.ticket_quiet(p->adapter.context,&identity,ticket,cancel);p->adapter_busy=0;*outcome=raw;
    if(!registration_same(&identity,&p->registration)){fault(p);return -1;}
    if(p->faults!=before)return -1;
    return raw==0?0:raw==1?1:-1;
}
static int card_valid(const struct pt_mixed_readers_card *c,const struct pt_amigus_voice_plan *a)
{
    uint32_t end;
    if(!c->reservation||!c->cache||!c->version||!c->serial||c->cache_slot>=32||(c->bits!=8&&c->bits!=16)||c->little_endian>1||c->source_channel>1||(c->bits==16&&(c->logical_bytes&1))||(c->address&3)||!c->logical_bytes||c->logical_bytes>c->full_capacity||(c->full_capacity&3)||c->address>=PT_AMIGUS_RAM_ADDRESS_SPACE||c->full_capacity>PT_AMIGUS_RAM_ADDRESS_SPACE-c->address)return 0;
    end=c->address+c->logical_bytes;
    return !(a->control&~0x800FU)&&(a->control&0x8000U)&&!!(a->control&1U)==(c->bits==16)&&!!(a->control&8U)==(c->bits==16&&c->little_endian)&&a->rate&&a->rate<=0x40000000UL&&!((a->start|a->loop|a->end_exclusive)&1U)&&a->start>=c->address&&a->start<a->end_exclusive&&a->end_exclusive<=end&&a->end_exclusive<PT_AMIGUS_RAM_ADDRESS_SPACE&&a->loop>=c->address&&a->loop<a->end_exclusive&&((a->control&2U)||a->loop==c->address);
}
static int identity_same(const struct pt_mixed_causal_command_identity *a,const struct pt_mixed_causal_command_identity *b)
{return registration_same(&a->registration,&b->registration)&&a->ticket==b->ticket&&a->owner==b->owner&&
 a->event==b->event&&a->binding.context==b->binding.context&&a->binding.context_bytes==b->binding.context_bytes&&
 !memcmp(a,b,sizeof(*a));}
static int identity_valid(const struct pt_private_mixed_causal_ram_port *p,const struct pt_mixed_causal_command_identity *i,uint64_t ticket)
{return registration_same(&p->registration,&i->registration)&&i->ticket==ticket&&ticket&&i->owner&&i->event&&
 i->binding.context&&i->binding.context_bytes;}
static int packet_valid(const struct pt_private_mixed_causal_ram_port *p,const struct pt_mixed_causal_packet *b)
{
    unsigned i,index,used=0;struct pt_mixed_readers_key key_zero;
    struct pt_mixed_readers_action action_zero;
    if(!registration_same(&p->registration,&b->registration)||!b->ticket||!b->count||b->count>16||b->frame==UINT64_MAX||b->first>=b->last||b->expected_mask&~0xFFFFFU)return 0;
    for(i=0;i<b->count;++i){const struct pt_mixed_readers_action *a=b->action+i;const struct pt_mixed_readers_key *k=b->key+i;
        if(a->kind!=PT_MIXED_READERS_TRIGGER||!index_of(a->route,a->slot,&index)||(used&(1U<<index))||
           !key_valid(p,k)||k->trigger!=b->ticket||k->action!=i||k->route!=a->route||k->slot!=a->slot)return 0;
        used|=1U<<index;
        if(a->route==PT_MIXED_READERS_PAULA){uintptr_t address=(uintptr_t)a->geometry.paula.data;size_t bytes=(size_t)a->geometry.paula.words*2U;
            if(!card_zero(b->card+i)||!address||(address&1U)||!bytes||address>UINTPTR_MAX-(bytes-1U)||!a->geometry.paula.period||a->geometry.paula.volume>64)return 0;
        }else if(!card_valid(b->card+i,&a->geometry.amigus))return 0;
    }
    memset(&key_zero,0,sizeof(key_zero));memset(&action_zero,0,sizeof(action_zero));
    for(i=b->count;i<16;++i)if(!key_same(b->key+i,&key_zero)||
        !action_same(b->action+i,&action_zero)||!zero_bytes(&b->action[i].geometry,sizeof(b->action[i].geometry))||
        !card_zero(b->card+i))return 0;
    return 1;
}
static int arm_locked(struct pt_private_mixed_causal_ram_port *p,struct pt_private_mixed_causal_ram_command *c,
    struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *b)
{
    struct pt_mixed_causal_registration copy;unsigned before=p->faults;int raw;
    memcpy(&c->packet,b,sizeof(*b));memcpy(&c->identity,identity,sizeof(*identity));c->owner=owner;
    c->arm_tick=b->first;c->known=c->live=c->armed=1;
    memcpy(&copy,&p->registration,sizeof(copy));p->adapter_busy=1;
    raw=p->adapter.arm_at(p->adapter.context,&copy,b->ticket,b->first,p->frequency);p->adapter_busy=0;c->arm_outcome=raw;
    if(!registration_same(&copy,&p->registration))fault(p);
    if(raw==0&&p->faults==before){memset(&c->packet,0,sizeof(c->packet));c->owner=NULL;
        c->live=c->armed=0;c->quiet_confirmed=1;return 0;}
    if(raw==1&&p->faults==before)return 1;
    c->uncertain=1;p->failed=p->suppressed=1;return -1;
}
static int publish(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_command_identity *identity,const struct pt_mixed_causal_packet *b)
{
    struct pt_private_mixed_causal_ram_port *p=context;uint64_t now;uint32_t frequency;int result=0;
    if(!p||!identity||!b||!enter(p))return -1;
    ++p->publishes;
    if(p->failed||p->source_attempted){result=-1;goto done;}
    if(p->pair_used||owner!=p->registration.owner||!identity_valid(p,identity,b->ticket)||
       !packet_valid(p,b)||!expected_same(p,b))goto done;
    if(!clock_locked(p,&now,&frequency)){result=-1;fault(p);goto done;}
    if(now>=b->first)goto done;
    p->pair_used=1;result=arm_locked(p,p->command,owner,identity,b);
 done:p->entry_busy=0;return result;
}
static int publish_successor(void *context,struct pt_mixed_causal_owner *owner,const struct pt_mixed_causal_publication *d)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct pt_private_mixed_causal_ram_command *first,*second;
    struct pt_mixed_readers_key predicted[20];unsigned mask,i,index;uint64_t now;uint32_t frequency;int result=0;
    if(!p||!d||!enter(p))return -1;
    ++p->publishes;
    if(p->failed||p->source_attempted){result=-1;goto done;}
    first=p->command;second=p->command+1;
    if(p->pair_used!=1||!first->known||!first->live||!first->armed||first->finished||first->disabled||first->uncertain||
       first->arm_outcome!=1||owner!=p->registration.owner||!d->serial||
       !identity_same(&d->predecessor,&first->identity)||!packet_same(&d->first,&first->packet)||
       !expected_same(p,&first->packet)||!packet_valid(p,&d->successor)||
       !identity_valid(p,&d->successor_identity,d->successor.ticket)||d->successor.ticket==first->identity.ticket||
       d->successor.frame<=first->packet.frame||d->successor.first<first->packet.last)goto done;
    mask=p->mask;memcpy(predicted,p->slot,sizeof(predicted));
    for(i=0;i<first->packet.count;++i){index_of(first->packet.action[i].route,first->packet.action[i].slot,&index);
        memcpy(predicted+index,first->packet.key+i,sizeof(predicted[index]));mask|=1U<<index;}
    if(d->successor.expected_mask!=mask)goto done;
    for(i=0;i<20;++i)if(!key_same(d->successor.expected+i,predicted+i))goto done;
    if(!clock_locked(p,&now,&frequency)){result=-1;fault(p);goto done;}
    if(now>=first->packet.first||now>=d->successor.first)goto done;
    p->pair_used=2;memcpy(&second->predecessor,&d->predecessor,sizeof(second->predecessor));
    second->successor=1;second->serial=d->serial;result=arm_locked(p,second,owner,&d->successor_identity,&d->successor);
 done:p->entry_busy=0;return result;
}
static int commit(void *context,const struct pt_mixed_causal_packet *b,struct pt_mixed_causal_actual *out)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct pt_private_mixed_causal_ram_command *c;
    unsigned i,j,index;uint32_t selected=0;unsigned placement[16]={0};
    if(!current(p)||!b||!out)return 0;
    ++p->commits;
    if(!p->dispatching||!p->fire_running||p->adapter_busy){if(p->entry_busy)fault(p);return 0;}
    c=command_find(p,b->ticket);
    if(p->failed||p->suppressed||p->source_attempted||!c||!c->live||!c->armed||c->finished||c->disabled||c->uncertain||
       !packet_valid(p,b)||!packet_same(&c->packet,b)||!expected_same(p,b)||!p->clock_seen||
       p->last_clock<b->first||p->last_clock>=b->last||(c->successor&&!c->predecessor_completed))return 0;
    /* Validate every scalar and reserve all32-reader placements before effects.
     * Card addresses remain numbers; no resource/master/editor traversal. */
    for(i=0;i<b->count;++i){if(reader_find(p,b->key+i))return 0;
        for(j=0;j<32;++j)if(!p->reader[j].live&&!(selected&(UINT32_C(1)<<j)))break;
        if(j==32)return 0;
        placement[i]=j;selected|=UINT32_C(1)<<j;}
    for(i=0;i<b->count;++i){const struct pt_mixed_readers_action *a=b->action+i;
        struct pt_private_mixed_causal_ram_reader *old,*r=p->reader+placement[i];
        index_of(a->route,a->slot,&index);old=reader_find(p,p->slot+index);if(old)old->active=0;
        memcpy(&r->key,b->key+i,sizeof(r->key));memcpy(&r->action,a,sizeof(r->action));memcpy(&r->card,b->card+i,sizeof(r->card));
        r->live=r->active=1;memcpy(p->slot+index,b->key+i,sizeof(p->slot[index]));p->mask|=1U<<index;++p->effects;}
    out->active_mask=out->adopted_mask=p->mask;memcpy(out->slot,p->slot,sizeof(p->slot));return 1;
}
static int command_quiet(void *context,const struct pt_mixed_causal_command_identity *identity,unsigned cancel)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct pt_private_mixed_causal_ram_command *c;int quiet,result=-1;
    if(!p||!identity||cancel>1||!enter(p))return -1;
    c=command_find(p,identity->ticket);
    if(!c||!identity_same(&c->identity,identity))goto done;
    if(!c->live&&c->quiet_confirmed){result=1;goto done;}
    if(!cancel&&!c->finished&&!c->disabled){result=0;goto done;}
    quiet=ticket_quiet(p,identity->ticket,cancel,&c->ticket_outcome);
    if(quiet!=1){if(quiet<0){c->uncertain=1;p->failed=p->suppressed=1;}result=quiet;goto done;}
    if(c==p->command&&!p->command[1].predecessor_completed)p->suppressed=1;
    memset(&c->packet,0,sizeof(c->packet));c->owner=NULL;c->live=c->armed=0;c->quiet_confirmed=1;result=1;
 done:p->entry_busy=0;return result;
}
static int names_reader(const struct pt_private_mixed_causal_ram_command *c,const struct pt_mixed_readers_key *key)
{unsigned i;for(i=0;i<20;++i)if(key_same(c->packet.expected+i,key))return 1;
 for(i=0;i<16;++i)if(key_same(c->packet.key+i,key))return 1;
 return 0;}
static int reader_quiet(void *context,const struct pt_mixed_causal_reader_identity *identity,unsigned cancel)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct pt_private_mixed_causal_ram_reader *r;unsigned i,index;int result=-1;
    if(!p||!identity||cancel>1||!enter(p))return -1;
    if(!registration_same(&p->registration,&identity->registration)||!key_valid(p,&identity->key))goto done;
    r=reader_find(p,&identity->key);if(!cancel&&r&&r->active){result=0;goto done;}
    for(i=0;i<2;++i){struct pt_private_mixed_causal_ram_command *c=p->command+i;int quiet;
        if(!c->live||!names_reader(c,&identity->key))continue;
        if(!cancel&&!c->finished&&!c->disabled){result=0;goto done;}
        quiet=ticket_quiet(p,c->identity.ticket,cancel,&c->ticket_outcome);
        if(quiet!=1){if(quiet<0){c->uncertain=1;p->failed=p->suppressed=1;}result=quiet;goto done;}
        c->armed=0;c->finished=c->disabled=1;geometry_drop(c);
        if(c==p->command&&!p->command[1].predecessor_completed)p->suppressed=1;
    }
    index_of(identity->key.route,identity->key.slot,&index);
    if((p->mask&(1U<<index))&&key_same(p->slot+index,&identity->key)){p->mask&=~(1U<<index);memset(p->slot+index,0,sizeof(p->slot[index]));}
    if(r)memset(r,0,sizeof(*r));
    result=1;
 done:p->entry_busy=0;return result;
}
static int source_probe(void *context,const struct pt_mixed_causal_registration *registration,unsigned shutdown)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct pt_mixed_causal_registration copy;unsigned i,before;int raw,result=-1;
    if(!p||!registration||!enter(p))return -1;
    if(!registration_same(&p->registration,registration))goto done;
    if(p->source_closed){result=1;goto done;}
    if(p->mask||!zero_bytes(p->slot,sizeof(p->slot)))goto done;
    for(i=0;i<2;++i)if(p->command[i].live||p->command[i].armed||p->command[i].owner)goto done;
    for(i=0;i<32;++i)if(p->reader[i].live)goto done;
    if(shutdown){if(p->source_attempted)goto done;p->source_attempted=1;++p->source_probes;}
    else{if(!p->source_attempted)goto done;++p->quiet_probes;}
    /* Genuine core invokes only after actual queue consumption; pointer identity
     * remains opaque even if the queue is already freed. No absence inference. */
    memcpy(&copy,&p->registration,sizeof(copy));before=p->faults;p->adapter_busy=1;
    raw=shutdown?p->adapter.source_close(p->adapter.context,&copy):p->adapter.source_quiet(p->adapter.context,&copy);
    p->adapter_busy=0;if(shutdown)p->source_outcome=raw;else p->quiet_outcome=raw;
    if(!registration_same(&copy,&p->registration))fault(p);
    if(p->faults!=before)goto done;
    if(raw==1){p->source_closed=1;result=1;}else{if(raw!=0)p->failed=1;result=raw==0?0:-1;}
 done:p->entry_busy=0;return result;
}
static int source_close(void *context,const struct pt_mixed_causal_registration *r){return source_probe(context,r,1);}
static int source_quiet(void *context,const struct pt_mixed_causal_registration *r){return source_probe(context,r,0);}
int pt_private_mixed_causal_ram_init(struct pt_private_mixed_causal_ram_port *p,uint64_t session,uint64_t generation,
 uint32_t frequency,const struct pt_private_mixed_causal_ram_adapter *a,unsigned residency,unsigned reads)
{
    if(!extent(p,sizeof(*p))||!extent(a,sizeof(*a))||!apart(p,sizeof(*p),a,sizeof(*a)))return 0;
    if(!extent(a->context,a->context_bytes)||!apart(p,sizeof(*p),a->context,a->context_bytes)||
       !apart(a,sizeof(*a),a->context,a->context_bytes)||!zero_bytes(p,sizeof(*p))||!session||!generation||
       (frequency!=709379U&&frequency!=715909U)||!a->clock||!a->arm_at||!a->ticket_quiet||!a->source_close||
       !a->source_quiet||!residency||residency>4096||reads<4||reads>64)return 0;
    p->self=p;p->initialized=1;p->adapter=*a;p->session=session;p->generation=generation;p->frequency=frequency;
    p->residency_ticks=residency;p->maximum_reads=reads;return 1;
}
int pt_private_mixed_causal_ram_bind(struct pt_private_mixed_causal_ram_port *p,struct pt_mixed_causal_owner *owner,
 struct pt_mixed_readers_output *queue,uint64_t session,uint64_t generation)
{
    if(!p||p->self!=p||!p->initialized)return 0;
    if(p->entry_busy||p->adapter_busy){fault(p);return 0;}
    if(p->bound||p->failed||p->source_attempted||!owner||!queue||session!=p->session||generation!=p->generation)return 0;
    p->registration=(struct pt_mixed_causal_registration){owner,queue,session,generation};p->bound=1;return 1;
}
struct pt_mixed_causal_port pt_private_mixed_causal_ram_api(struct pt_private_mixed_causal_ram_port *p)
{return (struct pt_mixed_causal_port){p,sizeof(*p),PT_MIXED_CAUSAL_PORT_VERSION,PT_MIXED_CAUSAL_PORT_REQUIRED,
 read_clock,publish,commit,command_quiet,reader_quiet,source_close,source_quiet,publish_successor};}
int pt_private_mixed_causal_ram_dispatch(struct pt_private_mixed_causal_ram_port *p,uint64_t ticket)
{
    struct pt_private_mixed_causal_ram_command *c,*second;struct pt_mixed_causal_diagnostic diagnostic;
    uint64_t entry;uint32_t frequency;unsigned i,before;int result=PT_MIXED_CAUSAL_INVALID,raw;
    if(!p||!enter(p))return result;
    c=command_find(p,ticket);if(!c||!c->live||!c->armed||c->finished||c->disabled||c->uncertain||p->source_attempted)goto done;
    if(p->failed||p->suppressed){result=PT_MIXED_CAUSAL_FAILED;goto done;}
    p->dispatching=1;p->trace_count=0;++p->entries;before=p->faults;p->ledger_result=PT_MIXED_CAUSAL_INVALID;
    if(!clock_locked(p,&entry,&frequency)){fault(p);result=PT_MIXED_CAUSAL_FAILED;goto ended;}
    ++p->fires;p->fire_running=1;raw=pt_mixed_causal_fire(c->owner,ticket);p->fire_running=0;
    p->ledger_result=c->fire_result=raw;result=raw;
    if(p->faults!=before||p->failed||p->last_clock<entry||p->last_clock-entry>p->residency_ticks)result=PT_MIXED_CAUSAL_FAILED;
    if(result==PT_MIXED_CAUSAL_EARLY)goto ended;
    if(result==PT_MIXED_CAUSAL_COMMITTED&&c==p->command){
        second=p->command+1;
        if(p->pair_used!=2||!second->live||!second->successor||second->uncertain||
           !identity_same(&second->predecessor,&c->identity)||!expected_same(p,&second->packet)||
           !pt_mixed_causal_diagnostic(c->owner,&diagnostic)||!diagnostic.completed||diagnostic.suppressed||
           !diagnostic.admitted||!diagnostic.published||!diagnostic.commit_called||diagnostic.commit_outcome!=1||
           diagnostic.first!=ticket||diagnostic.successor!=second->identity.ticket||diagnostic.serial!=second->serial||
           diagnostic.observed<c->packet.first||diagnostic.issued<diagnostic.observed||diagnostic.issued>=c->packet.last){
            result=PT_MIXED_CAUSAL_FAILED;
        }else{for(i=0;i<20;++i)if(!key_same(p->slot+i,second->packet.expected+i))break;
            if(i!=20)result=PT_MIXED_CAUSAL_FAILED;
            else{second->predecessor_completed=1;second->predecessor_observed=diagnostic.observed;
                second->predecessor_issued=diagnostic.issued;}}
    }
 ended:
    if(result!=PT_MIXED_CAUSAL_EARLY){c->finished=1;geometry_drop(c);
        if(result!=PT_MIXED_CAUSAL_COMMITTED){c->uncertain=1;p->failed=p->suppressed=1;}}
    p->dispatch_result=result;p->dispatching=0;
 done:p->entry_busy=0;return result;
}
