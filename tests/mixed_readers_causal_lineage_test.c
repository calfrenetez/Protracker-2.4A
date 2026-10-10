/* SOURCE fixture only, not executed. Production C compiles separately.
 * Reuse unchanged genuine master/Chip/card-cache/holder and paired-port helpers.
 * Their old mains remain defined and uncalled; no baseline pass is manufactured. */
#include "mixed_readers_causal_stop_fixture_support.inc"
#include "../src/core/mixed_readers_causal_lineage_internal.h"

struct tl_port {
    struct cc_port original;
    struct pt_mixed_causal_completed_value root,control;
    struct pt_mixed_causal_command_identity third_identity;
    unsigned control_completed,third_publications,hold_control,hold_third;
    unsigned third_mutation,third_reentry;int third_raw;
};
struct tl_case {
    struct trial *trial;struct tl_port port;struct ct_memory memory;
    struct pt_mixed_causal_config config;struct pt_mixed_causal_owner *owner;
    void *workspace;uint8_t *before;size_t before_bytes;
};
static unsigned tl_cases;
static struct pt_mixed_causal_owner *tl_fail_owner;
static int tl_current_fault(void *context,uint64_t token,uint64_t generation)
{
    int actual=held_current(context,token,generation);
    if(tl_fail_owner){struct pt_mixed_causal_owner *owner=tl_fail_owner;
        tl_fail_owner=NULL;pt_mixed_causal_fail_closed(owner);}
    return actual;
}
static void tl_terminal_fault(void *context,uint64_t token,int valid)
{
    held_terminal(context,token,valid);
    if(tl_fail_owner){struct pt_mixed_causal_owner *owner=tl_fail_owner;
        tl_fail_owner=NULL;pt_mixed_causal_fail_closed(owner);}
}
static int tl_completed_same(const struct pt_mixed_causal_completed_value *a,
    const struct pt_mixed_causal_completed_value *b)
{
    unsigned i;
    if(!cc_identity(&a->identity,&b->identity)||a->frame!=b->frame||a->first_tick!=b->first_tick||
       a->last_tick!=b->last_tick||a->observed!=b->observed||a->issued!=b->issued||
       a->post.active_mask!=b->post.active_mask||a->post.adopted_mask!=b->post.adopted_mask)return 0;
    for(i=0;i<20;++i)if(!keys_equal(a->post.slot+i,b->post.slot+i))return 0;
    return 1;
}
static void tl_capture_completed(struct pt_mixed_causal_completed_value *out,
    const struct pt_mixed_causal_command_identity *identity,
    const struct pt_mixed_causal_packet *packet,uint64_t observed,uint64_t issued,
    const struct pt_mixed_causal_actual *actual)
{
    memset(out,0,sizeof(*out));memcpy(&out->identity,identity,sizeof(*identity));
    out->frame=packet->frame;out->first_tick=packet->first;out->last_tick=packet->last;
    out->observed=observed;out->issued=issued;memcpy(&out->post,actual,sizeof(*actual));
}
static int tl_publish_stop(void *context,struct pt_mixed_causal_owner *owner,
    const struct pt_mixed_causal_control_stop_publication *d)
{
    struct tl_port *p=context;struct cc_port *old=&p->original;unsigned i;int raw=p->third_raw;
    ++p->third_publications;
    /* Independently stored port completions and current registry, not the
     * owner's copied values, authorize this model's third publication. */
    if(!old->first_completed||!p->control_completed||owner!=old->base.registration.owner||d->serial!=2||
       !tl_completed_same(&d->root,&p->root)||!tl_completed_same(&d->control,&p->control)||
       d->control.identity.ticket==d->root.identity.ticket||d->control.frame<=d->root.frame||
       d->packet.frame<=d->control.frame||!ct_expected(&old->base,&d->packet)||
       d->control.post.active_mask!=old->base.mask||d->control.post.adopted_mask!=old->base.mask)return 0;
    for(i=0;i<20;++i)if(!keys_equal(d->control.post.slot+i,old->base.slot+i))return 0;
    for(i=0;i<d->packet.count;++i){const struct pt_mixed_readers_action *a=d->packet.action+i;
        int zero=a->route==PT_MIXED_READERS_PAULA?
            !a->geometry.paula.data&&!a->geometry.paula.words&&!a->geometry.paula.period&&!a->geometry.paula.volume:
            !a->geometry.amigus.start&&!a->geometry.amigus.loop&&!a->geometry.amigus.end_exclusive&&
            !a->geometry.amigus.rate&&!a->geometry.amigus.control&&!a->geometry.amigus.left&&!a->geometry.amigus.right;
        if(a->kind!=PT_MIXED_READERS_STOP||!zero||
           d->packet.key[i].trigger!=p->root.identity.ticket||!ct_reader(&old->base,d->packet.key+i))return 0;
    }
    if(raw!=0&&!ct_publish(&old->base,owner,&d->stop,&d->packet))return 0;
    if(raw!=0)memcpy(&p->third_identity,&d->stop,sizeof(d->stop));
    if(p->third_reentry){struct pt_mixed_causal_diagnostic diag;p->third_reentry=0;
        assert(!pt_mixed_causal_diagnostic(owner,&diag));}
    if(p->third_mutation){struct pt_mixed_causal_control_stop_publication *bad=(void *)d;
        p->third_mutation=0;bad->control.post.slot[19].serial=99;}
    return raw;
}
static int tl_commit(void *context,const struct pt_mixed_causal_packet *packet,
    struct pt_mixed_causal_actual *actual)
{
    struct tl_port *p=context;struct cc_port *old=&p->original;
    struct ct_command *command=ct_command(&old->base,packet->ticket);
    struct pt_mixed_causal_command_identity identity={0};uint64_t before=old->base.ticks;int raw;
    if(command)memcpy(&identity,&command->identity,sizeof(identity));
    if(packet->action[0].kind==PT_MIXED_READERS_STOP){
        if(!p->control_completed)return 0;
        return ss_commit(old,packet,actual);
    }
    raw=cc_commit(old,packet,actual);
    if(raw==1&&before>=packet->first&&before<packet->last&&old->base.ticks<packet->last&&
       actual->active_mask==old->base.mask&&actual->adopted_mask==old->base.mask){
        unsigned i,matched=1;
        for(i=0;i<20;++i)if(!keys_equal(actual->slot+i,old->base.slot+i))matched=0;
        if(matched){
            if(packet->action[0].kind==PT_MIXED_READERS_TRIGGER)
                tl_capture_completed(&p->root,&identity,packet,before,old->base.ticks,actual);
            else{tl_capture_completed(&p->control,&identity,packet,before,old->base.ticks,actual);p->control_completed=1;}
        }
    }
    return raw;
}
static int tl_command_quiet(void *context,const struct pt_mixed_causal_command_identity *identity,unsigned cancel)
{
    struct tl_port *p=context;
    if(!cancel&&((p->hold_control&&identity->ticket==p->control.identity.ticket)||
       (p->hold_third&&identity->ticket==p->third_identity.ticket))){++p->original.base.command_proofs;return 0;}
    return cc_command_quiet(&p->original,identity,cancel);
}
static struct tl_case *tl_make(unsigned bits,unsigned cache_bits,unsigned bind)
{
    struct tl_case *c=calloc(1,sizeof(*c));struct pt_mixed_causal_control_stop_port port;assert(c);++tl_cases;
    c->trial=trial_make(bits,cache_bits);c->before=save(c->trial->resources,&c->before_bytes);
    c->port.original.base.ticks=100;c->port.original.base.frequency=709379;
    c->port.original.base.commit_raw=c->port.original.base.source_raw=1;
    c->port.original.publication_raw=c->port.third_raw=1;
    c->config.allocator=(struct pt_allocator){&c->memory,ct_allocate,ct_release};
    c->config.allocator_context=(struct pt_mixed_readers_span){&c->memory,sizeof(c->memory)};
    c->config.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};c->config.session=19;
    c->config.control_budget=pt_mixed_causal_control_size();c->config.queue_budget=pt_mixed_readers_control_size();
    c->config.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),1,31,ct_clock,ct_publish,tl_commit,
        tl_command_quiet,ss_reader_quiet,ct_source_close,ct_source_quiet,ct_publish_successor};
    c->workspace=calloc(1,pt_mixed_causal_workspace_size());assert(c->workspace);
    assert(pt_mixed_causal_open(&c->config,c->workspace,pt_mixed_causal_workspace_size(),&c->owner)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_borrow_queue(c->owner,&c->trial->queue)==PT_MIXED_READERS_OK&&c->memory.live==2);
    c->port.original.base.registration=(struct pt_mixed_causal_registration){c->owner,c->trial->queue,19,17};
    port=(struct pt_mixed_causal_control_stop_port){&c->port,sizeof(c->port),1,3,cc_publish_control,tl_publish_stop};
    if(bind)assert(pt_mixed_causal_control_stop_bind(c->owner,&port)==PT_MIXED_READERS_OK);
    return c;
}
static uint64_t tl_first(struct tl_case *c,unsigned count)
{
    uint64_t first=0;trigger_input(c->trial,count,0,960);
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&first)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(960);assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
    return first;
}
static struct pt_mixed_causal_control_request tl_control_request(struct tl_case *c,uint64_t first,unsigned count)
{
    struct pt_mixed_causal_control_request r;unsigned i;memset(&r,0,sizeof(r));
    holder_init(c->trial->command+1,101);c->trial->command[1].queue=c->trial->queue;
    r.predecessor=first;r.frame=1920;r.count=count;r.command=control_of(c->trial->command+1);
    for(i=0;i<count;++i){struct pt_mixed_causal_control_action *a=r.action+i;a->route=i<4?1:2;a->slot=i<4?i:i-4;
        if(a->route==1){a->period=(uint16_t)(214+i);a->volume=(uint8_t)(32+i);}
        else{a->rate=0x10000U+i;a->left=(uint8_t)i;a->right=(uint8_t)(250-i);}}
    return r;
}
static uint64_t tl_control(struct tl_case *c,uint64_t first,unsigned count,unsigned detach_first)
{
    struct pt_mixed_causal_control_request r=tl_control_request(c,first,count);uint64_t control=0;
    /* Keep actual C1 until C2 occupies the other bounded slot. Later C1's
     * genuine positive service, not the model's completed flag, frees C3 space. */
    c->port.original.hold_first=1;
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_PENDING);
    assert(pt_mixed_causal_control_enqueue(c->owner,&r,&control)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_publish(c->owner,control)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(1920);assert(pt_mixed_causal_fire(c->owner,control)==PT_MIXED_CAUSAL_COMMITTED);
    assert(c->port.control_completed);
    if(detach_first){c->port.original.hold_first=0;
        assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);}
    return control;
}
static struct pt_mixed_causal_stop_request tl_stop_request(struct tl_case *c,uint64_t control,unsigned count)
{
    struct pt_mixed_causal_stop_request r;unsigned i;memset(&r,0,sizeof(r));
    holder_init(c->trial->command+2,102);c->trial->command[2].queue=c->trial->queue;
    r.predecessor=control;r.frame=2880;r.count=count;r.command=control_of(c->trial->command+2);
    for(i=0;i<count;++i){r.action[i].route=i<4?1:2;r.action[i].slot=i<4?i:i-4;}
    return r;
}
static void tl_unused_third(struct tl_case *c)
{
    struct holder *h=c->trial->command+2;held_terminal(h,h->token,1);held_release(h,h->token);
}
static void tl_drain(struct tl_case *c,uint64_t first,uint64_t control,uint64_t third,
    unsigned readers,unsigned first_detached,unsigned control_detached,unsigned failed)
{
    enum pt_mixed_readers_result wanted=failed?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;unsigned i;
    c->port.original.hold_first=c->port.hold_control=c->port.hold_third=0;
    c->port.original.base.unknown_ticket=0;c->port.original.quiet_mask=c->port.original.stopped_mask;
    if(!first_detached)assert(pt_mixed_causal_service_command(c->owner,first,1,NULL)==wanted);
    if(control&&!control_detached)assert(pt_mixed_causal_service_command(c->owner,control,1,NULL)==wanted);
    if(third)assert(pt_mixed_causal_service_command(c->owner,third,1,NULL)==wanted);
    for(i=0;i<readers;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==wanted);
    assert(!pt_mixed_readers_commands_held(c->trial->queue)&&!pt_mixed_readers_readers_held(c->trial->queue));
}
static void tl_drop(struct tl_case *c,unsigned unknown_source)
{
    same_save(c->trial->resources,c->before,c->before_bytes);
    if(unknown_source){c->port.original.base.source_raw=-1;assert(!pt_mixed_causal_close(&c->owner)&&c->owner&&c->memory.live==1);
        c->trial->queue=NULL;assert(pt_mixed_causal_close(&c->owner)&&c->port.original.base.shutdowns==1&&c->port.original.base.probes==1);}
    else assert(pt_mixed_causal_close(&c->owner)&&c->port.original.base.shutdowns==1);
    assert(!c->owner&&!c->memory.live&&pt_mixed_causal_close(&c->owner));c->trial->queue=NULL;
    same_save(c->trial->resources,c->before,c->before_bytes);trial_drop(c->trial);
    free(c->before);free(c->workspace);free(c);
}
static void tl_success(unsigned bits,unsigned cache_bits,unsigned reader_first)
{
    struct tl_case *c=tl_make(bits,cache_bits,1);uint64_t first=tl_first(c,16),control=tl_control(c,first,6,1),third=0,out=777;
    struct pt_mixed_causal_stop_request r=tl_stop_request(c,control,6),saved;
    struct pt_mixed_readers_key keys[16];struct pt_cache_lease leases[16];struct pt_sample_version *pins[16];
    struct pt_mixed_readers_reader_receipt observation;unsigned i,writes,chips,calls;
    memcpy(&saved,&r,sizeof(saved));
    for(i=0;i<16;++i){keys[i]=c->port.original.base.slot[ct_index(i<4?1:2,i<4?i:i-4)];
        leases[i]=c->trial->reader[i].lease;pins[i]=c->trial->reader[i].pin;}
    writes=c->trial->resources->card.writes;chips=c->trial->resources->chips;calls=c->memory.calls;
    CT_POISON(c->trial->command,sizeof(c->trial->command[0]));
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK&&third&&third!=first&&third!=control);
    assert(!memcmp(&r,&saved,sizeof(r))&&pt_mixed_readers_commands_held(c->trial->queue)==2&&
        pt_mixed_readers_readers_held(c->trial->queue)==16);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_stop_enqueue(c->owner,&r,&out)==PT_MIXED_READERS_INVALID&&out==777);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK&&c->port.third_publications==1);
    /* Genuine queue C3 reuses C1's consumed internal event address. That root
     * pointer is compared as identity, never reread as root event metadata. */
    assert(c->port.root.identity.event==c->port.third_identity.event);
    c->port.original.base.ticks=oracle(2880)-1;assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_EARLY&&!c->port.original.stops);
    c->port.original.base.ticks++;assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_COMMITTED&&c->port.original.stops==6);
    for(i=0;i<16;++i){unsigned index=ct_index(keys[i].route,keys[i].slot);
        assert(c->trial->reader[i].pin==pins[i]&&c->trial->reader[i].live&&
            !memcmp(&leases[i],&c->trial->reader[i].lease,sizeof(leases[i])));
        if(i<6)assert(!(c->port.original.base.mask&(1U<<index))&&!c->port.original.base.slot[index].queue);
        else assert(keys_equal(keys+i,c->port.original.base.slot+index));
        assert(pt_mixed_causal_service_reader(c->owner,first,i,0,&observation)==PT_MIXED_READERS_PENDING);
        assert(observation.observed==oracle(960)&&observation.issued==oracle(960)&&
            observation.state==(i<6?PT_MIXED_READER_DRAINING:PT_MIXED_READER_ACTIVE));
    }
    assert(writes==c->trial->resources->card.writes&&chips==c->trial->resources->chips&&calls==c->memory.calls);
    assert(!pt_mixed_causal_close(&c->owner)&&c->owner&&!c->port.original.base.shutdowns);
    c->port.original.quiet_mask=c->port.original.stopped_mask;
    if(reader_first){
        for(i=0;i<6;++i){assert(pt_mixed_causal_service_reader(c->owner,first,i,0,NULL)==PT_MIXED_READERS_OK);
            assert(c->trial->reader[i].live&&!c->trial->reader[i].releases);}
        assert(pt_mixed_causal_service_command(c->owner,control,0,NULL)==PT_MIXED_READERS_OK);
        for(i=0;i<6;++i)assert(c->trial->reader[i].live&&!c->trial->reader[i].releases);
        assert(pt_mixed_causal_service_command(c->owner,third,0,NULL)==PT_MIXED_READERS_OK);
        for(i=0;i<6;++i)assert(!c->trial->reader[i].live&&c->trial->reader[i].releases==1);
        for(i=6;i<16;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==PT_MIXED_READERS_OK);
    }else tl_drain(c,first,control,third,16,1,0,0);
    CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));tl_drop(c,reader_first);
}
static void tl_capacity_and_detachment(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=tl_control(c,first,6,0),third=777;
    struct pt_mixed_causal_stop_request r=tl_stop_request(c,control,6),saved;
    unsigned proofs=c->port.original.base.command_proofs;memcpy(&saved,&r,sizeof(saved));
    assert(pt_mixed_readers_commands_held(c->trial->queue)==2);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    assert(!memcmp(&r,&saved,sizeof(r))&&proofs==c->port.original.base.command_proofs);
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_PENDING);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    c->port.original.hold_first=0;assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==1);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK&&third!=777);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==2);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK);
    tl_drain(c,first,control,third,6,1,0,0);tl_drop(c,0);
}
static void tl_disposed_control(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=tl_control(c,first,6,1),third=0;
    struct pt_mixed_causal_stop_request r=tl_stop_request(c,control,6);
    const struct pt_mixed_readers_event *retired_event=c->port.control.identity.event;
    assert(pt_mixed_causal_service_command(c->owner,control,0,NULL)==PT_MIXED_READERS_OK);
    CT_POISON(c->trial->command,sizeof(c->trial->command[0]));CT_POISON(c->trial->command+1,sizeof(c->trial->command[1]));
    /* Queue C2 has actually consumed this event, and third reuses the earlier
     * C1 slot. Poison only the expired C2 event prefix, not queue-owned flags. */
    CT_POISON((void *)retired_event,sizeof(*retired_event));
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(2880);assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_COMMITTED);
    CT_UNPOISON((void *)retired_event,sizeof(*retired_event));
    tl_drain(c,first,control,third,6,1,1,0);
    CT_UNPOISON(c->trial->command,sizeof(c->trial->command[0]));CT_UNPOISON(c->trial->command+1,sizeof(c->trial->command[1]));tl_drop(c,0);
}
static void tl_guards(void)
{
    struct tl_case *c=tl_make(24,16,0);struct pt_mixed_causal_control_stop_port p={&c->port,sizeof(c->port),1,3,cc_publish_control,tl_publish_stop},bad;
    struct pt_mixed_causal_stop_request r,saved;uint64_t first,control,third=777;unsigned i,currents;
    bad=p;bad.context_bytes--;assert(pt_mixed_causal_control_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.version++;assert(pt_mixed_causal_control_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.flags++;assert(pt_mixed_causal_control_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    bad=p;bad.publish_stop_after_control=NULL;assert(pt_mixed_causal_control_stop_bind(c->owner,&bad)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_control_stop_bind(c->owner,&p)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_stop_bind(c->owner,&p)==PT_MIXED_READERS_INVALID);
    first=tl_first(c,6);control=tl_control(c,first,6,1);r=tl_stop_request(c,control,6);memcpy(&saved,&r,sizeof(saved));
    currents=c->trial->command[2].currents;
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&r.frame)==PT_MIXED_READERS_INVALID&&!memcmp(&r,&saved,sizeof(r)));
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&c->trial->reader[0].token)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,(void *)c->trial->reader[0].spans[0].data)==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,(void *)((uint8_t *)&c->port+sizeof(c->port)-8))==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,(void *)c->memory.block[0],&third)==PT_MIXED_READERS_INVALID&&third==777);
    /* Whole queried owner capacity includes the added publication backup and
     * tombstone tail. Reject before interpreting these bytes as a request. */
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,
        (void *)((uint8_t *)c->memory.block[0]+pt_mixed_causal_control_size()-8))==PT_MIXED_READERS_INVALID);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,
        (void *)((uint8_t *)c->memory.block[0]+pt_mixed_causal_control_size()-8),&third)==PT_MIXED_READERS_INVALID&&third==777);
    for(i=0;i<5;++i){memcpy(&r,&saved,sizeof(r));
        if(i==0)r.action[1]=r.action[0];
        if(i==1)r.action[15].route=1;
        if(i==2)r.frame=1920;
        if(i==3)r.predecessor=first;
        if(i==4)r.count=17;
        assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    }
    memcpy(&r,&saved,sizeof(r));r.command.context=&r;r.command.context_bytes=sizeof(r);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    assert(currents==c->trial->command[2].currents);
    tl_unused_third(c);tl_drain(c,first,control,0,6,1,0,0);tl_drop(c,0);
}
static void tl_no_completed_control(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=0,third=777;
    struct pt_mixed_causal_control_request cr=tl_control_request(c,first,6);struct pt_mixed_causal_stop_request r;
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_enqueue(c->owner,&cr,&control)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_control_publish(c->owner,control)==PT_MIXED_READERS_OK);r=tl_stop_request(c,control,6);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    assert(pt_mixed_causal_fire(c->owner,control)==PT_MIXED_CAUSAL_EARLY);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    tl_unused_third(c);tl_drain(c,first,control,0,6,1,0,0);tl_drop(c,0);
}
static void tl_publication(unsigned mode)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=tl_control(c,first,6,1),third=0;
    struct pt_mixed_causal_stop_request r=tl_stop_request(c,control,6);unsigned failed=mode!=0;
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK);
    if(mode==0)c->port.third_raw=0;
    if(mode==1)c->port.third_raw=-1;
    if(mode==2)c->port.third_mutation=1;
    if(mode==3){c->port.third_raw=0;c->port.third_mutation=1;}
    if(mode==4)c->port.third_reentry=1;
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==
        (mode==0?PT_MIXED_READERS_PENDING:PT_MIXED_READERS_BACKEND));
    if(!mode){assert(!ct_command(&c->port.original.base,third)&&c->trial->command[2].live);
        c->port.third_raw=1;assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK);}
    else{c->port.original.base.ticks=oracle(2880);assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_FAILED);}
    tl_drain(c,first,control,third,6,1,0,failed);tl_drop(c,0);
}
static void tl_enqueue_actual_transfer(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=tl_control(c,first,6,1),third=0;
    struct pt_mixed_causal_stop_request r=tl_stop_request(c,control,6);
    r.command.current=tl_current_fault;tl_fail_owner=c->owner;
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK&&third&&c->trial->command[2].live);
    assert(pt_mixed_readers_commands_held(c->trial->queue)==2);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_INVALID);
    /* The actual unsubmitted C3 remains genuine queue ownership. Explicit local
     * shutdown consumes it; its holder cannot be released by the caller. */
    assert(pt_mixed_causal_stop(c->owner)==PT_MIXED_READERS_BACKEND);
    assert(!c->trial->command[2].live&&c->trial->command[2].releases==1);
    tl_drain(c,first,control,0,6,1,0,1);tl_drop(c,0);
}
static void tl_root_detach_outer_fault(void)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=0,control,third=777;
    struct pt_mixed_causal_stop_request r;unsigned currents;
    trigger_input(c->trial,6,0,960);c->trial->input->command.terminal=tl_terminal_fault;
    assert(pt_mixed_causal_enqueue(c->owner,c->trial->input,&first)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_publish(c->owner,first)==PT_MIXED_READERS_OK);
    c->port.original.base.ticks=oracle(960);assert(pt_mixed_causal_fire(c->owner,first)==PT_MIXED_CAUSAL_COMMITTED);
    control=tl_control(c,first,6,0);r=tl_stop_request(c,control,6);currents=c->trial->command[2].currents;
    c->port.original.hold_first=0;tl_fail_owner=c->owner;
    /* Lower genuine queue actually consumes C1 and returns OK. The new latch
     * still requires clean outer exclusion: a terminal callback fault cannot
     * authorize C3 from positive quiet or the newly free command count. */
    assert(pt_mixed_causal_service_command(c->owner,first,0,NULL)==PT_MIXED_READERS_OK);
    assert(!c->trial->command[0].live&&c->trial->command[0].releases==1&&
        pt_mixed_readers_commands_held(c->trial->queue)==1);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_INVALID&&third==777);
    assert(currents==c->trial->command[2].currents);
    tl_unused_third(c);tl_drain(c,first,control,0,6,1,0,1);tl_drop(c,0);
}
static void tl_expected_only(unsigned unknown)
{
    struct tl_case *c=tl_make(24,16,1);uint64_t first=tl_first(c,6),control=tl_control(c,first,1,1),third=0;unsigned i;
    struct pt_mixed_causal_stop_request r=tl_stop_request(c,control,1);
    assert(pt_mixed_causal_stop_after_control_enqueue(c->owner,&r,&third)==PT_MIXED_READERS_OK);
    assert(pt_mixed_causal_stop_after_control_publish(c->owner,third)==PT_MIXED_READERS_OK);
    if(unknown){c->port.original.base.unknown_ticket=third;
        assert(pt_mixed_causal_service_reader(c->owner,first,5,1,NULL)==PT_MIXED_READERS_BACKEND&&c->trial->reader[5].live);
        c->port.original.base.unknown_ticket=0;}
    assert(pt_mixed_causal_service_reader(c->owner,first,5,1,NULL)==
        (unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(ct_command(&c->port.original.base,third)->disabled&&!c->trial->reader[5].live&&!c->port.original.stops);
    assert(pt_mixed_causal_fire(c->owner,third)==PT_MIXED_CAUSAL_INVALID);
    assert(pt_mixed_causal_service_command(c->owner,control,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    assert(pt_mixed_causal_service_command(c->owner,third,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    for(i=0;i<5;++i)assert(pt_mixed_causal_service_reader(c->owner,first,i,1,NULL)==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK));
    tl_drop(c,0);
}
int main(void)
{
    unsigned bits,cache,order,mode;
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(order=0;order<2;++order)
        tl_success(bits,cache,order);
    tl_capacity_and_detachment();tl_disposed_control();tl_guards();tl_no_completed_control();
    for(mode=0;mode<5;++mode)tl_publication(mode);
    tl_enqueue_actual_transfer();tl_root_detach_outer_fault();tl_expected_only(0);tl_expected_only(1);
    printf("MIXED CAUSAL THREE STAGE HOST COMPLETE cases=%u;two live C slots;original R/master/cache;actual root detach;actual CONTROL;opaque retired C;original frames;independent C/R/SOURCE;NOT_NATIVE_TIMING_DEVICE_AUDIO\n",tl_cases);
    return 0;
}
