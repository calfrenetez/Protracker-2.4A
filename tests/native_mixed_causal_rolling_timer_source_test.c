/* Separate fixed-four rolling HOST resource fixture. Genuine rolling donor
 * entry remains uncalled; no MMIO/native vector/actual audio/timing claims. */
#include <stdlib.h>
#include <stddef.h>
#define PT_PRIVATE_PAIR_TIMER_HOST_RESOURCE_MODEL 1
#define PT_PRIVATE_ROLLING_TIMER_HOST_RESOURCE_MODEL 1
#include "../src/native/readers_ram/native_mixed_causal_rolling_timer_source.h"
static void *rt_calloc(size_t,size_t);
#define calloc rt_calloc
#define PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY inherited_rolling68_resource_donor_not_called
#include "native_mixed_causal_ram_rolling_song_test.c"
#undef PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY
#undef calloc
struct rt_hardware {
    struct pt_private_pair_timer_hw_state state,original;
    int (*entry)(void *);
    struct pt_private_mixed_causal_ram_port *port;
    uint64_t now;uint32_t frequency;
    unsigned disabled,forbidden,arms,rearms,acks,cancels,releases,reads,clocks,vectors;
    unsigned arm_mode,release_unknown,read_failure,exclude_reentry,acquire_clean0;
    unsigned acquire_mode,release_mode,acquires,bind_refusal,bind_refusals;
    unsigned mutation_at,mutations;
    int vector_result;
};
struct rt_carrier {
    struct sl_trial trial;
    struct pt_private_mixed_causal_rolling_timer_source source;
    struct rt_hardware hardware;
};
static struct rt_carrier *rt_current;
static unsigned rt_expand,rt_cases;
static void *rt_calloc(size_t count,size_t bytes)
{
    if(rt_expand){assert(count==1&&bytes==sizeof(struct sl_trial));rt_expand=0;
        rt_current=calloc(1,sizeof(*rt_current));assert(rt_current);
        assert(offsetof(struct rt_carrier,trial)==0);return &rt_current->trial;}
    return calloc(count,bytes);
}
/* Provider failure injection changes only this source-owned scalar. The new
 * guard must retain its real original identity and sticky unknown ownership.
 * No fixture mutates/fabricates RAM/core/reader completion or ACTIVE receipt. */
static void rt_mutate(struct rt_hardware *h,unsigned at)
{
    if(h->mutation_at==at){struct pt_private_mixed_causal_rolling_timer_source *s=h->state.source;
        assert(s&&s==&rt_current->source);h->mutation_at=0;++h->mutations;
        if(s->arms_seen)s->event[s->arms_seen-1].original.ticket=0;
        else s->slot_event[0]=99;}
}
static int rt_absent(const struct pt_private_pair_timer_hw_event *e)
{return !e->armed&&!e->pending&&!e->queued;}
static void rt_pump(struct rt_hardware *h)
{
    unsigned i;
    if(h->disabled||!h->entry||!h->state.vector_live||h->state.callbacks_inflight)return;
    for(i=0;i<2;++i)if(h->state.event[i].pending||h->state.event[i].queued)break;
    if(i==2)return;
    ++h->vectors;h->state.callbacks_inflight=1;
    h->vector_result=h->entry(h->state.source);
    assert(h->state.callbacks_inflight==1);h->state.callbacks_inflight=0;
    /* One real vector invocation, no automatic retry loop on failure. */
}
static void rt_advance(struct rt_hardware *h,uint64_t now)
{
    unsigned i;assert(now>=h->now);h->now=now;
    for(i=0;i<2;++i){struct pt_private_pair_timer_hw_event *e=h->state.event+i;
        if(e->armed&&now>=e->first){e->armed=0;e->pending=e->queued=1;}}
    rt_pump(h);
}
static void rt_early_wake(struct rt_hardware *h,unsigned slot)
{
    struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(slot<2&&e->armed&&h->now<e->first);
    e->armed=0;e->pending=e->queued=1;rt_pump(h);
}
static uint64_t rt_exclude(void *context,unsigned scope)
{
    struct rt_hardware *h=context;uint64_t token;
    assert(scope==PT_PRIVATE_PAIR_TIMER_TASK||scope==PT_PRIVATE_PAIR_TIMER_IRQ);
    token=((uint64_t)h->disabled<<32)|((uint64_t)h->forbidden<<1)|1U;
    ++h->disabled;if(scope==PT_PRIVATE_PAIR_TIMER_TASK)++h->forbidden;
    if(h->exclude_reentry){h->exclude_reentry=0;
        assert(pt_private_rolling_timer_task_enter(h->state.source)==-1);}
    return token;
}
static void rt_restore(void *context,unsigned scope,uint64_t token)
{
    struct rt_hardware *h=context;
    assert(token&&h->disabled&&scope>=PT_PRIVATE_PAIR_TIMER_TASK&&scope<=PT_PRIVATE_PAIR_TIMER_IRQ);
    h->disabled=(unsigned)(token>>32);h->forbidden=(unsigned)((token>>1)&0x7fffffffU);
    rt_pump(h);
}
static int rt_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct rt_hardware *h=context;assert(h->disabled);++h->clocks;rt_mutate(h,1);
    *ticks=h->now;*frequency=h->frequency;return 1;
}
static int rt_state(void *context,struct pt_private_pair_timer_hw_state *out)
{
    struct rt_hardware *h=context;assert(h->disabled);++h->reads;rt_mutate(h,2);
    if(h->read_failure){--h->read_failure;return 0;}
    memcpy(out,&h->state,sizeof(*out));out->entry=h->entry;return 1;
}
static int rt_acquire(void *context,void *source,int (*entry)(void *),
 const struct pt_private_pair_timer_hw_state *before,uint64_t *cookie)
{
    struct rt_hardware *h=context;assert(h->disabled&&source&&entry);
    assert(!memcmp(before,&h->original,sizeof(*before))&&!h->state.source&&!h->entry);
    assert(++h->acquires==1);
    if(h->acquire_clean0){*cookie=0;return 0;}
    h->state.source=source;h->state.entry=entry;h->state.cookie=0x314159U;h->state.vector_live=1;
    h->entry=entry;*cookie=h->state.cookie;
    if(h->acquire_mode==2)rt_mutate(h,7);
    if(h->acquire_mode==3){
        /* The provider corrupts only its actual caller-owned mutable local
         * argument after acquiring. No provider baseline or owner is forged. */
        ((struct pt_private_pair_timer_hw_state *)(void *)before)->before_image[0]^=1U;
        ++h->mutations;}
    return h->acquire_mode==1?-1:1;
}
static int rt_arm(void *context,uint64_t cookie,unsigned slot,uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct rt_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&cookie==h->state.cookie&&slot<2&&ticket&&rt_absent(e));++h->arms;
    if(h->arm_mode==3)return 0; /* Exact unchanged no-NEW-effects refusal. */
    memset(e,0,sizeof(*e));e->ticket=ticket;e->first=first;e->frequency=frequency;e->armed=1;
    rt_mutate(h,3);
    if(h->arm_mode==4)assert(pt_private_rolling_timer_irq(h->state.source)==-1);
    return h->arm_mode==2?-1:h->arm_mode?0:1; /* Real NEW effects retained. */
}
static int rt_rearm(void *context,uint64_t cookie,unsigned slot,uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct rt_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&cookie==h->state.cookie&&slot<2&&e->ticket==ticket&&e->first==first&&e->frequency==frequency);
    assert(e->pending&&e->queued&&h->now<first);++h->rearms;
    e->armed=1;e->pending=e->queued=0;rt_mutate(h,4);return 1;
}
static int rt_ack(void *context,uint64_t cookie,unsigned slot,uint64_t ticket)
{
    struct rt_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&cookie==h->state.cookie&&slot<2&&e->ticket==ticket&&e->pending);
    ++h->acks;memset(e,0,sizeof(*e));rt_mutate(h,5);return 1;
}
static int rt_cancel(void *context,uint64_t cookie,unsigned slot,uint64_t ticket)
{
    struct rt_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&!h->state.callbacks_inflight&&cookie==h->state.cookie&&slot<2&&e->ticket==ticket);
    ++h->cancels;memset(e,0,sizeof(*e));rt_mutate(h,6);return 1;
}
static int rt_release(void *context,void *source,uint64_t cookie,const struct pt_private_pair_timer_hw_state *before)
{
    struct rt_hardware *h=context;unsigned i;
    assert(h->disabled&&source==h->state.source&&cookie==h->state.cookie&&++h->releases==1);
    assert(!h->state.callbacks_inflight&&!memcmp(before,&h->original,sizeof(*before)));
    for(i=0;i<2;++i)assert(rt_absent(h->state.event+i));
    memcpy(&h->state,before,sizeof(h->state));h->entry=NULL;
    if(h->release_mode==1){
        /* Actual release clears provider source lookup. Retain the real supplied
         * Source argument, not a synthesized identity, for this corruption. */
        struct pt_private_mixed_causal_rolling_timer_source *s=source;
        assert(s==&rt_current->source&&h->mutation_at==8&&s->arms_seen==4);
        h->mutation_at=0;++h->mutations;s->event[3].original.ticket=0;}
    if(h->release_mode==2){
        /* Original baseline already used for actual release; damage only the
         * writable local argument supplied by the guarded source callback. */
        ((struct pt_private_pair_timer_hw_state *)(void *)before)->before_image[0]^=1U;
        ++h->mutations;}
    if(h->release_unknown){h->state.restore_pending=1;return -1;}
    return 1;
}
/* These exact invalid views are tried only inside the genuine fresh controller
 * bind, after RAM binding but before resource acquisition. They must not be
 * interpreted as typed registration/port objects or expose any callback. */
static void rt_bind_refusal(struct pt_private_mixed_causal_ram_port *p,
 const struct pt_mixed_causal_registration *r,unsigned which)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=&rt_current->source,saved=*s;
    struct rt_hardware *h=&rt_current->hardware,before=*h;
    struct pt_private_mixed_causal_ram_port port=*p;
    struct rt_registration_alignment {char byte;struct pt_mixed_causal_registration object;};
    struct rt_port_alignment {char byte;struct pt_private_mixed_causal_ram_port object;};
    const struct pt_mixed_causal_registration *invalid=r;
    struct pt_private_mixed_causal_ram_port *invalid_port=p;
    unsigned char *storage=NULL;
    assert(which>=1&&which<=5&&!s->bound&&!s->acquire_attempted&&!h->acquires&&!h->state.source&&!h->entry);
    if(which==1)invalid=(const struct pt_mixed_causal_registration *)(const void *)s;
    else if(which==2)invalid=(const struct pt_mixed_causal_registration *)(const void *)h;
    else if(which==3)invalid=(const struct pt_mixed_causal_registration *)(const void *)p;
    else if(which==4){storage=calloc(1,sizeof(*r)+1);assert(storage&&offsetof(struct rt_registration_alignment,object)>1);
       memcpy(storage+1,r,sizeof(*r));invalid=(const struct pt_mixed_causal_registration *)(const void *)(storage+1);
       assert((uintptr_t)invalid%offsetof(struct rt_registration_alignment,object));}
    else{storage=calloc(1,sizeof(*p)+1);assert(storage&&offsetof(struct rt_port_alignment,object)>1);
       memcpy(storage+1,p,sizeof(*p));invalid_port=(struct pt_private_mixed_causal_ram_port *)(void *)(storage+1);
       assert((uintptr_t)invalid_port%offsetof(struct rt_port_alignment,object));}
    if(which<=3)assert(!((uintptr_t)invalid%offsetof(struct rt_registration_alignment,object)));
    assert(pt_private_rolling_timer_bind(s,invalid_port,invalid)==0&&
       !memcmp(s,&saved,sizeof(saved))&&!memcmp(p,&port,sizeof(port))&&!memcmp(h,&before,sizeof(before)));
    free(storage);++h->bind_refusals;
}
static int rt_bind(void *context,const struct pt_mixed_causal_registration *r)
{
    struct sl_trial *e=&rt_current->trial;struct pt_editor_mixed_causal_prepare *s=sl_s(e);
    struct pt_private_mixed_causal_ram_port *p=context;
    assert(p==&e->port.ram&&e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER&&
       e->producer.original.busy&&e->producer.original.candidate_count==4&&e->producer.original.frames==3840&&
       e->producer.original.pin_count==e->base.editor->project->sample_count&&++e->port.binds==1&&
       e->base.ordinary.calls==2&&!s->pool&&s->causal&&s->queue&&r->owner==s->causal&&r->queue==s->queue&&
       r->session==31&&r->generation==17&&!pt_mixed_readers_commands_held(s->queue)&&!pt_mixed_readers_readers_held(s->queue));
    assert(pt_private_mixed_causal_ram_bind(p,s->causal,s->queue,r->session,r->generation));
    if(rt_current->hardware.bind_refusal)rt_bind_refusal(p,r,rt_current->hardware.bind_refusal);
    return pt_private_rolling_timer_bind(&rt_current->source,p,r);
}
static struct sl_trial *rt_make(unsigned bits,unsigned card)
{
    struct sl_trial *e;struct rt_hardware *h;struct pt_private_pair_timer_ops ops;unsigned i;
    rt_expand=1;e=sl_make(bits,16,0,card?CS_CARD16:CS_MIXED,0);
    assert(!rt_expand&&e==&rt_current->trial);++rt_cases;h=&rt_current->hardware;
    h->port=&e->port.ram;h->now=100;h->frequency=709379;h->state.available=1;
    for(i=0;i<8;++i)h->state.before_image[i]=0x5a110000U+i;
    memcpy(&h->original,&h->state,sizeof(h->original));
    ops=(struct pt_private_pair_timer_ops){h,sizeof(*h),PT_PRIVATE_PAIR_TIMER_OPS_VERSION,PT_PRIVATE_PAIR_TIMER_REQUIRED,
       rt_exclude,rt_restore,rt_clock,rt_state,rt_acquire,rt_arm,rt_rearm,rt_ack,rt_cancel,rt_release};
    assert(pt_private_rolling_timer_init(&rt_current->source,&ops));
    /* Replace only the fresh never-bound zero-ownership RAM constructor. */
    assert(!e->port.ram.bound&&!e->producer.original.adopted&&!e->port.binds);
    memset(&e->port.ram,0,sizeof(e->port.ram));
    e->port.declaration=pt_private_rolling_timer_adapter(&rt_current->source);
    assert(pt_private_mixed_causal_ram_init_rolling(&e->port.ram,31,17,709379,&e->port.declaration,128,8));
    e->base.input.contexts=(struct pt_sampler_storage_span){rt_current,sizeof(*rt_current)};
    e->base.input.causal.port=pt_private_mixed_causal_ram_api(&e->port.ram);e->base.input.bind_original=rt_bind;
    e->source->original.original.contexts=e->base.input.contexts;
    e->producer.original.configuration.preparation=e->base.input;
    e->producer.port=pt_private_mixed_causal_ram_rolling_api(&e->port.ram);return e;
}
static void rt_begin(void)
{assert(pt_private_rolling_timer_task_enter(&rt_current->source)==1);}
static int rt_end(void)
{return pt_private_rolling_timer_task_leave(&rt_current->source);}
static void rt_due(struct sl_trial *e,unsigned index)
{
    struct rt_hardware *h=&rt_current->hardware;unsigned vectors=h->vectors;
    unsigned effects=e->port.ram.effects,controls=e->port.ram.controls,stops=e->port.ram.stops;
    assert(index<4&&sl_history(e,index)->published&&rt_current->source.arms_seen==index+1);
    rt_advance(h,oracle(960U*(index+1)));
    assert(h->vectors==vectors+1&&h->vector_result==1&&h->acks==index+1&&
       rt_current->source.event[index].original.fire_outcome==PT_MIXED_CAUSAL_COMMITTED&&
       rt_current->source.event[index].original.terminal&&!rt_current->source.event[index].original.quiet&&
       rt_current->source.event[index].original.owned&&rt_absent(h->state.event+(index&1U)));
    assert(e->port.ram.effects==effects+(index==0?16U:0U)&&
       e->port.ram.controls==controls+(index==1||index==2?16U:0U)&&e->port.ram.stops==stops+(index==3?16U:0U));
}
/* Actual task service both advances genuine queue adoption/timing and wakes
 * WAIT_ACTIVE. This HOST provider has already acknowledged the exact terminal
 * delivery and has no callback in flight, so positive independent quiet can
 * legitimately retire it now. Readers/pins/caches are a separate live lifetime. */
static void rt_service_completed(struct sl_trial *e,unsigned index)
{
    struct rt_hardware *h=&rt_current->hardware;
    struct pt_private_mixed_causal_rolling_timer_source *s=&rt_current->source;
    struct pt_editor_mixed_causal_song_lineage_history *history=sl_history(e,index);
    struct pt_editor_mixed_command_ref original=history->reference;
    struct pt_editor_mixed_reader_ref readers[32];
    struct pt_mixed_causal_rolling_basis basis;
    unsigned chips=e->base.chip.calls,masters=e->base.masters.calls,writes=e->base.card->writes;
    unsigned pins=e->producer.original.pin_count,arms=h->arms,acks=h->acks,cancels=h->cancels,reads,row=2,matches=0;
    assert(index<3&&s->scope==PT_PRIVATE_PAIR_TIMER_TASK&&h->disabled&&h->forbidden&&
       history->published&&!history->detached&&original.serial&&
       rt_absent(h->state.event+(index&1U))&&!h->state.callbacks_inflight&&
       s->event[index].original.known&&s->event[index].original.terminal&&
       s->event[index].original.owned&&!s->event[index].original.quiet&&!s->event[index].original.uncertain);
    for(unsigned i=0;i<32;++i)readers[i]=e->producer.original.reader[i].reference;
    /* Sampler command slots and rolling factory rows are independently
     * recycled. Resolve the current row through its exact retained identity,
     * never through the sampler reference's numeric slot alone. */
    for(unsigned i=0;i<2;++i)if(e->joint.rolling.row[i].ticket==history->ticket&&
       e->joint.rolling.row[i].ref.slot==original.slot&&e->joint.rolling.row[i].ref.serial==original.serial){row=i;++matches;}
    assert(matches==1&&row<2);
    assert(e->joint.rolling.row[row].serial==index);
    assert(row==(e->joint.rolling.row[row].serial&1U));
    assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,index,0)==PT_MIXED_READERS_OK);
    assert(history->detached);
    assert(sl_history(e,0)->service_seen);
    assert(history->reference.slot==original.slot&&history->reference.serial==original.serial);
    assert(!sl_s(e)->command[original.slot].handle.address);
    assert(!sl_s(e)->command[original.slot].handle.token);
    assert(e->joint.rolling.row[row].detached);
    assert(e->joint.rolling.row[row].closed);
    assert(e->joint.rolling.row[row].ticket==history->ticket);
    assert(e->joint.rolling.row[row].ref.slot==original.slot);
    assert(e->joint.rolling.row[row].ref.serial==original.serial);
    assert(e->joint.rolling.row[row].serial==index);
    assert(s->event[index].original.ticket==history->ticket);
    assert(s->event[index].original.first==oracle(960U*(index+1)));
    assert(s->event[index].original.known&&s->event[index].original.terminal);
    assert(s->event[index].original.quiet);
    assert(!s->event[index].original.owned&&!s->event[index].original.uncertain);
    assert(!pt_mixed_readers_commands_held(sl_s(e)->queue));
    assert(pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
    assert(e->producer.original.result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
    assert(e->producer.original.pin_count==pins&&pins==e->base.editor->project->sample_count);
    assert(e->base.chip.calls==chips&&e->base.masters.calls==masters&&e->base.card->writes==writes);
    assert(h->arms==arms&&h->acks==acks&&h->cancels==cancels);
    for(unsigned i=0;i<32;++i)assert(!memcmp(readers+i,&e->producer.original.reader[i].reference,sizeof(readers[i])));
    /* The still-owned queue must not find this consumed original numeric
     * ticket. This does not service an expired handle or call a backend. */
    reads=h->reads;
    assert(pt_mixed_readers_service_command(sl_s(e)->queue,history->ticket,0,NULL)==PT_MIXED_READERS_INVALID&&h->reads==reads);
    memset(&basis,0,sizeof(basis));
    assert(pt_mixed_causal_factory_rolling_basis(sl_s(e)->causal,&basis)&&
       basis.root==sl_history(e,0)->ticket&&basis.predecessor==history->ticket&&basis.serial==index+1);
    if(index){assert(basis.reuse_required&&basis.reuse==sl_history(e,index-1)->ticket&&
       sl_history(e,index-1)->detached);}
    else assert(!basis.reuse_required);
}
/* At later reuse the original history already has a real retained quiet and
 * NULL/disposal proof. Never call sl_detach/service again on detached history. */
static void rt_retired_reuse(struct sl_trial *e,unsigned index)
{
    struct pt_private_mixed_causal_rolling_timer_source *s=&rt_current->source;
    struct rt_hardware *h=&rt_current->hardware;
    struct pt_mixed_causal_rolling_basis basis;
    assert(index<2&&sl_history(e,index)->detached&&sl_history(e,0)->service_seen&&
       s->event[index].original.known&&s->event[index].original.terminal&&s->event[index].original.quiet&&
       !s->event[index].original.owned&&!s->event[index].original.uncertain&&
       s->event[index].original.ticket==sl_history(e,index)->ticket&&
       s->event[index].original.first==oracle(960U*(index+1))&&rt_absent(h->state.event+(index&1U))&&
       pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
    memset(&basis,0,sizeof(basis));
    assert(pt_mixed_causal_factory_rolling_basis(sl_s(e)->causal,&basis)&&basis.reuse_required&&
       basis.reuse==sl_history(e,index)->ticket&&basis.root==sl_history(e,0)->ticket&&
       basis.predecessor==sl_history(e,index+1)->ticket&&basis.serial==index+2);
}
/* Genuine producer to a selected original publication. Never directly dispatch
 * a caller ticket: only real resource pending/entry invokes the new source. */
static struct sl_trial *rt_prefix(unsigned bits,unsigned card,unsigned boundary)
{
    struct sl_trial *e=rt_make(bits,card);unsigned i;
    rt_begin();sl_preflight(e);sl_to_publication(e,0);assert(rt_end()==1);
    for(i=0;i<boundary;++i){rt_begin();sl_publish_current(e,i);sl_packet(e,i);sl_wait(e,i+1);assert(rt_end()==1);
       rt_due(e,i);rt_begin();rt_service_completed(e,i);
       if(i>=1)rt_retired_reuse(e,i-1);sl_to_publication(e,i+1);assert(rt_end()==1);}
    return e;
}
static void rt_finish(struct sl_trial *e)
{
    struct rt_hardware *h=&rt_current->hardware;unsigned i;
    rt_begin();sl_close(e);sl_same(e);
    assert(rt_current->source.source_closed&&e->port.ram.source_closed&&!h->entry&&
       h->releases==(rt_current->source.acquire_absent?0U:1U)&&!memcmp(&h->state,&h->original,sizeof(h->state)));
    for(i=0;i<4;++i)if(rt_current->source.event[i].original.known)assert(rt_current->source.event[i].original.quiet&&
       !rt_current->source.event[i].original.owned&&!rt_current->source.event[i].original.uncertain);
    assert(h->disabled&&h->forbidden);assert(rt_end()==1);assert(!h->disabled&&!h->forbidden);
    sl_drop(e);rt_current=NULL;
}
static void rt_four(unsigned bits,unsigned card,unsigned early)
{
    struct sl_trial *e=rt_make(bits,card);struct rt_hardware *h=&rt_current->hardware;
    unsigned i,chip=0,master=0,writes=0;struct pt_editor_mixed_reader_ref original_readers[32];
    rt_begin();sl_preflight(e);sl_to_publication(e,0);assert(rt_end()==1);
    for(i=0;i<4;++i){struct pt_private_pair_timer_hw_event sibling;
       rt_begin();if(i>=2){rt_retired_reuse(e,i-2);sl_to_publication(e,i);}
       sl_publish_current(e,i);sl_packet(e,i);
       assert(rt_current->source.slot_event[i&1U]==i+1&&rt_current->source.event[i].slot==(i&1U)&&
          rt_current->source.event[i].original.ticket==sl_history(e,i)->ticket&&
          rt_current->source.event[i].original.first==oracle(960U*(i+1)));
       if(!i){chip=e->base.chip.calls;master=e->base.masters.calls;
          writes=e->base.card->writes;for(unsigned j=0;j<32;++j)original_readers[j]=e->producer.original.reader[j].reference;}
       else{assert(e->base.chip.calls==chip&&e->base.masters.calls==master&&e->base.card->writes==writes);
          for(unsigned j=0;j<32;++j)assert(!memcmp(original_readers+j,&e->producer.original.reader[j].reference,sizeof(original_readers[j])));}
       if(i<3)sl_wait(e,i+1);assert(rt_end()==1);memcpy(&sibling,h->state.event+(1U-(i&1U)),sizeof(sibling));
       if(early){unsigned effects=e->port.ram.effects+e->port.ram.controls+e->port.ram.stops;
          rt_advance(h,oracle(960U*(i+1))-1);rt_early_wake(h,i&1U);
          assert(h->vector_result==1&&h->rearms==i+1&&h->acks==i&&
             h->state.event[i&1U].first==oracle(960U*(i+1))&&h->state.event[i&1U].armed&&
             effects==e->port.ram.effects+e->port.ram.controls+e->port.ram.stops);}
       rt_due(e,i);assert(!memcmp(&sibling,h->state.event+(1U-(i&1U)),sizeof(sibling)));
       if(i<3){rt_begin();rt_service_completed(e,i);
          if(i<1)sl_to_publication(e,i+1);assert(rt_end()==1);}
    }
    assert(e->port.ram.effects==16&&e->port.ram.controls==32&&e->port.ram.stops==16&&h->arms==4&&h->acks==4&&
       rt_current->source.arms_seen==4&&rt_current->source.slot_event[0]==3&&rt_current->source.slot_event[1]==4);
    rt_finish(e);
}
static void rt_old_quiet(void)
{
    struct sl_trial *e=rt_prefix(24,0,2);struct rt_hardware *h=&rt_current->hardware;
    struct pt_private_pair_timer_hw_event saved;unsigned cancellations=h->cancels,acks=h->acks;
    struct pt_private_mixed_causal_ram_adapter a=pt_private_rolling_timer_adapter(&rt_current->source);
    rt_begin();sl_publish_current(e,2);sl_packet(e,2);saved=h->state.event[0];
    assert(saved.armed&&saved.ticket==sl_history(e,2)->ticket);
    assert(a.ticket_quiet(a.context,&rt_current->source.registration,sl_history(e,0)->ticket,1)==1);
    assert(h->cancels==cancellations&&h->acks==acks&&!memcmp(&saved,h->state.event,sizeof(saved))&&
       rt_current->source.event[2].original.ticket==sl_history(e,2)->ticket);
    assert(a.arm_at(a.context,&rt_current->source.registration,sl_history(e,0)->ticket,oracle(4800),709379)==0);
    assert(rt_current->source.arms_seen==3&&h->arms==3&&rt_end()==1);rt_finish(e);
}
static void rt_stale(unsigned slot)
{
    struct sl_trial *e=rt_prefix(24,0,slot?3:2);struct rt_hardware *h=&rt_current->hardware;
    struct pt_private_pair_timer_hw_event saved;unsigned dispatches,acks,commits;
    rt_begin();sl_publish_current(e,slot?3:2);sl_packet(e,slot?3:2);assert(rt_end()==1);
    saved=h->state.event[slot];dispatches=rt_current->source.dispatches;acks=h->acks;commits=e->port.ram.commits;
    /* One isolated provider stale-delivery injection; later restore only that
     * exact hardware field for cancellation cleanup, never retry playback. */
    h->state.event[slot]=(struct pt_private_pair_timer_hw_event){sl_history(e,slot)->ticket,oracle(960U*(slot+1)),709379,0,1,1};
    rt_pump(h);assert(h->vector_result==-1&&rt_current->source.failed&&h->acks==acks&&
       rt_current->source.dispatches==dispatches&&e->port.ram.commits==commits&&
       rt_current->source.event[slot+2].original.ticket==saved.ticket&&rt_current->source.event[slot+2].original.owned&&
       rt_current->source.event[slot+2].original.uncertain);
    h->state.event[slot]=saved;rt_finish(e);
}
static void rt_cancel_at(unsigned boundary)
{
    struct sl_trial *e=rt_prefix(16,0,boundary);unsigned arms=rt_current->hardware.arms;
    rt_begin();sl_publish_current(e,boundary);assert(rt_current->hardware.arms==arms+1&&rt_end()==1);
    rt_finish(e);
}
static void rt_arm_refusal(unsigned mode)
{
    struct sl_trial *e=rt_prefix(24,0,0);struct rt_hardware *h=&rt_current->hardware;enum pt_mixed_readers_result r;
    h->arm_mode=mode;rt_begin();r=pt_editor_mixed_causal_song_rolling_publish(&e->producer);
    assert(r!=PT_MIXED_READERS_OK&&h->arms==1&&rt_current->source.arms_seen==1&&
       rt_current->source.event[0].original.arm_outcome==(mode==2?-1:0)&&
       rt_current->source.event[0].original.ticket==sl_history(e,0)->ticket);
    if(mode==3)assert(rt_current->source.event[0].original.quiet&&!rt_current->source.event[0].original.owned&&!rt_current->source.failed);
    else assert(rt_current->source.event[0].original.owned&&rt_current->source.event[0].original.uncertain&&rt_current->source.failed);
    assert(rt_end()==1);rt_finish(e);
}
static void rt_callback_mutation(unsigned at)
{
    struct sl_trial *e=rt_prefix(24,0,0);struct rt_hardware *h=&rt_current->hardware;
    uint64_t ticket=sl_history(e,0)->ticket;unsigned arms=h->arms,acks=h->acks;
    if(at<=3){h->mutation_at=at;rt_begin();assert(pt_editor_mixed_causal_song_rolling_publish(&e->producer)!=PT_MIXED_READERS_OK);
       assert(h->mutations==1&&rt_current->source.failed&&h->arms<=arms+1&&rt_end()==1);}
    else{rt_begin();sl_publish_current(e,0);sl_wait(e,1);assert(rt_end()==1);h->mutation_at=at;
       if(at==4){rt_advance(h,oracle(960)-1);rt_early_wake(h,0);assert(h->vector_result==-1&&h->acks==acks);}
       else if(at==5){rt_advance(h,oracle(960));assert(h->vector_result==-1&&h->acks==acks+1&&e->port.ram.commits==1);}
       else{assert(at==6);rt_begin();assert(pt_editor_mixed_causal_song_rolling_service_command(&e->producer,0,1)!=PT_MIXED_READERS_OK);assert(rt_end()==1);}
       assert(h->mutations==1&&rt_current->source.failed&&rt_current->source.event[0].original.ticket==ticket&&
          rt_current->source.event[0].original.owned&&rt_current->source.event[0].original.uncertain);}
    rt_finish(e);
}
static void rt_delayed_exclusion(void)
{
    struct sl_trial *e=rt_prefix(24,0,0);struct rt_hardware *h=&rt_current->hardware;
    rt_begin();sl_publish_current(e,0);sl_wait(e,1);rt_advance(h,oracle(960));
    assert(h->disabled&&h->forbidden&&!h->vectors&&!e->port.ram.effects&&h->state.event[0].pending&&h->state.event[0].queued);
    assert(sl_step(e,17)==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE&&rt_end()==1);
    assert(h->vectors==1&&h->vector_result==1&&e->port.ram.effects==16&&!h->disabled&&!h->forbidden);rt_finish(e);
}
static void rt_source_readonly(void)
{
    struct sl_trial *e=rt_prefix(24,0,0);struct rt_hardware *h=&rt_current->hardware;unsigned n=0,reads;
    h->release_unknown=1;rt_begin();assert(pt_editor_mixed_causal_song_rolling_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
    sl_commands(e,1);sl_readers(e,1);
    while(!h->releases){assert(!pt_editor_mixed_causal_song_rolling_close(&e->producer)&&++n<1000);sl_commands(e,1);sl_readers(e,1);}
    assert(h->releases==1&&rt_current->source.source_attempted&&!rt_current->source.source_closed&&
       e->producer.original.pin_count&&e->base.binding->preparation_context==sl_s(e));reads=h->reads;
    assert(!pt_editor_mixed_causal_song_rolling_close(&e->producer)&&h->releases==1&&h->reads>reads&&rt_end()==1);
    /* Provider completes its already-attempted restoration; later source quiet
     * only observes, never repeats release, and storage survives outer leave. */
    memcpy(&h->state,&h->original,sizeof(h->state));h->entry=NULL;rt_finish(e);
}
static void rt_constructor_refusals(void)
{
    struct sl_trial *e=rt_make(16,0);struct pt_private_pair_timer_ops ops=rt_current->source.ops;
    struct pt_private_mixed_causal_rolling_timer_source fresh={0},saved={0};
    unsigned char *source=calloc(1,sizeof(fresh)+1),*input=calloc(1,sizeof(ops)+1);
    assert(source&&input&&sizeof(uint64_t)>1);
    memcpy(input+1,&ops,sizeof(ops));
    assert(!pt_private_rolling_timer_init((struct pt_private_mixed_causal_rolling_timer_source *)(void *)(source+1),&ops));
    assert(!pt_private_rolling_timer_init(&fresh,(const struct pt_private_pair_timer_ops *)(const void *)(input+1)));
    assert(!pt_private_rolling_timer_init(&fresh,(const struct pt_private_pair_timer_ops *)(const void *)&fresh));
    assert(!pt_private_rolling_timer_task_enter((struct pt_private_mixed_causal_rolling_timer_source *)(void *)(source+1)));
    assert(!memcmp(&fresh,&saved,sizeof(fresh))&&!rt_current->hardware.reads&&!rt_current->hardware.clocks&&
       !rt_current->hardware.arms&&!rt_current->hardware.releases);
    free(source);free(input);sl_same(e);sl_drop(e);rt_current=NULL;
}
static void rt_acquisition_refusal(unsigned mode)
{
    struct sl_trial *e=rt_make(24,0);struct rt_hardware *h=&rt_current->hardware;
    struct pt_private_mixed_causal_rolling_timer_source *s=&rt_current->source;
    unsigned n=0;enum pt_editor_mixed_causal_song_lineage_result result;
    assert(mode<=3);h->acquire_clean0=mode==0;h->acquire_mode=mode;
    if(mode==2)h->mutation_at=7;
    rt_begin();sl_preflight(e);
    /* A genuine unconfirmed bind is deliberately a controller FAULT. The
     * producer observes that retained first_error before its ordinary REFUSED
     * branch. Require that exact path; do not relabel it or clear any fault. */
    do{result=sl_step(e,17);assert(++n<100000);
       if(!sl_s(e)->original_binding_called)assert(result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING);
       else assert(result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT);
    }while(!sl_s(e)->original_binding_called);
    assert(result==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT&&
       e->producer.original.first_error==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT&&
       sl_s(e)->first_error==PT_EDITOR_MIXED_READERS_FAULT&&sl_s(e)->result==PT_EDITOR_MIXED_READERS_FAULT&&
       sl_s(e)->original_binding_outcome==(mode?-1:0)&&
       sl_s(e)->original_binding_called&&!sl_s(e)->original_binding_confirmed&&
       sl_s(e)->causal&&sl_s(e)->queue&&!sl_s(e)->pool&&e->base.ordinary.calls==2&&cp_live(&e->base.ordinary)==2&&
       e->port.binds==1&&e->port.ram.bound&&s->bound&&s->acquire_attempted&&h->acquires==1&&
       !s->acquired&&!s->arms_seen&&!h->arms&&!h->acks&&!h->cancels&&!h->releases&&!h->vectors&&
       !e->base.chip.calls&&!e->base.card->writes&&e->producer.original.pin_count&&
       e->base.binding->preparation_context==sl_s(e)&&!memcmp(&s->before,&h->original,sizeof(s->before)));
    if(!mode)assert(s->acquire_outcome==0&&s->acquire_absent&&!s->acquire_unknown&&!s->failed&&
       !s->cookie&&!h->entry&&!memcmp(&h->state,&h->original,sizeof(h->state)));
    else assert(s->acquire_unknown&&s->failed&&!s->acquire_absent&&!s->baseline_invalid&&
       s->acquire_outcome==(mode==1?-1:1)&&s->cookie==h->state.cookie&&h->state.source==s&&
       h->state.entry==pt_private_rolling_timer_irq&&h->entry==pt_private_rolling_timer_irq&&h->state.vector_live&&
       !h->state.callbacks_inflight&&!h->state.restore_pending&&
       !memcmp(h->state.before_image,h->original.before_image,sizeof(h->state.before_image))&&
       h->mutations==(mode>=2?1U:0U));
    assert(!e->producer.original.attempted_mask&&!e->producer.original.lineage_index&&
       !e->port.ram.effects&&!e->port.ram.controls&&!e->port.ram.stops&&!e->port.ram.commits&&
       !pt_mixed_readers_commands_held(sl_s(e)->queue)&&!pt_mixed_readers_readers_held(sl_s(e)->queue)&&
       !h->state.callbacks_inflight&&!h->state.restore_pending&&!s->source_attempted&&!s->source_closed);
    for(unsigned i=0;i<4;++i){assert(!sl_history(e,i)->ticket&&!sl_history(e,i)->reference.serial&&
       !sl_history(e,i)->published&&!sl_history(e,i)->uncertain&&!sl_history(e,i)->detached&&!s->event[i].original.known);}
    for(unsigned i=0;i<32;++i)assert(!e->producer.original.reader[i].reference.serial);
    for(unsigned i=0;i<2;++i)assert(rt_absent(h->state.event+i));
    assert(rt_end()==1);
    /* Real bounded cleanup accepts the retained error identity. It requires
     * actual SOURCE quiet, consumed pins/borrow/factory controls, preserved
     * hardware baseline and full outer leave before freeing this storage.
     * Clean acquire0 performs no release; acquired unknowns release once. */
    rt_finish(e);
}
static void rt_release_mutation(unsigned mode)
{
    struct sl_trial *e=rt_prefix(24,0,3);struct rt_hardware *h=&rt_current->hardware;
    struct pt_private_mixed_causal_rolling_timer_source *s=&rt_current->source;
    uint64_t tickets[4];unsigned n=0,reads,probes,arms,acks,cancels,pins;
    struct pt_mixed_causal_registration original_registration;
    assert(mode==1||mode==2);
    rt_begin();sl_publish_current(e,3);sl_packet(e,3);assert(rt_end()==1);rt_due(e,3);rt_begin();
    while(pt_mixed_readers_commands_held(sl_s(e)->queue)||pt_mixed_readers_readers_held(sl_s(e)->queue)){
       sl_commands(e,0);sl_readers(e,0);assert(++n<1000);}
    assert(s->arms_seen==4&&h->arms==4&&h->acks==4&&!s->source_attempted&&
       e->port.ram.effects==16&&e->port.ram.controls==32&&e->port.ram.stops==16);
    for(unsigned i=0;i<4;++i){tickets[i]=s->event[i].original.ticket;
       assert(tickets[i]==sl_history(e,i)->ticket&&s->event[i].original.known&&s->event[i].original.quiet&&
          !s->event[i].original.owned&&!s->event[i].original.uncertain);}
    /* Retain exact original numeric identity while both controls are live.
     * Core actually consumes its queue before SOURCE close; that later NULL
     * does not invalidate the saved opaque registration or original leases. */
    memcpy(&original_registration,&s->registration,sizeof(original_registration));
    assert(original_registration.owner==sl_s(e)->causal&&original_registration.owner);
    assert(original_registration.queue==sl_s(e)->queue&&original_registration.queue);
    assert(original_registration.session==31&&original_registration.generation==17);
    h->release_mode=mode;if(mode==1)h->mutation_at=8;
    assert(pt_editor_mixed_causal_song_rolling_cancel(&e->producer)!=PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID);
    n=0;while(!h->releases){assert(!pt_editor_mixed_causal_song_rolling_close(&e->producer)&&++n<1000);
       sl_commands(e,1);sl_readers(e,1);}
    pins=e->producer.original.pin_count;
    assert(pins);
    assert(h->releases==1);
    assert(h->mutations==1);
    assert(s->failed);
    assert(s->source_attempted&&!s->source_closed);
    assert(!s->baseline_invalid);
    assert(!memcmp(&s->registration,&original_registration,sizeof(original_registration)));
    assert(s->registration.owner==original_registration.owner);
    assert(s->registration.queue==original_registration.queue);
    assert(s->registration.session==original_registration.session);
    assert(s->registration.generation==original_registration.generation);
    assert(sl_s(e)->causal==original_registration.owner&&sl_s(e)->causal);
    /* Actual guard_release publishes independent queue/pool NULL consumption
     * before release is observed. Never follow the former queue address. */
    assert(!sl_s(e)->queue);
    assert(!sl_s(e)->pool);
    assert(e->borrow->address==sl_s(e));
    assert(e->base.binding->preparation_context==sl_s(e));
    assert(!h->entry);
    assert(!memcmp(&h->state,&h->original,sizeof(h->state)));
    assert(!memcmp(&s->before,&h->original,sizeof(s->before)));
    for(unsigned i=0;i<4;++i)assert(s->event[i].original.ticket==tickets[i]&&s->event[i].original.known&&
       s->event[i].original.quiet&&!s->event[i].original.owned&&!s->event[i].original.uncertain);
    reads=h->reads;probes=s->quiet_probes;arms=h->arms;acks=h->acks;cancels=h->cancels;
    /* One subsequent explicit cleanup observes real completed restoration.
     * Core calls only source_quiet; release and its original obligations are
     * never retried. Storage still lives under the entire task exclusion. */
    assert(!pt_editor_mixed_causal_song_rolling_close(&e->producer));
    assert(s->source_closed);
    assert(s->quiet_probes>probes&&h->reads>reads);
    assert(h->releases==1&&h->arms==arms&&h->acks==acks&&h->cancels==cancels);
    assert(!memcmp(&s->registration,&original_registration,sizeof(original_registration)));
    assert(!sl_s(e)->causal&&!sl_s(e)->queue&&!sl_s(e)->pool);
    assert(e->producer.original.pin_count==pins);
    assert(e->base.binding->preparation_context==sl_s(e));
    assert(h->disabled&&h->forbidden);
    assert(rt_end()==1);rt_finish(e);
}
static void rt_exclusion_reentry(void)
{
    struct sl_trial *e=rt_prefix(24,0,0);struct rt_hardware *h=&rt_current->hardware;
    struct pt_private_mixed_causal_rolling_timer_source *s=&rt_current->source;
    unsigned restores=s->exclusion_restores,entries=s->task_entries;
    assert(!h->disabled&&!h->forbidden&&s->acquired&&!s->arms_seen);
    h->exclude_reentry=1;
    assert(pt_private_rolling_timer_task_enter(s)==-1&&!h->exclude_reentry&&s->failed&&
       !s->scope&&!s->exclusion_token&&!s->entry_busy&&!s->ops_busy&&!s->exclusion_unknown&&
       s->exclusion_refusals==1&&s->exclusion_restores==restores+1&&s->task_entries==entries&&
       !h->disabled&&!h->forbidden&&!h->arms&&!h->vectors);
    rt_finish(e);
}
static void rt_bind_invalid_input(unsigned which)
{
    struct sl_trial *e=rt_make(16,0);struct rt_hardware *h=&rt_current->hardware;
    h->bind_refusal=which;rt_begin();sl_preflight(e);sl_to_publication(e,0);
    assert(h->bind_refusals==1&&h->acquires==1&&rt_current->source.acquired&&
       sl_s(e)->original_binding_confirmed&&!h->arms&&!h->vectors&&rt_end()==1);
    rt_finish(e);
}
int main(void)
{
    sl_case="resource-mixed16";rt_four(16,0,0);sl_case="resource-mixed24";rt_four(24,0,0);
    sl_case="resource-card24";rt_four(24,1,0);sl_case="resource-early-all4";rt_four(24,0,1);
    sl_case="resource-old-quiet";rt_old_quiet();
    for(unsigned i=0;i<2;++i){sl_case="resource-stale-reused";rt_stale(i);}
    for(unsigned i=0;i<4;++i){sl_case="resource-cancel";rt_cancel_at(i);}
    for(unsigned i=1;i<=4;++i){sl_case="resource-arm-refusal";rt_arm_refusal(i);}
    for(unsigned i=1;i<=6;++i){sl_case="resource-callback-mutation";rt_callback_mutation(i);}
    sl_case="resource-whole-exclusion";rt_delayed_exclusion();sl_case="resource-source-readonly";rt_source_readonly();
    sl_case="resource-constructor-refusal";rt_constructor_refusals();
    for(unsigned i=0;i<4;++i){sl_case="resource-acquisition-refusal";rt_acquisition_refusal(i);}
    for(unsigned i=1;i<=2;++i){sl_case="resource-release-mutation";rt_release_mutation(i);}
    sl_case="resource-exclusion-reentry";rt_exclusion_reentry();
    for(unsigned i=1;i<=5;++i){sl_case="resource-bind-invalid-input";rt_bind_invalid_input(i);}
    assert(rt_cases==36);
    puts("ROLLING TIMER T16-C16-C16-S16 HOST FIXTURE PASS:36 cases;genuine rolling producer;actual task service and retirement before continuation;two independently quiet recycled slots;four retained original identities;ticket-free vectors;all4 EARLY original deadlines;stale delivery refusal;callback custody;clean-unknown-mutated acquisition;release mutation retained obligations and read-only quiet;aligned disjoint constructors and bind refusers;whole exclusion and primitive reentry;once-only source close and read-only later quiet;exact960-1920-2880-3840;HOST_RESOURCE_MODEL_ONLY");return 0;
}
