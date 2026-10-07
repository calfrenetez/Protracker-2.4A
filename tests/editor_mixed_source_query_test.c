/* Private query-only increment. The accepted corrected seam/legacy/quantized
 * bodies execute once through their dedicated entry selector, never a generic
 * main rewrite across nested includes. This draft waits for that corrected seam.
 * Real controller/factory ownership, callbacks and proof services remain real;
 * numeric corruption/suffix cases below are explicitly diagnostic oracles. */
#define PT_EDITOR_MIXED_SOURCE_BORROW_TEST_MAIN esq_source_main
#include "editor_mixed_source_borrow_test.c"
#undef PT_EDITOR_MIXED_SOURCE_BORROW_TEST_MAIN

static void esq_probe(struct esb_trial *e,unsigned nested)
{
    struct emp_trial *f=&e->base;struct pt_editor_mixed_source_observation o;
    struct pt_editor_mixed_source_borrow b=*e->borrow;
    struct pt_editor_mixed before=*f->binding;
    unsigned char *control=malloc(sizeof(f->control)),*parent=malloc(sizeof(*e));
    unsigned char *input=malloc(sizeof(*e->source)),*editor=malloc(sizeof(*f->editor));
    unsigned reads=f->port.reads,calls=f->ordinary.calls,masters=f->masters.calls,chip=f->chip.calls;
    assert(control&&parent&&input&&editor);
    memcpy(control,&f->control,sizeof(f->control));memcpy(parent,e,sizeof(*e));
    memcpy(input,e->source,sizeof(*e->source));memcpy(editor,f->editor,sizeof(*f->editor));
    o=pt_editor_mixed_source_observe(&f->control,e->borrow);
    assert(o.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_HELD&&o.result==f->control.result&&
        o.first_error==f->control.first_error&&o.cancel_requested==f->control.source_cancel_requested&&
        o.source_busy==f->control.source_busy&&o.controller_busy==f->control.busy&&o.fixed_tags_current<=1);
    assert(!pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,e,sizeof(*e)));
    /* Fresh complete diagnostic snapshot is disjoint from live captured/ledger
     * storage. This exercises every numeric vector while the real latch is
     * held, rather than stopping only at the whole-parent overlap refusal. */
    assert(pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,control,sizeof(f->control)));
    if(f->command[0].serial)assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,f->command[0])!=
        PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    if(f->reader[0].serial)assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[0])!=
        PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    if(nested)esq_probe(e,0);
    assert(!memcmp(control,&f->control,sizeof(f->control))&&!memcmp(parent,e,sizeof(*e))&&
        !memcmp(input,e->source,sizeof(*e->source))&&!memcmp(editor,f->editor,sizeof(*f->editor))&&
        !memcmp(&b,e->borrow,sizeof(b))&&!memcmp(&before,f->binding,sizeof(before))&&
        reads==f->port.reads&&calls==f->ordinary.calls&&masters==f->masters.calls&&chip==f->chip.calls);
    free(control);free(parent);free(input);free(editor);
}
static void *esq_master_allocate(void *context,size_t bytes)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,masters));
    struct esb_trial *e=(void *)f;
    assert(e->borrow&&e->borrow->address==&f->control&&f->control.source_busy);
    esq_probe(e,1);++e->observed;return esb_master_new(context,bytes);
}
static void *esq_allocate(void *context,size_t bytes)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,ordinary));
    struct esb_trial *e=(void *)f;assert(f->control.busy);
    esq_probe(e,1);++e->observed;return emp_allocate(context,bytes);
}
static void esq_release(void *context,void *pointer)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,ordinary));
    struct esb_trial *e=(void *)f;unsigned i;assert(f->control.busy);
    /* A lower close has consumed its local publisher before this genuine
     * release callback, but the outer controller has not yet copied that NULL.
     * Numeric registration stays PRESENT until outer service publishes it. */
    for(i=0;i<2;++i)if(f->control.command[i].handle.address==pointer&&f->command[i].serial)
        assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,f->command[i])==
            PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    for(i=0;i<32;++i)if(f->control.reader[i].handle.address==pointer&&f->reader[i].serial)
        assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[i])==
            PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    esq_probe(e,1);++e->observed;emp_release(context,pointer);
}
static void *esq_chip_allocate(void *context,size_t bytes)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,chip));
    struct esb_trial *e=(void *)f;assert(f->control.busy);
    esq_probe(e,1);++e->observed;return emp_chip_new(context,bytes);
}
static void esq_chip_release(void *context,void *pointer,size_t bytes)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,chip));
    struct esb_trial *e=(void *)f;assert(f->control.busy);
    esq_probe(e,1);++e->observed;emp_chip_free(context,pointer,bytes);
}
static int esq_command_quiet(void *context,const struct pt_mixed_activation_command_identity *identity,unsigned cancel)
{
    struct emp_trial *f=emp_from_port(context);struct esb_trial *e=(void *)f;
    assert(f->control.busy);esq_probe(e,1);++e->observed;return emp_command_quiet(context,identity,cancel);
}
static int esq_reader_quiet(void *context,const struct pt_mixed_activation_reader_identity *identity,unsigned cancel)
{
    struct emp_trial *f=emp_from_port(context);struct esb_trial *e=(void *)f;
    assert(f->control.busy);esq_probe(e,1);++e->observed;return emp_reader_quiet(context,identity,cancel);
}
static int esq_source_close(void *context,const struct pt_mixed_activation_registration *registration)
{
    struct emp_trial *f=emp_from_port(context);struct esb_trial *e=(void *)f;
    assert(f->control.busy);esq_probe(e,1);++e->observed;return emp_source_close(context,registration);
}
static int esq_source_quiet(void *context,const struct pt_mixed_activation_registration *registration)
{
    struct emp_trial *f=emp_from_port(context);struct esb_trial *e=(void *)f;
    assert(f->control.busy);esq_probe(e,1);++e->observed;return emp_source_quiet(context,registration);
}
static void esq_callbacks(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;
    f->editor->sampler.allocator.allocate=esq_master_allocate;
    f->input.activation.allocator.allocate=esq_allocate;f->input.activation.allocator.release=esq_release;
    f->input.chip_allocate=esq_chip_allocate;f->input.chip_release=esq_chip_release;
    f->input.activation.port.command_quiet=esq_command_quiet;f->input.activation.port.reader_quiet=esq_reader_quiet;
    f->input.activation.port.source_close=esq_source_close;f->input.activation.port.source_quiet=esq_source_quiet;
    /* Clock/commit/publication and actual copied-only fire remain untouched.
     * All replaced task callbacks retain their original whole-P contexts. */
}
static void esq_drop_zero(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;struct pt_editor_mixed_source_borrow *b=e->borrow;
    uintptr_t expired=(uintptr_t)&f->control;unsigned i;
    assert(!b->address&&!b->serial&&!f->binding->preparation_close);
    for(i=0;i<2;++i){emp_free(&f->masters,e->source_pcm[i],64*sizeof(int32_t),1);e->source_pcm[i]=NULL;}
    if(e->extensions){f->editor->project->extensions=NULL;f->editor->project->extension_count=0;
        free(e->extensions);free(e->extension_bytes);}
    free(e->maximum_pcm);free(e->maximum_slices);free(e->role_mutable);free(e->role_immutable);
    free(e->source);free(e->batch);emp_drop(f,0);
    /* Only the actual original zero publisher remains readable. Neither this
     * zero diagnostic nor a later zero copy authenticates prior issuance. */
    {struct pt_editor_mixed_source_observation o=pt_editor_mixed_source_observe((void *)expired,b);
        assert(o.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_ZERO&&!o.result&&!o.first_error&&
            !o.cancel_requested&&!o.source_busy&&!o.controller_busy&&!o.fixed_tags_current);}
    assert(!pt_editor_mixed_source_allocation_disjoint((void *)expired,b,b,sizeof(*b)));
    /* ZERO domain refuses both registration queries before any controller
     * access. These numeric inputs are not claimed as issued references. */
    assert(pt_editor_mixed_source_command_registration((void *)expired,b,
        (struct pt_editor_mixed_command_ref){0,UINT64_MAX})==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    assert(pt_editor_mixed_source_reader_registration((void *)expired,b,
        (struct pt_editor_mixed_reader_ref){0,UINT64_MAX})==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    assert(pt_editor_mixed_source_borrow_close(b));free(b);
}
static void esq_identity(unsigned bits)
{
    struct esb_trial *e=esb_scope(bits,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_source_borrow copy,original;struct pt_editor_mixed_source_observation o;
    struct pt_editor_mixed_readers_prepare *copied=malloc(sizeof(*copied));
    int (*hook)(void *);unsigned char *control=malloc(sizeof(f->control));assert(copied&&control);
    esb_begin(e);esq_probe(e,1);o=pt_editor_mixed_source_observe(&f->control,e->borrow);
    assert(o.fixed_tags_current==1&&!o.first_error&&!o.cancel_requested);
    copy=*e->borrow;memcpy(copied,&f->control,sizeof(*copied));
    memcpy(control,&f->control,sizeof(f->control));
    assert(pt_editor_mixed_source_observe(&f->control,&copy).scope==PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID);
    assert(pt_editor_mixed_source_observe(copied,e->borrow).scope==PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID);
    assert(!pt_editor_mixed_source_allocation_disjoint(&f->control,&copy,control,sizeof(f->control)));
    original=*e->borrow;e->borrow->serial=original.serial+1;
    assert(pt_editor_mixed_source_observe(&f->control,e->borrow).scope==PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID);
    e->borrow->serial=0;
    assert(pt_editor_mixed_source_observe(&f->control,e->borrow).scope==PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID);
    *e->borrow=original;e->borrow->address=NULL;
    assert(pt_editor_mixed_source_observe((void *)(UINTPTR_MAX-7),e->borrow).scope==PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID);
    *e->borrow=original;hook=f->binding->preparation_close;f->binding->preparation_close=NULL;
    assert(pt_editor_mixed_source_observe(&f->control,e->borrow).scope==PT_EDITOR_MIXED_SOURCE_SCOPE_INVALID);
    f->binding->preparation_close=hook;
    assert(!memcmp(control,&f->control,sizeof(f->control))&&!memcmp(copied,&f->control,sizeof(*copied)));
    assert(pt_editor_mixed_source_enter(e->borrow));esq_probe(e,1);
    o=pt_editor_mixed_source_observe(&f->control,e->borrow);assert(o.source_busy&&!o.controller_busy);
    assert(pt_editor_mixed_source_leave(e->borrow));
    esb_promote(e);esb_activate(e,1);esq_probe(e,1);
    o=pt_editor_mixed_source_observe(&f->control,e->borrow);
    assert(o.fixed_tags_current==1&&!o.first_error&&!o.cancel_requested);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);
    copy=*e->borrow;o=pt_editor_mixed_source_observe((void *)(UINTPTR_MAX-7),&copy);
    assert(o.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_ZERO&&!o.result&&!o.first_error&&!o.fixed_tags_current);
    free(copied);free(control);esq_drop_zero(e);
}
static void esq_stale(unsigned mode)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
    struct pt_project p;struct pt_sampler m;struct pt_editor_mixed_source_observation o;
    void *fresh=malloc(129);assert(fresh);esb_begin(e);p=*f->editor->project;m=f->editor->sampler;
    if(!mode){++f->editor->history.revision;
        f->editor->project->samples=(void *)(UINTPTR_MAX-7);f->editor->project->events=(void *)(UINTPTR_MAX-7);
        f->editor->project->extensions=(void *)(UINTPTR_MAX-7);
        f->editor->sampler.table=(void *)(UINTPTR_MAX-7);f->editor->sampler.table_original=(void *)(UINTPTR_MAX-7);}
    else{assert(mode==1);++e->source->mutable[0].bytes;}
    esq_probe(e,1);o=pt_editor_mixed_source_observe(&f->control,e->borrow);
    assert(o.scope==PT_EDITOR_MIXED_SOURCE_SCOPE_HELD&&!o.fixed_tags_current&&!o.first_error&&!o.cancel_requested);
    assert(pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,fresh,129));
    assert(!pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,e->source_pcm[0]+63,sizeof(int32_t)));
    assert(pt_editor_mixed_source_enter(e->borrow));esq_probe(e,1);
    o=pt_editor_mixed_source_observe(&f->control,e->borrow);
    assert(o.first_error==PT_EDITOR_MIXED_READERS_STALE&&o.cancel_requested&&o.source_busy&&!o.fixed_tags_current);
    assert(pt_editor_mixed_source_leave(e->borrow));assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    esb_release(e);*f->editor->project=p;f->editor->sampler=m;if(mode)--e->source->mutable[0].bytes;
    emp_same(f);free(fresh);esb_drop(e);
}
static void esq_span_alias(struct esb_trial *e,const void *pointer,size_t bytes)
{
    struct emp_trial *f=&e->base;unsigned char *snapshot=malloc(sizeof(f->control));assert(snapshot&&bytes);
    memcpy(snapshot,&f->control,sizeof(f->control));
    assert(!pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,pointer,1));
    assert(!pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,(const char *)pointer+bytes-1,1));
    assert(!pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,pointer,bytes));
    assert(!memcmp(snapshot,&f->control,sizeof(f->control)));free(snapshot);
}
static int esq_disjoint(struct esb_trial *e,const void *pointer,size_t bytes)
{
    struct emp_trial *f=&e->base;unsigned char *snapshot=malloc(sizeof(f->control));int result;
    unsigned reads=f->port.reads,calls=f->ordinary.calls,masters=f->masters.calls,chip=f->chip.calls;
    assert(snapshot);memcpy(snapshot,&f->control,sizeof(f->control));
    result=pt_editor_mixed_source_allocation_disjoint(&f->control,e->borrow,pointer,bytes);
    assert(!memcmp(snapshot,&f->control,sizeof(f->control))&&reads==f->port.reads&&
        calls==f->ordinary.calls&&masters==f->masters.calls&&chip==f->chip.calls);
    free(snapshot);return result;
}
static void esq_spans(void)
{
    struct esb_trial *e=esb_scope(24,1);struct emp_trial *f=&e->base;unsigned i,n;
    void *extra[6],*scratch=malloc(512),*fresh=malloc(129);struct pt_sampler_storage_span original;
    unsigned char *snapshot=malloc(sizeof(f->control));assert(scratch&&fresh&&snapshot);
    for(i=0;i<6;++i){extra[i]=malloc(128+i);assert(extra[i]);}
    /* Six real complete extra parent allocations, plus the inherited real
     * E/F/B and separately mutable request/ref/batch outputs: total14. These
     * are declared extents, not audit owners or a fabricated producer. */
    e->source->immutable_count=e->source->mutable_count=7;
    for(i=0;i<3;++i)e->source->immutable[4+i]=(struct pt_sampler_storage_span){extra[i],128+i};
    for(i=0;i<3;++i)e->source->mutable[4+i]=(struct pt_sampler_storage_span){extra[3+i],131+i};
    esb_begin(e);esq_probe(e,1);
    esq_span_alias(e,e,sizeof(*e));esq_span_alias(e,e->source,sizeof(*e->source));esq_span_alias(e,e->borrow,sizeof(*e->borrow));
    for(i=0;i<7;++i){esq_span_alias(e,e->source->immutable[i].data,e->source->immutable[i].bytes);
        esq_span_alias(e,e->source->mutable[i].data,e->source->mutable[i].bytes);}
    esq_span_alias(e,e->source_pcm[0],64*sizeof(int32_t));
    esq_span_alias(e,f->editor->sampler.table,f->editor->sampler.table_bytes);
    /* The genuinely already-owned unused spare is captured before promotion.
     * Caller-side version inspection supplies exact complete extents; the
     * production query itself never follows a version/current table. */
    {unsigned spares=0;
        for(i=0;i<PT_PROJECT_SAMPLES;++i)if(f->editor->sampler.current[i]){
            struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned j,count=0;
            ++spares;assert(pt_sampler_version_spans(f->editor->sampler.current[i],spans,
                PT_SAMPLER_VERSION_SPANS,&count));
            for(j=0;j<count;++j)if(spans[j].bytes)esq_span_alias(e,spans[j].data,spans[j].bytes);}
        assert(spares);}
    assert(esq_disjoint(e,fresh,129));
    assert(!esq_disjoint(e,NULL,129));
    assert(!esq_disjoint(e,fresh,0));
    assert(!esq_disjoint(e,(void *)(UINTPTR_MAX-4),8));
    /* Explicit numeric suffix only: test exact adjacency/full-length caller
     * obligation without inventing any source/allocation ownership. */
    n=f->control.guard_count;assert(n+1<8192);original=f->control.guards[n];
    f->control.guards[n]=(struct pt_sampler_storage_span){(char *)scratch+64,64};++f->control.guard_count;
    memcpy(snapshot,&f->control,sizeof(f->control));
    assert(esq_disjoint(e,scratch,64));
    assert(!esq_disjoint(e,scratch,65));
    assert(esq_disjoint(e,(char *)scratch+128,1));
    assert(!memcmp(snapshot,&f->control,sizeof(f->control)));
    f->control.guards[n]=original;f->control.guard_count=n;
    /* Corruption-only refusal cases are restored before any genuine cleanup.
     * Walk the last guard/ledger slots rather than silently cap a long vector. */
    for(i=n;i<8192;++i)f->control.guards[i]=(struct pt_sampler_storage_span){scratch,64};
    f->control.guard_count=8192;memcpy(snapshot,&f->control,sizeof(f->control));
    assert(esq_disjoint(e,fresh,129));
    assert(!memcmp(snapshot,&f->control,sizeof(f->control)));
    f->control.guards[8191]=(struct pt_sampler_storage_span){fresh,129};
    assert(!esq_disjoint(e,fresh,129));
    f->control.guards[8191]=(struct pt_sampler_storage_span){NULL,1};
    assert(!esq_disjoint(e,fresh,129));
    f->control.guard_count=8193;assert(!esq_disjoint(e,fresh,129));
    for(i=n;i<8192;++i)f->control.guards[i]=(struct pt_sampler_storage_span){NULL,0};f->control.guard_count=n;
    f->control.source_immutable_count=15;assert(!esq_disjoint(e,fresh,129));
    f->control.source_immutable_count=7;f->control.source_mutable_count=8;
    assert(!esq_disjoint(e,fresh,129));f->control.source_mutable_count=7;
    f->control.ordinary[36]=(struct pt_sampler_storage_span){fresh,129};
    assert(!esq_disjoint(e,fresh,129));
    f->control.ordinary[36]=(struct pt_sampler_storage_span){scratch,0};
    assert(!esq_disjoint(e,fresh,129));
    f->control.ordinary[36]=(struct pt_sampler_storage_span){NULL,0};
    f->control.chip[31]=(struct pt_sampler_storage_span){fresh,129};
    assert(!esq_disjoint(e,fresh,129));
    f->control.chip[31]=(struct pt_sampler_storage_span){NULL,1};
    assert(!esq_disjoint(e,fresh,129));
    f->control.chip[31]=(struct pt_sampler_storage_span){NULL,0};
    /* New absent-to-current masters are produced by genuine pin jobs. The
     * captured-only query is not their pre-refresh ownership guard. No audit
     * or other external control allocation is made before this one refresh. */
    esb_promote(e);esb_activate(e,1);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)if(f->pin[i]){
        struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned j,count=0;
        assert(pt_sampler_version_spans(f->pin[i],spans,PT_SAMPLER_VERSION_SPANS,&count));
        for(j=0;j<count;++j)if(spans[j].bytes)esq_span_alias(e,spans[j].data,spans[j].bytes);}
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);
    for(i=0;i<6;++i)free(extra[i]);free(snapshot);free(scratch);free(fresh);esb_drop(e);
}
static void esq_registrations(struct esb_trial *e,unsigned present)
{
    struct emp_trial *f=&e->base;unsigned i;
    enum pt_editor_mixed_source_registration_observation expected=present?
        PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT:PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT;
    assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,f->command[0])==expected);
    for(i=0;i<f->count;++i)assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[i])==expected);
    esq_probe(e,1);
}
static void esq_record_refusals(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;struct pt_editor_mixed_command_ref c=f->command[0];
    struct pt_editor_mixed_reader_ref r=f->reader[0];unsigned i;
    struct pt_editor_mixed_command_record before_c=f->control.command[c.slot];
    struct pt_editor_mixed_reader_record before_r=f->control.reader[r.slot];
    unsigned char *control=malloc(sizeof(f->control));assert(control);
    assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,(struct pt_editor_mixed_command_ref){2,c.serial})==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,(struct pt_editor_mixed_reader_ref){32,r.serial})==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,(struct pt_editor_mixed_command_ref){c.slot,0})==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,(struct pt_editor_mixed_reader_ref){r.slot,UINT64_MAX})==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
    /* Separate partial-record diagnostic refusals, never synthetic ownership.
     * The genuine saved records are restored before further service calls. */
    for(i=0;i<5;++i){f->control.command[c.slot]=before_c;f->control.reader[r.slot]=before_r;
        if(i==0){f->control.command[c.slot].handle.address=NULL;f->control.reader[r.slot].handle.address=NULL;}
        else if(i==1){f->control.command[c.slot].handle.token=0;f->control.reader[r.slot].handle.token=0;}
        else if(i==2){f->control.command[c.slot].handle.address=NULL;f->control.command[c.slot].handle.token=0;
            f->control.reader[r.slot].handle.address=NULL;f->control.reader[r.slot].handle.token=0;}
        else if(i==3){f->control.command[c.slot].serial=0;f->control.reader[r.slot].serial=0;}
        else{f->control.command[c.slot].serial=f->control.serial+1;f->control.reader[r.slot].serial=f->control.serial+1;}
        memcpy(control,&f->control,sizeof(f->control));
        assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,c)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
        assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,r)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_INVALID);
        assert(!memcmp(control,&f->control,sizeof(f->control)));}
    f->control.command[c.slot]=before_c;f->control.reader[r.slot]=before_r;free(control);
}
static void esq_refs(unsigned reader_first,unsigned fault)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_command_ref old_c;struct pt_editor_mixed_reader_ref old_r;unsigned i,observed;
    enum pt_mixed_readers_result expected;esq_callbacks(e);esb_begin(e);esb_promote(e);esb_activate(e,1);
    emq_batch(f,e->batch,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);esq_registrations(e,1);esq_record_refusals(e);emp_issue(f,0,960);
    f->port.reader_allow=0;
    {enum pt_mixed_readers_result r=pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[0],1,NULL);
        assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_PENDING);}
    assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[0])==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    f->port.reader_allow=1;old_c=f->command[0];old_r=f->reader[0];observed=e->observed;
    if(fault)assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    expected=fault?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_OK;
    if(!reader_first){assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==expected);
        assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,old_c)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
        assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,old_r)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);}
    for(i=0;i<5;++i)assert(pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL)==expected);
    if(reader_first){assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,old_c)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
        /* A real retirement proof still retains the R holder while C references
         * remain. C detach releases those refs, but only later actual controller
         * close consumes the R slot. Do not retry its consumed backend proof. */
        assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,old_r)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
        assert(pt_editor_mixed_readers_prepare_service_command(&f->control,f->command[0],0,NULL)==expected);
        assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,old_c)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
        for(i=0;i<5;++i)assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,f->reader[i])==
            PT_EDITOR_MIXED_SOURCE_REGISTRATION_PRESENT);
    }else esq_registrations(e,0);
    assert(e->observed>observed);
    if(!fault&&!reader_first){
        emq_batch(f,e->batch,5,1);
        assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,1440,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
        emq_advance(f,0);emp_refs(f,0,0);
        assert(f->command[0].slot==old_c.slot&&f->command[0].serial>old_c.serial&&
            f->reader[0].slot==old_r.slot&&f->reader[0].serial>old_r.serial);
        assert(pt_editor_mixed_source_command_registration(&f->control,e->borrow,old_c)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
        assert(pt_editor_mixed_source_reader_registration(&f->control,e->borrow,old_r)==PT_EDITOR_MIXED_SOURCE_REGISTRATION_ABSENT);
        esq_registrations(e,1);emp_issue(f,0,1440);emp_drain(f,0,0,5,reader_first);esq_registrations(e,0);
    }
    f->port.close_result=f->port.quiet_result=0;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(!pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&f->control.activation);
    /* The genuine close consumed any R-first retained holders without another
     * backend proof, while separately pending source close/quiet keeps the hook. */
    esq_registrations(e,0);assert(!pt_editor_mixed_source_borrow_close(e->borrow));
    f->port.callback_owner=NULL;f->port.close_result=f->port.quiet_result=1;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    esq_probe(e,1);esb_release(e);emp_same(f);esb_drop(e);
}
int main(void)
{
    unsigned bits,order,fault;
    assert(esq_source_main()==0);
    for(bits=8;bits<=24;bits+=8)esq_identity(bits);
    esq_stale(0);esq_stale(1);
    puts("EDITOR SOURCE QUERY IDENTITY PASS:3 genuine8/16/24 scopes; original/copy/stale/partial/hook refusals; nested held-latch diagnostics and actual zero publisher after controller expiry;2 poisoned-tag/descriptor cleanups; no query mutation/callback/clock/reentry; SOFTWARE_ONLY");
    esq_spans();
    puts("EDITOR SOURCE QUERY SPANS PASS:complete14 caller spans and full original/spare/promoted capacities; numeric-only8192-last-guard/37-ordinary/32-Chip bounds and partial-span refusals; adjacency/full-request-tail oracles; pre-refresh promotion remains genuine sampler ownership; SOFTWARE_ONLY");
    for(order=0;order<2;++order)for(fault=0;fault<2;++fault)esq_refs(order,fault);
    puts("EDITOR SOURCE QUERY REGISTRATION PASS:4 genuine paired C/R proof-order/failure-plus-NULL groups; task callback nested queries and PRESENT-during-release then actual ABSENT; original issued refs/later serial reuse;5 separately-labelled partial-record refusal groups; pending source quiet/barrier retained; SOFTWARE_ONLY");
    puts("EDITOR SOURCE QUERY PASS:private read-only captured/registration diagnostics only; no new owner/hook/layout/link unit, arbitrary issuance/full-size/READY/ACTIVE/quiet certificate, full song producer or native/device/IRQ/timing/audio/listening authority");
    return 0;
}
