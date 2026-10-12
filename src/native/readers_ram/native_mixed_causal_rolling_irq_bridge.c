#include "native_mixed_causal_rolling_irq_bridge.h"
#include <string.h>
#include <limits.h>

/* Literal production inhibit: even a null, aliased or stale is_Data is never
 * interpreted. No macro, build flag or HOST constructor enables this entry.
 * An enabled native derivative requires separate reviewed Source and all actual
 * ABI/callgraph/storage/exclusion/aperture admissions. */
int pt_private_native_rolling_irq_dispatch(struct pt_private_native_rolling_irq32 *envelope)
{(void)envelope;return PT_MIXED_CAUSAL_INVALID;}

#ifdef PT_PRIVATE_ROLLING_IRQ_HOST_MODEL
struct b_align {char c;struct pt_private_rolling_irq_bridge value;};
struct e_align {char c;struct pt_private_native_rolling_irq32 value;};
struct o_align {char c;struct pt_private_rolling_irq_ops value;};
struct t_align {char c;struct pt_private_rolling_irq_tags value;};
struct p_align {char c;struct pt_private_mixed_causal_ram_port value;};
struct r_align {char c;struct pt_mixed_causal_registration value;};
static int span(const void *p,size_t n,size_t align)
{uintptr_t a=(uintptr_t)p;return a&&n&&align&&!(a%align)&&n<=UINTPTR_MAX-a;}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x+n<=y||y+m<=x;}
static int zero(const void *p,size_t n)
{const unsigned char *q=p;size_t i;for(i=0;i<n;++i)if(q[i])return 0;return 1;}
static int registration_same(const struct pt_mixed_causal_registration *a,
 const struct pt_mixed_causal_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static void fault(struct pt_private_rolling_irq_bridge *b)
{b->failed=1;if(b->faults!=UINT_MAX)++b->faults;
 if(b->staged&&b->staged<=4&&b->current<b->staged)b->original[b->current].uncertain=1;}
/* One HOST-only callback at a time under whole entry exclusion. This anchor
 * points to a fixed automatic witness and is never exposed in borrowed Source.
 * Callback rewrites of bridge counters/flags cannot erase actual reentry. No
 * allocation, nested witness list or native readiness is introduced. */
struct callback_witness {struct pt_private_rolling_irq_bridge *bridge;volatile unsigned faults;};
static struct callback_witness *active_callback;
static int callback_reentry(struct pt_private_rolling_irq_bridge *b)
{
    if(!active_callback)return 0;
    if(active_callback->faults!=UINT_MAX)++active_callback->faults;
    /* Numeric equality with the admitted saved address precedes any mutable
     * control access. Other or invalid caller addresses are never interpreted. */
    if(b==active_callback->bridge)fault(b);
    return 1;
}
static int admitted(struct pt_private_rolling_irq_bridge *b)
{return span(b,sizeof(*b),offsetof(struct b_align,value))&&b->self==b&&b->initialized==1&&
 b->host_model==1&&b->native_inhibited==1&&b->staged<=4&&
 (b->staged?b->current<b->staged:b->current==0)&&b->envelope&&b->ops.exclude&&b->ops.restore&&b->ops.clock;}
static void retain_snapshot(struct pt_private_rolling_irq_bridge *b,
 const struct pt_private_rolling_irq_bridge *saved,unsigned observed)
{
    /* Restore only bridge-owned callback custody, never RAM/core/history,
     * primitive hardware, actual quiet or health. Actual failure stays sticky. */
    memcpy(b,saved,sizeof(*b));
    b->faults=observed>UINT_MAX-b->faults?UINT_MAX:b->faults+observed;fault(b);
}
static uint64_t words(uint32_t hi,uint32_t lo)
{return ((uint64_t)hi<<32)|lo;}
static void envelope_set(struct pt_private_rolling_irq_bridge *b)
{
    struct pt_private_native_rolling_irq32 *e=b->envelope;
    const struct pt_private_rolling_irq_record *r=b->original+b->current;
    memset(e,0,sizeof(*e));e->version=PT_PRIVATE_NATIVE_ROLLING_IRQ32_VERSION;e->bytes=sizeof(*e);
    e->bridge_addr=b->tags.bridge;e->self_addr=b->tags.self;e->owner_addr=b->tags.owner;e->queue_addr=b->tags.queue;
    e->session=(uint32_t)b->registration.session;e->generation=(uint32_t)b->registration.generation;
    e->ticket_hi=(uint32_t)(r->identity.ticket>>32);e->ticket_lo=(uint32_t)r->identity.ticket;
    e->first_hi=(uint32_t)(r->first>>32);e->first_lo=(uint32_t)r->first;
    e->last_hi=(uint32_t)(r->last>>32);e->last_lo=(uint32_t)r->last;
    e->frequency=r->frequency;e->frame=(uint32_t)r->frame;e->serial=(uint32_t)r->serial;
    e->result=PT_MIXED_CAUSAL_INVALID;e->state=PT_PRIVATE_ROLLING_IRQ_ARMED;
}
static int envelope_current(struct pt_private_rolling_irq_bridge *b,
 const struct pt_private_native_rolling_irq32 *e)
{
    const struct pt_private_rolling_irq_record *r;
    if(!b->staged||b->current>=4||e!=b->envelope)return 0;
    r=b->original+b->current;
    return r->known&&r->armed&&!r->completed&&!r->uncertain&&
      e->version==PT_PRIVATE_NATIVE_ROLLING_IRQ32_VERSION&&e->bytes==sizeof(*e)&&
      e->bridge_addr==b->tags.bridge&&e->self_addr==b->tags.self&&
      e->owner_addr==b->tags.owner&&e->queue_addr==b->tags.queue&&
      e->session==b->registration.session&&e->generation==b->registration.generation&&
      words(e->ticket_hi,e->ticket_lo)==r->identity.ticket&&
      words(e->first_hi,e->first_lo)==r->first&&words(e->last_hi,e->last_lo)==r->last&&
      e->frequency==r->frequency&&e->frame==r->frame&&e->serial==r->serial&&
      (e->state==PT_PRIVATE_ROLLING_IRQ_ARMED||e->state==PT_PRIVATE_ROLLING_IRQ_EARLY);
}
static int row_current(struct pt_private_rolling_irq_bridge *b,
 const struct pt_private_rolling_irq_record *r)
{
    const struct pt_private_mixed_causal_ram_port *p=b->port;
    const struct pt_private_mixed_causal_ram_command *c;
    if(!p||r->row>=2||p->self!=p||!p->initialized||!p->bound||p->failed||p->suppressed||
       p->rolling_mode!=1||p->control_mode!=1||p->lineage_mode||p->source_attempted||
       !registration_same(&p->registration,&b->registration))return 0;
    c=p->command+r->row;
    return c->known&&c->live&&c->armed&&!c->finished&&!c->disabled&&!c->uncertain&&
      c->rolling_arm_state==PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED&&c->owner==b->registration.owner&&
      !memcmp(&c->identity,&r->identity,sizeof(c->identity))&&c->packet.ticket==r->identity.ticket&&
      registration_same(&c->packet.registration,&b->registration)&&c->packet.frame==r->frame&&
      c->packet.first==r->first&&c->packet.last==r->last&&c->packet.count==16&&c->serial==r->serial;
}
int pt_private_rolling_irq_host_init(struct pt_private_rolling_irq_bridge *b,
 struct pt_private_native_rolling_irq32 *e,const struct pt_private_rolling_irq_ops *o,
 const struct pt_private_rolling_irq_tags *t)
{
    if(callback_reentry(b))return 0;
    if(!span(b,sizeof(*b),offsetof(struct b_align,value))||!span(e,sizeof(*e),offsetof(struct e_align,value))||
       !span(o,sizeof(*o),offsetof(struct o_align,value))||!span(t,sizeof(*t),offsetof(struct t_align,value))||
       !apart(b,sizeof(*b),e,sizeof(*e))||!apart(b,sizeof(*b),o,sizeof(*o))||
       !apart(b,sizeof(*b),t,sizeof(*t))||!apart(e,sizeof(*e),o,sizeof(*o))||!apart(e,sizeof(*e),t,sizeof(*t))||
       !apart(o,sizeof(*o),t,sizeof(*t)))return 0;
    if(!span(o->context,o->context_bytes,1)||!o->exclude||!o->restore||!o->clock||
       !apart(b,sizeof(*b),o->context,o->context_bytes)||!apart(e,sizeof(*e),o->context,o->context_bytes)||
       !apart(o,sizeof(*o),o->context,o->context_bytes)||!apart(t,sizeof(*t),o->context,o->context_bytes)||
       !t->bridge||!t->self||!t->owner||!t->queue||t->bridge==t->self||t->owner==t->queue||
       t->bridge==t->owner||t->bridge==t->queue||t->self==t->owner||t->self==t->queue||
       !zero(b,sizeof(*b))||!zero(e,sizeof(*e)))return 0;
    b->self=b;b->envelope=e;b->ops=*o;b->tags=*t;b->initialized=b->host_model=b->native_inhibited=1;return 1;
}
int pt_private_rolling_irq_host_bind(struct pt_private_rolling_irq_bridge *b,
 struct pt_private_mixed_causal_ram_port *p,const struct pt_mixed_causal_registration *r)
{
    if(callback_reentry(b))return 0;
    if(!admitted(b)||!span(p,sizeof(*p),offsetof(struct p_align,value))||
       !span(r,sizeof(*r),offsetof(struct r_align,value))||!apart(b,sizeof(*b),p,sizeof(*p))||
       !apart(b,sizeof(*b),r,sizeof(*r))||!apart(b->envelope,sizeof(*b->envelope),p,sizeof(*p))||
       !apart(b->envelope,sizeof(*b->envelope),r,sizeof(*r))||!apart(p,sizeof(*p),r,sizeof(*r))||
       !apart(b->ops.context,b->ops.context_bytes,p,sizeof(*p))||
       !apart(b->ops.context,b->ops.context_bytes,r,sizeof(*r)))return 0;
    if(b->bound||b->failed||b->busy||b->ops_busy||b->scope!=PT_PRIVATE_ROLLING_IRQ_TASK||
       !r->owner||!r->queue||!r->session||!r->generation||r->session>UINT32_MAX||r->generation>UINT32_MAX)return 0;
    /* Numeric control extents only; no consumed/opaque control is traversed. */
    if(!span(r->owner,pt_mixed_causal_control_size(),1)||!span(r->queue,pt_mixed_readers_control_size(),1)||
       !apart(r->owner,pt_mixed_causal_control_size(),r->queue,pt_mixed_readers_control_size())||
       !apart(b,sizeof(*b),r->owner,pt_mixed_causal_control_size())||
       !apart(b,sizeof(*b),r->queue,pt_mixed_readers_control_size())||
       !apart(b->envelope,sizeof(*b->envelope),r->owner,pt_mixed_causal_control_size())||
       !apart(b->envelope,sizeof(*b->envelope),r->queue,pt_mixed_readers_control_size())||
       !apart(b->ops.context,b->ops.context_bytes,r->owner,pt_mixed_causal_control_size())||
       !apart(b->ops.context,b->ops.context_bytes,r->queue,pt_mixed_readers_control_size())||
       !apart(p,sizeof(*p),r->owner,pt_mixed_causal_control_size())||
       !apart(p,sizeof(*p),r->queue,pt_mixed_readers_control_size())||
       !apart(r,sizeof(*r),r->owner,pt_mixed_causal_control_size())||
       !apart(r,sizeof(*r),r->queue,pt_mixed_readers_control_size()))return 0;
    if(p->self!=p||!p->initialized||!p->bound||p->rolling_mode!=1||p->control_mode!=1||p->lineage_mode||
       p->failed||p->source_attempted||p->pair_used||p->adapter.context!=b||p->adapter.context_bytes!=sizeof(*b)||
       !registration_same(r,&p->registration))return 0;
    b->port=p;b->registration=*r;b->bound=1;return 1;
}
static int enter_scope(struct pt_private_rolling_irq_bridge *b,unsigned scope)
{
    struct pt_private_rolling_irq_bridge saved;struct pt_private_rolling_irq_ops ops;
    struct pt_private_native_rolling_irq32 image;struct callback_witness witness;uint64_t token;
    if(callback_reentry(b))return -1;
    if(!admitted(b))return -1;
    if(b->busy||b->ops_busy||b->scope||b->dispatching){fault(b);return -1;}
    b->busy=b->ops_busy=1;memcpy(&saved,b,sizeof(saved));ops=b->ops;memcpy(&image,b->envelope,sizeof(image));
    witness.bridge=b;witness.faults=0;active_callback=&witness;
    token=ops.exclude(ops.context,scope);active_callback=NULL;
    if(witness.faults||memcmp(b,&saved,sizeof(saved))||memcmp(b->envelope,&image,sizeof(image))){
        retain_snapshot(b,&saved,witness.faults);b->busy=b->ops_busy=0;
        if(token){++b->restores;ops.restore(ops.context,scope,token);}return -1;}
    b->busy=b->ops_busy=0;if(!token)return 0;
    b->exclusion_token=token;b->scope=scope;return 1;
}
static int leave_scope(struct pt_private_rolling_irq_bridge *b,unsigned scope)
{
    struct pt_private_rolling_irq_ops ops;uint64_t token;int result;
    if(callback_reentry(b))return -1;
    if(!admitted(b)||b->busy||b->ops_busy||b->dispatching||b->scope!=scope||!b->exclusion_token)return -1;
    ops=b->ops;token=b->exclusion_token;result=b->failed?-1:1;
    b->scope=0;b->exclusion_token=0;++b->restores;
    /* Deliberately no post-restore snapshot or mutation rollback: legitimate
     * deferred IRQ delivery may now change completion/bridge state. */
    ops.restore(ops.context,scope,token);return result;
}
int pt_private_rolling_irq_task_enter(struct pt_private_rolling_irq_bridge *b)
{return enter_scope(b,PT_PRIVATE_ROLLING_IRQ_TASK);}
int pt_private_rolling_irq_task_leave(struct pt_private_rolling_irq_bridge *b)
{return leave_scope(b,PT_PRIVATE_ROLLING_IRQ_TASK);}
int pt_private_rolling_irq_stage(struct pt_private_rolling_irq_bridge *b,uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct pt_private_mixed_causal_ram_port *p;struct pt_private_mixed_causal_ram_command *c=NULL;
    struct pt_private_rolling_irq_record *r;unsigned i;
    if(callback_reentry(b))return 0;
    if(!admitted(b)||!b->bound||b->scope!=PT_PRIVATE_ROLLING_IRQ_TASK||b->busy||b->ops_busy||
       b->failed||b->staged>=4||(b->staged&&!b->original[b->current].completed))return 0;
    p=b->port;
    if(!p||!p->entry_busy||!p->adapter_busy||p->rolling_mode!=1||p->control_mode!=1||p->lineage_mode||
       !registration_same(&p->registration,&b->registration))return 0;
    for(i=0;i<2;++i)if(p->command[i].known&&p->command[i].identity.ticket==ticket){c=p->command+i;break;}
    if(!c||c->serial!=b->staged||c->packet.frame!=960U*(b->staged+1)||c->packet.first!=first||
       p->frequency!=frequency||!frequency||c->packet.first>=c->packet.last)return 0;
    r=b->original+b->staged;memset(r,0,sizeof(*r));memcpy(&r->identity,&c->identity,sizeof(r->identity));r->first=first;r->last=c->packet.last;
    r->frame=c->packet.frame;r->serial=c->serial;r->frequency=frequency;r->row=i;r->known=r->armed=1;
    r->arm_raw=r->dispatch_raw=r->ledger_raw=PT_MIXED_CAUSAL_INVALID;
    if(!row_current(b,r)){memset(r,0,sizeof(*r));return 0;}
    b->current=b->staged++;envelope_set(b);return 1;
}
int pt_private_rolling_irq_arm_result(struct pt_private_rolling_irq_bridge *b,uint64_t ticket,int raw)
{
    struct pt_private_rolling_irq_record *r;
    if(callback_reentry(b))return -1;
    if(!admitted(b)||!b->staged||b->scope!=PT_PRIVATE_ROLLING_IRQ_TASK||b->ops_busy)return -1;
    r=b->original+b->current;if(r->identity.ticket!=ticket)return -1;r->arm_raw=raw;
    if(raw==1&&!b->failed&&envelope_current(b,b->envelope)&&row_current(b,r))return 1;
    if(raw==0&&!b->failed&&envelope_current(b,b->envelope)&&row_current(b,r)){
        r->armed=0;return 0; /* Actual unchanged adapter absence, not inferred. */}
    fault(b);return -1;
}
int pt_private_rolling_irq_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_private_rolling_irq_bridge *b=context,saved;struct pt_private_rolling_irq_ops ops;
    struct pt_private_native_rolling_irq32 image;struct callback_witness witness;
    uint64_t now=0;uint32_t hz=0;int raw;
    if(callback_reentry(b))return 0;
    if(!admitted(b)||!ticks||!frequency||b->failed||
       (b->scope!=PT_PRIVATE_ROLLING_IRQ_TASK&&b->scope!=PT_PRIVATE_ROLLING_IRQ_IRQ)||b->ops_busy){
        if(admitted(b))fault(b);return 0;}
    b->ops_busy=1;memcpy(&saved,b,sizeof(saved));ops=b->ops;memcpy(&image,b->envelope,sizeof(image));
    witness.bridge=b;witness.faults=0;active_callback=&witness;
    raw=ops.clock(ops.context,&now,&hz);active_callback=NULL;
    if(witness.faults||memcmp(b,&saved,sizeof(saved))||memcmp(b->envelope,&image,sizeof(image))){
        retain_snapshot(b,&saved,witness.faults);b->ops_busy=0;return 0;}
    b->ops_busy=0;if(raw!=1||!hz){fault(b);return 0;}*ticks=now;*frequency=hz;return 1;
}
int pt_private_rolling_irq_host_entry(struct pt_private_rolling_irq_bridge *b,
 struct pt_private_native_rolling_irq32 *e)
{
    struct pt_private_rolling_irq_record *r;uint64_t ticket;int raw,result,entered;
    if(callback_reentry(b))return PT_MIXED_CAUSAL_FAILED;
    if(!admitted(b)||!span(e,sizeof(*e),offsetof(struct e_align,value))||e!=b->envelope||
       !b->bound||b->failed||!envelope_current(b,e))return PT_MIXED_CAUSAL_INVALID;
    r=b->original+b->current;if(!row_current(b,r))return PT_MIXED_CAUSAL_INVALID;
    entered=enter_scope(b,PT_PRIVATE_ROLLING_IRQ_IRQ);if(entered!=1)return PT_MIXED_CAUSAL_FAILED;
    b->dispatching=1;++b->irq_entries;++e->calls;ticket=r->identity.ticket;
    /* Actual pre/post clocks, original full20 registry, genuine fire/adoption
     * and copied histories all remain the existing RAM/core's authority. */
    raw=pt_private_mixed_causal_ram_dispatch(b->port,ticket);++b->dispatches;
    r->dispatch_raw=raw;r->ledger_raw=b->port->ledger_result;result=raw;
    if(b->failed||!envelope_current(b,e)){fault(b);result=PT_MIXED_CAUSAL_FAILED;}
    if(result==PT_MIXED_CAUSAL_COMMITTED){r->armed=0;r->completed=1;e->state=PT_PRIVATE_ROLLING_IRQ_COMPLETED;}
    else if(result==PT_MIXED_CAUSAL_EARLY)e->state=PT_PRIVATE_ROLLING_IRQ_EARLY;
    else{fault(b);e->state=PT_PRIVATE_ROLLING_IRQ_UNCERTAIN;}
    e->result=result;b->dispatching=0;
    if(leave_scope(b,PT_PRIVATE_ROLLING_IRQ_IRQ)!=1)result=PT_MIXED_CAUSAL_FAILED;
    return result;
}
#endif
