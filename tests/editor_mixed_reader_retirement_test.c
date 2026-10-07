/* Private retirement SOURCE draft, NOT RUN. Inherited query/readiness and all
 * accepted underlying fixture bodies execute once under a dedicated selector.
 * No raw queue state, key, terminal flag or proof is fabricated by these tests.
 * Copied port/model state and diagnosed former-source poison are explicit test
 * oracles, distinct from genuine controller/factory/core ownership operations. */
#define PT_EDITOR_MIXED_READER_READINESS_TEST_MAIN ert_readiness_main
#include "editor_mixed_reader_readiness_test.c"
#undef PT_EDITOR_MIXED_READER_READINESS_TEST_MAIN

static int ert_command_quiet(void *context,
 const struct pt_mixed_activation_command_identity *identity,unsigned cancel)
{
    struct emp_trial *f=emp_from_port(context);
    if(f->port_hook==20){++f->port.command_calls;f->port_hook=0;return 0;}
    return emp_command_quiet(context,identity,cancel);
}
static struct esb_trial *ert_make(unsigned bits,unsigned cache,unsigned little)
{
    struct esb_trial *e=esb_scope(bits,0);struct emp_trial *f=&e->base;
    f->cache_bits=cache;f->little=little;
    f->input.activation.port.command_quiet=ert_command_quiet;
    esb_begin(e);esb_promote(e);esb_activate(e,1);emq_batch(f,e->batch,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);return e;
}
static struct pt_mixed_reader_retirement ert_retire(struct esb_trial *e,unsigned index,
 unsigned cancel,enum pt_mixed_readers_result result,unsigned consumed,unsigned calls)
{
    struct emp_trial *f=&e->base;unsigned before=f->port.reader_calls;
    struct pt_mixed_reader_retirement out=pt_editor_mixed_source_retire_original_reader(
        &f->control,e->borrow,f->reader[index],cancel);
    assert(out.result==result&&out.retirement_consumed==consumed&&
        f->port.reader_calls==before+calls);
    return out;
}
static void ert_source_finish(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;unsigned calls,releases;
    f->port.close_result=f->port.quiet_result=0;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(!f->control.queue&&!f->control.pool&&f->control.activation&&
        !pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&
        !pt_editor_mixed_source_borrow_close(e->borrow));
    calls=f->port.reader_calls;releases=f->ordinary.releases;
    /* Actual R/C owner consumption does not authorize source quiet or retry.
     * The original absent ref returns INVALID+0 and never probes the backend. */
    ert_retire(e,0,1,PT_MIXED_READERS_INVALID,0,0);
    assert(calls==f->port.reader_calls&&releases==f->ordinary.releases);
    f->port.callback_owner=NULL;f->port.quiet_result=1;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&
        pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    esb_release(e);emp_same(f);esb_drop(e);
}
static void ert_order(unsigned bits,unsigned cache,unsigned little,unsigned reader_first)
{
    struct esb_trial *e=ert_make(bits,cache,little);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_reader_ref original[5];unsigned i,calls,command_calls;
    memcpy(original,f->reader,sizeof(original));emp_issue(f,0,960);
    if(!reader_first)assert(pt_editor_mixed_readers_prepare_service_command(
        &f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<5;++i){struct pt_sampler_mixed_reader_handle h=f->control.reader[f->reader[i].slot].handle;
        ert_retire(e,i,1,PT_MIXED_READERS_OK,1,1);
        assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[i])==
            (reader_first?PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT:PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT));
        if(reader_first){
            assert(f->control.reader[f->reader[i].slot].handle.address==h.address&&
                f->control.reader[f->reader[i].slot].handle.token==h.token);
            /* The port itself erases its actual copied reader after quiet1;
             * a repeated raw reader probe would assert. Suppression is real. */
            ert_retire(e,i,1,PT_MIXED_READERS_OK,1,0);
        }else ert_retire(e,i,1,PT_MIXED_READERS_INVALID,0,0);
    }
    assert(!pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&
        !pt_editor_mixed_source_borrow_close(e->borrow));
    if(reader_first){
        calls=f->port.reader_calls;command_calls=f->port.command_calls;f->port_hook=20;
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_PENDING);
        assert(f->port.command_calls==command_calls+1&&f->port.reader_calls==calls);
        for(i=0;i<5;++i)ert_retire(e,i,1,PT_MIXED_READERS_OK,1,0);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
        for(i=0;i<5;++i){
            assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,original[i])==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
            ert_retire(e,i,1,PT_MIXED_READERS_OK,1,0);
            assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,original[i])==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
            ert_retire(e,i,1,PT_MIXED_READERS_INVALID,0,0);
        }
    }
    ert_source_finish(e);
}
static void ert_outer_cancel(void)
{
    struct esb_trial *e=ert_make(24,16,0);struct emp_trial *f=&e->base;unsigned i;
    struct pt_editor_mixed_reader_record saved;unsigned commands;
    emp_issue(f,0,960);saved=f->control.reader[f->reader[4].slot];
    assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    f->port.reader_allow=0;commands=f->port.command_calls;
    ert_retire(e,4,1,PT_MIXED_READERS_BACKEND,0,1);
    assert(!memcmp(&saved.handle,&f->control.reader[f->reader[4].slot].handle,sizeof(saved.handle))&&
        f->control.reader[f->reader[4].slot].serial==saved.serial&&f->port.command_calls==commands&&
        pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[4])==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    /* This separate explicitly allowed real service is not an automatic retry
     * in the implementation. Clean pending+outer error has not consumed R. */
    f->port.reader_allow=1;
    for(i=0;i<5;++i){ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,1);
        ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);}
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i){ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);
        ert_retire(e,i,1,PT_MIXED_READERS_INVALID,0,0);}
    ert_source_finish(e);
}
static void ert_failure_null(unsigned reader_first)
{
    struct esb_trial *e=ert_make(16,16,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_reader_ref ref=f->reader[4];unsigned i,calls,releases;
    emp_issue(f,0,960);
    if(!reader_first)assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    if(reader_first){
        ert_retire(e,4,1,PT_MIXED_READERS_OK,1,1);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    }
    /* Accepted retirement, then the genuine same reader_free's allocator
     * callback reenters/faults the controller. Original NULL still consumes
     * the outer reader record and retirement_consumed remains1 with BACKEND. */
    f->ordinary.hook=1;calls=f->port.reader_calls;releases=f->ordinary.releases;
    ert_retire(e,4,1,PT_MIXED_READERS_BACKEND,1,reader_first?0:1);
    assert(!f->ordinary.hook&&f->reentered==1&&f->ordinary.releases==releases+1&&
        pt_editor_mixed_source_reader_registration(&f->control,e->borrow,ref)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
    ert_retire(e,4,1,PT_MIXED_READERS_INVALID,0,0);
    assert(f->port.reader_calls==calls+(reader_first?0:1)&&f->ordinary.releases==releases+1);
    /* Outer fault makes later good envelopes semantically UNKNOWN, but core
     * still consumes their exact retirement and exposes error+consumed. */
    for(i=0;i<4;++i)ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,1);
    ert_source_finish(e);
}
static void ert_local_unpublished(void)
{
    struct esb_trial *e=ert_make(8,8,0);struct emp_trial *f=&e->base;unsigned i,calls;
    uint64_t ticket=err_enqueue(e);assert(ticket&&!f->port.publishes&&!f->port.reader_calls);
    assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    calls=f->port.reader_calls;
    /* Local unpublished queue cancellation releases genuine transferred R/C
     * holders with no backend reader proof or actual copied port registration.
     * Exact terminal factory owners still have to consume their original slot. */
    for(i=0;i<5;++i){ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);
        ert_retire(e,i,1,PT_MIXED_READERS_INVALID,0,0);}
    assert(f->port.reader_calls==calls&&!f->port.command_calls&&!f->port.publishes);
    ert_source_finish(e);
}
static void ert_refusals(void)
{
    struct esb_trial *e=ert_make(24,16,0);struct emp_trial *f=&e->base;unsigned i,calls,reentries;
    struct pt_editor_mixed_source_borrow copy=*e->borrow;
    struct pt_editor_mixed_reader_ref old=f->reader[4];struct pt_mixed_reader_retirement out;
    struct pt_sampler_mixed_reader_handle h=f->control.reader[old.slot].handle,saved=h;
    struct pt_editor_mixed_reader_ref bad=old;unsigned char *snapshot=malloc(sizeof(f->control));
    assert(snapshot);emp_issue(f,0,960);calls=f->port.reader_calls;reentries=f->control.reentries;
    memcpy(snapshot,&f->control,sizeof(f->control));
    out=pt_editor_mixed_source_retire_original_reader(&f->control,&copy,old,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    bad.serial=UINT64_MAX;out=pt_editor_mixed_source_retire_original_reader(&f->control,e->borrow,bad,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    bad.slot=PT_SAMPLER_MIXED_READERS;out=pt_editor_mixed_source_retire_original_reader(&f->control,e->borrow,bad,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    out=pt_editor_mixed_source_retire_original_reader(&f->control,e->borrow,old,2);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    ++h.token;out=pt_sampler_mixed_retire_original_reader(f->control.pool,&h,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed&&h.token==saved.token+1&&h.address==saved.address);
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,(void *)saved.address,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,(void *)f->control.pool,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,(void *)f->control.queue,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    /* The actual activation parent survives terminal controller close. Admit
     * the whole handle SLOT numerically, including boundary straddles and
     * unused parent capacity, before any pointed-to handle byte is read. */
    size_t pool_bytes=pt_sampler_mixed_pool_size();
    unsigned char *workspace=malloc(f->activation_capacity),*pool=malloc(pool_bytes);
    uintptr_t parent=(uintptr_t)f->activation_workspace;
    unsigned counts[]={f->ordinary.calls,f->ordinary.releases,f->masters.calls,f->masters.releases,
        f->chip.calls,f->chip.releases,f->port.reads,f->port.publishes,f->port.commits,
        f->port.command_calls,f->port.close_calls,f->port.quiet_calls,f->card->writes};
    assert(workspace&&pool&&parent>=sizeof(h)/2&&f->activation_capacity>=sizeof(h));
    memcpy(workspace,f->activation_workspace,f->activation_capacity);
    memcpy(pool,f->control.pool,pool_bytes);
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,
        (void *)((char *)f->activation_workspace+f->activation_capacity-8),1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    /* These integer addresses are never dereferenced by the fixture. The
     * original failed end-8 case above remains a separate exact regression. */
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,(void *)(parent-sizeof(h)/2),1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,
        (void *)(parent+f->activation_capacity-sizeof(h)),1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    out=pt_sampler_mixed_retire_original_reader(f->control.pool,(void *)(UINTPTR_MAX-7),1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    out=pt_mixed_readers_retire_original(f->control.queue,UINT64_MAX,4,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed);
    assert(!memcmp(snapshot,&f->control,sizeof(f->control))&&f->port.reader_calls==calls&&f->control.reentries==reentries);
    assert(!memcmp(workspace,f->activation_workspace,f->activation_capacity)&&
        !memcmp(pool,f->control.pool,pool_bytes)&&counts[0]==f->ordinary.calls&&counts[1]==f->ordinary.releases&&
        counts[2]==f->masters.calls&&counts[3]==f->masters.releases&&counts[4]==f->chip.calls&&
        counts[5]==f->chip.releases&&counts[6]==f->port.reads&&counts[7]==f->port.publishes&&
        counts[8]==f->port.commits&&counts[9]==f->port.command_calls&&counts[10]==f->port.close_calls&&
        counts[11]==f->port.quiet_calls&&counts[12]==f->card->writes);
    free(workspace);free(pool);
    free(snapshot);
    /* Genuine SOURCE exclusion refuses/faults an actual nested task operation;
     * the exact original leave is still required and cancellation is sticky. */
    assert(pt_editor_mixed_source_enter(e->borrow));
    out=pt_editor_mixed_source_retire_original_reader(&f->control,e->borrow,old,1);
    assert(out.result==PT_MIXED_READERS_BACKEND&&!out.retirement_consumed&&f->control.source_busy&&
        f->control.reentries==reentries+1&&f->port.reader_calls==calls);
    assert(pt_editor_mixed_source_leave(e->borrow));
    for(i=0;i<5;++i){ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,1);
        ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);}
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);
    ert_source_finish(e);
}
static void ert_slot_reuse(void)
{
    struct esb_trial *e=ert_make(8,8,1);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_reader_ref old=f->reader[0];struct pt_mixed_reader_retirement out;
    unsigned i,calls;emp_issue(f,0,960);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<5;++i)ert_retire(e,i,1,PT_MIXED_READERS_OK,1,1);
    emq_batch(f,e->batch,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1920,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);emp_issue(f,0,1920);
    assert(f->reader[0].slot==old.slot&&f->reader[0].serial>old.serial&&
        pt_editor_mixed_source_reader_registration(&f->control,e->borrow,old)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
    calls=f->port.reader_calls;
    out=pt_editor_mixed_source_retire_original_reader(&f->control,e->borrow,old,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed&&f->port.reader_calls==calls);
    for(i=0;i<5;++i)ert_retire(e,i,1,PT_MIXED_READERS_OK,1,1);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==PT_MIXED_READERS_OK);
    for(i=0;i<5;++i)ert_retire(e,i,1,PT_MIXED_READERS_OK,1,0);
    ert_source_finish(e);
}
static void ert_stale_captured_cleanup(void)
{
    struct esb_trial *e=ert_make(24,16,0);struct emp_trial *f=&e->base;
    struct pt_project project=*f->editor->project;struct pt_sample_version *current=f->editor->sampler.current[0];
    unsigned i,revision=f->editor->history.revision;
    emp_issue(f,0,960);++f->editor->history.revision;
    f->editor->project->samples=(void *)(UINTPTR_MAX-7);
    f->editor->project->events=(void *)(UINTPTR_MAX-7);
    f->editor->project->extensions=(void *)(UINTPTR_MAX-7);
    f->editor->sampler.current[0]=(void *)(UINTPTR_MAX-7);
    /* Diagnosed former arrays/current pointers are poison. The new retirement
     * path reads captured numeric capacities and fixed headers, but follows
     * only its genuine already-owned persistent pin for resource retirement. */
    for(i=0;i<5;++i){ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,1);
        ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);}
    /* Restore diagnostic header poison before existing independent public C
     * close: this increment intentionally does not relax public output_apart. */
    *f->editor->project=project;f->editor->sampler.current[0]=current;f->editor->history.revision=revision;
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)ert_retire(e,i,1,PT_MIXED_READERS_BACKEND,1,0);
    ert_source_finish(e);
}
static enum pt_mixed_readers_reply ert_model_reader(void *context,const struct pt_mixed_readers_domain *d,
 unsigned cancel,struct pt_mixed_readers_reader_receipt *out)
{
    struct model *m=context;struct model_reader *r=model_reader(m,&d->key);
    /* Genuine separately injected software backend rejects a repeated consumed
     * R proof: no production queue/holder/ref or proof state is fabricated. */
    assert(r&&!r->retired);return model_reader_service(context,d,cancel,out);
}
static void ert_core_errors(unsigned mode)
{
    struct trial *t=trial_make(24,16);struct pt_mixed_reader_retirement out;
    uint64_t ticket;unsigned i,calls;
    t->config.backend.reader=ert_model_reader;open_trial(t);trigger_input(t,5,0,960);
    assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);fire(&t->model,ticket);
    if(mode==0)t->model.malformed=4; /* Good exact envelope, bad semantic state. */
    else{assert(mode==1);t->model.hook=4;} /* Actual nested stop faults queue. */
    out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_BACKEND&&out.retirement_consumed&&t->model.reader_calls==1);
    calls=t->model.reader_calls;out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_BACKEND&&out.retirement_consumed&&t->model.reader_calls==calls);
    assert(t->reader[4].live&&!t->reader[4].releases);t->model.malformed=0;
    for(i=0;i<4;++i){out=pt_mixed_readers_retire_original(t->queue,ticket,i,1);
        assert(out.result==PT_MIXED_READERS_BACKEND&&out.retirement_consumed);}
    assert(pt_mixed_readers_service_command(t->queue,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)assert(!t->reader[i].live&&t->reader[i].releases==1);
    calls=t->model.reader_calls;out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed&&t->model.reader_calls==calls);
    trial_drop(t);
}
static enum pt_mixed_readers_reply ert_model_envelope_reader(void *context,
 const struct pt_mixed_readers_domain *d,unsigned cancel,
 struct pt_mixed_readers_reader_receipt *out)
{
    struct model *m=context;struct model_reader *r=model_reader(m,&d->key);
    assert(r&&!r->retired);
    if(m->malformed==3){enum pt_mixed_readers_reply reply;
        /* Genuine separately injected backend observes its actual copied
         * reader, then corrupts only the outgoing envelope. It makes no
         * retirement transition, so a later independently requested exact
         * cleanup remains possible. No production proof/holder is written. */
        assert(cancel==1);reply=model_reader_service(context,d,0,out);
        assert(reply==PT_MIXED_OBSERVATION&&!r->retired&&out->key.route==0);
        out->state=PT_MIXED_READER_RETIRED;return PT_MIXED_READER_RETIRE_PROOF;
    }
    return ert_model_reader(context,d,cancel,out);
}
static void ert_identity_invalid_envelope(void)
{
    struct trial *t=trial_make(24,16);struct pt_mixed_reader_retirement out;
    struct holder original;struct model_reader copied;struct model_reader *r;
    struct pt_mixed_readers_output *q;uint64_t ticket;unsigned i,calls,commands;
    t->config.backend.reader=ert_model_envelope_reader;
    open_trial(t);trigger_input(t,5,0,960);
    assert(pt_mixed_readers_enqueue(t->queue,t->input,&ticket)==PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_publish(t->queue,ticket)==PT_MIXED_READERS_OK);
    fire(&t->model,ticket);r=model_reader(&t->model,model_command(&t->model,ticket)->key+4);
    assert(r&&!r->retired);memcpy(&original,t->reader+4,sizeof(original));
    memcpy(&copied,r,sizeof(copied));q=t->queue;
    calls=t->model.reader_calls;commands=t->model.command_calls;t->model.malformed=3;
    out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_BACKEND&&!out.retirement_consumed&&
        t->model.reader_calls==calls+1&&t->model.command_calls==commands&&
        !memcmp(&original,t->reader+4,sizeof(original))&&
        !memcmp(&copied,r,sizeof(copied))&&t->reader[4].live&&
        !t->reader[4].terminals&&!t->reader[4].releases);
    /* This actual invalid envelope must leave the original reader and command
     * retained. Queries and a refused original close perform no retry. */
    assert(pt_mixed_readers_readers_held(t->queue)==5&&
        pt_mixed_readers_commands_held(t->queue)==1&&
        !pt_mixed_readers_close(&t->queue)&&t->queue==q&&
        t->model.reader_calls==calls+1&&t->model.command_calls==commands);
    for(i=0;i<5;++i)assert(t->reader[i].live&&!t->reader[i].releases);
    /* A separate explicit test action now permits one genuine exact envelope
     * for the SAME original identity. Sticky failure still returns BACKEND;
     * accepted settlement yields consumed1 but C retains the live holder. */
    t->model.malformed=0;calls=t->model.reader_calls;
    out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_BACKEND&&out.retirement_consumed&&
        t->model.reader_calls==calls+1&&t->reader[4].live&&
        !t->reader[4].terminals&&!t->reader[4].releases);
    calls=t->model.reader_calls;out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_BACKEND&&out.retirement_consumed&&
        t->model.reader_calls==calls&&t->model.command_calls==commands);
    for(i=0;i<4;++i){out=pt_mixed_readers_retire_original(t->queue,ticket,i,1);
        assert(out.result==PT_MIXED_READERS_BACKEND&&out.retirement_consumed);}
    assert(pt_mixed_readers_service_command(t->queue,ticket,1,NULL)==PT_MIXED_READERS_BACKEND);
    for(i=0;i<5;++i)assert(!t->reader[i].live&&t->reader[i].releases==1);
    calls=t->model.reader_calls;out=pt_mixed_readers_retire_original(t->queue,ticket,4,1);
    assert(out.result==PT_MIXED_READERS_INVALID&&!out.retirement_consumed&&t->model.reader_calls==calls);
    trial_drop(t);
}
int main(void)
{
    unsigned bits,cache,little;
    assert(ert_readiness_main()==0);
    for(bits=8;bits<=24;bits+=8)for(cache=8;cache<=16;cache+=8)for(little=0;little<2;++little)
        ert_order(bits,cache,little,little);
    puts("EDITOR READER RETIREMENT ORDER PASS:12 genuine mixed Paula/card8+16/endian 8/16/24 scopes; C-first and R-first; retained settlement suppresses backend repeat; independently pending C then original terminal NULL; source quiet remains separate; SOFTWARE_ONLY");
    ert_outer_cancel();ert_failure_null(0);ert_failure_null(1);ert_local_unpublished();
    puts("EDITOR READER RETIREMENT ERROR PASS:clean pending under sticky outer cancellation differs from consumed R-first; accepted retirement plus allocator callback fault preserves error+consumption and actual failure-plus-NULL; local unpublished retirement makes no backend R call; no backend envelope validity inferred; SOFTWARE_ONLY");
    ert_refusals();ert_slot_reuse();ert_stale_captured_cleanup();ert_core_errors(0);ert_core_errors(1);
    ert_identity_invalid_envelope();
    puts("EDITOR READER RETIREMENT REFUSAL PASS:exact original borrow/ref/handle/ticket and whole numeric slots; copied/stale/absent refs refuse; genuine nested exclusion; poisoned former tables/current pointer never traversed by private cleanup; good-envelope invalid-semantic proof and actual callback fault preserve consumption while repeat-rejecting backend is not called again; malformed-identity envelope retains original ownership/error+0 without implicit retry, then separately requested exact proof; SOFTWARE_ONLY");
    puts("EDITOR READER RETIREMENT PASS:private by-value normal result plus retirement_consumed; <=one genuine backend R operation per call; existing public services/query bodies/enums/layouts/owners/hooks/link units unchanged; no key/receipt/validity/ACTIVE/source quiet/native voice-stop/device/timing/audio/listening certificate");
    return 0;
}
