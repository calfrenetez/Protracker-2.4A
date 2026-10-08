/* Private SOURCE proposal; no test/native run has occurred. The hardware
 * resource model drives the actual adapter entry without a caller ticket.
 * Only cp_make's first allocation expands; genuine allocators/core stay intact. */
#include <stdlib.h>
#include <stddef.h>
#define PT_PRIVATE_PAIR_TIMER_HOST_RESOURCE_MODEL 1
#include "../src/native/readers_ram/native_mixed_causal_pair_timer_source.h"
static void *ts_calloc(size_t,size_t);
#define calloc ts_calloc
#define PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN inherited_timer_preparation_suite_not_called
#include "editor_mixed_causal_prepare_test.c"
#undef PT_EDITOR_MIXED_CAUSAL_PREPARE_TEST_MAIN
#undef calloc

struct ts_hardware {
    struct pt_private_pair_timer_hw_state state,original;
    int (*entry)(void *);
    struct pt_private_mixed_causal_ram_port *port;
    uint64_t now;
    uint32_t frequency;
    unsigned disabled,forbidden,arms,rearms,acks,cancels,releases,reads,clocks,vectors;
    unsigned arm_mode,release_unknown,read_failure,exclude_reentry,acquire_clean0;
    int vector_result;
};
struct ts_carrier {
    struct cp_trial trial;
    struct pt_private_mixed_causal_ram_port port;
    struct pt_private_mixed_causal_pair_timer_source source;
    struct ts_hardware hardware;
    void *poison;
    unsigned poisoned;
};
static struct ts_carrier *ts_current;
static unsigned ts_expand,ts_cases;
static void *ts_calloc(size_t count,size_t bytes)
{
    if(ts_expand){assert(count==1&&bytes==sizeof(struct cp_trial));ts_expand=0;
        ts_current=calloc(1,sizeof(*ts_current));assert(ts_current);
        assert(offsetof(struct ts_carrier,trial)==0);return &ts_current->trial;}
    return calloc(count,bytes);
}
static int ts_absent(const struct pt_private_pair_timer_hw_event *e)
{return !e->armed&&!e->pending&&!e->queued;}
static void ts_pump(struct ts_hardware *h)
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
static void ts_advance(struct ts_hardware *h,uint64_t now)
{
    unsigned i;assert(now>=h->now);h->now=now;
    for(i=0;i<2;++i){struct pt_private_pair_timer_hw_event *e=h->state.event+i;
        if(e->armed&&now>=e->first){e->armed=0;e->pending=e->queued=1;}}
    ts_pump(h);
}
static void ts_early_wake(struct ts_hardware *h,unsigned slot)
{
    struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(slot<2&&e->armed&&h->now<e->first);
    e->armed=0;e->pending=e->queued=1;ts_pump(h);
}
static uint64_t ts_exclude(void *context,unsigned scope)
{
    struct ts_hardware *h=context;uint64_t token;
    assert(scope==PT_PRIVATE_PAIR_TIMER_TASK||scope==PT_PRIVATE_PAIR_TIMER_IRQ);
    token=((uint64_t)h->disabled<<32)|((uint64_t)h->forbidden<<1)|1U;
    ++h->disabled;if(scope==PT_PRIVATE_PAIR_TIMER_TASK)++h->forbidden;
    if(h->exclude_reentry){h->exclude_reentry=0;
        assert(pt_private_pair_timer_task_enter(h->state.source)==-1);}
    return token;
}
static void ts_restore(void *context,unsigned scope,uint64_t token)
{
    struct ts_hardware *h=context;
    assert(token&&h->disabled&&scope>=PT_PRIVATE_PAIR_TIMER_TASK&&scope<=PT_PRIVATE_PAIR_TIMER_IRQ);
    h->disabled=(unsigned)(token>>32);h->forbidden=(unsigned)((token>>1)&0x7fffffffU);
    ts_pump(h);
}
static int ts_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct ts_hardware *h=context;assert(h->disabled);++h->clocks;
    *ticks=h->now;*frequency=h->frequency;return 1;
}
static int ts_state(void *context,struct pt_private_pair_timer_hw_state *out)
{
    struct ts_hardware *h=context;assert(h->disabled);++h->reads;
    if(h->read_failure){--h->read_failure;return 0;}
    memcpy(out,&h->state,sizeof(*out));out->entry=h->entry;return 1;
}
static int ts_acquire(void *context,void *source,int (*entry)(void *),
 const struct pt_private_pair_timer_hw_state *before,uint64_t *cookie)
{
    struct ts_hardware *h=context;assert(h->disabled&&source&&entry);
    assert(!memcmp(before,&h->original,sizeof(*before))&&!h->state.source&&!h->entry);
    if(h->acquire_clean0){*cookie=0;return 0;}
    h->state.source=source;h->state.entry=entry;h->state.cookie=0x314159U;h->state.vector_live=1;
    h->entry=entry;*cookie=h->state.cookie;return 1;
}
static int ts_arm(void *context,uint64_t cookie,unsigned slot,uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct ts_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&cookie==h->state.cookie&&slot<2&&ticket&&ts_absent(e));++h->arms;
    memset(e,0,sizeof(*e));e->ticket=ticket;e->first=first;e->frequency=frequency;e->armed=1;
    if(h->arm_mode==2){assert(pt_private_mixed_causal_ram_dispatch(h->port,ticket)==PT_MIXED_CAUSAL_INVALID);}
    return h->arm_mode?0:1; /* Modes1/2 have genuine NEW resource effects. */
}
static int ts_rearm(void *context,uint64_t cookie,unsigned slot,uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct ts_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&cookie==h->state.cookie&&slot<2&&e->ticket==ticket&&e->first==first&&e->frequency==frequency);
    assert(e->pending&&e->queued&&h->now<first);++h->rearms;
    e->armed=1;e->pending=e->queued=0;return 1;
}
static int ts_ack(void *context,uint64_t cookie,unsigned slot,uint64_t ticket)
{
    struct ts_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&cookie==h->state.cookie&&slot<2&&e->ticket==ticket&&e->pending);
    ++h->acks;memset(e,0,sizeof(*e));return 1;
}
static int ts_cancel(void *context,uint64_t cookie,unsigned slot,uint64_t ticket)
{
    struct ts_hardware *h=context;struct pt_private_pair_timer_hw_event *e=h->state.event+slot;
    assert(h->disabled&&!h->state.callbacks_inflight&&cookie==h->state.cookie&&slot<2&&e->ticket==ticket);
    ++h->cancels;memset(e,0,sizeof(*e));return 1;
}
static int ts_release(void *context,void *source,uint64_t cookie,const struct pt_private_pair_timer_hw_state *before)
{
    struct ts_hardware *h=context;unsigned i;
    assert(h->disabled&&source==h->state.source&&cookie==h->state.cookie&&++h->releases==1);
    assert(!h->state.callbacks_inflight&&!memcmp(before,&h->original,sizeof(*before)));
    for(i=0;i<2;++i)assert(ts_absent(h->state.event+i));
    memcpy(&h->state,before,sizeof(h->state));h->entry=NULL;
    if(h->release_unknown){h->read_failure=1;return -1;}
    return 1;
}
static int ts_bind(void *context,const struct pt_mixed_causal_registration *r)
{
    struct pt_private_mixed_causal_ram_port *p=context;struct cp_trial *f=&ts_current->trial;
    assert(p==&ts_current->port&&++f->port.bind_calls==1&&f->ordinary.calls==2);
    assert(!f->control.pool&&!f->chip.calls&&!f->card->writes);
    assert(r->owner==f->control.causal&&r->queue==f->control.queue&&r->session==31&&r->generation==17);
    assert(pt_private_mixed_causal_ram_bind(p,f->control.causal,f->control.queue,r->session,r->generation));
    return pt_private_pair_timer_bind(&ts_current->source,p,r);
}
static void ts_release_command(void *context,void *block)
{
    struct cp_memory *m=context;unsigned i;
    if(block==ts_current->poison){
        for(i=0;i<80&&m->live[i].p!=block;++i){}assert(i<80&&m->live[i].n);
        memset(block,0xa5,m->live[i].n);ts_current->poison=NULL;++ts_current->poisoned;}
    cp_release(context,block);
}
static struct cp_trial *ts_make(void)
{
    struct cp_trial *f;struct ts_hardware *h;struct pt_private_pair_timer_ops ops;
    struct pt_private_mixed_causal_ram_adapter adapter;unsigned i;
    ts_expand=1;f=cp_make(24,16,0);assert(!ts_expand&&f==&ts_current->trial);++ts_cases;
    h=&ts_current->hardware;h->port=&ts_current->port;h->now=100;h->frequency=709379;
    h->state.available=1;for(i=0;i<8;++i)h->state.before_image[i]=0xdead0000U+i;
    memcpy(&h->original,&h->state,sizeof(h->original));
    ops=(struct pt_private_pair_timer_ops){h,sizeof(*h),PT_PRIVATE_PAIR_TIMER_OPS_VERSION,PT_PRIVATE_PAIR_TIMER_REQUIRED,
        ts_exclude,ts_restore,ts_clock,ts_state,ts_acquire,ts_arm,ts_rearm,ts_ack,ts_cancel,ts_release};
    assert(pt_private_pair_timer_init(&ts_current->source,&ops));
    adapter=pt_private_pair_timer_adapter(&ts_current->source);
    assert(pt_private_mixed_causal_ram_init(&ts_current->port,31,17,709379,&adapter,64,8));
    f->input.contexts.bytes=sizeof(*ts_current);f->input.causal.port=pt_private_mixed_causal_ram_api(&ts_current->port);
    f->input.bind_original=ts_bind;f->input.causal.allocator.release=ts_release_command;
    return f;
}
static void ts_begin(void)
{assert(pt_private_pair_timer_task_enter(&ts_current->source)==1);}
static int ts_end(void)
{return pt_private_pair_timer_task_leave(&ts_current->source);}
static void ts_pair(struct cp_trial *f)
{
    ts_begin();cp_open(f);cp_requests(f,6);cp_prepare(f,0,960);cp_refs(f,0,0);assert(cp_admit(f,0));
    cp_prepare(f,1,1920);cp_refs(f,1,16);assert(cp_admit(f,1));
    assert(ts_current->source.event[0].first==oracle(960)&&ts_current->source.event[1].first==oracle(1920));
    assert(ts_current->hardware.arms==2&&!ts_current->port.effects&&ts_end()==1);
}
static void ts_observe(struct cp_trial *f,unsigned base,uint64_t tick)
{
    unsigned i;struct pt_private_mixed_causal_ram_port *p=&ts_current->port;
    for(i=0;i<6;++i){struct pt_editor_mixed_reader_record *r=f->control.reader+f->reader[base+i].slot;
        struct pt_mixed_readers_reader_receipt receipt;
        assert(pt_mixed_causal_service_reader(f->control.causal,r->ticket,r->action,0,&receipt)==PT_MIXED_READERS_PENDING);
        assert(receipt.state==PT_MIXED_READER_ACTIVE&&receipt.adoption==PT_MIXED_ADOPTED);
        assert(receipt.observed==tick&&receipt.issued==tick&&keys_equal(&receipt.key,p->command[base?1:0].packet.key+i));}
}
static void ts_finish(struct cp_trial *f)
{
    struct ts_hardware *h=&ts_current->hardware;unsigned i;
    ts_begin();if(!pt_editor_mixed_causal_prepare_close(&f->control))assert(pt_editor_mixed_causal_prepare_close(&f->control));
    assert(!f->control.causal&&!f->control.queue&&!f->control.pool&&!f->binding->preparation_context);
    assert(ts_current->source.source_closed&&ts_current->port.source_closed&&
        h->releases==(ts_current->source.acquire_absent?0U:1U)&&!h->entry);
    assert(!memcmp(&h->state,&h->original,sizeof(h->state))&&!ts_current->port.mask);
    for(i=0;i<2;++i)assert(!ts_current->port.command[i].live&&!ts_current->port.command[i].owner&&ts_absent(h->state.event+i));
    for(i=0;i<32;++i)assert(!f->control.reader[i].handle.address&&!ts_current->port.reader[i].live);
    cp_same(f);assert(!cp_live(&f->ordinary)&&!cp_live(&f->chip));assert(ts_end()==1);
    /* SOURCE and all child ownership are positively quiet before terminal
     * facade disposal/editor detach. cp_drop's inherited suite is UNCALLED. */
    cp_drop(f);ts_current=NULL;
}
static void ts_drain(struct cp_trial *f,unsigned failed)
{cp_drain_command(f,0,failed);cp_drain_command(f,1,failed);cp_drain_readers(f,0,6,failed);cp_drain_readers(f,16,6,failed);}
static void ts_success(void)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;
    struct pt_private_mixed_causal_ram_port *p=&ts_current->port;struct pt_private_pair_timer_hw_event second;
    unsigned vectors;void *reader;ts_pair(f);memcpy(&second,h->state.event+1,sizeof(second));
    ts_advance(h,oracle(960));assert(h->vectors==1&&h->vector_result==1&&p->commits==1&&p->effects==6);
    assert(p->command[1].predecessor_completed&&p->command[1].predecessor_observed==oracle(960));
    assert(p->command[1].predecessor_issued==oracle(960)&&!memcmp(&second,h->state.event+1,sizeof(second)));
    ts_begin();ts_observe(f,0,oracle(960));reader=f->control.reader[f->reader[0].slot].handle.address;
    ts_current->poison=f->control.command[f->command[0].slot].handle.address;cp_drain_command(f,0,0);
    assert(ts_current->poisoned==1&&!ts_current->poison&&!p->command[0].owner&&!p->command[0].live);
    assert(ts_absent(h->state.event)&&h->state.event[1].armed&&p->command[1].armed);
    assert(pt_editor_mixed_causal_prepare_service_reader(&f->control,f->reader[0],0,NULL)==PT_MIXED_READERS_PENDING);
    assert(f->control.reader[f->reader[0].slot].handle.address==reader&&!memcmp(&second,h->state.event+1,sizeof(second)));
    assert(ts_end()==1);vectors=h->vectors;ts_advance(h,oracle(1920));
    assert(h->vectors==vectors+1&&h->vector_result==1&&p->commits==2&&p->effects==12);
    ts_begin();ts_observe(f,16,oracle(1920));cp_drain_command(f,1,0);
    cp_drain_readers(f,0,6,0);cp_drain_readers(f,16,6,0);assert(ts_end()==1);ts_finish(f);
}
static void ts_early(void)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;
    struct pt_mixed_causal_packet saved[2];ts_pair(f);
    memcpy(saved,&ts_current->port.command[0].packet,sizeof(saved[0]));
    memcpy(saved+1,&ts_current->port.command[1].packet,sizeof(saved[1]));
    ts_advance(h,oracle(960)-1);ts_early_wake(h,0);
    assert(h->vector_result==1&&h->rearms==1&&h->acks==0&&!ts_current->port.effects);
    assert(ts_current->source.event[0].fire_outcome==PT_MIXED_CAUSAL_EARLY&&h->state.event[0].first==oracle(960));
    assert(h->state.event[0].armed&&!h->state.event[0].pending&&!h->state.event[0].queued);
    assert(!memcmp(saved,&ts_current->port.command[0].packet,sizeof(saved[0]))&&!memcmp(saved+1,&ts_current->port.command[1].packet,sizeof(saved[1])));
    ts_advance(h,oracle(960));ts_advance(h,oracle(1920));
    assert(ts_current->port.commits==2&&h->acks==2&&h->rearms==1);
    ts_begin();ts_drain(f,0);assert(ts_end()==1);ts_finish(f);
}
static void ts_pending_excluded(unsigned prior)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;ts_pair(f);
    h->disabled=prior;h->forbidden=7;ts_begin();ts_advance(h,oracle(960));
    assert(h->state.event[0].pending&&h->state.event[0].queued&&!h->vectors&&!ts_current->port.effects);
    assert(h->disabled==prior+1&&h->forbidden==8&&ts_end()==1);
    assert(h->disabled==prior&&h->forbidden==7);
    if(prior){assert(!h->vectors&&!ts_current->port.effects);h->disabled=0;ts_pump(h);}
    assert(h->vectors==1&&ts_current->port.commits==1);
    assert(!memcmp(h->state.before_image,h->original.before_image,sizeof(h->state.before_image)));
    ts_advance(h,oracle(1920));ts_begin();ts_drain(f,0);assert(ts_end()==1);ts_finish(f);
}
static void ts_arm_unknown(unsigned mode)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;uint64_t ticket;
    ts_begin();cp_open(f);cp_requests(f,6);cp_prepare(f,0,960);cp_refs(f,0,0);h->arm_mode=mode;
    assert(pt_editor_mixed_causal_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_causal_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_BACKEND);
    assert(ts_current->source.event[0].arm_outcome==0&&ts_current->source.event[0].owned&&ts_current->source.event[0].uncertain);
    assert(h->state.event[0].armed&&ts_current->port.command[0].uncertain&&f->control.command[f->command[0].slot].handle.address);
    cp_drain_command(f,0,1);cp_drain_readers(f,0,6,1);assert(h->cancels==1&&!ts_current->port.effects);
    assert(ts_current->source.failed&&ts_current->port.failed);
    assert(ts_end()==1); /* Exact restoration adds no NEW failure. */
    ts_finish(f);
}
static void ts_first_due_before_successor(void)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;
    ts_begin();cp_open(f);cp_requests(f,6);cp_prepare(f,0,960);cp_refs(f,0,0);assert(cp_admit(f,0));assert(ts_end()==1);
    ts_advance(h,oracle(960));
    assert(h->vectors==1&&h->vector_result==-1&&ts_current->port.fires==1&&!ts_current->port.effects);
    assert(ts_current->source.event[0].terminal&&ts_current->source.event[0].owned&&ts_current->port.failed);
    ts_begin();cp_drain_command(f,0,1);cp_drain_readers(f,0,6,1);assert(ts_end()==1);ts_finish(f);
}
static void ts_unknown_source(void)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;unsigned reads,releases,arms,cancels,acks;
    ts_begin();cp_open(f);h->release_unknown=1;
    assert(!pt_editor_mixed_causal_prepare_close(&f->control));
    assert(h->releases==1&&!h->entry&&ts_current->source.source_attempted&&!ts_current->source.source_closed);
    assert(f->binding->preparation_context==&f->control&&f->control.causal);
    reads=h->reads;releases=h->releases;arms=h->arms;cancels=h->cancels;acks=h->acks;
    (void)pt_editor_mixed_causal_prepare_close(&f->control);
    assert(h->reads>reads&&h->releases==releases&&h->arms==arms&&h->cancels==cancels&&h->acks==acks);
    assert(ts_current->source.source_closed&&ts_current->port.source_closed&&ts_end()==1);ts_finish(f);
}
static void ts_exclusion_reentry(void)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;
    ts_begin();cp_open(f);assert(ts_end()==1);h->disabled=3;h->forbidden=5;h->exclude_reentry=1;
    assert(pt_private_pair_timer_task_enter(&ts_current->source)==-1);
    assert(h->disabled==3&&h->forbidden==5&&!ts_current->source.scope&&!ts_current->source.exclusion_token);
    assert(!ts_current->source.exclusion_unknown&&ts_current->source.exclusion_refusals==1);
    h->disabled=0;ts_finish(f);
}
static void ts_empty_clean_acquire(void)
{
    struct cp_trial *f=ts_make();struct ts_hardware *h=&ts_current->hardware;
    h->acquire_clean0=1;ts_begin();
    assert(pt_editor_mixed_causal_prepare_begin(&f->control,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(pt_editor_mixed_causal_prepare_advance_validation(&f->control,7)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(f->control.original_binding_called&&!f->control.original_binding_confirmed&&f->control.original_binding_outcome==0);
    assert(f->control.causal&&f->control.queue&&!f->control.pool&&cp_live(&f->ordinary)==2);
    assert(ts_current->source.bound&&ts_current->source.acquire_absent&&!ts_current->source.source_attempted);
    assert(!h->entry&&!h->releases&&!memcmp(&h->state,&h->original,sizeof(h->state)));
    assert(ts_end()==1);ts_finish(f);
}
int main(void)
{
    ts_success();ts_early();ts_pending_excluded(0);ts_pending_excluded(2);
    ts_arm_unknown(1);ts_arm_unknown(2);ts_first_due_before_successor();ts_unknown_source();ts_exclusion_reentry();ts_empty_clean_acquire();
    assert(ts_cases==10);
    puts("CAUSAL PAIR TIMER RESOURCE MODEL PASS:10 genuine owner cases;ticket-free due IRQ,original960/1920,independent first C and retained R/successor,EARLY absolute rearm,whole exclusion/pending/prior state,reentrant NEW raw0 UNKNOWN,first due before successor,once unknown source removal then read-only quiet,empty clean0 acquisition closure;HOST_MODEL_ONLY");
    return 0;
}
