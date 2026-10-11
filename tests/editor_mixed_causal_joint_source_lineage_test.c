/* Private SOURCE-only genuine bridge fixture. Donor bodies are support only;
 * own main counts only js_make calls. No model READY/key/receipt injection. */
#include "joint_source_control16_fixture_support.inc"
#include "../src/editor/editor_mixed_causal_lineage_source_internal.h"

struct js_trial {
    struct c16_trial old;
    struct js_trial *self;
    struct pt_editor_mixed_causal_lineage_source_inputs *source;
    struct pt_editor_mixed_causal_source_borrow *borrow;
    int32_t *borrowed[2];unsigned char *mutable;
    unsigned master_reentry,master_hits;
};
typedef char js_trial_original_prefix[offsetof(struct js_trial,old)==0?1:-1];
static unsigned js_cases;
static void *js_master_allocate(void *context,size_t n)
{
    struct cs_memory *m=context;struct js_trial *e=(void *)m->owner;
    assert(e->self==e);
    if(e->master_reentry){e->master_reentry=0;++e->master_hits;
        assert(e->borrow&&e->borrow->address&&cl_s(&e->old.old)->source_busy);
        assert(!pt_editor_mixed_causal_source_enter(e->borrow));}
    return cs_master_new(context,n);
}
static struct js_trial *js_make(unsigned bits,unsigned cache_bits)
{
    struct js_trial *e=calloc(1,sizeof(*e));struct c16_trial *w=e?&e->old:NULL;struct cl_trial *c=w?&w->old:NULL;struct cs_trial *f=(void *)c;struct pt_allocator a;struct pt_amigus_reservation_api api;
    struct pt_project *p;unsigned i,j,little=0;assert(f);e->self=e;++cs_cases;f->bits=bits;f->cache_bits=cache_bits;f->little=little;
    f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
    f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
    f->request=calloc(16,sizeof(*f->request));f->command=calloc(2,sizeof(*f->command));f->reader=calloc(32,sizeof(*f->reader));
    f->stop=calloc(1,sizeof(*f->stop));
    assert(f->card&&f->document&&f->editor&&f->binding&&f->request&&f->command&&f->reader&&f->stop);f->bus.owner=f;f->ordinary.owner=f;f->masters.owner=f;f->chip.owner=f;f->port.owner=f;f->port.bind_raw=1;f->port.quiet_raw=1;
    a=(struct pt_allocator){&f->masters,js_master_allocate,cs_master_free};
    pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,SIZE_MAX)==PT_PROJECT_OK);
    p=&f->document->project;
    for(i=0;i<16;++i)p->channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
    for(i=0;i<2;++i){int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
        for(j=0;j<64;++j)f->original[i][j]=values[j%12]*(int32_t)(1U<<(bits-8));
        e->borrowed[i]=malloc(sizeof(f->original[i]));assert(e->borrowed[i]);
        memcpy(e->borrowed[i],f->original[i],sizeof(f->original[i]));
        p->samples[i].pcm=(struct pt_pcm){e->borrowed[i],64,6,8000,2,(uint8_t)bits};p->samples[i].volume=64;}
    assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,SIZE_MAX);
    /* An unused already-owned master with real spare PCM capacity exercises
     * the entire capacity ledger, independently of played sample geometry. */
    {struct pt_pcm spare;int32_t *values=cs_new(&f->masters,64*sizeof(int32_t));
        for(i=0;i<64;++i)values[i]=(int32_t)i;
        spare=(struct pt_pcm){values,64,1,8000,1,24};
        assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK);
        assert(!spare.data);}
    for(i=0;i<p->sample_count;++i){
        if(i<2)memcpy(f->pcm+i,&p->samples[i].pcm,sizeof(f->pcm[i]));
        else assert(pt_sampler_pin(&f->editor->sampler,p,i,f->editor->sampler.generation,&f->pcm[i],&f->pin[i])==PT_EDIT_OK);
    }
    /* Pinning materializes frames*channels values from borrowed input. Retain
     * independent full declared materialized capacity and borrowed-tail images;
     * borrowed capacity is not the size of the new version allocation. */
    for(i=0;i<2;++i){assert(f->pcm[i].capacity<=64);
        memcpy(f->master_before[i],f->pcm[i].data,f->pcm[i].capacity*sizeof(int32_t));
        memcpy(f->borrowed_before[i],f->original[i],sizeof(f->original[i]));}
    assert(pt_editor_mixed_attach(f->binding,f->editor));
    f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
    api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
    assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,cl_owned,cl_write));
    f->input.original.binding=f->binding;f->input.original.backend=&f->card->cache;
    f->input.original.causal.allocator=(struct pt_allocator){&f->ordinary,cs_allocate,cs_release};
    f->input.original.causal.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
    f->input.original.causal.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.original.causal.session=31;
    f->input.original.causal.control_budget=pt_mixed_causal_control_size();
    f->input.original.causal.queue_budget=pt_mixed_readers_control_size();
    f->input.original.causal.port=(struct pt_mixed_causal_port){&f->port,sizeof(f->port),PT_MIXED_CAUSAL_PORT_VERSION,
        PT_MIXED_CAUSAL_PORT_REQUIRED,cs_clock,cs_publish,cs_commit,cs_command_quiet,cs_reader_quiet,
        cs_source_close,cs_source_quiet,cs_publish_successor};
    f->input.original.bind_original=cs_bind;
    f->port.model.base.ticks=100;f->port.model.base.frequency=709379;
    f->port.model.base.commit_raw=f->port.model.base.source_raw=1;f->port.model.publication_raw=1;
    f->input.stop=(struct pt_mixed_causal_stop_port){&f->port,sizeof(f->port),
        PT_MIXED_CAUSAL_STOP_VERSION,PT_MIXED_CAUSAL_STOP_REQUIRED,cs_publish_stop};
    f->input.original.chip_context=&f->chip;f->input.original.chip_allocate=cs_chip_new;f->input.original.chip_release=cs_chip_free;
    f->input.original.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
    f->input.original.chip_budget=4096;f->input.original.contexts=(struct pt_sampler_storage_span){f,sizeof(*f)};
    f->causal_capacity=pt_mixed_causal_workspace_size()+128;
    f->factory_capacity=pt_sampler_mixed_workspace_size()+128;
    f->causal_workspace=calloc(1,f->causal_capacity);f->factory_workspace=calloc(1,f->factory_capacity);
    assert(f->causal_workspace&&f->factory_workspace);
    f->input.original.causal_workspace=f->causal_workspace;f->input.original.causal_capacity=f->causal_capacity;
    f->input.original.factory_workspace=f->factory_workspace;f->input.original.factory_capacity=f->factory_capacity;
    f->saved=cs_save(f,&f->saved_bytes);
    ++cl_cases;c->port.owner=c;c->port.model.base.ticks=100;c->port.model.base.frequency=709379;
    c->port.model.base.commit_raw=c->port.model.base.source_raw=1;
    c->port.control_raw=c->port.third_raw=c->port.quiet_raw=1;
    c->change=calloc(1,sizeof(*c->change));c->stop=calloc(1,sizeof(*c->stop));
    c->command=calloc(3,sizeof(*c->command));assert(c->change&&c->stop&&c->command);
    memcpy(&c->input.original,&f->input.original,sizeof(c->input.original));
    c->input.original.causal.allocator=(struct pt_allocator){&f->ordinary,cl_allocate,cl_release};
    c->input.original.causal.port=(struct pt_mixed_causal_port){&c->port,sizeof(c->port),
        PT_MIXED_CAUSAL_PORT_VERSION,PT_MIXED_CAUSAL_PORT_REQUIRED,cl_clock,cl_publish,cl_commit,
        cl_command_quiet,cl_reader_quiet,cl_source_close,cl_source_quiet,cl_publish_successor};
    c->input.original.bind_original=cl_bind;
    c->input.original.chip_allocate=cl_chip_new;c->input.original.chip_release=cl_chip_free;
    c->input.original.contexts=(struct pt_sampler_storage_span){c,sizeof(*c)};
    c->input.lineage=(struct pt_mixed_causal_control_stop_port){&c->port,sizeof(c->port),
        PT_MIXED_CAUSAL_CONTROL_STOP_VERSION,PT_MIXED_CAUSAL_CONTROL_STOP_REQUIRED,
        cl_publish_control,cl_publish_stop};
    w->self=w;w->wide=calloc(1,sizeof(*w->wide));assert(w->wide);
    c->input.original.contexts=(struct pt_sampler_storage_span){w,sizeof(*w)};
    c->input.original.causal.allocator.allocate=c16_allocate;
    c->input.lineage.publish_control=c16_publish_control;
    e->source=calloc(1,sizeof(*e->source));e->borrow=calloc(1,sizeof(*e->borrow));e->mutable=calloc(1,256);
    assert(e->source&&e->borrow&&e->mutable);
    c->input.original.contexts=(struct pt_sampler_storage_span){e,sizeof(*e)};
    e->source->original.binding=f->binding;e->source->original.preparation=&c->input.original;
    e->source->preparation=&c->input;e->source->original.contexts=c->input.original.contexts;
    e->source->original.causal_workspace=(struct pt_sampler_storage_span){f->causal_workspace,f->causal_capacity};
    e->source->original.factory_workspace=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
    e->source->original.backend_parent=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
    e->source->original.immutable[0]=e->source->original.causal_workspace;
    e->source->original.immutable[1]=e->source->original.factory_workspace;
    e->source->original.immutable[2]=e->source->original.backend_parent;
    e->source->original.immutable[3]=(struct pt_sampler_storage_span){f->saved,f->saved_bytes};
    e->source->original.immutable_count=4;
    /* Genuine fixture request/ref/control publishers are mutable for their full
     * lifetime. All complete capacities stay declared and protected; none is a
     * controller input/output target because mode2 excludes mutable spans too. */
    e->source->original.mutable[0]=(struct pt_sampler_storage_span){e->mutable,256};
    e->source->original.mutable[1]=(struct pt_sampler_storage_span){f->request,16*sizeof(*f->request)};
    e->source->original.mutable[2]=(struct pt_sampler_storage_span){f->reader,32*sizeof(*f->reader)};
    e->source->original.mutable[3]=(struct pt_sampler_storage_span){c->command,3*sizeof(*c->command)};
    e->source->original.mutable[4]=(struct pt_sampler_storage_span){w->wide,sizeof(*w->wide)};
    e->source->original.mutable[5]=(struct pt_sampler_storage_span){c->change,sizeof(*c->change)};
    e->source->original.mutable[6]=(struct pt_sampler_storage_span){c->stop,sizeof(*c->stop)};
    e->source->original.mutable_count=7;
    ++js_cases;return e;
}

static void js_same(struct js_trial *e)
{
    unsigned i;assert(e->self==e);cs_same(&e->old.old.old);
    for(i=0;i<2;++i)assert(!memcmp(e->borrowed[i],e->old.old.old.borrowed_before[i],64*sizeof(int32_t)));
}
static void js_begin(struct js_trial *e)
{
    struct cl_trial *c=&e->old.old;struct cs_trial *f=&c->old;
    unsigned masters=f->masters.calls,owned=f->owned_calls;
    assert(!f->editor->sampler.current[0]&&!f->editor->sampler.current[1]);
    assert(pt_editor_mixed_causal_lineage_source_begin(&c->joint,e->source,e->borrow)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(e->borrow->address==cl_s(c)&&e->borrow->serial&&cl_s(c)->source_mode==2&&
        f->binding->preparation_context==cl_s(c)&&f->binding->preparation_close&&
        !cl_s(c)->source_activated&&!cl_s(c)->pool&&!cl_s(c)->queue&&!cl_s(c)->causal&&
        !f->ordinary.calls&&!f->chip.calls&&!f->card->writes&&!c->port.binds&&
        f->masters.calls==masters&&f->owned_calls==owned);
    js_same(e);
}
static void js_promote(struct js_trial *e)
{
    struct cs_trial *f=&e->old.old.old;unsigned i;
    for(i=0;i<2;++i){struct pt_sampler_pin_job job={0};struct pt_pcm pcm;
        struct pt_sample_version *independent=NULL;unsigned ready=0,steps=0;
        assert(pt_editor_mixed_causal_source_enter(e->borrow));
        assert(pt_sampler_pin_job_begin(&job,&f->editor->sampler,f->editor->project,i,f->editor->sampler.generation)==PT_EDIT_OK);
        while(!ready){enum pt_edit_result r=pt_sampler_pin_job_step(&job,3,&f->pcm[i],&f->pin[i],&ready);
            assert(r==PT_EDIT_OK&&++steps<10000);}
        assert(!job.value&&f->pin[i]&&f->pin[i]==f->editor->sampler.current[i]);
        assert(pt_sampler_pin_current(&f->editor->sampler,f->editor->project,i,
            f->editor->sampler.generation,f->pin[i],&pcm,&independent)==PT_EDIT_OK);
        pt_sampler_pin_job_cancel(&job);pt_sampler_unpin(f->pin[i]);f->pin[i]=independent;
        assert(independent==f->editor->sampler.current[i]&&pcm.data==f->pcm[i].data);
        memcpy(f->master_before[i],f->pcm[i].data,f->pcm[i].capacity*sizeof(int32_t));
        assert(pt_editor_mixed_causal_source_leave(e->borrow));
    }
    assert(!cl_s(&e->old.old)->source_busy);js_same(e);
}
static void js_activate(struct js_trial *e)
{
    struct cl_trial *c=&e->old.old;unsigned guards=cl_s(c)->guard_count;
    assert(pt_editor_mixed_causal_lineage_source_activate(&c->joint,e->borrow,&c->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(cl_s(c)->source_activated&&cl_s(c)->guard_count>guards&&!c->old.ordinary.calls&&
        !c->old.chip.calls&&!c->old.card->writes&&!c->port.binds);
}
static void js_open(struct js_trial *e)
{
    struct cl_trial *c=&e->old.old;enum pt_editor_mixed_readers_result r;unsigned steps=0;
    js_begin(e);js_promote(e);js_activate(e);
    do{r=pt_editor_mixed_causal_prepare_advance_validation(cl_s(c),7);assert(++steps<10000);}
    while(r==PT_EDITOR_MIXED_READERS_PENDING);
    assert(r==PT_EDITOR_MIXED_READERS_OPEN&&c->port.binds==1&&c->old.ordinary.calls==3&&
        cl_s(c)->pool&&cl_s(c)->queue&&cl_s(c)->causal&&!c->old.chip.calls&&!c->old.card->writes&&
        !pt_mixed_readers_commands_held(cl_s(c)->queue)&&!pt_mixed_readers_readers_held(cl_s(c)->queue));
}
static uint64_t js_first(struct js_trial *e)
{
    struct cl_trial *c=&e->old.old;unsigned i;uint64_t ticket;
    struct {
        struct pt_editor_mixed_readers_request input[16];
        struct pt_editor_mixed_command_ref command;
        struct pt_editor_mixed_reader_ref reader;
    } local;
    struct pt_editor_mixed_command_ref *commands=c->command;
    struct pt_editor_mixed_reader_ref *readers=c->old.reader;
    js_open(e);cs_requests(&c->old,16);cl_source_local(c,(uintptr_t)&local,sizeof(local));
    memcpy(local.input,c->old.request,sizeof(local.input));memset(&local.command,0,sizeof(local.command));
    assert(pt_editor_mixed_causal_prepare_batch_begin(cl_s(c),960,local.input,16,&local.command)==PT_EDITOR_MIXED_READERS_PENDING);
    cl_command_copy(c,commands,0,&local.command);cl_advance(c,0);
    for(i=0;i<16;++i){
        assert(pt_editor_mixed_causal_prepare_reader_reference(cl_s(c),c->command[0],i,&local.reader)==PT_EDITOR_MIXED_READERS_OPEN);
        cl_reader_copy(c,readers,i,&local.reader);
    }
    ticket=cl_enqueue(c,0);assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[0])==PT_MIXED_READERS_OK);
    c->port.model.base.ticks=oracle(960)-1;assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_EARLY);
    c->port.model.base.ticks++;assert(pt_mixed_causal_fire(cl_s(c)->causal,ticket)==PT_MIXED_CAUSAL_COMMITTED&&
        c->port.root.frame==960&&c->port.root.observed==oracle(960)&&c->port.root.issued==oracle(960));
    c->port.model.hold_first=1;
    assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[0],0,NULL)==PT_MIXED_READERS_PENDING);
    return ticket;
}
static void js_drain(struct js_trial *e)
{
    struct cl_trial *c=&e->old.old;struct pt_editor_mixed_causal_prepare *s=cl_s(c);unsigned i,unpublished=0;
    c->allocation_reentry=c->allocation_mutation=c->scratch_alias=c->port.task_hook=c->old.owned_hook=0;
    c->old.ordinary.alias=c->old.chip.alias=NULL;
    c->port.model.hold_first=c->port.hold_control=c->port.hold_third=0;c->port.model.quiet_mask=~0U;
    if(s->pool){
        for(i=0;i<PT_SAMPLER_MIXED_COMMANDS;++i)if(s->command[i].handle.address&&s->command[i].transferred){
            struct pt_editor_mixed_command_ref ref={i,s->command[i].serial};
            enum pt_mixed_readers_result r=pt_editor_mixed_causal_prepare_service_command(s,ref,1,NULL);
            if(r==PT_MIXED_READERS_INVALID){assert(c->unpublished_ticket&&!unpublished&&
                c->unpublished_ref.slot==ref.slot&&c->unpublished_ref.serial==ref.serial&&
                s->command[i].ticket==c->unpublished_ticket&&s->command[i].transferred&&!s->first_error);++unpublished;}
            else assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND);
        }
        for(i=0;i<PT_SAMPLER_MIXED_READERS;++i)if(s->reader[i].handle.address&&s->reader[i].ticket){
            struct pt_editor_mixed_reader_ref ref={i,s->reader[i].serial};
            enum pt_mixed_readers_result r=pt_editor_mixed_causal_prepare_service_reader(s,ref,1,NULL);
            assert(r==PT_MIXED_READERS_OK||r==PT_MIXED_READERS_BACKEND||r==PT_MIXED_READERS_INVALID);
        }
    }
    if(c->unpublished_ticket)assert(unpublished==1);
}
static void js_drop(struct js_trial *e)
{
    struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;struct cs_trial *f=&c->old;
    struct pt_editor_mixed_causal_source_borrow *borrow=e->borrow;
    struct pt_editor_mixed_causal_lineage_source_inputs *source=e->source;
    int32_t *borrowed0=e->borrowed[0],*borrowed1=e->borrowed[1];
    unsigned char *mutable=e->mutable;struct pt_editor_mixed_causal_lineage_control16_batch *wide=w->wide;unsigned i;
    js_same(e);c->port.quiet_raw=1;js_drain(e);
    if(borrow->address){
        assert(!pt_editor_mixed_causal_prepare_close(cl_s(c))&&!cl_s(c)->pool&&!cl_s(c)->queue&&!cl_s(c)->causal);
        assert(pt_editor_mixed_causal_source_children_closed(cl_s(c),borrow));
        assert(pt_editor_mixed_causal_source_enter(borrow));
        for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
        assert(pt_editor_mixed_causal_source_leave(borrow));
        assert(pt_editor_mixed_causal_source_borrow_close(borrow)&&!borrow->address&&!borrow->serial);
    }
    assert(pt_editor_mixed_causal_prepare_close(cl_s(c)));js_same(e);
    /* The genuine unpublished observation was already checked by js_drain.
     * Retire only its fixture tag after actual command and SOURCE retirement. */
    if(c->unpublished_ticket){
        assert(c->unpublished_ref.slot<PT_SAMPLER_MIXED_COMMANDS&&c->unpublished_ref.serial);
        assert(!cl_s(c)->pool&&!cl_s(c)->queue&&!cl_s(c)->causal);
        assert(!cl_s(c)->command[c->unpublished_ref.slot].handle.address&&
            !cl_s(c)->command[c->unpublished_ref.slot].handle.token&&
            !cl_s(c)->command[c->unpublished_ref.slot].serial&&
            !cl_s(c)->command[c->unpublished_ref.slot].ticket&&
            !cl_s(c)->command[c->unpublished_ref.slot].count&&
            !cl_s(c)->command[c->unpublished_ref.slot].transferred);
        for(i=0;i<PT_SAMPLER_MIXED_ACTIONS;++i)
            assert(!cl_s(c)->command[c->unpublished_ref.slot].reader[i]);
        c->unpublished_ticket=0;c->unpublished_ref.slot=0;c->unpublished_ref.serial=0;
    }
    /* cl_drop now sees a genuinely closed original SOURCE; its unchanged
     * ledger/resource teardown cannot substitute for that earlier proof. */
    cl_drop(c);free(wide);free(borrow);free(source);free(borrowed0);free(borrowed1);free(mutable);
}
static void js_success(unsigned bits,unsigned narrow,unsigned order)
{
    struct js_trial *e=js_make(bits,16);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;
    struct c16_counts first;struct pt_editor_mixed_reader_record rows[32];
    struct {
        struct pt_editor_mixed_causal_lineage_stop_batch stop;
        struct pt_editor_mixed_causal_lineage_control16_batch wide;
        struct pt_editor_mixed_command_ref refused;
    } local;
    uint64_t root,control,stop;unsigned i;
    root=js_first(e);c16_input(w,narrow);first=c16_counts(c);memcpy(rows,cl_s(c)->reader,sizeof(rows));
    c16_prepare(w,narrow);control=cl_enqueue(c,1);
    assert(pt_mixed_readers_commands_held(cl_s(c)->queue)==2&&
        pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1])==PT_MIXED_READERS_OK);c16_packet(w,root,control);
    c->port.model.base.ticks=oracle(1920)-1;assert(pt_mixed_causal_fire(cl_s(c)->causal,control)==PT_MIXED_CAUSAL_EARLY);
    c->port.model.base.ticks++;assert(pt_mixed_causal_fire(cl_s(c)->causal,control)==PT_MIXED_CAUSAL_COMMITTED);
    cl_stop_input(c,16);cl_source_local(c,(uintptr_t)&local,sizeof(local));
    memcpy(&local.stop,c->stop,sizeof(local.stop));local.refused=(struct pt_editor_mixed_command_ref){99,999};
    assert(pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(&c->joint.original,&local.stop,&local.refused)==PT_EDITOR_MIXED_READERS_INVALID&&
        local.refused.slot==99&&local.refused.serial==999&&c->old.ordinary.calls==first.calls+1);
    cl_detach_root(c);cl_later_prepare(c,1);stop=cl_enqueue(c,2);
    assert(control!=root&&stop!=control&&stop!=root&&c->command[2].slot==c->command[0].slot&&
        c->command[2].serial!=c->command[0].serial&&pt_mixed_readers_commands_held(cl_s(c)->queue)==2);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[2])==PT_MIXED_READERS_OK);
    c->port.model.base.ticks=oracle(2880)-1;assert(pt_mixed_causal_fire(cl_s(c)->causal,stop)==PT_MIXED_CAUSAL_EARLY);
    c->port.model.base.ticks++;assert(pt_mixed_causal_fire(cl_s(c)->causal,stop)==PT_MIXED_CAUSAL_COMMITTED&&
        !memcmp(rows,cl_s(c)->reader,sizeof(rows))&&c->port.model.controls==16&&c->port.model.stops==16&&
        c->old.ordinary.calls==first.calls+2&&c->old.chip.calls==first.chip&&c->old.masters.calls==first.masters&&
        c->old.card->writes==first.writes&&cs_cache_pins(&c->old)==first.pins&&c->high_water<=3+2+16);
    memcpy(&local.wide,w->wide,sizeof(local.wide));
    assert(pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->joint.original,&local.wide,&local.refused)==PT_EDITOR_MIXED_READERS_CAPACITY);
    for(i=0;i<16;++i){struct {
            struct pt_mixed_readers_reader_receipt actual,before;
            struct pt_mixed_readers_key key;
        } receipt;
        cl_source_local(c,(uintptr_t)&receipt,sizeof(receipt));memset(&receipt.actual,0xa5,sizeof(receipt.actual));
        memcpy(&receipt.before,&receipt.actual,sizeof(receipt.before));
        assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),c->old.reader[i],0,&receipt.actual)==PT_MIXED_READERS_PENDING&&
            !memcmp(&receipt.actual,&receipt.before,sizeof(receipt.actual))&&
            pt_mixed_causal_reader_key(cl_s(c)->causal,root,i,&receipt.key)==PT_MIXED_READERS_STALE);}
    if(!order){assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[1],0,NULL)==PT_MIXED_READERS_OK);
        assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[2],0,NULL)==PT_MIXED_READERS_OK);}
    c->port.model.quiet_mask=~0U;
    for(i=0;i<16;++i){assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),c->old.reader[i],0,NULL)==PT_MIXED_READERS_OK);
        if(order)assert(cl_s(c)->reader[c->old.reader[i].slot].handle.address);}
    if(order){assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[1],0,NULL)==PT_MIXED_READERS_OK);
        assert(pt_editor_mixed_causal_prepare_service_command(cl_s(c),c->command[2],0,NULL)==PT_MIXED_READERS_OK);}
    assert(!pt_mixed_readers_commands_held(cl_s(c)->queue)&&!pt_mixed_readers_readers_held(cl_s(c)->queue));js_drop(e);
}
static void js_early(unsigned busy)
{
    struct js_trial *e=js_make(24,16);struct cl_trial *c=&e->old.old;js_begin(e);
    if(busy){assert(pt_editor_mixed_causal_source_enter(e->borrow));
        assert(!pt_editor_mixed_causal_prepare_close(cl_s(c))&&cl_s(c)->source_busy&&cl_s(c)->source_held);
        assert(pt_editor_mixed_causal_source_leave(e->borrow));}
    assert(!pt_editor_mixed_causal_prepare_close(cl_s(c))&&pt_editor_mixed_causal_source_children_closed(cl_s(c),e->borrow)&&
        !c->old.ordinary.calls&&!c->old.chip.calls&&!c->old.card->writes&&!c->port.binds);js_drop(e);
}
static void js_identity(unsigned copy_wrapper)
{
    struct js_trial *e=js_make(16,8);struct cl_trial *c=&e->old.old;
    struct pt_editor_mixed_causal_source_borrow copy;struct pt_editor_mixed_causal_lineage_source_prepare *other;
    unsigned char *before=malloc(sizeof(c->joint));assert(before);js_begin(e);memcpy(&copy,e->borrow,sizeof(copy));
    memcpy(before,&c->joint,sizeof(c->joint));other=malloc(sizeof(*other));assert(other);memcpy(other,&c->joint,sizeof(*other));
    assert(pt_editor_mixed_causal_lineage_source_activate(copy_wrapper?other:&c->joint,copy_wrapper?e->borrow:&copy,&c->input)==PT_EDITOR_MIXED_READERS_INVALID&&
        !memcmp(before,&c->joint,sizeof(c->joint))&&!cl_s(c)->first_error&&!c->old.ordinary.calls&&!c->port.binds);
    assert(pt_editor_mixed_causal_source_activate(cl_s(c),e->borrow,&c->input.original)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!pt_editor_mixed_causal_source_borrow_close(&copy)&&copy.address==e->borrow->address&&copy.serial==e->borrow->serial);
    free(other);free(before);js_drop(e);
}
static void js_begin_guard(unsigned mode)
{
    struct js_trial *e=js_make(24,16);struct cl_trial *c=&e->old.old;
    struct pt_editor_mixed_causal_lineage_source_inputs saved=*e->source;
    struct pt_editor_mixed_causal_source_borrow *out=e->borrow;
    struct pt_editor_mixed_causal_lineage_source_inputs *input=e->source;
    unsigned char *before=malloc(sizeof(*e));assert(before);assert(mode<6);
    if(mode==0)e->source->original.contexts=(struct pt_sampler_storage_span){&c->joint,sizeof(c->joint.original)};
    if(mode==1)out=(void *)((unsigned char *)&c->joint+sizeof(c->joint)-sizeof(*out));
    if(mode==2){input=&c->joint.saved;memcpy(input,e->source,sizeof(*input));}
    if(mode==3){e->source->preparation=&c->joint.original.saved;e->source->original.preparation=&e->source->preparation->original;}
    if(mode==4)e->source->original.immutable[0]=(struct pt_sampler_storage_span){(unsigned char *)&c->joint+sizeof(c->joint)-8,e->source->original.causal_workspace.bytes};
    if(mode==5)e->source->original.mutable[0]=(struct pt_sampler_storage_span){(unsigned char *)&c->joint+sizeof(c->joint)-8,256};
    memcpy(before,e,sizeof(*e));
    assert(pt_editor_mixed_causal_lineage_source_begin(&c->joint,input,out)==PT_EDITOR_MIXED_READERS_INVALID&&
        !memcmp(before,e,sizeof(*e))&&!e->borrow->address&&!e->borrow->serial&&!c->old.ordinary.calls&&
        !c->old.chip.calls&&!c->old.card->writes&&!c->port.binds&&!c->old.binding->preparation_context);
    memcpy(e->source,&saved,sizeof(saved));memset(&c->joint,0,sizeof(c->joint));free(before);js_drop(e);
}
static void js_activate_guard(unsigned mode)
{
    struct js_trial *e=js_make(24,16);struct cl_trial *c=&e->old.old;
    struct pt_editor_mixed_causal_lineage_inputs saved;unsigned char *before;unsigned guards;
    js_begin(e);js_promote(e);memcpy(&saved,&c->input,sizeof(saved));assert(mode<4);
    if(mode==0)++c->input.lineage.flags;
    if(mode==1)++c->input.lineage.version;
    if(mode==2)c->input.lineage.publish_control=NULL;
    if(mode==3)c->input.lineage.context=&c->old.ordinary;
    before=malloc(sizeof(c->joint));assert(before);memcpy(before,&c->joint,sizeof(c->joint));guards=cl_s(c)->guard_count;
    assert(pt_editor_mixed_causal_lineage_source_activate(&c->joint,e->borrow,&c->input)==PT_EDITOR_MIXED_READERS_INVALID&&
        !memcmp(before,&c->joint,sizeof(c->joint))&&!cl_s(c)->source_activated&&cl_s(c)->source_held&&
        cl_s(c)->guard_count==guards&&!c->old.ordinary.calls&&!c->old.chip.calls&&!c->old.card->writes&&!c->port.binds);
    memcpy(&c->input,&saved,sizeof(saved));free(before);js_drop(e);
}
static void js_changed(unsigned mode)
{
    struct js_trial *e=js_make(24,16);struct cl_trial *c=&e->old.old;unsigned calls;
    js_open(e);calls=c->old.ordinary.calls;assert(mode<3);
    if(mode==0)++e->source->original.mutable[0].bytes;
    if(mode==1)++c->input.lineage.flags;
    if(mode==2)++c->old.editor->sampler.generation;
    assert(pt_editor_mixed_causal_prepare_get(cl_s(c))==PT_EDITOR_MIXED_READERS_STALE&&
        cl_s(c)->first_error==PT_EDITOR_MIXED_READERS_STALE&&cl_s(c)->source_held&&c->old.ordinary.calls==calls);
    if(mode==0)--e->source->original.mutable[0].bytes;
    if(mode==1)--c->input.lineage.flags;
    /* Fixed generation staleness is retained through actual cleanup. Neither
     * restored inputs nor a true quiet response erases the first STALE. */
    assert(cl_s(c)->first_error==PT_EDITOR_MIXED_READERS_STALE);js_drop(e);
}
static void js_allocator(unsigned mode)
{
    struct js_trial *e=js_make(24,16);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;
    struct c16_counts before;enum pt_editor_mixed_readers_result r;
    struct {
        struct pt_editor_mixed_causal_lineage_control16_batch wide;
        struct pt_editor_mixed_command_ref out;
    } *local;
    struct pt_editor_mixed_reader_record rows[32];assert(mode<4);(void)js_first(e);c16_input(w,0);
    before=c16_counts(c);memcpy(rows,cl_s(c)->reader,sizeof(rows));
    local=malloc(sizeof(*local));assert(local);
    cl_source_local(c,(uintptr_t)local,sizeof(*local));assert(!w->active_wide);
    memcpy(&local->wide,w->wide,sizeof(local->wide));local->out=(struct pt_editor_mixed_command_ref){99,999};
    if(mode==0)c->old.ordinary.fail=before.calls+1;
    if(mode==1)c->allocation_reentry=1;
    if(mode==2)w->mutate_wide=1;
    if(mode==3)c->scratch_alias=1;
    w->active_wide=&local->wide;
    r=pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->joint.original,&local->wide,&local->out);
    w->active_wide=NULL;
    assert(r!=PT_EDITOR_MIXED_READERS_PENDING&&local->out.slot==99&&local->out.serial==999&&
        c->old.ordinary.calls==before.calls+1&&c->joint.original.stage==2&&!c->joint.original.constructing&&
        !memcmp(rows,cl_s(c)->reader,sizeof(rows))&&c->old.chip.calls==before.chip&&
        c->old.masters.calls==before.masters&&c->old.card->writes==before.writes&&cs_cache_pins(&c->old)==before.pins);
    if(mode==0)assert(r==PT_EDITOR_MIXED_READERS_CAPACITY&&!cl_s(c)->first_error&&c->old.ordinary.refusals);
    else assert(cl_s(c)->first_error);
    if(mode==2){const struct pt_editor_mixed_command_record *record=cl_s(c)->command+1;
        assert(record->handle.address&&record->handle.token&&record->serial&&!record->transferred&&
            !record->ticket&&cs_live(&c->old.ordinary)==before.live+1);
        assert(local->wide.frame==w->wide->frame+1&&w->wide->frame==1920);}
    else assert(!cl_s(c)->command[1].handle.address&&cs_live(&c->old.ordinary)==before.live);
    if(mode==1)assert(c->old.ordinary.releases==before.releases+1);
    if(mode==3)assert(c->scratch_hits==1&&!c->scratch_alias&&!c->old.ordinary.alias_releases);
    assert(pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(&c->joint.original,&local->wide,&local->out)!=PT_EDITOR_MIXED_READERS_PENDING&&
        c->old.ordinary.calls==before.calls+1);js_drop(e);free(local);
}
static void js_publication(unsigned unknown)
{
    struct js_trial *e=js_make(24,16);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;
    uint64_t ticket;enum pt_mixed_readers_result r;(void)js_first(e);c16_input(w,0);c16_prepare(w,0);
    ticket=cl_enqueue(c,1);c->port.control_raw=unknown?-1:0;
    r=pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1]);
    assert(r==(unknown?PT_MIXED_READERS_BACKEND:PT_MIXED_READERS_PENDING)&&
        cl_s(c)->command[c->command[1].slot].transferred&&cl_s(c)->command[c->command[1].slot].ticket==ticket&&
        pt_mixed_readers_readers_held(cl_s(c)->queue)==16&&!c->port.model.controls);
    if(!unknown){assert(!ct_command(&c->port.model.base,ticket)&&!cl_s(c)->first_error);
        c->unpublished_ref=c->command[1];c->unpublished_ticket=ticket;}
    else assert(ct_command(&c->port.model.base,ticket));js_drop(e);
}
static void js_source_quiet(unsigned unknown)
{
    struct js_trial *e=js_make(24,16);struct c16_trial *w=&e->old;struct cl_trial *c=&w->old;uint64_t control;
    (void)js_first(e);c16_input(w,0);c16_prepare(w,0);control=cl_enqueue(c,1);
    assert(pt_editor_mixed_causal_prepare_publish(cl_s(c),c->command[1])==PT_MIXED_READERS_OK);
    c->port.model.base.ticks=oracle(1920);assert(pt_mixed_causal_fire(cl_s(c)->causal,control)==PT_MIXED_CAUSAL_COMMITTED);
    js_drain(e);c->port.model.base.source_raw=unknown?-1:0;c->port.quiet_raw=0;
    assert(!pt_editor_mixed_causal_prepare_close(cl_s(c))&&!cl_s(c)->pool&&!cl_s(c)->queue&&cl_s(c)->causal&&
        cs_live(&c->old.ordinary)==1&&cl_s(c)->source_held&&!pt_editor_mixed_causal_source_children_closed(cl_s(c),e->borrow));
    assert(!pt_editor_dispose(c->old.editor));c->port.quiet_raw=1;
    assert(!pt_editor_mixed_causal_prepare_close(cl_s(c))&&!cl_s(c)->causal&&cl_s(c)->source_held&&
        pt_editor_mixed_causal_source_children_closed(cl_s(c),e->borrow)&&c->port.model.base.shutdowns==1);js_drop(e);
}
static void js_master_reentry(void)
{
    struct js_trial *e=js_make(24,16);struct cl_trial *c=&e->old.old;js_begin(e);e->master_reentry=1;
    js_promote(e);assert(e->master_hits==1&&!e->master_reentry&&cl_s(c)->first_error==PT_EDITOR_MIXED_READERS_FAULT&&
        !cl_s(c)->source_busy&&cl_s(c)->source_held&&c->old.pin[0]&&c->old.pin[1]&&
        !c->old.ordinary.calls&&!c->port.binds);
    assert(pt_editor_mixed_causal_lineage_source_activate(&c->joint,e->borrow,&c->input)==PT_EDITOR_MIXED_READERS_INVALID);js_drop(e);
}
static void js_mutable_receipt(void)
{
    struct js_trial *e=js_make(24,16);struct cl_trial *c=&e->old.old;
    struct pt_mixed_readers_reader_receipt *out;struct c16_counts counts;
    size_t bytes=pt_sampler_mixed_pool_size();unsigned char before[256];
    unsigned char *pool=malloc(bytes),*whole=malloc(sizeof(c->joint));assert(pool&&whole);
    (void)js_first(e);assert(sizeof(*out)<=256);
    out=(void *)(e->mutable+256-sizeof(*out));
    assert((uintptr_t)out%offsetof(struct {char prefix;struct pt_mixed_readers_reader_receipt value;},value)==0);
    memset(e->mutable,0xa5,256);memcpy(before,e->mutable,256);memcpy(pool,cl_s(c)->pool,bytes);
    memcpy(whole,&c->joint,sizeof(c->joint));counts=c16_counts(c);
    /* This exact full caller aggregate is in an unused but declared mutable
     * capacity tail. INVALID must precede the quiet callback and output write. */
    assert(pt_editor_mixed_causal_prepare_service_reader(cl_s(c),c->old.reader[0],1,out)==PT_MIXED_READERS_INVALID&&
        !memcmp(before,e->mutable,256)&&!memcmp(pool,cl_s(c)->pool,bytes)&&
        !memcmp(whole,&c->joint,sizeof(c->joint))&&!cl_s(c)->first_error);
    c16_counts_same(c,&counts);free(pool);free(whole);js_drop(e);
}
int main(void)
{
    unsigned i;
    js_success(24,0,0);js_success(24,0,1);js_success(16,1,0);js_success(16,1,1);
    for(i=0;i<2;++i)js_early(i);
    for(i=0;i<2;++i)js_identity(i);
    for(i=0;i<6;++i)js_begin_guard(i);
    for(i=0;i<4;++i)js_activate_guard(i);
    for(i=0;i<3;++i)js_changed(i);
    for(i=0;i<4;++i)js_allocator(i);
    for(i=0;i<2;++i)js_publication(i);
    for(i=0;i<2;++i)js_source_quiet(i);
    js_master_reentry();js_mutable_receipt();
    assert(js_cases==31);
    puts("EDITOR MIXED JOINT SOURCE LINEAGE PASS:31 genuine bridge instances;4 wide/old8 exact960/1920/2880 schedules and both retirement orders;2 early cancellation;2 original publisher/wrapper identity;6 complete containing admission guards;4 malformed composite activation;3 immutable/current staleness;4 consumed allocator/reentry/input/scratch outcomes;2 actual publication pending/unknown;2 independent SOURCE pending/unknown quiet;1 real master allocation reentry;1 exact mutable-capacity receipt refusal before callbacks;SOFTWARE_ONLY");
    return 0;
}
