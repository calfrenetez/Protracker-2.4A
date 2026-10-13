/* Source-only genuine-owner proposal. Exact donor helpers supply real sampler,
 * version/cache leases and independent immutable resource references. Neither
 * dormant donor main is called or counted as fixture coverage. */
#include "owner4_future60_donor.c"
#include "../src/editor/paula_owner4.h"
struct owner_allocator {struct memory *fast;unsigned refuse,release_fault,change_project;struct pt_sampler_paula *mutate_bridge;};
struct owner_test {
    struct future_fixture donor;struct owner_allocator allocation;
    struct pt_allocator allocator;struct pt_paula_owner4 *owner;
    void *outer;size_t outer_bytes;unsigned edit_calls;
};
static struct owner_test *held_owner4[6],*callback_owner4;
static unsigned owner4_cases,owner4_restored,owner4_holds;
static void *owner_test_allocate(void *v,size_t bytes)
{
    struct owner_allocator *a=v;void *p;
    if(a->refuse){--a->refuse;return NULL;}
    p=allocate(a->fast,bytes);
    if(p&&a->change_project){--a->change_project;a->mutate_bridge->project=NULL;}
    return p;
}
static void owner_test_release(void *v,void *p)
{
    struct owner_allocator *a=v;
    if(a->release_fault){
        a->release_fault=0;assert(callback_owner4);
        assert(pt_paula_owner4_status(callback_owner4->owner)==PT_OWNER4_REFUSED);
    }
    release(a->fast,p);
}
static int owner_reentry_apply(void *v,const struct pt_paula_future_packet *packet)
{
    assert(callback_owner4);
    assert(pt_paula_owner4_status(callback_owner4->owner)==PT_OWNER4_REFUSED);
    return resource_apply(v,packet);
}
static void owner_test_setup(struct owner_test *x)
{
    struct future_fixture *d=&x->donor;
    memset(x,0,sizeof(*x));sequence_init(&d->sequence,0);
    x->allocation.fast=&d->sequence.f.fast;
    x->allocator=(struct pt_allocator){&x->allocation,owner_test_allocate,owner_test_release};
    d->resource.now=100;d->resource.frequency=709379;
    d->resource.arm_raw=d->resource.apply_raw=d->resource.cancel_raw=d->resource.ack_raw=d->resource.leave_raw=1;
    d->ops=(struct pt_paula_future_ops){&d->resource,sizeof(d->resource),resource_enter,resource_leave,resource_clock,
        resource_arm,resource_rearm,resource_apply,resource_ack,resource_cancel,resource_ticket_quiet,
        resource_reader_quiet,resource_source_close,resource_source_quiet};
    d->control_bytes=pt_paula_segment_future_size();d->control=allocate(&d->sequence.f.fast,d->control_bytes);
    x->outer_bytes=pt_paula_owner4_size();x->outer=allocate(&d->sequence.f.fast,x->outer_bytes);
    assert(d->control&&x->outer);memset(d->control,0,d->control_bytes);memset(x->outer,0,x->outer_bytes);
    /* Genuine factory future storage, not a fabricated initialized owner. */
    d->resource.future=d->control;
    assert(d->sequence.f.sampler.generation==0);
}
static enum pt_paula_owner4_result owner_test_construct(struct owner_test *x,unsigned order)
{
    struct future_fixture *d=&x->donor;
    return pt_paula_owner4_construct(x->outer,x->outer_bytes,d->control,d->control_bytes,
        d->sequence.workspace,d->sequence.workspace_bytes,&x->allocator,&d->sequence.f.bridge,
        &d->sequence.config,&d->ops,d->sequence.revision,
        order?PT_OWNER4_READER_FIRST:PT_OWNER4_COMMAND_FIRST,&x->owner);
}
static enum pt_paula_segment_sequence_result owner_until(struct owner_test *x)
{
    enum pt_paula_segment_sequence_result r=PT_PAULA_SEQUENCE_PENDING;unsigned n;
    for(n=0;n<4000;++n){
        r=pt_paula_owner4_step(x->owner,x->donor.sequence.revision,127);
        if(r==PT_PAULA_SEQUENCE_READY)return r;
        assert(r==PT_PAULA_SEQUENCE_PENDING||r==PT_PAULA_SEQUENCE_WAIT_ACTIVE||r==PT_PAULA_SEQUENCE_WAIT_PRESSURE);
    }
    assert(0);return r;
}
static void owner_publish_and_fire(struct owner_test *x,unsigned i,unsigned detach)
{
    struct future_resource *r=&x->donor.resource;const struct pt_paula_future_packet *p;
    assert(owner_until(x)==PT_PAULA_SEQUENCE_READY);
    assert(pt_paula_owner4_publish(x->owner,x->donor.sequence.revision)==PT_SCHEDULED_OK);
    assert(r->count==i+1);p=r->slot[i].packet;
    assert(p&&p->geometry.frame==(uint64_t)(i+1)*960U&&p->first<p->last);
    r->now=p->first;assert(pt_paula_owner4_deliver(x->owner,p->ticket)==PT_PAULA_FUTURE_OK);
    assert(pt_paula_owner4_poll_command(x->owner,i)==PT_SCHEDULED_PENDING);
    assert(pinned(&x->donor.sequence.f)==3);
    if(detach){actual_resource_absence(&x->donor,i);assert(pt_paula_owner4_poll_command(x->owner,i)==PT_SCHEDULED_OK);}
}
static enum pt_edit_result edit_only_when_closed(struct owner_test *x)
{
    struct fixture *f=&x->donor.sequence.f;
    /* This fixture route is explicit. Other direct editing APIs are unhooked. */
    if(pt_paula_owner4_status(x->owner)!=PT_OWNER4_CLOSED)return PT_EDIT_CONFLICT;
    ++x->edit_calls;
    return pt_sampler_attributes(&f->sampler,&f->document.project,&f->history,0,"owner4 closed",63,0);
}
static enum pt_edit_result pcm_edit_only_when_closed(struct owner_test *x)
{
    struct fixture *f=&x->donor.sequence.f;
    /* Only this finite fixture route is guarded; direct mutation APIs stay unhooked. */
    if(pt_paula_owner4_status(x->owner)!=PT_OWNER4_CLOSED)return PT_EDIT_CONFLICT;
    ++x->edit_calls;
    return pt_sampler_edit(&f->sampler,&f->document.project,&f->history,0,PT_PCM_REVERSE,0,768,0);
}
static void owner_test_cleanup(struct owner_test *x)
{
    struct future_fixture *d=&x->donor;
    assert(pt_paula_owner4_status(x->owner)==PT_OWNER4_CLOSED);
    assert(!pinned(&d->sequence.f)&&!d->resource.current&&!d->resource.pending&&!d->resource.superseded);
    assert(d->resource.source_calls==1&&!d->resource.locked&&d->resource.enters==d->resource.leaves);
    release(&d->sequence.f.fast,d->control);release(&d->sequence.f.fast,x->outer);
    release(&d->sequence.f.fast,d->sequence.workspace);
    d->control=NULL;x->outer=NULL;d->sequence.workspace=NULL;
    close_fixture(&d->sequence.f);++owner4_restored;++owner4_cases;
}
static void owner_positive(unsigned order,unsigned count,unsigned edit)
{
    struct owner_test x;struct fixture *f;struct pt_project project;struct pt_pattern_history history;
    struct pt_pcm pcm[3];struct pt_event events[256];unsigned i,steps,effects,cancels=0,generation;
    owner_test_setup(&x);f=&x.donor.sequence.f;
    assert(owner_test_construct(&x,order)==PT_OWNER4_OPEN&&x.owner);
    for(i=0;i<count;++i)owner_publish_and_fire(&x,i,i+1<count);
    memcpy(&project,&f->document.project,sizeof(project));memcpy(&history,&f->history,sizeof(history));
    memcpy(pcm,f->pcm,sizeof(pcm));memcpy(events,project.events,sizeof(events));generation=f->sampler.generation;
    assert(edit_only_when_closed(&x)==PT_EDIT_CONFLICT&&!x.edit_calls);
    assert(pcm_edit_only_when_closed(&x)==PT_EDIT_CONFLICT&&!x.edit_calls);
    assert(pt_paula_owner4_request_stop(x.owner)==PT_OWNER4_STOPPING);
    for(i=0;i<count;++i)cancels+=x.donor.resource.slot[i].cancel;
    effects=x.donor.resource.effects;
    assert(pt_paula_owner4_request_stop(x.owner)==PT_OWNER4_STOPPING);
    {unsigned repeated=0;for(i=0;i<count;++i)repeated+=x.donor.resource.slot[i].cancel;assert(repeated==cancels);}
    assert(pt_paula_owner4_publish(x.owner,x.donor.sequence.revision)==PT_SCHEDULED_INVALID);
    assert(pt_paula_owner4_deliver(x.owner,count?x.donor.resource.slot[count-1].ticket:1)==PT_PAULA_FUTURE_STALE);
    assert(effects==x.donor.resource.effects);
    if(count){
        assert(pt_paula_owner4_advance_stop(x.owner)==PT_OWNER4_STOPPING);
        assert(!memcmp(&project,&f->document.project,sizeof(project))&&!memcmp(&history,&f->history,sizeof(history)));
        assert(!memcmp(pcm,f->pcm,sizeof(pcm))&&!memcmp(events,project.events,sizeof(events))&&f->sampler.generation==generation);
        assert(pinned(f)==3&&f->chip.bytes==2304);
        for(i=0;i<3;++i)assert(!memcmp(f->pcm[i].data,x.donor.sequence.master[i],sizeof(x.donor.sequence.master[i])));
        actual_resource_absence(&x.donor,count-1);
        x.donor.resource.allow_reader=1;
    }
    /* Source quiet remains pending independently after actual timer/reader quiet.
     * Advance only the matched owner; this does not fabricate a retirement. */
    for(steps=0;steps<20&&x.donor.resource.source_calls==0;++steps)
        assert(pt_paula_owner4_advance_stop(x.owner)==PT_OWNER4_STOPPING);
    assert(x.donor.resource.source_calls==1&&!x.donor.resource.allow_source);
    assert(pt_paula_owner4_advance_stop(x.owner)==PT_OWNER4_STOPPING);
    assert(edit_only_when_closed(&x)==PT_EDIT_CONFLICT&&!x.edit_calls);
    assert(pcm_edit_only_when_closed(&x)==PT_EDIT_CONFLICT&&!x.edit_calls);
    x.donor.resource.allow_source=1;
    for(steps=0;steps<20&&pt_paula_owner4_status(x.owner)!=PT_OWNER4_CLOSED;++steps)
        assert(pt_paula_owner4_advance_stop(x.owner)!=PT_OWNER4_HOLD);
    assert(pt_paula_owner4_status(x.owner)==PT_OWNER4_CLOSED);
    effects=x.donor.resource.enters;
    assert(pt_paula_owner4_request_stop(x.owner)==PT_OWNER4_CLOSED);
    assert(pt_paula_owner4_advance_stop(x.owner)==PT_OWNER4_CLOSED);
    assert(pt_paula_owner4_step(x.owner,x.donor.sequence.revision,127)==PT_PAULA_SEQUENCE_INVALID);
    assert(pt_paula_owner4_publish(x.owner,x.donor.sequence.revision)==PT_SCHEDULED_INVALID);
    assert(pt_paula_owner4_deliver(x.owner,1)==PT_PAULA_FUTURE_STALE);
    assert(x.donor.resource.enters==effects); /* permanent close never callbacks/reopens */
    if(edit){
        assert(edit_only_when_closed(&x)==PT_EDIT_OK&&x.edit_calls==1);
        assert(f->sampler.generation>generation&&f->history.count==history.count+1);
        assert(f->document.project.samples[0].volume==63&&pt_sampler_paula_sync(&f->bridge));
        assert(pt_pattern_undo(&f->document.project,&f->history,-1)==PT_EDIT_OK);
        assert(f->document.project.samples[0].volume==64);
        assert(pt_pattern_undo(&f->document.project,&f->history,1)==PT_EDIT_OK);
        assert(f->document.project.samples[0].volume==63&&pt_sampler_paula_sync(&f->bridge));
        for(i=0;i<3;++i)assert(!memcmp(f->pcm[i].data,x.donor.sequence.master[i],sizeof(x.donor.sequence.master[i])));
        {
            struct pt_pcm old_pcm;struct pt_sample_version *old_pin=NULL,*edited;
            struct pt_cache_lease old_cache,new_cache;enum pt_cache_result cr;
            const uint8_t *old_data,*rejected;size_t old_bytes,rejected_bytes;uint8_t cache_before[768];
            unsigned before_generation=f->sampler.generation;size_t before_count=f->history.count;
            uint64_t before_version=f->bridge.version;
            /* A genuine extra caller pin retains the exact old immutable master
             * independently of history/current ownership across edit and undo/redo. */
            assert(pt_sampler_pin(&f->sampler,&f->document.project,0,before_generation,&old_pcm,&old_pin)==PT_EDIT_OK);
            assert(old_pin==f->sampler.current[0]&&old_pcm.data==f->pcm[0].data);
            cr=pt_sampler_paula_acquire(&f->bridge,0,0,0,&old_cache);
            assert(cr==PT_CACHE_HIT||cr==PT_CACHE_LOAD);
            assert(pt_sampler_paula_location(&f->bridge,0,old_cache,&old_data,&old_bytes)&&old_bytes==sizeof(cache_before));
            memcpy(cache_before,old_data,sizeof(cache_before));
            assert(pcm_edit_only_when_closed(&x)==PT_EDIT_OK&&x.edit_calls==2);
            edited=f->sampler.current[0];
            assert(edited&&edited!=old_pin&&f->document.project.samples[0].pcm.data!=old_pcm.data);
            assert(f->sampler.generation==before_generation+1&&f->history.count==before_count+1);
            for(i=0;i<768;++i)
                assert(f->document.project.samples[0].pcm.data[i]==x.donor.sequence.master[0][767-i]);
            assert(!memcmp(old_pcm.data,x.donor.sequence.master[0],sizeof(x.donor.sequence.master[0])));
            assert(pt_sampler_paula_sync(&f->bridge)&&f->bridge.version==before_version+1);
            assert(f->bridge.cache.entry[old_cache.slot].valid==2&&f->bridge.cache.entry[old_cache.slot].pins==1);
            assert(!memcmp(old_data,cache_before,sizeof(cache_before)));
            rejected=old_data;rejected_bytes=old_bytes;
            assert(!pt_sampler_paula_location(&f->bridge,0,old_cache,&rejected,&rejected_bytes));
            assert(rejected==old_data&&rejected_bytes==old_bytes);
            assert(pt_sampler_paula_unpin(&f->bridge,old_cache));
            cr=pt_sampler_paula_acquire(&f->bridge,0,0,0,&new_cache);
            assert(cr==PT_CACHE_HIT||cr==PT_CACHE_LOAD);
            assert(pt_sampler_paula_location(&f->bridge,0,new_cache,&rejected,&rejected_bytes)&&rejected_bytes==sizeof(cache_before));
            for(i=0;i<768;++i)assert((int8_t)rejected[i]==x.donor.sequence.master[0][767-i]);
            assert(pt_sampler_paula_unpin(&f->bridge,new_cache));
            assert(pt_pattern_undo(&f->document.project,&f->history,-1)==PT_EDIT_OK);
            assert(f->sampler.current[0]==old_pin&&f->sampler.generation==before_generation+2);
            assert(!memcmp(f->document.project.samples[0].pcm.data,x.donor.sequence.master[0],sizeof(x.donor.sequence.master[0])));
            assert(pt_sampler_paula_sync(&f->bridge)&&f->bridge.version==before_version+2);
            assert(pt_pattern_undo(&f->document.project,&f->history,1)==PT_EDIT_OK);
            assert(f->sampler.current[0]==edited&&f->sampler.generation==before_generation+3);
            for(i=0;i<768;++i)
                assert(f->document.project.samples[0].pcm.data[i]==x.donor.sequence.master[0][767-i]);
            assert(pt_sampler_paula_sync(&f->bridge)&&f->bridge.version==before_version+3);
            assert(!memcmp(old_pcm.data,x.donor.sequence.master[0],sizeof(x.donor.sequence.master[0])));
            pt_sampler_unpin(old_pin);
            for(i=0;i<3;++i)assert(!memcmp(f->pcm[i].data,x.donor.sequence.master[i],sizeof(x.donor.sequence.master[i])));
        }
    }
    owner_test_cleanup(&x);
}
static void owner_unpublished(void)
{
    struct owner_test x;unsigned n;owner_test_setup(&x);
    assert(owner_test_construct(&x,0)==PT_OWNER4_OPEN);
    assert(owner_until(&x)==PT_PAULA_SEQUENCE_READY&&!x.donor.resource.count);
    /* A real original ticket exists in the genuine queue before publication. */
    assert(pt_paula_owner4_request_stop(x.owner)==PT_OWNER4_STOPPING);
    x.donor.resource.allow_source=1;
    for(n=0;n<20&&pt_paula_owner4_status(x.owner)!=PT_OWNER4_CLOSED;++n)
        assert(pt_paula_owner4_advance_stop(x.owner)!=PT_OWNER4_HOLD);
    assert(pt_paula_owner4_status(x.owner)==PT_OWNER4_CLOSED&&!x.donor.resource.effects);
    owner_test_cleanup(&x);
}
static void owner_alias(unsigned mode)
{
    struct owner_test x;struct future_fixture *d;struct pt_project *p;void *outer,*future,*workspace;
    struct pt_paula_owner4 **out;unsigned char *saved,*control_before[3];size_t bytes,control_bytes[3];void *controls[3];unsigned calls,i;
    owner_test_setup(&x);d=&x.donor;p=&d->sequence.f.document.project;
    outer=x.outer;future=d->control;workspace=d->sequence.workspace;out=&x.owner;
    if(!mode){out=(struct pt_paula_owner4 **)(void *)p->events;bytes=256*sizeof(*p->events);saved=malloc(bytes);memcpy(saved,p->events,bytes);}
    else if(mode==1){outer=p->events;bytes=256*sizeof(*p->events);saved=malloc(bytes);memcpy(saved,p->events,bytes);}
    else if(mode==2){future=p->samples[0].pcm.data;bytes=768*sizeof(int32_t);saved=malloc(bytes);memcpy(saved,p->samples[0].pcm.data,bytes);}
    else {workspace=p->samples[2].pcm.data;bytes=768*sizeof(int32_t);saved=malloc(bytes);memcpy(saved,p->samples[2].pcm.data,bytes);}
    assert(saved);calls=d->sequence.f.fast.calls;
    controls[0]=x.outer;controls[1]=d->control;controls[2]=d->sequence.workspace;
    control_bytes[0]=x.outer_bytes;control_bytes[1]=d->control_bytes;control_bytes[2]=d->sequence.workspace_bytes;
    for(i=0;i<3;++i){control_before[i]=malloc(control_bytes[i]);assert(control_before[i]);memcpy(control_before[i],controls[i],control_bytes[i]);}
    assert(pt_paula_owner4_construct(outer,x.outer_bytes,future,d->control_bytes,workspace,d->sequence.workspace_bytes,
        &x.allocator,&d->sequence.f.bridge,&d->sequence.config,&d->ops,d->sequence.revision,PT_OWNER4_COMMAND_FIRST,out)==PT_OWNER4_REFUSED);
    assert(!x.owner&&calls==d->sequence.f.fast.calls&&!d->resource.enters);
    for(i=0;i<3;++i){assert(!memcmp(control_before[i],controls[i],control_bytes[i]));free(control_before[i]);}
    assert(!memcmp(saved,mode<2?(void *)p->events:mode==2?(void *)p->samples[0].pcm.data:(void *)p->samples[2].pcm.data,bytes));free(saved);
    release(&d->sequence.f.fast,d->control);release(&d->sequence.f.fast,x.outer);release(&d->sequence.f.fast,d->sequence.workspace);
    close_fixture(&d->sequence.f);++owner4_restored;++owner4_cases;
}
static void owner_hold(unsigned mode)
{
    struct owner_test *x;struct future_resource *r;unsigned effects,enters,n;
    assert(mode<6&&!held_owner4[mode]);x=calloc(1,sizeof(*x));assert(x);held_owner4[mode]=x;
    owner_test_setup(x);r=&x->donor.resource;
    if(mode==2)x->allocation.refuse=1;
    if(mode==5){x->allocation.change_project=1;x->allocation.mutate_bridge=&x->donor.sequence.f.bridge;}
    if(mode==3)x->donor.ops.apply=owner_reentry_apply;
    assert(owner_test_construct(x,0)==((mode==2||mode==5)?PT_OWNER4_HOLD:PT_OWNER4_OPEN)&&x->owner);
    if(mode==2||mode==5){
        assert(!x->donor.sequence.producer&&!r->count&&!r->source_calls);
        if(mode==5)assert(!x->allocation.change_project&&!x->donor.sequence.f.bridge.project);
    }else if(mode==4){
        assert(pt_paula_owner4_request_stop(x->owner)==PT_OWNER4_STOPPING);
        r->allow_source=1;callback_owner4=x;x->allocation.release_fault=1;
        for(n=0;n<20&&pt_paula_owner4_status(x->owner)==PT_OWNER4_STOPPING;++n)
            (void)pt_paula_owner4_advance_stop(x->owner);
        callback_owner4=NULL;
        assert(!x->allocation.release_fault);
    }else{
        assert(owner_until(x)==PT_PAULA_SEQUENCE_READY);
        if(mode==1)r->leave_raw=0;
        if(mode==3)callback_owner4=x;
        assert(pt_paula_owner4_publish(x->owner,x->donor.sequence.revision)==(mode==1?PT_SCHEDULED_CLOCK:PT_SCHEDULED_OK));
        /* Failed restoration happens during pre-submit clock: no packet was armed. */
        if(mode==1)assert(!r->count&&!r->arms&&!r->effects&&!r->locked&&r->enters==r->leaves);
        if(mode==0){r->now=r->slot[0].packet->last;assert(pt_paula_owner4_deliver(x->owner,r->slot[0].ticket)==PT_PAULA_FUTURE_LATE);}
        if(mode==3){r->now=r->slot[0].packet->first;assert(pt_paula_owner4_deliver(x->owner,r->slot[0].ticket)==PT_PAULA_FUTURE_UNKNOWN);callback_owner4=NULL;}
    }
    assert(pt_paula_owner4_status(x->owner)==PT_OWNER4_HOLD);
    effects=r->effects;enters=r->enters;
    assert(edit_only_when_closed(x)==PT_EDIT_CONFLICT&&!x->edit_calls);
    assert(pcm_edit_only_when_closed(x)==PT_EDIT_CONFLICT&&!x->edit_calls);
    assert(pt_paula_owner4_request_stop(x->owner)==PT_OWNER4_HOLD);
    assert(pt_paula_owner4_advance_stop(x->owner)==PT_OWNER4_HOLD);
    assert(r->effects==effects&&r->enters==enters);
    for(n=0;n<3;++n)assert(!memcmp(x->donor.sequence.f.pcm[n].data,x->donor.sequence.master[n],sizeof(x->donor.sequence.master[n])));
    /* Deliberate HOLD: all controls/versions/callback contexts stay reachable.
     * No attempted recovery, fabricated child owner, forced free or counter repair. */
    ++owner4_holds;++owner4_cases;
}
int main(void)
{
    unsigned order,count,mode;
    for(order=0;order<2;++order)for(count=0;count<=4;++count)owner_positive(order,count,!order&&count==4);
    owner_unpublished();for(mode=0;mode<4;++mode)owner_alias(mode);
    for(mode=0;mode<6;++mode)owner_hold(mode);
    assert(owner4_cases==21&&owner4_restored==15&&owner4_holds==6);
    puts("paula owner4 proposal cases=21 restored=15 deliberate_HOLD=6");
    return 0;
}
