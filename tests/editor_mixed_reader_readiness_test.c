/* Private SOURCE draft, NOT RUN. All accepted query/early-source/legacy/Q
 * suites execute once through a dedicated inherited entry selector. New tests
 * use genuine controller/factory/queue registration and explicit real service;
 * no caller readiness flag, fabricated ACTIVE key or proof mutation is used. */
#define PT_EDITOR_MIXED_SOURCE_QUERY_TEST_MAIN err_query_main
#include "editor_mixed_source_query_test.c"
#undef PT_EDITOR_MIXED_SOURCE_QUERY_TEST_MAIN
#include "../src/core/mixed_scheduled_readers_internal.h"
#include "../src/editor/sampler_mixed_readers_internal.h"

static int err_owned(void *context)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,bus));
    struct esb_trial *e=(void *)f;unsigned mode=f->owned_hook;int raw;
    f->owned_hook=0;raw=emp_owned(context);++e->steps;
    if(mode==100)return 0; /* Actual held-resource current refusal. */
    if(mode==101){unsigned n=f->control.reentries;
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,f->reader[4])==PT_MIXED_READERS_BACKEND);
        assert(f->control.reentries==n+1);}
    else if(mode==102){struct pt_sampler_mixed_reader_handle h=f->control.reader[f->reader[4].slot].handle;
        assert(pt_sampler_mixed_reader_readiness(f->control.pool,h)==PT_MIXED_READERS_BACKEND);}
    else if(mode==103){struct pt_editor_mixed_reader_record *r=f->control.reader+f->reader[4].slot;
        assert(pt_mixed_readers_reader_readiness(f->control.queue,r->ticket,r->action)==PT_MIXED_READERS_BACKEND);}
    else if(mode==104)++f->editor->history.revision;
    else assert(!mode);
    return raw;
}
static struct esb_trial *err_make(unsigned bits,unsigned cache,unsigned little)
{
    struct esb_trial *e=esb_scope(bits,0);struct emp_trial *f=&e->base;
    f->cache_bits=cache;f->little=little;
    assert(pt_amigus_wavetable_cache_detach(&f->card->cache));
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,
        &f->bus,err_owned,emp_write));
    esb_begin(e);esb_promote(e);esb_activate(e,1);
    emq_batch(f,e->batch,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);return e;
}
static uint64_t err_enqueue(struct esb_trial *e)
{
    uint64_t ticket=UINT64_MAX;struct emp_trial *f=&e->base;
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&ticket)==PT_MIXED_READERS_OK);
    assert(ticket&&ticket!=UINT64_MAX);return ticket;
}
static void err_healthy_probe(struct esb_trial *e,unsigned index,enum pt_mixed_readers_result wanted)
{
    struct emp_trial *f=&e->base;struct pt_editor_mixed_reader_record rr=f->control.reader[f->reader[index].slot];
    struct sma_port port=f->port;struct pt_editor_mixed_source_borrow borrow=*e->borrow;
    unsigned calls=f->ordinary.calls,releases=f->ordinary.releases,chip=f->chip.calls,masters=f->masters.calls;
    unsigned observed=e->steps;
    size_t qbytes=pt_mixed_readers_control_size(),pbytes=pt_sampler_mixed_pool_size(),rbytes=pt_sampler_mixed_reader_size();
    unsigned char *q=malloc(qbytes),*p=malloc(pbytes),*r=malloc(rbytes),*controller=malloc(sizeof(f->control));
    assert(q&&p&&r&&controller);
    memcpy(q,f->control.queue,qbytes);memcpy(p,f->control.pool,pbytes);
    memcpy(r,rr.handle.address,rbytes);memcpy(controller,&f->control,sizeof(f->control));
    assert(pt_mixed_readers_reader_readiness(f->control.queue,rr.ticket,rr.action)==wanted);
    assert(pt_sampler_mixed_reader_readiness(f->control.pool,rr.handle)==wanted);
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,f->reader[index])==wanted);
    /* Exactly real current callbacks, no clock/publication/service/quiet call,
     * key/key_seen write, receipt, registration, proof or ownership mutation. */
    assert(e->steps>observed&&!memcmp(q,f->control.queue,qbytes)&&!memcmp(p,f->control.pool,pbytes)&&
        !memcmp(r,rr.handle.address,rbytes)&&!memcmp(controller,&f->control,sizeof(f->control))&&
        !memcmp(&port,&f->port,sizeof(port))&&!memcmp(&borrow,e->borrow,sizeof(borrow))&&
        calls==f->ordinary.calls&&releases==f->ordinary.releases&&chip==f->chip.calls&&masters==f->masters.calls);
    free(q);free(p);free(r);free(controller);
}
static void err_source_finish(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;unsigned calls;
    f->port.close_result=f->port.quiet_result=0;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(f->control.activation&&!f->control.queue&&!f->control.pool&&
        !pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&
        !pt_editor_mixed_source_borrow_close(e->borrow));
    calls=f->port.reader_calls;
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,f->reader[0])!=PT_MIXED_READERS_PENDING);
    assert(calls==f->port.reader_calls&&e->borrow->address==&f->control);
    f->port.callback_owner=NULL;f->port.close_result=f->port.quiet_result=1;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&
        pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    esb_release(e);emp_same(f);esb_drop(e);
}
static void err_lifetime(unsigned bits,unsigned cache,unsigned little,unsigned reader_first)
{
    struct esb_trial *e=err_make(bits,cache,little);struct emp_trial *f=&e->base;
    struct pt_mixed_readers_reader_receipt receipt,before;struct pt_mixed_readers_key key,saved;
    struct pt_editor_mixed_reader_ref old=f->reader[0];uint64_t ticket;unsigned i;
    /* A factory PREPARING/READY reader is not a queue registration; no wait
     * certificate is invented before actual enqueue owns its original ticket. */
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,old)==PT_MIXED_READERS_INVALID);
    ticket=err_enqueue(e);
    for(i=0;i<5;++i)err_healthy_probe(e,i,PT_MIXED_READERS_PENDING);
    memset(&key,0x5a,sizeof(key));saved=key;
    assert(pt_sampler_mixed_reader_key(f->control.pool,f->control.reader[old.slot].handle,&key)==PT_MIXED_READERS_STALE);
    assert(!memcmp(&key,&saved,sizeof(key)));
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK);
    for(i=0;i<5;++i)err_healthy_probe(e,i,PT_MIXED_READERS_PENDING);
    f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,ticket)==PT_MIXED_ACTIVATION_COMMITTED);
    /* Copied fire alone does not update the queue's real ADOPTED/timed state. */
    for(i=0;i<5;++i)err_healthy_probe(e,i,PT_MIXED_READERS_PENDING);
    for(i=0;i<5;++i){memset(&receipt,0x5a,sizeof(receipt));before=receipt;
        assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],0,&receipt)==PT_MIXED_READERS_PENDING);
        assert(!memcmp(&receipt,&before,sizeof(receipt))); /* Existing public suppression unchanged. */
        err_healthy_probe(e,i,PT_MIXED_READERS_OK);}
    /* A genuine subsequent batch still obtains fresh keys and actual holder
     * current; earlier readiness did not initialize that factory key state. */
    memset(e->batch,0,sizeof(*e->batch));e->batch->count=1;
    e->batch->action[0]=f->request[4];e->batch->action[0].kind=PT_MIXED_READERS_CONTROL;
    e->batch->action[0].expected=NULL;e->batch->action[0].reader=f->reader[4];
    memset(&e->batch->action[0].geometry,0,sizeof(e->batch->action[0].geometry));
    e->batch->action[0].geometry.amigus.rate=UINT32_C(0x1234);
    e->batch->action[0].geometry.amigus.left=111;e->batch->action[0].geometry.amigus.right=222;
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,e->batch,f->command+1)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,1);
    /* Unenqueued CONTROL preparation owns no new persistent reader or proof.
     * Its actual command is cancelled by the final genuine controller close;
     * there is no invented per-command controller cancellation API. */
    if(!reader_first)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<5;++i){assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_OK);
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,f->reader[i])!=PT_MIXED_READERS_PENDING);}
    if(reader_first){assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,old)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);}
    err_source_finish(e);
}

static void err_refusals(unsigned mode)
{
    struct esb_trial *e=err_make(24,16,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_source_borrow copy=*e->borrow;struct pt_editor_mixed_reader_ref ref=f->reader[4];
    struct pt_editor_mixed_reader_record rr;struct pt_sample *samples;
    struct pt_event *events;struct pt_extension *extensions;unsigned calls,reentries;uint64_t ticket;
    ticket=err_enqueue(e);rr=f->control.reader[ref.slot];calls=e->steps;reentries=f->control.reentries;
    if(mode==0){
        assert(pt_editor_mixed_source_reader_readiness(&f->control,&copy,ref)==PT_MIXED_READERS_INVALID);
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,(struct pt_editor_mixed_reader_ref){32,ref.serial})==PT_MIXED_READERS_INVALID);
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,(struct pt_editor_mixed_reader_ref){ref.slot,UINT64_MAX})==PT_MIXED_READERS_INVALID);
        assert(pt_sampler_mixed_reader_readiness(f->control.pool,(struct pt_sampler_mixed_reader_handle){rr.handle.address,rr.handle.token+1})==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_readers_reader_readiness(f->control.queue,ticket+1,rr.action)==PT_MIXED_READERS_INVALID);
        assert(pt_mixed_readers_reader_readiness(f->control.queue,ticket,16)==PT_MIXED_READERS_INVALID);
        assert(calls==e->steps&&reentries==f->control.reentries&&!f->control.first_error);
    }else if(mode==1){
        f->owned_hook=100;
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,ref)==PT_MIXED_READERS_STALE);
        assert(e->steps==calls+1&&!f->control.source_busy&&!f->control.busy);
    }else if(mode>=2&&mode<=4){
        f->owned_hook=99+mode; /* Nested genuine controller/factory/core operation. */
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,ref)==PT_MIXED_READERS_BACKEND);
        assert(e->steps==calls+1&&!f->control.source_busy&&!f->control.busy);
    }else if(mode==5){
        f->owned_hook=104;
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,ref)==PT_MIXED_READERS_BACKEND);
        assert(f->control.first_error==PT_EDITOR_MIXED_READERS_STALE&&e->steps==calls+1);
    }else if(mode==6){
        samples=f->editor->project->samples;events=f->editor->project->events;extensions=f->editor->project->extensions;
        ++f->editor->history.revision;f->editor->project->samples=(void *)(UINTPTR_MAX-7);
        f->editor->project->events=(void *)(UINTPTR_MAX-7);f->editor->project->extensions=(void *)(UINTPTR_MAX-7);
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,ref)==PT_MIXED_READERS_STALE);
        assert(calls==e->steps&&f->control.first_error==PT_EDITOR_MIXED_READERS_STALE);
        f->editor->project->samples=samples;f->editor->project->events=events;f->editor->project->extensions=extensions;
    }else if(mode==7){
        assert(pt_editor_mixed_source_enter(e->borrow));
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,ref)==PT_MIXED_READERS_BACKEND);
        assert(f->control.source_busy&&f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT&&calls==e->steps);
        assert(pt_editor_mixed_source_leave(e->borrow));
    }else{
        assert(mode==8&&pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,ref)==PT_MIXED_READERS_BACKEND&&calls==e->steps);
    }
    /* Stop unpublished genuine domains locally; no fabricated retirement, raw
     * child close, proof retry or callback readiness certificate. */
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    esb_release(e);emp_same(f);esb_drop(e);
}

static void err_replacement(unsigned stop)
{
    struct esb_trial *e=err_make(24,16,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_reader_ref predecessor=f->reader[4];
    uint64_t first=err_enqueue(e),next=UINT64_MAX;unsigned i;
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK);
    f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,first)==PT_MIXED_ACTIVATION_COMMITTED);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    err_healthy_probe(e,4,PT_MIXED_READERS_OK);
    memset(e->batch,0,sizeof(*e->batch));e->batch->count=1;e->batch->action[0]=f->request[4];
    if(stop){e->batch->action[0].kind=PT_MIXED_READERS_STOP;e->batch->action[0].expected=NULL;
        e->batch->action[0].reader=predecessor;memset(&e->batch->action[0].geometry,0,sizeof(e->batch->action[0].geometry));}
    else{e->batch->action[0].geometry.amigus.trigger.volume=0;e->batch->action[0].geometry.amigus.trigger.pan=0;
        e->batch->levels[0]=(struct pt_sampler_mixed_trigger_levels){PT_SAMPLER_MIXED_TRIGGER_QUANTIZED,111,222};}
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);
    if(!stop)assert(pt_editor_mixed_readers_prepare_reader_reference(&f->control,f->command[0],0,f->reader+5)==PT_EDITOR_MIXED_READERS_OPEN);
    assert(pt_editor_mixed_readers_prepare_enqueue(&f->control,f->command[0],&next)==PT_MIXED_READERS_OK&&next!=first);
    /* Queued replacement/STOP already refuses old admission. This is neither
     * a clean wait nor permission to poll another proof/current key. */
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,predecessor)==PT_MIXED_READERS_STALE);
    err_healthy_probe(e,0,PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK);
    f->port.ticks=oracle(1440);
    assert(pt_mixed_activation_fire(f->control.activation,next)==PT_MIXED_ACTIVATION_COMMITTED);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,predecessor)==PT_MIXED_READERS_STALE);
    if(!stop)err_healthy_probe(e,5,PT_MIXED_READERS_OK);
    for(i=0;i<5+!stop;++i){assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==PT_MIXED_READERS_OK);
        assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,f->reader[i])!=PT_MIXED_READERS_PENDING);}
    err_source_finish(e);
}
static void err_slot_reuse(void)
{
    struct esb_trial *e=err_make(8,8,1);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_reader_ref old=f->reader[0];uint64_t ticket=err_enqueue(e);unsigned reentries;
    assert(pt_editor_mixed_readers_prepare_publish(&f->control,f->command[0])==PT_MIXED_READERS_OK);
    f->port.ticks=oracle(960);
    assert(pt_mixed_activation_fire(f->control.activation,ticket)==PT_MIXED_ACTIVATION_COMMITTED);
    emp_drain(f,0,0,5,0);
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,old)==PT_MIXED_READERS_INVALID);
    emq_batch(f,e->batch,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);assert(f->reader[0].slot==old.slot&&f->reader[0].serial>old.serial);
    (void)err_enqueue(e);reentries=f->control.reentries;
    assert(pt_editor_mixed_source_reader_readiness(&f->control,e->borrow,old)==PT_MIXED_READERS_INVALID);
    assert(reentries==f->control.reentries&&!f->control.first_error);
    err_healthy_probe(e,0,PT_MIXED_READERS_PENDING);
    err_source_finish(e);
}

static void err_issued_unadopted(void)
{
    struct trial *t=trial_make(8,8);struct model_command *c;struct model_reader *r;
    uint64_t ticket;unsigned currents,reader_calls;
    open_trial(t);trigger_input(t,1,0,960);
    assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);
    c=model_command(&t->model,ticket);r=model_reader(&t->model,c->key);assert(c&&r);
    /* Separate autonomous software-backend state: an exact original in-window
     * TRIGGER issue has been observed, while adoption is still RESERVED. The
     * existing real command callback accepts this state/timing combination.
     * Only COPIED backend records change here; no queue/holder/ref is written. */
    t->model.ticks=c->first;c->issued=1;c->fired=1;
    assert(!r->adopted&&r->state==PT_MIXED_READER_RESERVED&&!t->model.effects);
    assert(pt_mixed_readers_service_command(t->queue,ticket,0,NULL)==PT_MIXED_READERS_OK);
    currents=t->reader[0].currents;reader_calls=t->model.reader_calls;
    assert(pt_mixed_readers_reader_readiness(t->queue,ticket,0)==PT_MIXED_READERS_PENDING);
    assert(t->reader[0].currents==currents+1&&t->model.reader_calls==reader_calls);
    /* Later actual copied-model adoption, followed by a distinct explicit
     * genuine reader observation, is required before readiness becomes OK. */
    r->adopted=1;r->state=PT_MIXED_READER_ACTIVE;r->observed=r->issued=c->first;
    t->model.active[0]=r->key;++t->model.effects;
    assert(pt_mixed_readers_reader_readiness(t->queue,ticket,0)==PT_MIXED_READERS_PENDING);
    assert(pt_mixed_readers_service_reader(t->queue,ticket,0,0,NULL)==PT_MIXED_READERS_PENDING);
    assert(pt_mixed_readers_reader_readiness(t->queue,ticket,0)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_service_reader(t->queue,ticket,0,1,NULL)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_reader_readiness(t->queue,ticket,0)==PT_MIXED_READERS_INVALID);
    trial_drop(t);
}

int main(void)
{
    unsigned bits,cache,little,mode;
    assert(err_query_main()==0);
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(little=0;little<2;++little)
        err_lifetime(bits,cache,little,little);
    puts("EDITOR READER READINESS LIFETIME PASS:12 genuine mixed Paula/card8+16/endian 8/16/24 scopes; queued/published/fired remains clean PENDING until explicit real observation service; existing pending receipt suppression unchanged; complete actual ACTIVE gate then fresh CONTROL construction; by-value probes change no keys/proofs/outputs; independent C/R/source quiet lifetimes; SOFTWARE_ONLY");
    for(mode=0;mode<9;++mode)err_refusals(mode);
    puts("EDITOR READER READINESS REFUSAL PASS:9 genuine original/ref/holder-current/controller+factory+core reentry/stale-poison/cancel groups; terminal refusal never becomes PENDING; no clock/submit/service/proof/output or schedule change; cleanup uses original actual owners; SOFTWARE_ONLY");
    err_replacement(0);err_replacement(1);err_slot_reuse();
    puts("EDITOR READER READINESS ORDER PASS:genuine queued replacement and STOP refuse predecessor before and after activation; unaffected route remains active; actual retirement/NULL/serial slot reuse never becomes waiting; independent source quiet stays required; SOFTWARE_ONLY");
    err_issued_unadopted();
    puts("EDITOR READER READINESS TIMING PASS:genuine core-issued receipt may retain exact timing before adoption; clean RESERVED custody remains PENDING through real holder-current until separate copied-model adoption plus explicit reader observation; no caller key/ACTIVE flag or queue/proof mutation; SOFTWARE_ONLY");
    puts("EDITOR READER READINESS PASS:separate private bounded task operation with real current callbacks; four inert queries and existing getter/service/enums/layouts/link units unchanged; no lasting READY/ACTIVE/key/quiet/native/device/IRQ/timing/audio/listening authority");
    return 0;
}
