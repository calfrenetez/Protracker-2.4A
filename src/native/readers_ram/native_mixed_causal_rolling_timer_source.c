/* No native resource binding, assembly, timer MMIO or playback wiring. */
#include "native_mixed_causal_rolling_timer_source.h"
#include <string.h>
struct source_alignment {char byte;struct pt_private_mixed_causal_rolling_timer_source object;};
struct ops_alignment {char byte;struct pt_private_pair_timer_ops object;};
struct port_alignment {char byte;struct pt_private_mixed_causal_ram_port object;};
struct registration_alignment {char byte;struct pt_mixed_causal_registration object;};
static int aligned(const void *p,size_t n)
{return p&&n&&!((uintptr_t)p%n);}
static int extent(const void *p,size_t n)
{return p&&n&&(uintptr_t)p<=UINTPTR_MAX-(n-1U);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return extent(p,n)&&extent(q,m)&&(a<=b?n<=b-a:m<=a-b);}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int same(const struct pt_mixed_causal_registration *a,const struct pt_mixed_causal_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static int current(const struct pt_private_mixed_causal_rolling_timer_source *s)
{return extent(s,sizeof(*s))&&aligned(s,offsetof(struct source_alignment,object))&&s->self==s&&s->initialized;}
static void fault(struct pt_private_mixed_causal_rolling_timer_source *s)
{unsigned i;s->failed=1;++s->faults;for(i=0;i<PT_PRIVATE_ROLLING_TIMER_EVENTS;++i)if(s->event[i].original.owned)s->event[i].original.uncertain=1;}
static int task(const struct pt_private_mixed_causal_rolling_timer_source *s)
{return current(s)&&s->scope==PT_PRIVATE_PAIR_TIMER_TASK&&!s->ops_busy&&!s->irq_active;}
/* Ops callbacks cannot change this source-owned control. Recover only that
 * exact saved source custody, retaining sticky failure/uncertainty; never rewind
 * RAM/core/history/provider state or repeat an unknown external operation. */
static int call_guard(struct pt_private_mixed_causal_rolling_timer_source *s,
 const struct pt_private_mixed_causal_rolling_timer_source *saved)
{
    if(memcmp(s,saved,sizeof(*s))){unsigned faults=s->faults,invalid=s->baseline_invalid;
        if(memcmp(&s->before,&saved->before,sizeof(s->before)))invalid=1;
        memcpy(s,saved,sizeof(*s));s->ops_busy=0;
        if(faults>s->faults)s->faults=faults;
        s->baseline_invalid|=invalid;fault(s);return 0;}
    s->ops_busy=0;return 1;
}
static int observation(struct pt_private_mixed_causal_rolling_timer_source *s,struct pt_private_pair_timer_hw_state *h)
{
    struct pt_private_mixed_causal_rolling_timer_source saved;int raw;
    memset(h,0,sizeof(*h));s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
    raw=s->ops.state(s->ops.context,h);
    return call_guard(s,&saved)&&raw==1;
}
static int ours(const struct pt_private_mixed_causal_rolling_timer_source *s,const struct pt_private_pair_timer_hw_state *h)
{return h->source==s&&h->entry==pt_private_rolling_timer_irq&&h->cookie==s->cookie&&s->cookie&&h->vector_live==1&&!h->restore_pending&&
 h->available==s->before.available&&!memcmp(h->before_image,s->before.before_image,sizeof(h->before_image));}
static int conserved(const struct pt_private_pair_timer_hw_state *a,const struct pt_private_pair_timer_hw_state *b,unsigned sibling)
{return a->source==b->source&&a->entry==b->entry&&a->cookie==b->cookie&&a->available==b->available&&
 a->vector_live==b->vector_live&&a->callbacks_inflight==b->callbacks_inflight&&a->restore_pending==b->restore_pending&&
 !memcmp(a->before_image,b->before_image,sizeof(a->before_image))&&
 !memcmp(a->event+sibling,b->event+sibling,sizeof(a->event[sibling]));}
static int absent_event(const struct pt_private_pair_timer_hw_event *h)
{return !h->armed&&!h->pending&&!h->queued;}
static int expected_event(const struct pt_private_pair_timer_hw_event *h,const struct pt_private_pair_timer_event *e)
{return h->ticket==e->ticket&&h->first==e->first&&h->frequency==e->frequency;}
static int source_proof(struct pt_private_mixed_causal_rolling_timer_source *s);
static int leave(struct pt_private_mixed_causal_rolling_timer_source *s,unsigned scope);
static int enter(struct pt_private_mixed_causal_rolling_timer_source *s,unsigned scope)
{
    unsigned before;uint64_t token;struct pt_private_mixed_causal_rolling_timer_source saved;
    if(!current(s))return 0;
    if(s->scope||s->entry_busy||s->ops_busy||s->irq_active){fault(s);return -1;}
    s->entry_busy=1;before=s->faults;s->ops_busy=1;
    memcpy(&saved,s,sizeof(saved));token=s->ops.exclude(s->ops.context,scope);(void)call_guard(s,&saved);
    if(!token){s->exclusion_unknown=1;++s->exclusion_refusals;fault(s);s->entry_busy=0;return -1;}
    s->scope=scope;s->exclusion_token=token;s->entry_busy=0;
    if(s->faults!=before){++s->exclusion_refusals;fault(s);
        /* Keep the actual token until the trusted primitive restores its
         * exact prior state. Reentry never erases acquired exclusion. */
        (void)leave(s,scope);return -1;}
    return 1;
}
static int leave(struct pt_private_mixed_causal_rolling_timer_source *s,unsigned scope)
{
    uint64_t token;unsigned before;
    if(!current(s)||s->scope!=scope||s->ops_busy||s->dispatch_active)return 0;
    token=s->exclusion_token;before=s->faults;s->scope=0;s->exclusion_token=0;
    /* Restore may now deliver a genuinely pending IRQ after the entire task
     * entry ends. State is ready; this is not a callback-local unlock. */
    s->ops.restore_exclusion(s->ops.context,scope,token);
    ++s->exclusion_restores;
    return before==s->faults?1:-1;
}
int pt_private_rolling_timer_task_enter(struct pt_private_mixed_causal_rolling_timer_source *s)
{int r=enter(s,PT_PRIVATE_PAIR_TIMER_TASK);if(r==1)++s->task_entries;return r;}
int pt_private_rolling_timer_task_leave(struct pt_private_mixed_causal_rolling_timer_source *s)
{return leave(s,PT_PRIVATE_PAIR_TIMER_TASK);}
static int clock_callback(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=context,saved;int raw;
    uint64_t observed=0;uint32_t observed_frequency=0;
    if(!current(s)||!s->scope||s->ops_busy||!ticks||!frequency)return 0;
    s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
    raw=s->ops.clock(s->ops.context,&observed,&observed_frequency);
    if(!call_guard(s,&saved)||raw!=1)return 0;
    *ticks=observed;*frequency=observed_frequency;return 1; /* Actual observation. */
}
static int arm_callback(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=context,saved;
    struct pt_private_pair_timer_hw_state before,after;struct pt_private_pair_timer_event *e,*old;
    unsigned slot,j,index,previous;int raw,guarded;struct pt_private_mixed_causal_ram_command *row;
    if(!task(s)||!s->bound||!extent(r,sizeof(*r))||!aligned(r,offsetof(struct registration_alignment,object)))return -1;
    if(s->failed||!s->acquired||s->source_attempted)return -1;
    if(!same(r,&s->registration)||!ticket||s->arms_seen>=PT_PRIVATE_ROLLING_TIMER_EVENTS||
       s->port->rolling_mode!=1||s->port->control_mode!=1||s->port->lineage_mode||
       (frequency!=709379U&&frequency!=715909U))return 0;
    for(j=0;j<s->arms_seen;++j){e=&s->event[j].original;
        if(e->ticket==ticket||e->frequency!=frequency||e->first>=first)return 0;}
    index=s->arms_seen;slot=index&1U;row=s->port->command+slot;
    /* This source is entered only by the exact original rolling publication,
     * not an arbitrary public adapter call with a fabricated new ticket. */
    if(!s->port->entry_busy||!s->port->adapter_busy||!row->known||!row->live||!row->armed||
       row->quiet_confirmed||row->owner!=r->owner||row->identity.ticket!=ticket||
       !same(&row->identity.registration,r)||!same(&row->packet.registration,r)||
       row->packet.ticket!=ticket||row->packet.first!=first||row->packet.frame!=960U*(index+1)||
       row->packet.count!=16||row->serial!=index||row->rolling_arm_state!=PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED)return 0;
    previous=s->slot_event[slot];
    if(previous){
        if(previous>index){fault(s);return -1;}old=&s->event[previous-1].original;
        if(!old->known||old->owned||old->uncertain||!old->quiet||s->event[previous-1].slot!=slot)return 0;
    }
    /* The genuine rolling publication has already authenticated exact prior C
     * retirement/disposal. Before arm, install this original attempt irrevocably. */
    ++s->arms_seen;s->slot_event[slot]=index+1;s->event[index].slot=slot;e=&s->event[index].original;
    memcpy(&e->registration,r,sizeof(*r));e->ticket=ticket;e->first=first;e->frequency=frequency;
    e->known=e->owned=1;
    /* Capture original custody BEFORE even a provider state callback. A failed
     * pre-arm read still has an exact owned no-arm identity for cancellation;
     * it cannot erase an earlier attempted callback or be silently retried. */
    if(!observation(s,&before)||!ours(s,&before)||before.callbacks_inflight||!absent_event(before.event+slot)){
        fault(s);return -1;}
    s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
    raw=s->ops.arm(s->ops.context,s->cookie,slot,ticket,first,frequency);
    guarded=call_guard(s,&saved);e=&s->event[index].original;e->arm_outcome=raw;
    if(!guarded||!observation(s,&after)){fault(s);e->uncertain=1;return -1;}
    if(raw==0&&!memcmp(&before,&after,sizeof(before))){e->owned=0;e->quiet=1;return 0;}
    j=1U-slot;
    if(raw==1&&ours(s,&after)&&expected_event(after.event+slot,e)&&after.event[slot].armed&&
       !after.event[slot].pending&&!after.event[slot].queued&&conserved(&before,&after,j))return 1;
    e->uncertain=1;fault(s);return -1;
}
static int quiet_callback(void *context,const struct pt_mixed_causal_registration *r,uint64_t ticket,unsigned cancel)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=context,saved;
    struct pt_private_pair_timer_hw_state before,after;struct pt_private_pair_timer_event *e,*occupant;
    unsigned index,slot,j;int raw,guarded;
    if(!task(s)||!s->bound||!extent(r,sizeof(*r))||!aligned(r,offsetof(struct registration_alignment,object))||!same(r,&s->registration)||cancel>1)return -1;
    for(index=0;index<s->arms_seen;++index)if(s->event[index].original.ticket==ticket)break;
    if(index==s->arms_seen)return -1;
    e=&s->event[index].original;slot=s->event[index].slot;
    if(!e->known||!same(r,&e->registration)||slot>=2||s->dispatch_active||!observation(s,&before))return -1;
    if(s->source_closed)return e->quiet&&source_proof(s)==1?1:-1;
    if(!ours(s,&before)||before.callbacks_inflight){fault(s);return -1;}
    if(s->slot_event[slot]!=index+1){
        /* A retained positive old proof must never cancel/ack its successor. */
        j=s->slot_event[slot];
        if(!e->quiet||e->owned||e->uncertain||!j||j>s->arms_seen){fault(s);return -1;}
        occupant=&s->event[j-1].original;
        if(!absent_event(before.event+slot)&&!expected_event(before.event+slot,occupant)){fault(s);return -1;}
        return 1;
    }
    if(!absent_event(before.event+slot)&&!expected_event(before.event+slot,e)){fault(s);return -1;}
    if(!cancel&&!e->terminal&&!e->quiet)return 0;
    if(cancel&&!e->cancel_attempted&&!absent_event(before.event+slot)){
        e->cancel_attempted=1;s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
        raw=s->ops.cancel(s->ops.context,s->cookie,slot,ticket);guarded=call_guard(s,&saved);
        e=&s->event[index].original;e->cancel_outcome=raw;
        if(!guarded||!observation(s,&after)){fault(s);return -1;}
        j=1U-slot;
        if(conserved(&before,&after,j)&&ours(s,&after))memcpy(&before,&after,sizeof(before));
        else{fault(s);return -1;}
        if(raw!=0&&raw!=1){e->uncertain=1;return -1;}
    }
    if(absent_event(before.event+slot)&&!before.callbacks_inflight){e->owned=0;e->quiet=1;e->uncertain=0;return 1;}
    return e->uncertain?-1:0;
}
static int source_proof(struct pt_private_mixed_causal_rolling_timer_source *s)
{
    struct pt_private_pair_timer_hw_state h;
    if(s->exclusion_unknown||s->baseline_invalid)return -1;
    if(!observation(s,&h))return -1;
    return !memcmp(&h,&s->before,sizeof(h))&&!h.callbacks_inflight&&!h.restore_pending?1:0;
}
static int close_callback(void *context,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=context;
    struct pt_private_pair_timer_hw_state h,baseline,argument;struct pt_private_mixed_causal_rolling_timer_source saved;
    unsigned i;int raw,proof,guarded;
    if(!task(s)||!s->bound||!extent(r,sizeof(*r))||!aligned(r,offsetof(struct registration_alignment,object))||!same(r,&s->registration)||s->source_attempted||s->irq_active||s->dispatch_active)return -1;
    for(i=0;i<PT_PRIVATE_ROLLING_TIMER_EVENTS;++i)if(s->event[i].original.owned||s->event[i].original.uncertain||
       (s->event[i].original.known&&!s->event[i].original.quiet))return -1;
    s->source_attempted=1;s->source_outcome=-1;
    if(s->exclusion_unknown||s->baseline_invalid)return -1;
    memcpy(&baseline,&s->before,sizeof(baseline));
    if(!observation(s,&h)||memcmp(&baseline,&s->before,sizeof(baseline)))return -1;
    if(!memcmp(&h,&baseline,sizeof(h))&&!h.callbacks_inflight&&!h.restore_pending){
        s->source_outcome=1;s->source_closed=1;return 1;}
    /* Refuse a lost/foreign identity. Unknown acquire never licenses removal
     * from another owner. Later SOURCE quiet is still read-only. */
    if(!ours(s,&h)||h.callbacks_inflight||
       memcmp(h.before_image,baseline.before_image,sizeof(h.before_image)))return -1;
    for(i=0;i<2;++i)if(!absent_event(h.event+i))return -1;
    memcpy(&argument,&baseline,sizeof(argument));
    s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
    raw=s->ops.release(s->ops.context,s,s->cookie,&argument);guarded=call_guard(s,&saved);s->source_outcome=raw;
    if(!guarded||memcmp(&argument,&baseline,sizeof(argument))||s->baseline_invalid){fault(s);return -1;}
    proof=source_proof(s);if(raw==1&&proof==1){s->source_closed=1;return 1;}
    return proof<0||raw!=0?-1:0;
}
static int source_quiet_callback(void *context,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=context;int proof;
    if(!task(s)||!s->bound||!extent(r,sizeof(*r))||!aligned(r,offsetof(struct registration_alignment,object))||!same(r,&s->registration)||!s->source_attempted||s->dispatch_active)return -1;
    ++s->quiet_probes;proof=source_proof(s);if(proof==1)s->source_closed=1;return proof;
}
int pt_private_rolling_timer_init(struct pt_private_mixed_causal_rolling_timer_source *s,const struct pt_private_pair_timer_ops *o)
{
    if(!extent(s,sizeof(*s))||!aligned(s,offsetof(struct source_alignment,object))||
       !extent(o,sizeof(*o))||!aligned(o,offsetof(struct ops_alignment,object))||!apart(s,sizeof(*s),o,sizeof(*o))||
       !extent(o->context,o->context_bytes)||!apart(s,sizeof(*s),o->context,o->context_bytes)||
       !apart(o,sizeof(*o),o->context,o->context_bytes)||
       !zero(s,sizeof(*s))||o->version!=PT_PRIVATE_PAIR_TIMER_OPS_VERSION||
       o->capabilities!=PT_PRIVATE_PAIR_TIMER_REQUIRED||!o->exclude||!o->restore_exclusion||
       !o->clock||!o->state||!o->acquire||!o->arm||!o->rearm||!o->ack||!o->cancel||!o->release)return 0;
    s->self=s;s->ops=*o;s->initialized=1;return 1;
}
int pt_private_rolling_timer_bind(struct pt_private_mixed_causal_rolling_timer_source *s,
 struct pt_private_mixed_causal_ram_port *port,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_pair_timer_hw_state after,baseline,argument;struct pt_private_mixed_causal_rolling_timer_source saved;
    uint64_t cookie=0;unsigned i;int raw,guarded;struct pt_private_pair_timer_hw_state initial;
    size_t owner_bytes=pt_mixed_causal_control_size(),queue_bytes=pt_mixed_readers_control_size();
    /* Authenticate the existing source, then finish every fixed new-input
     * numeric extent/alignment/disjointness check before reading typed r/port.
     * An aligned alias is not a registration or a port object. */
    if(!task(s)||s->bound||s->failed||
       !extent(r,sizeof(*r))||!aligned(r,offsetof(struct registration_alignment,object))||
       !extent(port,sizeof(*port))||!aligned(port,offsetof(struct port_alignment,object))||
       !extent(s->ops.context,s->ops.context_bytes)||
       !apart(s,sizeof(*s),s->ops.context,s->ops.context_bytes)||
       !apart(s,sizeof(*s),port,sizeof(*port))||!apart(s,sizeof(*s),r,sizeof(*r))||
       !apart(s->ops.context,s->ops.context_bytes,port,sizeof(*port))||
       !apart(s->ops.context,s->ops.context_bytes,r,sizeof(*r))||!apart(port,sizeof(*port),r,sizeof(*r)))return 0;
    /* r is now a complete aligned disjoint input. Check the opaque control
     * extents numerically before reading any port profile/adapter fields. */
    if(!r->owner||!r->queue||!r->session||!r->generation||
       !apart(r->owner,owner_bytes,r->queue,queue_bytes)||
       !apart(r,sizeof(*r),r->owner,owner_bytes)||!apart(r,sizeof(*r),r->queue,queue_bytes)||
       !apart(s,sizeof(*s),r->owner,owner_bytes)||!apart(s,sizeof(*s),r->queue,queue_bytes)||
       !apart(s->ops.context,s->ops.context_bytes,r->owner,owner_bytes)||
       !apart(s->ops.context,s->ops.context_bytes,r->queue,queue_bytes)||
       !apart(port,sizeof(*port),r->owner,owner_bytes)||!apart(port,sizeof(*port),r->queue,queue_bytes))return 0;
    if(port->self!=port||!port->initialized||!port->bound||port->rolling_mode!=1||
       port->control_mode!=1||port->lineage_mode||port->adapter.context!=s||
       port->adapter.context_bytes!=sizeof(*s)||port->adapter.clock!=clock_callback||
       port->adapter.arm_at!=arm_callback||port->adapter.ticket_quiet!=quiet_callback||
       port->adapter.source_close!=close_callback||port->adapter.source_quiet!=source_quiet_callback||
       !same(r,&port->registration))return 0;
    if(!observation(s,&initial)||!initial.available||initial.source||initial.entry||initial.cookie||
       initial.vector_live||initial.callbacks_inflight||initial.restore_pending)return 0;
    for(i=0;i<2;++i)if(!absent_event(initial.event+i))return 0;
    memcpy(&s->before,&initial,sizeof(initial));s->port=port;memcpy(&s->registration,r,sizeof(*r));s->bound=s->acquire_attempted=1;
    memcpy(&baseline,&s->before,sizeof(baseline));memcpy(&argument,&baseline,sizeof(argument));
    s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
    raw=s->ops.acquire(s->ops.context,s,pt_private_rolling_timer_irq,&argument,&cookie);
    guarded=call_guard(s,&saved);s->cookie=cookie;s->acquire_outcome=raw;
    if(!guarded||!observation(s,&after)||memcmp(&argument,&baseline,sizeof(argument))||
       memcmp(&s->before,&baseline,sizeof(baseline))){
        if(memcmp(&s->before,&baseline,sizeof(baseline)))s->baseline_invalid=1;
        s->acquire_unknown=1;fault(s);return -1;}
    if(raw==0&&!memcmp(&after,&s->before,sizeof(after))){s->acquire_absent=1;return 0;}
    if(raw==1&&ours(s,&after)&&after.available==baseline.available&&!after.callbacks_inflight&&
       !memcmp(after.event,baseline.event,sizeof(after.event))&&
       !memcmp(after.before_image,baseline.before_image,sizeof(after.before_image))){s->acquired=1;return 1;}
    s->acquire_unknown=1;fault(s);return -1;
}
struct pt_private_mixed_causal_ram_adapter pt_private_rolling_timer_adapter(struct pt_private_mixed_causal_rolling_timer_source *s)
{return (struct pt_private_mixed_causal_ram_adapter){s,sizeof(*s),clock_callback,arm_callback,quiet_callback,close_callback,source_quiet_callback};}
int pt_private_rolling_timer_irq(void *context)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=context;struct pt_private_pair_timer_hw_state h,after;
    struct pt_private_pair_timer_event *e;struct pt_private_mixed_causal_rolling_timer_source saved;
    unsigned i,j,index;int raw,guarded,result=0;
    if(!current(s)||!s->bound||!s->acquired||s->source_attempted)return 0;
    if(enter(s,PT_PRIVATE_PAIR_TIMER_IRQ)!=1)return -1;
    s->irq_active=1;++s->irq_entries;
    if(s->failed||!observation(s,&h)||!ours(s,&h)){result=-1;goto done;}
    for(i=0;i<2;++i){
        if(!h.event[i].pending&&!h.event[i].queued)continue;
        index=s->slot_event[i];
        if(!index||index>s->arms_seen||index>PT_PRIVATE_ROLLING_TIMER_EVENTS||s->event[index-1].slot!=i){fault(s);result=-1;goto done;}
        --index;e=&s->event[index].original;
        if(!e->known||!e->owned||e->quiet||e->uncertain||!same(&e->registration,&s->registration)||
           !expected_event(h.event+i,e)){fault(s);result=-1;goto done;}
        s->dispatch_active=1;++s->dispatches;
        s->ops_busy=1;memcpy(&saved,s,sizeof(saved));s->ops_busy=0;
        /* Genuine dispatch may call the clock adapter, hence it is not ops_busy.
         * Its nested guarded clock may change only sticky failure atoms. */
        saved.ops_busy=0;
        raw=pt_private_mixed_causal_ram_dispatch(s->port,e->ticket);
        guarded=call_guard(s,&saved);s->dispatch_active=0;e=&s->event[index].original;
        e->fire_outcome=s->last_dispatch=raw;
        if(!guarded){fault(s);result=-1;goto done;}
        if(raw==PT_MIXED_CAUSAL_EARLY){
            s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
            raw=s->ops.rearm(s->ops.context,s->cookie,i,e->ticket,e->first,e->frequency);
            guarded=call_guard(s,&saved);e=&s->event[index].original;e->rearm_outcome=raw;
            if(!guarded||raw!=1||!observation(s,&after)||!ours(s,&after)||
               !expected_event(after.event+i,e)||!after.event[i].armed||
               after.event[i].pending||after.event[i].queued){fault(s);result=-1;goto done;}
            j=1U-i;if(!conserved(&h,&after,j)){fault(s);result=-1;goto done;}
            memcpy(&h,&after,sizeof(h));result=1;continue;
        }
        /* Ack only this completed delivery, never its sibling. Genuine port
         * keeps actual irreversible adoption and its post-clock tombstone. */
        e->terminal=1;s->ops_busy=1;memcpy(&saved,s,sizeof(saved));
        raw=s->ops.ack(s->ops.context,s->cookie,i,e->ticket);guarded=call_guard(s,&saved);
        e=&s->event[index].original;e->ack_outcome=raw;
        if(!guarded||raw!=1||!observation(s,&after)||!ours(s,&after)||
           !absent_event(after.event+i)){e->uncertain=1;fault(s);result=-1;goto done;}
        j=1U-i;if(!conserved(&h,&after,j)){fault(s);result=-1;goto done;}
        memcpy(&h,&after,sizeof(h));result=1;
        if(e->fire_outcome!=PT_MIXED_CAUSAL_COMMITTED){fault(s);result=-1;goto done;}
    }
 done:s->irq_active=0;
    if(leave(s,PT_PRIVATE_PAIR_TIMER_IRQ)!=1)result=-1;
    return result;
}
