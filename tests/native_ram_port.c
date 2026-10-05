/* Test-only RAM port; no application binding or native/device qualification.
 * Ordinary RAM adoption is the only effect. Generated memcpy/memset/64-bit
 * helpers, indirect calls, IRQ ABI, residency, WCET and total stack UNKNOWN.
 */
#include "native_ram_port.h"
#include <string.h>

static int key_same(const struct pt_readers_key *a,const struct pt_readers_key *b)
{
    return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&
        a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&
        a->action==b->action&&a->slot==b->slot;
}
static int action_same(const struct pt_scheduled_action *a,const struct pt_scheduled_action *b)
{return a->kind==b->kind&&a->slot==b->slot&&a->data==b->data&&a->words==b->words&&a->period==b->period&&a->volume==b->volume;}
static int packet_same(const struct pt_readers_activation_packet *a,const struct pt_readers_activation_packet *b)
{
    unsigned i;
    if(a->session!=b->session||a->generation!=b->generation||a->ticket!=b->ticket||a->frame!=b->frame||
       a->first!=b->first||a->last!=b->last||a->count!=b->count||a->expected_mask!=b->expected_mask)return 0;
    for(i=0;i<4;++i)if(!key_same(a->expected+i,b->expected+i))return 0;
    for(i=0;i<a->count;++i)if(!key_same(a->key+i,b->key+i)||!action_same(a->action+i,b->action+i))return 0;
    return 1;
}
static int expected_same(const struct pt_private_ram_port *p,const struct pt_readers_activation_packet *b)
{
    unsigned i;
    if(b->session!=p->session||b->generation!=p->generation||b->expected_mask!=p->mask)return 0;
    for(i=0;i<4;++i)if(!key_same(b->expected+i,p->slot+i))return 0;
    return 1;
}
static struct pt_private_ram_command *command_find(struct pt_private_ram_port *p,uint64_t ticket)
{unsigned i;for(i=0;i<2;++i)if(p->command[i].live&&p->command[i].packet.ticket==ticket)return p->command+i;return NULL;}
static struct pt_private_ram_reader *reader_find(struct pt_private_ram_port *p,const struct pt_readers_key *key)
{unsigned i;for(i=0;i<8;++i)if(p->reader[i].live&&key_same(&p->reader[i].key,key))return p->reader+i;return NULL;}
static void data_drop(struct pt_private_ram_command *c)
{unsigned i;for(i=0;i<c->packet.count;++i){c->packet.action[i].data=NULL;c->packet.action[i].words=0;}}
static int actual_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_private_ram_port *p=context;uint64_t t;uint32_t f;
    if(p->dispatching&&p->trace_count>=p->maximum_reads)return 0;
    if(p->adapter.clock(p->adapter.context,&t,&f)!=1)return 0;
    if(p->dispatching){
        p->trace[p->trace_count++]=(struct pt_private_ram_trace){t,f};
    }
    /* Preserve actual read bytes even when classification is invalid. */
    if(f!=p->frequency||(p->clock_seen&&t<p->last_clock))return 0;
    p->last_clock=t;p->clock_seen=1;*ticks=t;*frequency=f;return 1;
}
static int publish(void *context,struct pt_readers_activation *ledger,const struct pt_readers_activation_packet *b)
{
    struct pt_private_ram_port *p=context;struct pt_private_ram_command *c;unsigned i;int armed;
    uint64_t now;uint32_t frequency;
    ++p->publishes;
    if(p->source_closed||p->dispatching||p->armed_index!=-1||b->count<1||b->count>4||!expected_same(p,b)||
       b->first>=b->last||b->first<p->early_ticks)return 0;
    if(!actual_clock(p,&now,&frequency)||now>=b->first-p->early_ticks)return 0;
    for(i=0;i<2;++i)if(!p->command[i].live)break;if(i==2)return 0;
    c=p->command+i;c->packet=*b;c->ledger=ledger;c->arm_tick=b->first-p->early_ticks;
    c->live=1;c->armed=1;p->armed_index=(int)i;
    /* External exclusion covers this complete task-side publication and arm.
     * Adapter must expose only this fixed port to the IRQ, not packet b. */
    armed=p->adapter.arm_at(p->adapter.context,c->arm_tick,frequency);
    if(armed==1)return 1;
    if(armed==0){p->armed_index=-1;memset(c,0,sizeof(*c));return 0;}
    c->uncertain=1;return -1;
}
static int commit(void *context,const struct pt_readers_activation_packet *b,struct pt_readers_activation_actual *out)
{
    struct pt_private_ram_port *p=context;struct pt_private_ram_command *c=command_find(p,b->ticket);
    unsigned i,j,used=0,reserved=0,placement[4]={0};
    ++p->commits;
    if(!p->dispatching||p->source_closed||!c||!c->armed||c->finished||c->uncertain||b->count<1||b->count>4||
       !packet_same(&c->packet,b)||!expected_same(p,b)||!p->clock_seen||p->last_clock<b->first||p->last_clock>=b->last)return 0;
    /* All four untouched/addressed actual keys and every action are checked
     * before the first effect. No descriptors, callbacks or owner traversal. */
    for(i=0;i<b->count;++i){const struct pt_scheduled_action *a=b->action+i;const struct pt_readers_key *k=b->key+i;
        if(a->slot>=4||(used&(1U<<a->slot))||k->slot!=a->slot||!k->queue||k->session!=p->session||
           k->generation!=p->generation||!k->trigger||!k->owner||!k->serial)return 0;
        used|=1U<<a->slot;
        if(a->kind==PT_SCHEDULED_TRIGGER){
            if(!a->data||((uintptr_t)a->data&1U)||!a->words||!a->period||a->volume>64||reader_find(p,k))return 0;
            for(j=0;j<8;++j)if(!p->reader[j].live&&!(reserved&(1U<<j)))break;
            if(j==8)return 0;placement[i]=j;reserved|=1U<<j;
        }else{
            struct pt_private_ram_reader *r=reader_find(p,k);
            if(!(p->mask&(1U<<a->slot))||!key_same(p->slot+a->slot,k)||!r||!r->active||a->data||a->words)return 0;
            if(a->kind==PT_SCHEDULED_CONTROL){if(!a->period||a->volume>64)return 0;}
            else if(a->kind!=PT_SCHEDULED_STOP||a->period||a->volume)return 0;
        }
    }
    for(i=0;i<b->count;++i){const struct pt_scheduled_action *a=b->action+i;unsigned slot=a->slot;
        struct pt_private_ram_reader *old=(p->mask&(1U<<slot))?reader_find(p,p->slot+slot):NULL;
        if(a->kind==PT_SCHEDULED_TRIGGER){struct pt_private_ram_reader *r=p->reader+placement[i];
            if(old)old->active=0;
            *r=(struct pt_private_ram_reader){b->key[i],a->data,(size_t)a->words*2U,a->period,a->volume,1,1};
            p->slot[slot]=b->key[i];p->mask|=1U<<slot;
        }else if(a->kind==PT_SCHEDULED_STOP){
            old->active=0;p->mask&=~(1U<<slot);memset(p->slot+slot,0,sizeof(p->slot[slot]));
        }else{old->period=a->period;old->volume=a->volume;}
        ++p->effects;
    }
    out->active_mask=p->mask;out->adopted_mask=p->mask;
    for(i=0;i<4;++i)out->slot[i]=p->slot[i];
    return 1; /* Actual persistent RAM pointer/key/geometry adoption above. */
}
static int command_quiet(void *context,uint64_t ticket,unsigned cancel)
{
    struct pt_private_ram_port *p=context;struct pt_private_ram_command *c=command_find(p,ticket);int quiet;
    if(p->dispatching)return -1;if(!c)return 1;
    if(!cancel&&!c->finished&&!p->source_closed)return 0;
    quiet=p->source_closed?1:p->adapter.ticket_quiet(p->adapter.context,ticket,cancel);
    if(quiet!=1)return quiet==0?0:-1;
    if(c->armed)p->armed_index=-1;
    /* Drops every copied packet data pointer AND callback ledger/ticket
     * identity. Independent adopted RAM readers remain untouched. */
    memset(c,0,sizeof(*c));return 1;
}
static int reader_quiet(void *context,const struct pt_readers_key *key,unsigned cancel)
{
    struct pt_private_ram_port *p=context;struct pt_private_ram_reader *r=reader_find(p,key);unsigned i,j;
    if(p->dispatching)return -1;
    if(!cancel&&r&&r->active)return 0;
    for(i=0;i<2;++i){struct pt_private_ram_command *c=p->command+i;unsigned names=0;int quiet;
        if(!c->live)continue;for(j=0;j<c->packet.count;++j)if(key_same(c->packet.key+j,key))names=1;
        if(!names)continue;
        if(c->armed&&!c->finished){
            if(!cancel&&!p->source_closed)return 0;
            quiet=p->source_closed?1:p->adapter.ticket_quiet(p->adapter.context,c->packet.ticket,1);
            if(quiet!=1)return quiet==0?0:-1;
            c->armed=0;c->finished=1;p->armed_index=-1;
        }
        /* Whole pending batch is disabled before pointers are removed. Value
         * identities may remain for command detachment; source addresses may not. */
        data_drop(c);
    }
    if(r){unsigned slot=r->key.slot;
        if((p->mask&(1U<<slot))&&key_same(p->slot+slot,key)){
            p->mask&=~(1U<<slot);memset(p->slot+slot,0,sizeof(p->slot[slot]));
        }
        memset(r,0,sizeof(*r));
    }
    return 1;
}
int pt_private_ram_init(struct pt_private_ram_port *p,uint64_t session,uint64_t generation,uint32_t frequency,
    const struct pt_private_ram_adapter *a,unsigned early,unsigned residency,unsigned reads)
{
    if(!p||!a||!session||!generation||(frequency!=709379&&frequency!=715909)||!a->clock||!a->arm_at||
       !a->ticket_quiet||!a->source_close||early<1||early>4096||residency<early||residency>4096||reads<4||reads>64)return 0;
    /* Private entry must prove p/a/opaque contexts/output storage disjoint
     * before this call; this is not a public allocator/alias-safe constructor. */
    memset(p,0,sizeof(*p));p->adapter=*a;p->session=session;p->generation=generation;p->frequency=frequency;
    p->early_ticks=early;p->residency_ticks=residency;p->maximum_reads=reads;p->armed_index=-1;return 1;
}
struct pt_readers_activation_port pt_private_ram_api(struct pt_private_ram_port *p)
{return (struct pt_readers_activation_port){p,sizeof(*p),PT_READERS_ACTIVATION_PORT_VERSION,PT_READERS_ACTIVATION_PORT_REQUIRED,
    actual_clock,publish,commit,command_quiet,reader_quiet};}
int pt_private_ram_dispatch(struct pt_private_ram_port *p)
{
    struct pt_private_ram_command *c;uint64_t entry,now;uint32_t frequency;int result=PT_READERS_ACTIVATION_FAILED;
    if(!p||p->dispatching||p->source_closed||p->armed_index<0||p->armed_index>=2)return result;
    c=p->command+p->armed_index;if(!c->live||!c->armed||c->finished||c->uncertain)return result;
    p->dispatching=1;p->trace_count=0;++p->entries;
    p->ledger_result=PT_READERS_ACTIVATION_INVALID; /* not called yet */
    if(!actual_clock(p,&entry,&frequency)||entry<c->arm_tick)goto ended;
    now=entry;
    /* Keep two read slots for genuine ledger pre/post clock observations.
     * No original frame/epoch/window is changed, even on late entry/failure. */
    while(now<c->packet.first){
        if(p->trace_count+2U>=p->maximum_reads||now>=c->packet.last||now-entry>p->residency_ticks||
           !expected_same(p,&c->packet)||!actual_clock(p,&now,&frequency))goto ended;
    }
    if(now>=c->packet.last||now-entry>p->residency_ticks||!expected_same(p,&c->packet))goto ended;
    ++p->fires;result=pt_readers_activation_fire(c->ledger,c->packet.ticket);
    p->ledger_result=result;
    /* The final ledger read already exists in the fixed actual trace. Its
     * source residency budget is an additional strict diagnostic, not proof
     * of the unmeasured trampoline/library/higher-IRQ overhead. The raw ledger
     * result remains separate: this port diagnostic cannot retroactively undo
     * a genuine RAM adoption already committed inside its original window. */
    if(p->last_clock-entry>p->residency_ticks)result=PT_READERS_ACTIVATION_FAILED;
ended:
    c->armed=0;c->finished=1;p->armed_index=-1;data_drop(c);
    p->dispatch_result=result;p->dispatching=0;return result;
}
int pt_private_ram_source_close(struct pt_private_ram_port *p)
{
    unsigned i;int closed;if(!p||p->dispatching)return -1;if(p->source_closed)return 1;
    closed=p->adapter.source_close(p->adapter.context);if(closed!=1)return closed==0?0:-1;
    p->source_closed=1;p->armed_index=-1;
    for(i=0;i<2;++i)if(p->command[i].live){p->command[i].armed=0;p->command[i].finished=1;data_drop(p->command+i);}
    return 1;
}
