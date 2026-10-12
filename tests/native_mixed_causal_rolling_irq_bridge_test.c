/* Genuine producer/factory/controller/core HOST ABI-contract model. No native
 * vector, timer, MMIO, hardware, audio or scheduled frontend binding. */
#include <stdlib.h>
#include <stddef.h>
#define PT_PRIVATE_ROLLING_IRQ_HOST_MODEL 1
#include "../src/native/readers_ram/native_mixed_causal_rolling_irq_bridge.h"
static void *nb_calloc(size_t,size_t);
#define calloc nb_calloc
#define PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY inherited_rolling68_irq_donor_not_called
#include "native_mixed_causal_ram_rolling_song_test.c"
#undef PT_PRIVATE_ROLLING_RAM_FIXTURE_ENTRY
#undef calloc
struct nb_model {
    unsigned disabled,forbidden,excludes,restores,clocks;
    unsigned mutate_clock,reenter_clock,mutate_exclude,reenter_exclude,deferred,outer_live_checks,bind_refusal,bind_refusals;
    int delivered;
};
struct nb_carrier {
    struct sl_trial trial;
    struct pt_private_rolling_irq_bridge bridge;
    struct pt_private_native_rolling_irq32 envelope;
    struct nb_model model;
};
static struct nb_carrier *nb_current;
static unsigned nb_expand,nb_cases;
static void *nb_calloc(size_t count,size_t bytes)
{
    if(nb_expand){assert(count==1&&bytes==sizeof(struct sl_trial));nb_expand=0;
        nb_current=calloc(1,sizeof(*nb_current));assert(nb_current);
        assert(!offsetof(struct nb_carrier,trial));return &nb_current->trial;}
    return calloc(count,bytes);
}
static struct nb_carrier *nb_from_bridge(void *context)
{return (void *)((char *)context- offsetof(struct nb_carrier,bridge));}
static uint64_t nb_exclude(void *context,unsigned scope)
{
    struct nb_model *m=context;uint64_t token;
    assert(scope==PT_PRIVATE_ROLLING_IRQ_TASK||scope==PT_PRIVATE_ROLLING_IRQ_IRQ);
    token=((uint64_t)m->disabled<<32)|((uint64_t)m->forbidden<<1)|1U;
    ++m->disabled;if(scope==PT_PRIVATE_ROLLING_IRQ_TASK)++m->forbidden;++m->excludes;
    if(m->mutate_exclude){m->mutate_exclude=0;nb_current->envelope.ticket_lo^=1U;}
    if(m->reenter_exclude){m->reenter_exclude=0;
        assert(pt_private_rolling_irq_task_enter(&nb_current->bridge)==-1);}
    return token;
}
static void nb_restore(void *context,unsigned scope,uint64_t token)
{
    struct nb_model *m=context;struct nb_carrier *n=nb_current;
    assert(n&&m==&n->model&&token&&m->disabled&&
       n->bridge.self==&n->bridge&&n->bridge.envelope==&n->envelope);
    ++m->restores;m->disabled=(unsigned)(token>>32);m->forbidden=(unsigned)((token>>1)&0x7fffffffU);
    /* Demonstrate lifetime through the complete outer restore. Genuine deferred
     * delivery changes bridge/core state; the bridge must not roll that back. */
    if(scope==PT_PRIVATE_ROLLING_IRQ_TASK){++m->outer_live_checks;
        if(m->deferred){m->deferred=0;
            m->delivered=pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope);
            assert(n->bridge.self==&n->bridge&&n->bridge.envelope==&n->envelope);}}
}
static int nb_primitive_clock(void *context,uint64_t *ticks,uint32_t *frequency)
{
    struct nb_model *m=context;struct nb_carrier *n=nb_current;
    assert(n&&m==&n->model&&m->disabled);++m->clocks;
    if(m->mutate_clock==m->clocks){m->mutate_clock=0;n->envelope.ticket_lo^=1U;}
    if(m->reenter_clock){unsigned which=m->reenter_clock;m->reenter_clock=0;
        if(which==2){n->bridge.current=99;
            assert(pt_private_rolling_irq_task_enter(&n->bridge)==-1);}
        else if(which==3){struct pt_private_rolling_irq_bridge saved=n->bridge;
            assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_FAILED);
            /* Deliberately try to erase this callback's real guard fault. This
             * is an adversarial injection, not cleanup/quiet restoration; the
             * genuine RAM/core/owners and the hidden witness are not modified. */
            memcpy(&n->bridge,&saved,sizeof(saved));}
        else assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_FAILED);}
    return sl_adapter_clock(&n->trial.port.external,ticks,frequency);
}
static int nb_arm(void *context,const struct pt_mixed_causal_registration *r,
 uint64_t ticket,uint64_t first,uint32_t frequency)
{
    struct nb_carrier *n=nb_from_bridge(context);int raw;
    assert(n==nb_current&&n->model.disabled&&n->model.forbidden);
    if(!pt_private_rolling_irq_stage(&n->bridge,ticket,first,frequency))return 0;
    raw=sl_adapter_arm(&n->trial.port.external,r,ticket,first,frequency);
    return pt_private_rolling_irq_arm_result(&n->bridge,ticket,raw);
}
static int nb_quiet(void *context,const struct pt_mixed_causal_registration *r,uint64_t ticket,unsigned cancel)
{
    struct nb_carrier *n=nb_from_bridge(context);
    assert(n==nb_current&&n->model.disabled&&n->model.forbidden);
    return sl_adapter_quiet(&n->trial.port.external,r,ticket,cancel);
}
static int nb_close_source(void *context,const struct pt_mixed_causal_registration *r)
{
    struct nb_carrier *n=nb_from_bridge(context);
    assert(n==nb_current&&n->model.disabled&&n->model.forbidden);
    return sl_adapter_close(&n->trial.port.external,r);
}
static int nb_quiet_source(void *context,const struct pt_mixed_causal_registration *r)
{
    struct nb_carrier *n=nb_from_bridge(context);
    assert(n==nb_current&&n->model.disabled&&n->model.forbidden);
    return sl_adapter_source_quiet(&n->trial.port.external,r);
}
static void nb_bind_refuse(struct pt_private_mixed_causal_ram_port *p,
 const struct pt_mixed_causal_registration *r,unsigned which)
{
    struct nb_carrier *n=nb_current;struct pt_private_rolling_irq_bridge before=n->bridge;
    struct pt_private_native_rolling_irq32 image=n->envelope;
    struct pt_private_mixed_causal_ram_port port=*p;struct nb_model model=n->model;
    struct nb_registration_alignment {char byte;struct pt_mixed_causal_registration value;};
    struct nb_port_alignment {char byte;struct pt_private_mixed_causal_ram_port value;};
    struct pt_mixed_causal_registration bad_copy=*r;
    const struct pt_mixed_causal_registration *bad_r=r;struct pt_private_mixed_causal_ram_port *bad_p=p;
    unsigned char *storage=NULL;
    assert(which>=1&&which<=7&&!n->bridge.bound);
    if(which==1)bad_r=(const void *)&n->bridge;
    else if(which==2)bad_r=(const void *)&n->model;
    else if(which==3)bad_r=(const void *)p;
    else if(which==4){storage=calloc(1,sizeof(*r)+1);assert(storage);
        memcpy(storage+1,r,sizeof(*r));bad_r=(const void *)(storage+1);
        assert((uintptr_t)bad_r%offsetof(struct nb_registration_alignment,value));}
    else if(which==5){storage=calloc(1,sizeof(*p)+1);assert(storage);memcpy(storage+1,p,sizeof(*p));bad_p=(void *)(storage+1);
        assert((uintptr_t)bad_p%offsetof(struct nb_port_alignment,value));}
    else{bad_r=&bad_copy;
        /* Aligned invalid numeric views only, never actual owner controls or
         * a fabricated accepted registration/completion. */
        if(which==6)bad_copy.owner=(const void *)&bad_copy;else bad_copy.queue=(const void *)&bad_copy;}
    assert(!pt_private_rolling_irq_host_bind(&n->bridge,bad_p,bad_r)&&
       !memcmp(&n->bridge,&before,sizeof(before))&&!memcmp(&n->envelope,&image,sizeof(image))&&
       !memcmp(p,&port,sizeof(port))&&!memcmp(&n->model,&model,sizeof(model)));
    free(storage);++n->model.bind_refusals;
}
static int nb_bind(void *context,const struct pt_mixed_causal_registration *r)
{
    struct nb_carrier *n=nb_current;struct sl_trial *e=&n->trial;
    struct pt_editor_mixed_causal_prepare *s=sl_s(e);struct pt_private_mixed_causal_ram_port *p=context;
    assert(p==&e->port.ram&&e->producer.original.phase==PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER&&
       e->producer.original.busy&&e->producer.original.candidate_count==4&&e->producer.original.frames==3840&&
       e->producer.original.pin_count==e->base.editor->project->sample_count&&++e->port.binds==1&&
       e->base.ordinary.calls==2&&!s->pool&&s->causal&&s->queue&&r->owner==s->causal&&r->queue==s->queue&&
       r->session==31&&r->generation==17&&!pt_mixed_readers_commands_held(s->queue)&&!pt_mixed_readers_readers_held(s->queue));
    assert(pt_private_mixed_causal_ram_bind(p,s->causal,s->queue,r->session,r->generation));
    if(n->model.bind_refusal)nb_bind_refuse(p,r,n->model.bind_refusal);
    return pt_private_rolling_irq_host_bind(&n->bridge,p,r);
}
static struct sl_trial *nb_make(unsigned bits,unsigned card)
{
    struct sl_trial *e;struct pt_private_rolling_irq_ops ops;
    struct pt_private_rolling_irq_tags tags={0x100U,0x200U,0x300U,0x400U};
    struct pt_private_native_rolling_irq32 image;
    nb_expand=1;e=sl_make(bits,16,0,card?CS_CARD16:CS_MIXED,0);assert(!nb_expand&&e==&nb_current->trial);++nb_cases;
    image=nb_current->envelope;
    assert(pt_private_native_rolling_irq_dispatch(NULL)==PT_MIXED_CAUSAL_INVALID&&
       pt_private_native_rolling_irq_dispatch((void *)(uintptr_t)1)==PT_MIXED_CAUSAL_INVALID&&
       pt_private_native_rolling_irq_dispatch(&nb_current->envelope)==PT_MIXED_CAUSAL_INVALID&&
       !memcmp(&image,&nb_current->envelope,sizeof(image)));
    ops=(struct pt_private_rolling_irq_ops){&nb_current->model,sizeof(nb_current->model),nb_exclude,nb_restore,nb_primitive_clock};
    assert(pt_private_rolling_irq_host_init(&nb_current->bridge,&nb_current->envelope,&ops,&tags));
    assert(!e->port.ram.bound&&!e->producer.original.adopted&&!e->port.binds);
    memset(&e->port.ram,0,sizeof(e->port.ram));
    e->port.declaration=(struct pt_private_mixed_causal_ram_adapter){&nb_current->bridge,sizeof(nb_current->bridge),
       pt_private_rolling_irq_clock,nb_arm,nb_quiet,nb_close_source,nb_quiet_source};
    assert(pt_private_mixed_causal_ram_init_rolling(&e->port.ram,31,17,709379,&e->port.declaration,128,8));
    e->base.input.contexts=(struct pt_sampler_storage_span){nb_current,sizeof(*nb_current)};
    e->base.input.causal.port=pt_private_mixed_causal_ram_api(&e->port.ram);e->base.input.bind_original=nb_bind;
    e->source->original.original.contexts=e->base.input.contexts;
    e->producer.original.configuration.preparation=e->base.input;
    e->producer.port=pt_private_mixed_causal_ram_rolling_api(&e->port.ram);return e;
}
static void nb_begin(void)
{assert(pt_private_rolling_irq_task_enter(&nb_current->bridge)==1);}
static int nb_end(void)
{return pt_private_rolling_irq_task_leave(&nb_current->bridge);}
static struct sl_trial *nb_ready(unsigned bits,unsigned card)
{
    struct sl_trial *e=nb_make(bits,card);nb_begin();sl_preflight(e);sl_to_publication(e,0);assert(nb_end()==1);return e;
}
static void nb_finish(struct sl_trial *e)
{
    struct nb_carrier *n=nb_current;unsigned checks=n->model.outer_live_checks;
    nb_begin();sl_close(e);sl_same(e);
    assert(e->port.ram.source_closed&&n->bridge.self==&n->bridge&&n->bridge.envelope==&n->envelope&&
       n->bridge.registration.session==31&&n->bridge.registration.generation==17&&n->model.disabled&&n->model.forbidden);
    assert(nb_end()==(n->bridge.failed?-1:1));
    assert(!n->model.disabled&&!n->model.forbidden&&n->model.outer_live_checks==checks+1);
    sl_drop(e);nb_current=NULL;
}
static void nb_service(struct sl_trial *e,unsigned i)
{
    struct nb_carrier *n=nb_current;
    struct pt_editor_mixed_causal_song_lineage_history *h=sl_history(e,i);
    struct pt_editor_mixed_command_ref original=h->reference;
    struct pt_private_mixed_causal_ram_command *ram=sl_ram_command(e,i);
    struct pt_editor_mixed_reader_ref readers[32];
    struct pt_mixed_causal_rolling_basis basis;
    enum pt_mixed_readers_result serviced;
    uint64_t ticket=h->ticket;
    unsigned row=2,matches=0,j,proofs;
    unsigned chips=e->base.chip.calls,masters=e->base.masters.calls,writes=e->base.card->writes;
    unsigned pins=e->producer.original.pin_count,arms=e->port.external.arms;
    assert(i<4&&n&&n->bridge.scope==PT_PRIVATE_ROLLING_IRQ_TASK&&
       n->model.disabled&&n->model.forbidden&&h->published&&!h->detached&&
       original.serial&&ticket&&ram&&ram->known&&ram->identity.ticket==ticket&&ram->serial==i&&
       n->bridge.original[i].known&&n->bridge.original[i].completed&&
       !n->bridge.original[i].armed&&!n->bridge.original[i].uncertain&&
       n->bridge.original[i].identity.ticket==ticket);
    /* Controller row parity and allocator-selected command slot differ.
     * Resolve the genuine current row through its complete retained ref/ticket
     * before service, then retain and verify that exact row after NULL disposal. */
    for(j=0;j<2;++j)if(e->joint.rolling.row[j].ref.slot==original.slot&&
       e->joint.rolling.row[j].ref.serial==original.serial&&e->joint.rolling.row[j].ticket==ticket){row=j;++matches;}
    assert(matches==1&&row<2&&e->joint.rolling.row[row].serial==i&&row==(i&1U));
    for(j=0;j<32;++j)memcpy(readers+j,&e->producer.original.reader[j].reference,sizeof(readers[j]));
    serviced=pt_editor_mixed_causal_song_rolling_service_command(&e->producer,i,0);
    if(serviced!=PT_MIXED_READERS_OK)fprintf(stderr,"IRQ_SERVICE index=%u actual=%u ticket=%llu row=%u\n",
       i,(unsigned)serviced,(unsigned long long)ticket,row);
    assert(serviced==PT_MIXED_READERS_OK);
    assert(h->detached&&h->reference.slot==original.slot&&h->reference.serial==original.serial&&h->ticket==ticket);
    /* This flag is the genuine initial TRIGGER adoption/timing observation.
     * Production sets it only for i==0&&!cancel; CONTROL/STOP retain zero. */
    assert(sl_history(e,0)->service_seen&&h->service_seen==(i==0));
    assert(!sl_s(e)->command[original.slot].handle.address&&!sl_s(e)->command[original.slot].handle.token);
    assert(e->joint.rolling.row[row].detached&&e->joint.rolling.row[row].closed&&
       e->joint.rolling.row[row].serial==i&&e->joint.rolling.row[row].ticket==ticket&&
       e->joint.rolling.row[row].ref.slot==original.slot&&e->joint.rolling.row[row].ref.serial==original.serial);
    assert(ram->known&&ram->identity.ticket==ticket&&ram->serial==i&&ram->finished&&
       !ram->live&&!ram->armed&&!ram->uncertain&&ram->quiet_confirmed&&ram->rolling_ticket_raw==1&&ram->ticket_outcome==1);
    assert(!pt_mixed_readers_commands_held(sl_s(e)->queue)&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16);
    assert(e->producer.original.pin_count==pins&&e->borrow->address==sl_s(e)&&
       e->base.chip.calls==chips&&e->base.masters.calls==masters&&e->base.card->writes==writes&&e->port.external.arms==arms);
    for(j=0;j<32;++j)assert(!memcmp(readers+j,&e->producer.original.reader[j].reference,sizeof(readers[j])));
    /* The consumed numeric ticket cannot call the adapter or service an expired
     * handle. This is an observation of the owned queue, not repeat producer service. */
    proofs=e->port.external.proofs;
    assert(pt_mixed_readers_service_command(sl_s(e)->queue,ticket,0,NULL)==PT_MIXED_READERS_INVALID&&e->port.external.proofs==proofs);
    if(i<3){memset(&basis,0,sizeof(basis));
       assert(pt_mixed_causal_factory_rolling_basis(sl_s(e)->causal,&basis)&&
          basis.root==sl_history(e,0)->ticket&&basis.predecessor==ticket&&basis.serial==i+1);
       if(i)assert(basis.reuse_required&&basis.reuse==sl_history(e,i-1)->ticket&&sl_history(e,i-1)->detached);
       else assert(!basis.reuse_required);}
}
static void nb_four(unsigned bits,unsigned card,unsigned early,unsigned deferred)
{
    struct sl_trial *e=nb_ready(bits,card);struct nb_carrier *n=nb_current;
    struct pt_editor_mixed_reader_ref readers[32];unsigned chip=0,master=0,writes=0,i,j;
    for(i=0;i<4;++i){struct pt_private_rolling_irq_record *r=n->bridge.original+i;
        nb_begin();sl_publish_current(e,i);sl_packet(e,i);
        assert(n->bridge.staged==i+1&&r->known&&r->armed&&r->serial==i&&r->identity.ticket==sl_history(e,i)->ticket&&
           r->frame==960U*(i+1)&&r->first==oracle(960U*(i+1))&&r->last==oracle(960U*(i+1)+1)&&
           n->envelope.ticket_lo==(uint32_t)r->identity.ticket&&n->envelope.serial==i);
        if(!i){chip=e->base.chip.calls;master=e->base.masters.calls;writes=e->base.card->writes;
            for(j=0;j<32;++j)memcpy(readers+j,&e->producer.original.reader[j].reference,sizeof(readers[j]));}
        else{assert(e->base.chip.calls==chip&&e->base.masters.calls==master&&e->base.card->writes==writes);
            for(j=0;j<32;++j)assert(!memcmp(readers+j,&e->producer.original.reader[j].reference,sizeof(readers[j])));}
        if(i<3)sl_wait(e,i+1);
        if(deferred){e->port.external.ticks=r->first;n->model.deferred=1;assert(nb_end()==1);
            assert(n->model.delivered==PT_MIXED_CAUSAL_COMMITTED);}
        else{assert(nb_end()==1);
            if(early){unsigned effects=e->port.ram.effects+e->port.ram.controls+e->port.ram.stops;
                e->port.external.ticks=r->first-1;
                assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_EARLY&&
                   r->armed&&!r->completed&&n->envelope.state==PT_PRIVATE_ROLLING_IRQ_EARLY&&
                   r->first==oracle(960U*(i+1))&&effects==e->port.ram.effects+e->port.ram.controls+e->port.ram.stops);}
            e->port.external.ticks=r->first;
            assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_COMMITTED);}
        assert(r->completed&&!r->armed&&!r->uncertain&&r->dispatch_raw==PT_MIXED_CAUSAL_COMMITTED&&
           r->ledger_raw==PT_MIXED_CAUSAL_COMMITTED&&n->envelope.state==PT_PRIVATE_ROLLING_IRQ_COMPLETED&&
           e->port.ram.trace_count==3&&!n->bridge.scope&&!n->model.disabled&&!n->model.forbidden);
        nb_begin();nb_service(e,i);if(i<3)sl_to_publication(e,i+1);assert(nb_end()==1);
    }
    assert(e->port.ram.effects==16&&e->port.ram.controls==32&&e->port.ram.stops==16&&
       n->bridge.staged==4&&n->bridge.dispatches==(early?8U:4U)&&!e->port.ram.mask);
    for(i=0;i<4;++i)assert(n->bridge.original[i].identity.ticket==sl_history(e,i)->ticket&&
       n->bridge.original[i].first==oracle(960U*(i+1)));
    nb_finish(e);
}
static void nb_invalid_bind(unsigned which)
{
    struct sl_trial *e=nb_make(16,0);nb_current->model.bind_refusal=which;
    nb_begin();sl_preflight(e);sl_to_publication(e,0);
    assert(nb_current->model.bind_refusals==1&&nb_current->bridge.bound&&!e->port.external.arms&&nb_end()==1);nb_finish(e);
}
static void nb_constructor_overlap(unsigned partial)
{
    struct sl_trial *e=nb_make(16,0);struct nb_carrier *n=nb_current;
    struct pt_private_rolling_irq_bridge fresh={0},before=fresh;
    struct pt_private_native_rolling_irq32 image={0},saved=image;
    struct pt_private_rolling_irq_ops ops={&n->model,sizeof(n->model),nb_exclude,nb_restore,nb_primitive_clock};
    struct nb_ops_alignment {char byte;struct pt_private_rolling_irq_ops value;};
    struct nb_tags_alignment {char byte;struct pt_private_rolling_irq_tags value;};
    unsigned char *storage=calloc(1,sizeof(ops)+sizeof(struct pt_private_rolling_irq_tags));
    unsigned char *copy=malloc(sizeof(ops)+sizeof(struct pt_private_rolling_irq_tags));
    struct nb_model model=n->model;size_t offset=partial?offsetof(struct nb_ops_alignment,value):0;
    const struct pt_private_rolling_irq_tags *tags;
    assert(storage&&copy&&offset<sizeof(ops));memcpy(storage,&ops,sizeof(ops));
    memcpy(copy,storage,sizeof(ops)+sizeof(struct pt_private_rolling_irq_tags));tags=(const void *)(storage+offset);
    assert(!((uintptr_t)tags%offsetof(struct nb_tags_alignment,value))&&
       !pt_private_rolling_irq_host_init(&fresh,&image,(const void *)storage,tags)&&
       !memcmp(&fresh,&before,sizeof(before))&&!memcmp(&image,&saved,sizeof(saved))&&
       !memcmp(&n->model,&model,sizeof(model))&&
       !memcmp(copy,storage,sizeof(ops)+sizeof(struct pt_private_rolling_irq_tags)));
    free(copy);free(storage);nb_begin();sl_preflight(e);sl_to_publication(e,0);assert(nb_end()==1);nb_finish(e);
}
static void nb_clock_fault(unsigned post,unsigned reenter)
{
    struct sl_trial *e=nb_ready(24,0);struct nb_carrier *n=nb_current;
    nb_begin();sl_publish_current(e,0);sl_packet(e,0);sl_wait(e,1);assert(nb_end()==1);
    e->port.external.ticks=oracle(960);
    if(reenter)n->model.reenter_clock=reenter;else n->model.mutate_clock=n->model.clocks+(post?3U:1U);
    assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_FAILED&&
       n->bridge.failed&&n->bridge.original[0].uncertain&&!n->bridge.original[0].completed&&
       e->port.ram.failed&&e->port.ram.suppressed&&sl_history(e,0)->published&&!sl_history(e,0)->detached&&
       pt_mixed_readers_commands_held(sl_s(e)->queue)==1&&pt_mixed_readers_readers_held(sl_s(e)->queue)==16&&
       e->producer.original.pin_count&&e->borrow->address==sl_s(e)&&!n->bridge.scope&&!n->model.disabled);
    assert(e->port.ram.effects==(post?16U:0U)&&n->bridge.original[0].identity.ticket==sl_history(e,0)->ticket);
    /* No repair/rewind of actual adoption, histories, modes or failure health.
     * Only later genuine cancel/quiet callbacks may consume retained owners. */
    nb_finish(e);
}
static void nb_stale_envelope(void)
{
    struct sl_trial *e=nb_ready(16,0);struct nb_carrier *n=nb_current;uint32_t original;unsigned entries,excludes;
    nb_begin();sl_publish_current(e,0);sl_packet(e,0);sl_wait(e,1);assert(nb_end()==1);
    original=n->envelope.ticket_lo;entries=e->port.ram.entries;excludes=n->model.excludes;
    n->envelope.ticket_lo^=1U;
    assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_INVALID&&
       e->port.ram.entries==entries&&n->model.excludes==excludes&&!e->port.ram.effects&&
       n->bridge.original[0].identity.ticket==sl_history(e,0)->ticket);
    /* Exact test-injected scalar only, after refusal; no playback retry. */
    n->envelope.ticket_lo=original;nb_finish(e);
}
static void nb_exclusion_fault(unsigned reenter)
{
    struct sl_trial *e=nb_ready(16,0);struct nb_carrier *n=nb_current;unsigned entries,restores;
    nb_begin();sl_publish_current(e,0);sl_packet(e,0);sl_wait(e,1);assert(nb_end()==1);
    entries=e->port.ram.entries;restores=n->model.restores;e->port.external.ticks=oracle(960);
    if(reenter)n->model.reenter_exclude=1;else n->model.mutate_exclude=1;
    assert(pt_private_rolling_irq_host_entry(&n->bridge,&n->envelope)==PT_MIXED_CAUSAL_FAILED&&
       n->bridge.failed&&n->bridge.original[0].uncertain&&e->port.ram.entries==entries&&
       !e->port.ram.effects&&n->model.restores==restores+1&&!n->bridge.scope&&
       !n->bridge.ops_busy&&!n->bridge.busy&&!n->model.disabled&&!n->model.forbidden&&
       pt_mixed_readers_commands_held(sl_s(e)->queue)==1&&e->port.external.live[0]);
    nb_finish(e);
}
static void nb_arm_refusal(unsigned unknown)
{
    struct sl_trial *e=nb_ready(16,0);struct nb_carrier *n=nb_current;enum pt_mixed_readers_result result;
    nb_begin();e->port.external.arm_raw=unknown?-1:0;result=pt_editor_mixed_causal_song_rolling_publish(&e->producer);
    assert(n->bridge.staged==1&&n->bridge.original[0].arm_raw==(unknown?-1:0)&&!e->port.ram.effects&&
       e->port.external.arms==1&&!n->bridge.original[0].completed);
    if(unknown)assert(result!=PT_MIXED_READERS_OK&&result!=PT_MIXED_READERS_PENDING&&
       n->bridge.failed&&n->bridge.original[0].uncertain&&e->port.ram.failed&&e->port.external.live[0]);
    else assert(result==PT_MIXED_READERS_PENDING&&!n->bridge.failed&&!n->bridge.original[0].armed&&
       !n->bridge.original[0].uncertain&&!e->port.external.live[0]&&e->port.ram.command[0].quiet_confirmed);
    assert(nb_end()==(unknown?-1:1));nb_finish(e);
}
int main(void)
{
    sl_case="irq-bridge-mixed16";nb_four(16,0,0,0);
    sl_case="irq-bridge-mixed24";nb_four(24,0,0,0);
    sl_case="irq-bridge-card16";nb_four(16,1,0,0);
    sl_case="irq-bridge-card24";nb_four(24,1,0,0);
    sl_case="irq-bridge-all4-early";nb_four(24,0,1,0);
    sl_case="irq-bridge-outer-leave-delivery";nb_four(24,0,0,1);
    for(unsigned i=1;i<=7;++i){sl_case="irq-bridge-alias-alignment-bind";nb_invalid_bind(i);}
    sl_case="irq-bridge-constructor-full-overlap";nb_constructor_overlap(0);
    sl_case="irq-bridge-constructor-partial-overlap";nb_constructor_overlap(1);
    sl_case="irq-bridge-pre-clock-mutation";nb_clock_fault(0,0);
    sl_case="irq-bridge-post-clock-mutation-adoption-retained";nb_clock_fault(1,0);
    sl_case="irq-bridge-clock-reentry";nb_clock_fault(0,1);
    sl_case="irq-bridge-selector-mutation-reentry";nb_clock_fault(0,2);
    sl_case="irq-bridge-callback-fault-rewind";nb_clock_fault(0,3);
    sl_case="irq-bridge-stale-envelope";nb_stale_envelope();
    sl_case="irq-bridge-exclusion-mutation";nb_exclusion_fault(0);
    sl_case="irq-bridge-exclusion-reentry";nb_exclusion_fault(1);
    sl_case="irq-bridge-clean-arm0";nb_arm_refusal(0);
    sl_case="irq-bridge-unknown-arm";nb_arm_refusal(1);
    assert(nb_cases==25);
    puts("ROLLING IRQ BRIDGE HOST FIXTURE PASS:25 cases;genuine producer-factory-controller-core;fixed80-byte envelope with explicit HOST address tags;four original identities and absolute windows;actual quiet-service-retirement before continuation;full20 and original16 leases;EARLY unchanged original deadline;whole exclusion and outer-leave lifetime;stale-alias-alignment refusal;clock-exclusion-selector mutation-reentry and fault-rewind refusal retain adoption and ownership;clean0 and unknown arm separate;native entry inhibited before payload reads;exact960-1920-2880-3840;HOST_CONTRACT_MODEL_ONLY");return 0;
}
