/* No native resource binding, assembly, timer MMIO or playback wiring. */
#include "native_mixed_causal_pair_timer_source.h"
#include <string.h>
static int extent(const void *p,size_t n)
{return p&&n&&(uintptr_t)p<=UINTPTR_MAX-(n-1U);}
static int apart(const void *p,size_t n,const void *q,size_t m)
{uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;return extent(p,n)&&extent(q,m)&&(a<=b?n<=b-a:m<=a-b);}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
static int same(const struct pt_mixed_causal_registration *a,const struct pt_mixed_causal_registration *b)
{return a->owner==b->owner&&a->queue==b->queue&&a->session==b->session&&a->generation==b->generation;}
static int current(const struct pt_private_mixed_causal_pair_timer_source *s)
{return s&&s->self==s&&s->initialized;}
static void fault(struct pt_private_mixed_causal_pair_timer_source *s)
{unsigned i;s->failed=1;++s->faults;for(i=0;i<2;++i)if(s->event[i].owned)s->event[i].uncertain=1;}
static int task(const struct pt_private_mixed_causal_pair_timer_source *s)
{return current(s)&&s->scope==PT_PRIVATE_PAIR_TIMER_TASK&&!s->ops_busy&&!s->irq_active;}
static int observation(struct pt_private_mixed_causal_pair_timer_source *s,struct pt_private_pair_timer_hw_state *h)
{int raw;unsigned before=s->faults;struct pt_private_pair_timer_hw_state baseline;
 memcpy(&baseline,&s->before,sizeof(baseline));memset(h,0,sizeof(*h));s->ops_busy=1;
 raw=s->ops.state(s->ops.context,h);s->ops_busy=0;
 if(s->bound&&memcmp(&baseline,&s->before,sizeof(baseline))){s->baseline_invalid=1;fault(s);return 0;}
 return raw==1&&before==s->faults;}
static int ours(const struct pt_private_mixed_causal_pair_timer_source *s,const struct pt_private_pair_timer_hw_state *h)
{return h->source==s&&h->entry==pt_private_pair_timer_irq&&h->cookie==s->cookie&&s->cookie&&h->vector_live==1&&!h->restore_pending;}
static int conserved(const struct pt_private_pair_timer_hw_state *a,const struct pt_private_pair_timer_hw_state *b,unsigned sibling)
{return a->source==b->source&&a->entry==b->entry&&a->cookie==b->cookie&&a->available==b->available&&
 a->vector_live==b->vector_live&&a->callbacks_inflight==b->callbacks_inflight&&a->restore_pending==b->restore_pending&&
 !memcmp(a->before_image,b->before_image,sizeof(a->before_image))&&
 !memcmp(a->event+sibling,b->event+sibling,sizeof(a->event[sibling]));}
static int absent_event(const struct pt_private_pair_timer_hw_event *h)
{return !h->armed&&!h->pending&&!h->queued;}
static int expected_event(const struct pt_private_pair_timer_hw_event *h,const struct pt_private_pair_timer_event *e)
{return h->ticket==e->ticket&&h->first==e->first&&h->frequency==e->frequency;}
static int source_proof(struct pt_private_mixed_causal_pair_timer_source *s);
static int leave(struct pt_private_mixed_causal_pair_timer_source *s,unsigned scope);
static int enter(struct pt_private_mixed_causal_pair_timer_source *s,unsigned scope)
{
    unsigned before;uint64_t token;
    if(!current(s))return 0;
    if(s->scope||s->entry_busy||s->ops_busy||s->irq_active){fault(s);return -1;}
    s->entry_busy=1;before=s->faults;s->ops_busy=1;
    token=s->ops.exclude(s->ops.context,scope);s->ops_busy=0;
    if(!token){s->exclusion_unknown=1;++s->exclusion_refusals;fault(s);s->entry_busy=0;return -1;}
    s->scope=scope;s->exclusion_token=token;s->entry_busy=0;
    if(s->faults!=before){++s->exclusion_refusals;fault(s);
        /* Keep the actual token until the trusted primitive restores its
         * exact prior state. Reentry never erases acquired exclusion. */
        (void)leave(s,scope);return -1;}
    return 1;
}
static int leave(struct pt_private_mixed_causal_pair_timer_source *s,unsigned scope)
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
int pt_private_pair_timer_task_enter(struct pt_private_mixed_causal_pair_timer_source *s)
{int r=enter(s,PT_PRIVATE_PAIR_TIMER_TASK);if(r==1)++s->task_entries;return r;}
int pt_private_pair_timer_task_leave(struct pt_private_mixed_causal_pair_timer_source *s)
{return leave(s,PT_PRIVATE_PAIR_TIMER_TASK);}
static int clock_callback(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct pt_private_mixed_causal_pair_timer_source *s=context;unsigned before;int raw;
    struct pt_private_pair_timer_hw_state baseline;
    if(!current(s)||!s->scope||s->ops_busy||!ticks||!frequency)return 0;
    memcpy(&baseline,&s->before,sizeof(baseline));
    before=s->faults;s->ops_busy=1;raw=s->ops.clock(s->ops.context,ticks,frequency);s->ops_busy=0;
    if(s->bound&&memcmp(&baseline,&s->before,sizeof(baseline))){s->baseline_invalid=1;fault(s);return 0;}
    return before==s->faults&&raw==1?1:0; /* Real observed value, never desired first. */
}
static int arm_callback(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct pt_private_mixed_causal_pair_timer_source *s=context;
    struct pt_private_pair_timer_hw_state before,after;struct pt_private_pair_timer_event *e;
    unsigned slot,j,faults;int raw;
    if(!task(s)||!s->bound||!r)return -1;
    if(s->failed||!s->acquired||s->source_attempted)return -1;
    if(!same(r,&s->registration)||!ticket||s->arms_seen>=2||
       (frequency!=709379U&&frequency!=715909U))return 0;
    for(j=0;j<s->arms_seen;++j)if(s->event[j].ticket==ticket||
       s->event[j].frequency!=frequency||s->event[j].first>=first)return 0;
    if(!observation(s,&before)||!ours(s,&before)){fault(s);return -1;}
    slot=s->arms_seen++;e=s->event+slot;
    memcpy(&e->registration,r,sizeof(*r));e->ticket=ticket;e->first=first;e->frequency=frequency;
    e->known=e->owned=1;faults=s->faults;s->ops_busy=1;
    raw=s->ops.arm(s->ops.context,s->cookie,slot,ticket,first,frequency);s->ops_busy=0;e->arm_outcome=raw;
    if(!observation(s,&after)||faults!=s->faults){fault(s);e->uncertain=1;return -1;}
    if(raw==0&&!memcmp(&before,&after,sizeof(before))){e->owned=0;e->quiet=1;return 0;}
    j=1U-slot;
    if(raw==1&&ours(s,&after)&&expected_event(after.event+slot,e)&&after.event[slot].armed&&
       !after.event[slot].pending&&!after.event[slot].queued&&
       conserved(&before,&after,j))return 1;
    e->uncertain=1;fault(s);return -1;
}
static int quiet_callback(void *context,const struct pt_mixed_causal_registration *r,uint64_t ticket,unsigned cancel)
{
    struct pt_private_mixed_causal_pair_timer_source *s=context;
    struct pt_private_pair_timer_hw_state before,after;struct pt_private_pair_timer_event *e=NULL;
    unsigned i,j,faults;int raw;
    if(!task(s)||!s->bound||!r||!same(r,&s->registration)||cancel>1)return -1;
    for(i=0;i<2;++i)if(s->event[i].known&&s->event[i].ticket==ticket){e=s->event+i;break;}
    if(!e||!same(r,&e->registration)||s->dispatch_active||!observation(s,&before))return -1;
    if(s->source_closed){return e->quiet&&source_proof(s)==1?1:-1;}
    if(!ours(s,&before)||before.callbacks_inflight){fault(s);return -1;}
    if(!absent_event(before.event+i)&&!expected_event(before.event+i,e)){fault(s);return -1;}
    if(!cancel&&!e->terminal&&!e->quiet)return 0;
    if(cancel&&!e->cancel_attempted&&!absent_event(before.event+i)){
        e->cancel_attempted=1;faults=s->faults;s->ops_busy=1;
        raw=s->ops.cancel(s->ops.context,s->cookie,i,ticket);s->ops_busy=0;e->cancel_outcome=raw;
        if(!observation(s,&after)||faults!=s->faults){fault(s);return -1;}
        j=1U-i;
        if(conserved(&before,&after,j)&&ours(s,&after))memcpy(&before,&after,sizeof(before));
        else{fault(s);return -1;}
        if(raw!=0&&raw!=1){e->uncertain=1;return -1;}
    }
    if(absent_event(before.event+i)&&!before.callbacks_inflight){e->owned=0;e->quiet=1;e->uncertain=0;return 1;}
    return e->uncertain?-1:0;
}
static int source_proof(struct pt_private_mixed_causal_pair_timer_source *s)
{
    struct pt_private_pair_timer_hw_state h;
    if(s->exclusion_unknown||s->baseline_invalid)return -1;
    if(!observation(s,&h))return -1;
    return !memcmp(&h,&s->before,sizeof(h))&&!h.callbacks_inflight&&!h.restore_pending?1:0;
}
static int close_callback(void *context,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_mixed_causal_pair_timer_source *s=context;
    struct pt_private_pair_timer_hw_state h,baseline,argument;unsigned i,before;int raw,proof;
    if(!task(s)||!s->bound||!r||!same(r,&s->registration)||s->source_attempted||s->irq_active||s->dispatch_active)return -1;
    for(i=0;i<2;++i)if(s->event[i].owned||s->event[i].uncertain||
       (s->event[i].known&&!s->event[i].quiet))return -1;
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
    before=s->faults;s->ops_busy=1;
    raw=s->ops.release(s->ops.context,s,s->cookie,&argument);s->ops_busy=0;s->source_outcome=raw;
    if(memcmp(&s->before,&baseline,sizeof(baseline)))s->baseline_invalid=1;
    if(s->faults!=before||memcmp(&argument,&baseline,sizeof(argument))||s->baseline_invalid){fault(s);return -1;}
    proof=source_proof(s);if(raw==1&&proof==1){s->source_closed=1;return 1;}
    return proof<0||raw!=0?-1:0;
}
static int source_quiet_callback(void *context,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_mixed_causal_pair_timer_source *s=context;int proof;
    if(!task(s)||!s->bound||!r||!same(r,&s->registration)||!s->source_attempted||s->dispatch_active)return -1;
    ++s->quiet_probes;proof=source_proof(s);if(proof==1)s->source_closed=1;return proof;
}
int pt_private_pair_timer_init(struct pt_private_mixed_causal_pair_timer_source *s,const struct pt_private_pair_timer_ops *o)
{
    if(!extent(s,sizeof(*s))||!extent(o,sizeof(*o))||!apart(s,sizeof(*s),o,sizeof(*o))||
       !extent(o->context,o->context_bytes)||!apart(s,sizeof(*s),o->context,o->context_bytes)||
       !apart(o,sizeof(*o),o->context,o->context_bytes)||
       !zero(s,sizeof(*s))||o->version!=PT_PRIVATE_PAIR_TIMER_OPS_VERSION||
       o->capabilities!=PT_PRIVATE_PAIR_TIMER_REQUIRED||!o->exclude||!o->restore_exclusion||
       !o->clock||!o->state||!o->acquire||!o->arm||!o->rearm||!o->ack||!o->cancel||!o->release)return 0;
    s->self=s;s->ops=*o;s->initialized=1;return 1;
}
int pt_private_pair_timer_bind(struct pt_private_mixed_causal_pair_timer_source *s,
 struct pt_private_mixed_causal_ram_port *port,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_pair_timer_hw_state after,baseline,argument;unsigned before,i;int raw;
    size_t owner_bytes=pt_mixed_causal_control_size(),queue_bytes=pt_mixed_readers_control_size();
    if(!task(s)||s->bound||s->failed||!port||!r||!r->owner||!r->queue||!r->session||!r->generation||
       !extent(port,sizeof(*port))||!apart(s,sizeof(*s),port,sizeof(*port))||
       !apart(s->ops.context,s->ops.context_bytes,port,sizeof(*port))||
       !apart(s,sizeof(*s),r,sizeof(*r))||!apart(s->ops.context,s->ops.context_bytes,r,sizeof(*r))||
       !apart(s,sizeof(*s),r->owner,owner_bytes)||!apart(s,sizeof(*s),r->queue,queue_bytes)||
       !apart(s->ops.context,s->ops.context_bytes,r->owner,owner_bytes)||
       !apart(s->ops.context,s->ops.context_bytes,r->queue,queue_bytes)||
       !apart(port,sizeof(*port),r->owner,owner_bytes)||!apart(port,sizeof(*port),r->queue,queue_bytes)||
       port->self!=port||!port->initialized||!port->bound||!same(r,&port->registration))return 0;
    if(!observation(s,&s->before)||!s->before.available||s->before.source||s->before.entry||s->before.cookie||
       s->before.vector_live||s->before.callbacks_inflight||s->before.restore_pending)return 0;
    for(i=0;i<2;++i)if(!absent_event(s->before.event+i))return 0;
    s->port=port;memcpy(&s->registration,r,sizeof(*r));s->bound=s->acquire_attempted=1;
    memcpy(&baseline,&s->before,sizeof(baseline));memcpy(&argument,&baseline,sizeof(argument));
    before=s->faults;s->ops_busy=1;
    raw=s->ops.acquire(s->ops.context,s,pt_private_pair_timer_irq,&argument,&s->cookie);
    s->ops_busy=0;s->acquire_outcome=raw;
    if(!observation(s,&after)||s->faults!=before||memcmp(&argument,&baseline,sizeof(argument))||
       memcmp(&s->before,&baseline,sizeof(baseline))){
        if(memcmp(&s->before,&baseline,sizeof(baseline)))s->baseline_invalid=1;
        s->acquire_unknown=1;fault(s);return -1;}
    if(raw==0&&!memcmp(&after,&s->before,sizeof(after))){s->acquire_absent=1;return 0;}
    if(raw==1&&ours(s,&after)&&after.available==baseline.available&&!after.callbacks_inflight&&
       !memcmp(after.event,baseline.event,sizeof(after.event))&&
       !memcmp(after.before_image,baseline.before_image,sizeof(after.before_image))){s->acquired=1;return 1;}
    s->acquire_unknown=1;fault(s);return -1;
}
struct pt_private_mixed_causal_ram_adapter pt_private_pair_timer_adapter(struct pt_private_mixed_causal_pair_timer_source *s)
{return (struct pt_private_mixed_causal_ram_adapter){s,sizeof(*s),clock_callback,arm_callback,quiet_callback,close_callback,source_quiet_callback};}
int pt_private_pair_timer_irq(void *context)
{
    struct pt_private_mixed_causal_pair_timer_source *s=context;struct pt_private_pair_timer_hw_state h,after;
    struct pt_private_pair_timer_event *e;unsigned i,j,before;int raw,result=0;
    if(!current(s)||!s->bound||!s->acquired||s->source_attempted)return 0;
    if(enter(s,PT_PRIVATE_PAIR_TIMER_IRQ)!=1)return -1;
    s->irq_active=1;++s->irq_entries;
    if(s->failed||!observation(s,&h)||!ours(s,&h)){result=-1;goto done;}
    for(i=0;i<2;++i){
        if(!h.event[i].pending&&!h.event[i].queued)continue;
        e=s->event+i;
        if(!e->known||!e->owned||e->quiet||e->uncertain||!same(&e->registration,&s->registration)||
           !expected_event(h.event+i,e)){fault(s);result=-1;goto done;}
        s->dispatch_active=1;++s->dispatches;
        raw=pt_private_mixed_causal_ram_dispatch(s->port,e->ticket);
        s->dispatch_active=0;e->fire_outcome=s->last_dispatch=raw;
        if(raw==PT_MIXED_CAUSAL_EARLY){
            before=s->faults;s->ops_busy=1;
            raw=s->ops.rearm(s->ops.context,s->cookie,i,e->ticket,e->first,e->frequency);
            s->ops_busy=0;e->rearm_outcome=raw;
            if(raw!=1||before!=s->faults||!observation(s,&after)||!ours(s,&after)||
               !expected_event(after.event+i,e)||!after.event[i].armed||
               after.event[i].pending||after.event[i].queued){fault(s);result=-1;goto done;}
            j=1U-i;if(!conserved(&h,&after,j)){fault(s);result=-1;goto done;}
            memcpy(&h,&after,sizeof(h));result=1;continue;
        }
        /* Ack only this completed delivery, never its sibling. Genuine port
         * keeps actual irreversible adoption and its post-clock tombstone. */
        e->terminal=1;before=s->faults;s->ops_busy=1;
        raw=s->ops.ack(s->ops.context,s->cookie,i,e->ticket);s->ops_busy=0;e->ack_outcome=raw;
        if(raw!=1||before!=s->faults||!observation(s,&after)||!ours(s,&after)||
           !absent_event(after.event+i)){e->uncertain=1;fault(s);result=-1;goto done;}
        j=1U-i;if(!conserved(&h,&after,j)){fault(s);result=-1;goto done;}
        memcpy(&h,&after,sizeof(h));result=1;
        if(e->fire_outcome!=PT_MIXED_CAUSAL_COMMITTED){fault(s);result=-1;goto done;}
    }
 done:s->irq_active=0;
    if(leave(s,PT_PRIVATE_PAIR_TIMER_IRQ)!=1)result=-1;
    return result;
}
