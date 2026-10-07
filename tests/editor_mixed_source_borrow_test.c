/* Private focused early-source seam fixture. Existing committed legacy and
 * quantized bodies are included verbatim and their entry executes exactly once.
 * Dedicated entry selection changes only the translation unit entry. No genuine
 * queue/cache/owner/READY authority is manufactured by this new fixture. */
#define PT_EDITOR_MIXED_READERS_QUANTIZED_TEST_MAIN esb_quantized_main
#include "editor_mixed_readers_quantized_test.c"
#undef PT_EDITOR_MIXED_READERS_QUANTIZED_TEST_MAIN
#include "../src/editor/editor_mixed_readers_source_internal.h"

struct esb_trial {
    struct emp_trial base;
    struct pt_editor_mixed_source_inputs *source;
    struct pt_editor_mixed_source_borrow *borrow;
    struct pt_editor_mixed_readers_quantized_batch *batch;
    int32_t *source_pcm[2];
    struct pt_extension *extensions;
    uint8_t *extension_bytes;
    int32_t *maximum_pcm;uint32_t *maximum_slices;
    void *role_mutable,*role_immutable;size_t role_capacity;
    unsigned promoting,reenter_master,observed,steps;
    unsigned char parent_tail[257];
};
static void *esb_master_new(void *context,size_t n)
{
    struct emp_trial *f=(void *)((char *)context- offsetof(struct emp_trial,masters));
    struct esb_trial *e=(void *)f;
    if(e->promoting){
        uint32_t revision=f->editor->history.revision,generation=f->editor->sampler.generation;
        assert(e->borrow&&e->borrow->address==&f->control&&f->control.source_busy&&
            f->binding->preparation_context==&f->control&&f->binding->preparation_close);
        ++e->observed;
        if(e->reenter_master){unsigned calls=f->ordinary.calls,releases=f->masters.releases;
            e->reenter_master=0;
            assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
            assert(f->control.source_busy&&f->control.source_cancel_requested&&
                f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT&&
                e->borrow->address==&f->control&&!f->control.activation&&!f->control.pool);
            assert(calls==f->ordinary.calls&&releases==f->masters.releases);}
        assert(revision==f->editor->history.revision&&generation==f->editor->sampler.generation);
    }
    return emp_new(&f->masters,n);
}
static struct emp_trial *esb_make(unsigned bits,unsigned cache_bits,unsigned little)
{
    struct emp_trial *f=calloc(1,sizeof(struct esb_trial));struct pt_allocator a;struct pt_amigus_reservation_api api;
    struct pt_project *p;unsigned i,j;assert(f);f->bits=bits;f->cache_bits=cache_bits;f->little=little;
    f->card=calloc(1,sizeof(*f->card));f->document=calloc(1,sizeof(*f->document));
    f->editor=calloc(1,sizeof(*f->editor));f->binding=calloc(1,sizeof(*f->binding));
    f->request=calloc(16,sizeof(*f->request));f->command=calloc(2,sizeof(*f->command));f->reader=calloc(32,sizeof(*f->reader));
    assert(f->card&&f->document&&f->editor&&f->binding&&f->request&&f->command&&f->reader);f->bus.owner=f;
    a=(struct pt_allocator){&f->masters,esb_master_new,emp_master_free};
    pt_document_init(f->document,&a);assert(pt_document_new(f->document,16,SIZE_MAX)==PT_PROJECT_OK);
    p=&f->document->project;
    for(i=0;i<16;++i)p->channels.track[i].route=i<4?PT_PAULA:PT_AMIGUS;
    for(i=0;i<2;++i){int32_t values[]={127,-128,1,-1,3,-3,64,-64,0,2,-2,126};
        for(j=0;j<64;++j)f->original[i][j]=values[j%12]*(int32_t)(1U<<(bits-8));
        ((struct esb_trial *)f)->source_pcm[i]=emp_new(&f->masters,64*sizeof(int32_t));
        memcpy(((struct esb_trial *)f)->source_pcm[i],f->original[i],64*sizeof(int32_t));
        p->samples[i].pcm=(struct pt_pcm){((struct esb_trial *)f)->source_pcm[i],64,6,8000,2,(uint8_t)bits};p->samples[i].volume=64;}
    assert(pt_editor_init(f->editor,p));pt_sampler_init(&f->editor->sampler,&a,SIZE_MAX);
    /* An unused already-owned master with real spare PCM capacity exercises
     * the entire capacity ledger, independently of played sample geometry. */
    {struct pt_pcm spare;int32_t *values=emp_new(&f->masters,64*sizeof(int32_t));
        for(i=0;i<64;++i)values[i]=(int32_t)i;
        spare=(struct pt_pcm){values,64,1,8000,1,24};
        assert(pt_sampler_append_owned(&f->editor->sampler,p,&f->editor->history,&spare,&a,"unused spare")==PT_EDIT_OK);
        assert(!spare.data);}
    assert(pt_editor_mixed_attach(f->binding,f->editor));
    f->library.available=f->library.supported=f->library.count=1;f->card->healthy=1;
    api=(struct pt_amigus_reservation_api){&f->library,open_library,close_library,find,supported,reserve,release};
    assert(pt_amigus_reservation_open_resource(&f->card->reservation,&api,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_wavetable_cache_attach(&f->card->cache,&f->card->reservation,0,4096,4096,&f->bus,emp_owned,emp_write));
    f->input.binding=f->binding;f->input.backend=&f->card->cache;
    f->input.activation.allocator=(struct pt_allocator){&f->ordinary,emp_allocate,emp_release};
    f->input.activation.allocator_context=(struct pt_mixed_readers_span){&f->ordinary,sizeof(f->ordinary)};
    f->input.activation.grid=(struct pt_mixed_readers_grid){100,17,709379,48000};f->input.activation.session=31;
    f->input.activation.control_budget=pt_mixed_activation_control_size();
    f->input.activation.queue_budget=pt_mixed_readers_control_size();
    f->input.activation.port=(struct pt_mixed_activation_port){&f->port,sizeof(f->port),PT_MIXED_ACTIVATION_PORT_VERSION,
        PT_MIXED_ACTIVATION_PORT_REQUIRED,emp_clock,emp_publish,sma_commit,emp_command_quiet,emp_reader_quiet,emp_source_close,emp_source_quiet};
    f->port.ticks=100;f->port.reader_allow=1;f->port.close_result=f->port.quiet_result=1;
    f->input.chip_context=&f->chip;f->input.chip_allocate=emp_chip_new;f->input.chip_release=emp_chip_free;
    f->input.factory_budget=pt_sampler_mixed_pool_size()+2*pt_sampler_mixed_command_size()+32*pt_sampler_mixed_reader_size();
    f->input.chip_budget=4096;f->input.contexts=(struct pt_sampler_storage_span){f,sizeof(struct esb_trial)};
    f->activation_capacity=pt_mixed_activation_workspace_size()+128;
    f->factory_capacity=pt_sampler_mixed_workspace_size()+128;
    f->activation_workspace=calloc(1,f->activation_capacity);f->factory_workspace=calloc(1,f->factory_capacity);
    assert(f->activation_workspace&&f->factory_workspace);
    f->input.activation_workspace=f->activation_workspace;f->input.activation_capacity=f->activation_capacity;
    f->input.factory_workspace=f->factory_workspace;f->input.factory_capacity=f->factory_capacity;
    f->saved=emp_save(f,&f->saved_bytes);return f;
}
static struct esb_trial *esb_scope(unsigned bits,unsigned maximum)
{
    struct esb_trial *e=(void *)esb_make(bits,16,0);struct emp_trial *f=&e->base;unsigned i;
    e->source=calloc(1,sizeof(*e->source));e->borrow=calloc(1,sizeof(*e->borrow));
    e->batch=calloc(1,sizeof(*e->batch));assert(e->source&&e->borrow&&e->batch);
    if(maximum==2){
        struct pt_project *p=f->editor->project;
        struct pt_sample *samples=emp_new(&f->masters,PT_PROJECT_SAMPLES*sizeof(*samples));
        assert(pt_editor_mixed_detach(f->binding)&&pt_editor_dispose(f->editor));
        memset(samples,0,PT_PROJECT_SAMPLES*sizeof(*samples));
        memcpy(samples,f->document->storage.samples,2*sizeof(*samples));
        emp_free(&f->masters,f->document->storage.samples,0,0);
        f->document->storage.samples=samples;f->document->storage.sample_capacity=PT_PROJECT_SAMPLES;
        p->samples=samples;p->sample_count=PT_PROJECT_SAMPLES;
        e->maximum_pcm=calloc(PT_PROJECT_SAMPLES,sizeof(*e->maximum_pcm));
        e->maximum_slices=calloc(PT_PROJECT_SAMPLES,sizeof(*e->maximum_slices));
        assert(e->maximum_pcm&&e->maximum_slices);
        for(i=2;i<PT_PROJECT_SAMPLES;++i){samples[i].pcm=(struct pt_pcm){e->maximum_pcm+i,1,1,8000,1,8};
            samples[i].slices=e->maximum_slices+i;samples[i].slice_count=1;samples[i].volume=64;}
        assert(pt_editor_init(f->editor,p));
        pt_sampler_init(&f->editor->sampler,&f->document->allocator,SIZE_MAX);
        assert(pt_editor_mixed_attach(f->binding,f->editor));
    }
    if(maximum){
        e->extensions=calloc(4090,sizeof(*e->extensions));e->extension_bytes=calloc(4090,1);
        assert(e->extensions&&e->extension_bytes);
        for(i=0;i<4090;++i){e->extensions[i].id=UINT32_C(0x7a7a7a7a);
            e->extensions[i].length=1;e->extensions[i].data=e->extension_bytes+i;}
        f->editor->project->extensions=e->extensions;f->editor->project->extension_count=4090;
        free(f->saved);f->saved=emp_save(f,&f->saved_bytes);
    }
    e->source->binding=f->binding;e->source->contexts=f->input.contexts;e->source->preparation=&f->input;
    e->source->immutable_count=4;
    e->source->immutable[0]=(struct pt_sampler_storage_span){f->activation_workspace,f->activation_capacity};
    e->source->immutable[1]=(struct pt_sampler_storage_span){f->factory_workspace,f->factory_capacity};
    e->source->immutable[2]=(struct pt_sampler_storage_span){f->card,sizeof(*f->card)};
    e->source->immutable[3]=(struct pt_sampler_storage_span){f->saved,f->saved_bytes};
    e->source->mutable_count=4;
    e->source->mutable[0]=(struct pt_sampler_storage_span){f->request,16*sizeof(*f->request)};
    e->source->mutable[1]=(struct pt_sampler_storage_span){f->command,2*sizeof(*f->command)};
    e->source->mutable[2]=(struct pt_sampler_storage_span){f->reader,32*sizeof(*f->reader)};
    e->source->mutable[3]=(struct pt_sampler_storage_span){e->batch,sizeof(*e->batch)};
    e->source->activation_workspace=e->source->immutable[0];
    e->source->factory_workspace=e->source->immutable[1];
    e->source->backend_parent=e->source->immutable[2];
    return e;
}
static void esb_begin(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;unsigned calls=f->masters.calls;
    assert(!f->editor->sampler.current[0]&&!f->editor->sampler.current[1]);
    assert(pt_editor_mixed_source_begin(&f->control,e->source,e->borrow)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(e->borrow->address==&f->control&&e->borrow->serial&&f->control.phase==PT_EDITOR_MIXED_READERS_SOURCE&&
        f->binding->preparation_context==&f->control&&f->binding->preparation_close&&!f->control.inputs);
    assert(calls==f->masters.calls&&!f->ordinary.calls&&!f->chip.calls&&!f->card->writes&&!f->port.reads);
    assert(pt_editor_mixed_readers_prepare_get(&f->control)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_readers_prepare_advance_validation(&f->control,1)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,e->batch,f->command)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(pt_editor_mixed_readers_prepare_service_command(&f->control,(struct pt_editor_mixed_command_ref){0,0},0,NULL)==PT_MIXED_READERS_INVALID);
    assert(!f->control.first_error&&!f->ordinary.calls&&pt_editor_mixed_source_children_closed(&f->control,e->borrow));
}
static void esb_promote(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;unsigned i;
    assert(pt_project_validate(f->editor->project,NULL)==PT_PROJECT_OK);
    for(i=0;i<f->editor->project->sample_count;++i){
        struct pt_sampler_pin_job job={0};struct pt_sample_version *independent=NULL;struct pt_pcm pcm;
        unsigned ready=0;assert(pt_editor_mixed_source_enter(e->borrow));e->promoting=1;
        assert(pt_sampler_pin_job_begin(&job,&f->editor->sampler,f->editor->project,i,f->editor->sampler.generation)==PT_EDIT_OK);
        while(!ready){assert(pt_sampler_pin_job_step(&job,17,&f->pcm[i],&f->pin[i],&ready)==PT_EDIT_OK);++e->steps;}
        assert(!job.value&&f->pin[i]&&f->editor->sampler.current[i]==f->pin[i]);
        /* The actual job result is retained before an independent current pin
         * replaces that producer reference. Real zero job cancellation follows. */
        assert(pt_sampler_pin_current(&f->editor->sampler,f->editor->project,i,
            f->editor->sampler.generation,f->pin[i],&pcm,&independent)==PT_EDIT_OK);
        pt_sampler_pin_job_cancel(&job);pt_sampler_unpin(f->pin[i]);f->pin[i]=independent;
        assert(f->pcm[i].data==pcm.data&&f->pin[i]==f->editor->sampler.current[i]);
        e->promoting=0;assert(pt_editor_mixed_source_leave(e->borrow));
        assert(!f->control.source_busy&&!f->control.first_error);
    }
    assert(e->observed&&e->steps>f->editor->project->sample_count);emp_same(f);
}
static void esb_activate(struct esb_trial *e,unsigned complete)
{
    struct emp_trial *f=&e->base;unsigned before=f->control.guard_count,n=0;
    enum pt_editor_mixed_readers_result r;
    assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_PENDING);
    assert(f->control.source_activated&&f->control.guard_count>before&&!f->ordinary.calls);
    if(complete){do{r=pt_editor_mixed_readers_prepare_advance_validation(&f->control,7);assert(++n<10000);}
        while(r==PT_EDITOR_MIXED_READERS_PENDING);
        assert(r==PT_EDITOR_MIXED_READERS_OPEN&&f->ordinary.calls==3&&!f->chip.calls&&!f->card->writes&&!f->port.reads);}
}
static void esb_release(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;unsigned i,calls;enum pt_editor_mixed_readers_result r;
    assert(pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    assert(e->borrow->address==&f->control&&f->binding->preparation_context==&f->control);
    assert(pt_editor_mixed_source_enter(e->borrow));
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(f->pin[i]);f->pin[i]=NULL;}
    assert(pt_editor_mixed_source_leave(e->borrow));
    assert(pt_editor_mixed_source_borrow_close(e->borrow)&&!e->borrow->address&&!e->borrow->serial);
    assert(f->binding->preparation_context==&f->control&&f->binding->preparation_close&&f->control.source_released);
    calls=f->ordinary.calls;r=pt_editor_mixed_readers_prepare_advance_validation(&f->control,1);
    assert(r!=PT_EDITOR_MIXED_READERS_PENDING&&r!=PT_EDITOR_MIXED_READERS_OPEN&&calls==f->ordinary.calls);
    assert(pt_editor_mixed_readers_prepare_close(&f->control));
    assert(!f->binding->preparation_context&&!f->binding->preparation_close);
}
static void esb_drop(struct esb_trial *e)
{
    struct emp_trial *f=&e->base;struct pt_editor_mixed_source_borrow *b=e->borrow;unsigned i;
    for(i=0;i<2;++i){emp_free(&f->masters,e->source_pcm[i],64*sizeof(int32_t),1);e->source_pcm[i]=NULL;}
    if(e->extensions){f->editor->project->extensions=NULL;f->editor->project->extension_count=0;
        free(e->extensions);free(e->extension_bytes);}
    free(e->maximum_pcm);free(e->maximum_slices);free(e->role_mutable);free(e->role_immutable);
    free(e->source);free(e->batch);emp_drop(f,0);
    /* Consumed original zero closure does not inspect the now-freed controller. */
    assert(pt_editor_mixed_source_borrow_close(b));free(b);
}
static void esb_admission(unsigned mode)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_source_inputs v=*e->source;
    struct pt_editor_mixed_source_borrow *out=e->borrow;
    unsigned char *control=malloc(sizeof(f->control));struct pt_editor_mixed before=*f->binding;
    unsigned calls=f->masters.calls;assert(control);
    switch(mode){
    case 0:v.contexts.bytes=sizeof(f->control)-1;break;
    case 1:v.binding=NULL;break;
    case 2:v.preparation=(void *)((uintptr_t)&f->input+1);break;
    case 3:v.immutable_count=15;break;
    case 4:v.immutable_count=14;v.mutable_count=1;break;
    case 5:v.immutable[5]=(struct pt_sampler_storage_span){f->activation_workspace,1};break;
    case 6:v.mutable[5]=(struct pt_sampler_storage_span){f->request,1};break;
    case 7:v.immutable[0].bytes=0;break;
    case 8:v.mutable[0].data=(void *)(UINTPTR_MAX-1);v.mutable[0].bytes=4;break;
    case 9:v.mutable[0]=v.mutable[1];break;
    case 10:v.mutable[0]=(struct pt_sampler_storage_span){f->request,16*sizeof(*f->request)+1};
        v.mutable[1]=(struct pt_sampler_storage_span){(const char *)f->request+16*sizeof(*f->request),1};break;
    case 11:v.immutable[0]=(struct pt_sampler_storage_span){e->source_pcm[0]+63,sizeof(int32_t)};break;
    case 12:v.mutable[0]=(struct pt_sampler_storage_span){e->parent_tail,sizeof(e->parent_tail)};break;
    case 13:out=(void *)e->parent_tail;break;
    case 14:e->borrow->serial=1;break;
    case 15:v.preparation=(void *)&f->control;break;
    case 16:v.immutable[0]=(struct pt_sampler_storage_span){e->borrow,sizeof(*e->borrow)};break;
    default:assert(mode==17);v.contexts.bytes=SIZE_MAX;break;
    }
    memcpy(control,&f->control,sizeof(f->control));
    assert(pt_editor_mixed_source_begin(&f->control,&v,out)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!memcmp(control,&f->control,sizeof(f->control))&&!memcmp(&before,f->binding,sizeof(before))&&
        calls==f->masters.calls&&!f->ordinary.calls&&!f->chip.calls&&!f->card->writes&&!f->port.reads);
    free(control);e->borrow->serial=0;esb_drop(e);
}
static void esb_identity(unsigned bits)
{
    struct esb_trial *e=esb_scope(bits,0);struct emp_trial *f=&e->base;
    struct pt_editor_mixed_source_borrow copy;struct pt_editor_mixed_readers_prepare *copied;
    unsigned guards;uint64_t serial;
    esb_begin(e);copy=*e->borrow;copied=malloc(sizeof(*copied));assert(copied);memcpy(copied,&f->control,sizeof(*copied));
    assert(!pt_editor_mixed_source_enter(&copy)&&!pt_editor_mixed_source_leave(&copy)&&
        !pt_editor_mixed_source_borrow_close(&copy)&&!pt_editor_mixed_source_children_closed(copied,e->borrow)&&
        pt_editor_mixed_source_activate(copied,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    free(copied);guards=f->control.guard_count;
    /* No READY authority: actual missing masters leave early adoption intact. */
    assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(guards==f->control.guard_count&&f->control.phase==PT_EDITOR_MIXED_READERS_SOURCE&&e->borrow->address==&f->control);
    esb_promote(e);esb_activate(e,1);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    assert(!pt_editor_prepare_change(f->editor)&&e->borrow->address==&f->control);
    esb_release(e);serial=f->control.source_serial;
    memset(&f->control,0,sizeof(f->control));
    assert(pt_editor_mixed_source_begin(&f->control,e->source,e->borrow)==PT_EDITOR_MIXED_READERS_PENDING&&e->borrow->serial>serial);
    assert(!pt_editor_mixed_source_enter(&copy)&&!pt_editor_mixed_source_borrow_close(&copy));
    serial=e->borrow->serial;e->borrow->serial=copy.serial;
    assert(!pt_editor_mixed_source_enter(e->borrow)&&!pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    e->borrow->serial=serial;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_master_reentry(void)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
    struct pt_sampler_pin_job job={0};unsigned releases;
    esb_begin(e);assert(pt_project_validate(f->editor->project,NULL)==PT_PROJECT_OK);
    assert(pt_editor_mixed_source_enter(e->borrow));e->promoting=e->reenter_master=1;releases=f->masters.releases;
    assert(pt_sampler_pin_job_begin(&job,&f->editor->sampler,f->editor->project,0,
        f->editor->sampler.generation)==PT_EDIT_OK);
    assert(job.value&&!f->pin[0]&&!f->editor->sampler.current[0]&&e->observed==1&&
        f->control.source_cancel_requested&&f->control.source_busy&&f->masters.releases==releases);
    e->promoting=0;assert(pt_editor_mixed_source_leave(e->borrow));
    assert(pt_editor_mixed_source_enter(e->borrow));pt_sampler_pin_job_cancel(&job);
    assert(!job.value&&f->masters.releases==releases+1);assert(pt_editor_mixed_source_leave(e->borrow));
    assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_busy(void)
{
    struct esb_trial *e=esb_scope(16,0);struct emp_trial *f=&e->base;struct pt_editor_mixed_source_borrow copy;
    unsigned before;esb_begin(e);copy=*e->borrow;assert(pt_editor_mixed_source_enter(e->borrow));before=f->control.reentries;
    assert(!pt_editor_mixed_source_enter(&copy)&&!pt_editor_mixed_source_leave(&copy)&&
        !pt_editor_mixed_source_borrow_close(&copy)&&f->control.source_busy&&f->control.reentries==before);
    assert(pt_editor_mixed_source_activate(&f->control,&copy,&f->input)==PT_EDITOR_MIXED_READERS_INVALID&&
        f->control.reentries==before);
    assert(!pt_editor_mixed_source_enter(e->borrow)&&f->control.source_busy&&f->control.reentries==before+1);
    assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_FAULT&&
        f->control.source_busy&&f->control.reentries==before+2);
    assert(!pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&!pt_editor_mixed_source_borrow_close(e->borrow));
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&f->control.source_busy);
    assert(pt_editor_mixed_source_leave(e->borrow)&&!f->control.source_busy&&!pt_editor_mixed_source_leave(e->borrow));
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_stale(unsigned mode)
{
    struct esb_trial *e=esb_scope(16,0);struct emp_trial *f=&e->base;struct pt_project header;
    esb_begin(e);header=*f->editor->project;
    if(!mode){++f->editor->history.revision;
        f->editor->project->samples=(void *)(UINTPTR_MAX-7);f->editor->project->events=(void *)(UINTPTR_MAX-7);
        f->editor->project->extensions=(void *)(UINTPTR_MAX-7);}
    else{assert(mode==1);++e->source->mutable[0].bytes;}
    assert(pt_editor_mixed_source_enter(e->borrow)&&f->control.source_busy&&
        f->control.first_error==PT_EDITOR_MIXED_READERS_STALE&&f->control.source_cancel_requested);
    assert(pt_editor_mixed_source_leave(e->borrow)&&!f->control.source_busy);
    assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);
    *f->editor->project=header;if(mode)--e->source->mutable[0].bytes;
    emp_same(f);esb_drop(e);
}
static void esb_alias(unsigned mode)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;unsigned char saved[16];void *alias;
    esb_begin(e);esb_promote(e);esb_activate(e,0);
    switch(mode){case 0:alias=(char *)e->batch+sizeof(*e->batch)-8;break;
    case 1:alias=(char *)f->command+2*sizeof(*f->command)-8;break;
    case 2:alias=(char *)f->reader+32*sizeof(*f->reader)-8;break;
    case 3:alias=e->source_pcm[0]+63;break;
    case 4:alias=e->parent_tail+sizeof(e->parent_tail)-8;break;
    case 5:alias=e->borrow;break;
    case 6:alias=(char *)f->editor->sampler.table+f->editor->sampler.table_bytes-8;break;
    case 7:alias=(void *)(f->pcm[31].data+63);break;
    case 8:alias=(void *)(f->pcm[0].data+11);break;
    default:assert(mode==9);alias=(char *)f->activation_workspace+f->activation_capacity-8;break;}
    memcpy(saved,alias,(mode==3||mode==7||mode==8)?4:8);f->ordinary.alias=alias;
    assert(pt_editor_mixed_readers_prepare_advance_validation(&f->control,1)==PT_EDITOR_MIXED_READERS_FAULT);
    assert(!memcmp(saved,alias,(mode==3||mode==7||mode==8)?4:8)&&!f->ordinary.alias_releases&&!f->control.activation&&!f->control.pool);
    f->ordinary.alias=NULL;assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_phase(unsigned phase)
{
    struct esb_trial *e=esb_scope(8,0);struct emp_trial *f=&e->base;unsigned n=0,calls;
    esb_begin(e);esb_promote(e);esb_activate(e,0);
    while(f->control.phase!=phase){assert(pt_editor_mixed_readers_prepare_advance_validation(&f->control,1)==PT_EDITOR_MIXED_READERS_PENDING);assert(++n<10000);}
    calls=f->ordinary.calls;assert(pt_editor_mixed_readers_prepare_cancel(&f->control)==PT_EDITOR_MIXED_READERS_CANCELLED);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    assert(calls==f->ordinary.calls&&!f->port.reads&&!f->port.publishes&&!f->card->writes);
    esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_paired_pending(void)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;unsigned i,calls,releases,close_calls,quiet_calls;
    uintptr_t command_address,activation_address;
    esb_begin(e);esb_promote(e);esb_activate(e,1);emq_batch(f,e->batch,5,1);
    assert(pt_editor_mixed_readers_prepare_batch_begin_quantized(&f->control,960,e->batch,f->command)==PT_EDITOR_MIXED_READERS_PENDING);
    emq_advance(f,0);emp_refs(f,0,0);emp_issue(f,0,960);
    f->port.reader_allow=0;f->port.close_result=f->port.quiet_result=0;
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&!pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    assert(!pt_editor_mixed_source_borrow_close(e->borrow)&&e->borrow->address==&f->control);
    f->port.reader_allow=1;
    for(i=0;i<5;++i){enum pt_mixed_readers_result r=pt_editor_mixed_readers_prepare_service_reader(&f->control,f->reader[i],1,NULL);
        assert(r==PT_MIXED_READERS_BACKEND||r==PT_MIXED_READERS_OK);}
    /* Reader retirement is independent of command detach. Retain only the
     * numeric original allocation identity; never close a copied/raw owner. */
    assert(f->control.command[f->command[0].slot].serial==f->command[0].serial&&
        f->control.command[f->command[0].slot].handle.address&&
        f->control.command[f->command[0].slot].handle.token&&
        f->control.command[f->command[0].slot].transferred&&
        f->control.command[f->command[0].slot].ticket&&!f->port.close_calls);
    command_address=(uintptr_t)f->control.command[f->command[0].slot].handle.address;
    {enum pt_mixed_readers_result r=pt_editor_mixed_readers_prepare_service_command(
        &f->control,f->command[0],0,NULL);
        assert(r==PT_MIXED_READERS_BACKEND||r==PT_MIXED_READERS_OK);}
    /* Retain actual controller NULL consumption even if service returned an
     * outer failure. No receipt/key/copy certifies independent command quiet. */
    assert(!f->control.command[f->command[0].slot].handle.address&&
        !f->control.command[f->command[0].slot].handle.token&&
        !f->control.command[f->command[0].slot].serial&&
        !f->control.command[f->command[0].slot].ticket);
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
        assert((uintptr_t)f->control.ordinary[i].data!=command_address);
    assert(!pt_editor_mixed_readers_prepare_close(&f->control)&&f->control.activation&&f->port.close_calls==1);
    assert(!pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&!pt_editor_mixed_source_borrow_close(e->borrow));
    /* Explicit injected backend callback quiescence, as in the inherited
     * barrier fixtures, precedes the separate read-only source-quiet proof. */
    assert(f->port.callback_owner==f->control.activation);
    f->port.callback_owner=NULL;
    activation_address=(uintptr_t)f->control.activation;
    releases=f->ordinary.releases;close_calls=f->port.close_calls;quiet_calls=f->port.quiet_calls;
    f->port.quiet_result=1;f->port_hook=6;
    /* A positive quiet callback that reenters cannot certify source closure.
     * Retain the actual original activation and SOURCE hook through refusal. */
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(f->control.activation&&(uintptr_t)f->control.activation==activation_address&&
        !f->control.queue&&!f->control.pool&&f->binding->preparation_context==&f->control&&
        f->binding->preparation_close&&f->control.source_held&&e->borrow->address==&f->control);
    assert(f->reentered==1&&!f->port_hook&&!f->port.callback_owner&&
        f->control.first_error==PT_EDITOR_MIXED_READERS_FAULT&&f->control.source_cancel_requested&&
        f->ordinary.releases==releases&&f->port.close_calls==close_calls&&f->port.quiet_calls==quiet_calls+1);
    {unsigned held=0;for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
        if((uintptr_t)f->control.ordinary[i].data==activation_address&&
            f->control.ordinary[i].bytes==pt_mixed_activation_control_size())++held;
        assert(held==1);}
    assert(!pt_editor_mixed_source_children_closed(&f->control,e->borrow)&&
        !pt_editor_mixed_source_borrow_close(e->borrow)&&e->borrow->address==&f->control);
    /* A distinct stable read-only quiet call now consumes the actual slot.
     * The failed outer result still does not erase that actual NULL outcome. */
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    assert(!f->control.activation&&!f->control.queue&&!f->control.pool&&f->reentered==1&&
        f->ordinary.releases==releases+1&&f->port.close_calls==close_calls&&f->port.quiet_calls==quiet_calls+2);
    for(i=0;i<PT_EDITOR_MIXED_READERS_ORDINARY;++i)
        assert((uintptr_t)f->control.ordinary[i].data!=activation_address);
    assert(pt_editor_mixed_source_children_closed(&f->control,e->borrow));
    calls=f->ordinary.calls;releases=f->ordinary.releases;close_calls=f->port.close_calls;quiet_calls=f->port.quiet_calls;
    esb_release(e);
    assert(calls==f->ordinary.calls&&releases==f->ordinary.releases&&close_calls==f->port.close_calls&&quiet_calls==f->port.quiet_calls);
    assert(!f->port.mask);emp_same(f);esb_drop(e);
}
static void esb_maximum(void)
{
    struct esb_trial *e=esb_scope(24,1);struct emp_trial *f=&e->base;unsigned i,before;
    esb_begin(e);before=f->control.guard_count;assert(before>4090&&before<8192);
    esb_promote(e);esb_activate(e,1);assert(f->control.guard_count>before&&f->control.guard_count<8192);
    for(i=0;i<4090;++i){unsigned j,hits=0;
        for(j=0;j<f->control.guard_count;++j)if(f->control.guards[j].data==e->extension_bytes+i&&f->control.guards[j].bytes==1)++hits;
        assert(hits==1);}
    assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_maximum_source(void)
{
    struct esb_trial *e=esb_scope(24,2);struct emp_trial *f=&e->base;unsigned before,i;
    struct pt_sampler_storage_span *guards;
    esb_begin(e);before=f->control.guard_count;
    assert(before>4590&&before<8192&&f->editor->project->sample_count==PT_PROJECT_SAMPLES);
    guards=malloc(before*sizeof(*guards));assert(guards);memcpy(guards,f->control.guards,before*sizeof(*guards));
    assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
    assert(before==f->control.guard_count&&!memcmp(guards,f->control.guards,before*sizeof(*guards))&&
        f->control.phase==PT_EDITOR_MIXED_READERS_SOURCE&&e->borrow->address==&f->control&&!f->ordinary.calls);
    for(i=2;i<PT_PROJECT_SAMPLES;++i){unsigned j,pcm=0,slices=0;
        for(j=0;j<before;++j){if(f->control.guards[j].data==e->maximum_pcm+i&&f->control.guards[j].bytes==sizeof(int32_t))++pcm;
            if(f->control.guards[j].data==e->maximum_slices+i&&f->control.guards[j].bytes==sizeof(uint32_t))++slices;}
        assert(pcm==1&&slices==1);}
    free(guards);assert(!pt_editor_mixed_readers_prepare_close(&f->control));
    esb_release(e);emp_same(f);esb_drop(e);
}
static void esb_empty_release(void)
{
    struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
    esb_begin(e);esb_promote(e);esb_activate(e,0);
    assert(f->control.phase==PT_EDITOR_MIXED_READERS_ACTIVATION&&!f->control.first_error&&!f->ordinary.calls);
    esb_release(e);assert(f->control.first_error==PT_EDITOR_MIXED_READERS_CANCELLED&&!f->ordinary.calls);
    emp_same(f);esb_drop(e);
}
/* Full separately allocated role buffers are valid at SOURCE admission. Only
 * later preparation attempts try to reuse them or narrow a fixed parent. */
static void esb_role_buffers(struct esb_trial *e)
{
 size_t a=pt_mixed_activation_workspace_size(),b=pt_sampler_mixed_workspace_size();
 e->role_capacity=(a>b?a:b)+256;
 if(e->role_capacity<sizeof(struct pt_amigus_wavetable_cache)+256)
    e->role_capacity=sizeof(struct pt_amigus_wavetable_cache)+256;
 if(e->role_capacity<sizeof(struct pt_amigus_reservation)+256)
    e->role_capacity=sizeof(struct pt_amigus_reservation)+256;
 e->role_mutable=calloc(1,e->role_capacity);e->role_immutable=calloc(1,e->role_capacity);
 assert(e->role_mutable&&e->role_immutable);
 e->source->mutable_count=e->source->immutable_count=5;
 e->source->mutable[4]=(struct pt_sampler_storage_span){e->role_mutable,e->role_capacity};
 e->source->immutable[4]=(struct pt_sampler_storage_span){e->role_immutable,e->role_capacity};
}
static void esb_role_admission(unsigned mode)
{
 struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
 struct pt_editor_mixed_source_inputs v=*e->source;struct pt_editor_mixed before=*f->binding;
 unsigned char *control=malloc(sizeof(f->control));unsigned calls=f->masters.calls;assert(control);
 switch(mode){case 0:--v.activation_workspace.bytes;break;
 case 1:v.factory_workspace.data=(const char *)v.factory_workspace.data+pt_sampler_mixed_workspace_alignment();break;
 case 2:--v.backend_parent.bytes;break;
 case 3:v.activation_workspace=v.factory_workspace;break;
 default:assert(mode==4);v.backend_parent=v.activation_workspace;break;}
 memcpy(control,&f->control,sizeof(f->control));
 assert(pt_editor_mixed_source_begin(&f->control,&v,e->borrow)==PT_EDITOR_MIXED_READERS_INVALID);
 assert(!memcmp(control,&f->control,sizeof(f->control))&&!memcmp(&before,f->binding,sizeof(before))&&
    !e->borrow->address&&!e->borrow->serial&&calls==f->masters.calls&&!f->ordinary.calls);
 free(control);esb_drop(e);
}
static void esb_role_alias(unsigned mode)
{
 struct esb_trial *e=esb_scope(24,0);struct emp_trial *f=&e->base;
 struct pt_editor_mixed_readers_prepare_inputs original;struct pt_sampler_storage_span *guards;
 struct pt_editor_mixed_source_inputs scope;struct pt_editor_mixed_source_borrow borrow;
 struct pt_amigus_reservation *reservation;unsigned before,calls;size_t shift;
 unsigned char *mutable_bytes,*immutable_bytes,*control;
 esb_role_buffers(e);esb_begin(e);esb_promote(e);
 original=f->input;scope=*e->source;borrow=*e->borrow;reservation=f->card->cache.reservation;
 before=f->control.guard_count;guards=malloc(before*sizeof(*guards));
 mutable_bytes=malloc(e->role_capacity);immutable_bytes=malloc(e->role_capacity);control=malloc(sizeof(f->control));
 assert(guards&&mutable_bytes&&immutable_bytes&&control);memcpy(control,&f->control,sizeof(f->control));
 memcpy(guards,f->control.guards,before*sizeof(*guards));
 memcpy(mutable_bytes,e->role_mutable,e->role_capacity);memcpy(immutable_bytes,e->role_immutable,e->role_capacity);
 shift=pt_mixed_activation_workspace_alignment();calls=f->masters.calls;
 assert(shift&&e->role_capacity-shift>=pt_mixed_activation_workspace_size());
 assert(pt_sampler_mixed_workspace_alignment()&&
    e->role_capacity-pt_sampler_mixed_workspace_alignment()>=pt_sampler_mixed_workspace_size());
 switch(mode){case 0:f->input.activation_workspace=e->role_mutable;f->input.activation_capacity=e->role_capacity;break;
 case 1:f->input.activation_workspace=(char *)e->role_mutable+shift;f->input.activation_capacity=e->role_capacity-shift;break;
 case 2:f->input.factory_workspace=e->role_mutable;f->input.factory_capacity=e->role_capacity;break;
 case 3:shift=pt_sampler_mixed_workspace_alignment();
    f->input.factory_workspace=(char *)e->role_mutable+shift;f->input.factory_capacity=e->role_capacity-shift;break;
 case 4:f->input.activation_workspace=e->role_immutable;f->input.activation_capacity=e->role_capacity;break;
 case 5:f->input.factory_workspace=e->role_immutable;f->input.factory_capacity=e->role_capacity;break;
 case 6:f->input.activation_workspace=(char *)f->activation_workspace+shift;f->input.activation_capacity=f->activation_capacity-shift;break;
 case 7:shift=pt_sampler_mixed_workspace_alignment();
    f->input.factory_workspace=(char *)f->factory_workspace+shift;f->input.factory_capacity=f->factory_capacity-shift;break;
 case 8:f->input.backend=e->role_mutable;break;
 case 9:f->input.backend=e->role_immutable;break;
 case 10:f->input.backend=(void *)e->borrow;break;
 case 11:f->input.backend=(void *)e->source;break;
 case 12:f->card->cache.reservation=e->role_mutable;break;
 case 13:f->card->cache.reservation=(void *)e->source;break;
 case 14:f->card->cache.reservation=(void *)e->borrow;break;
 default:assert(mode==15);f->card->cache.reservation=(void *)&f->control;break;}
 assert(pt_editor_mixed_source_activate(&f->control,e->borrow,&f->input)==PT_EDITOR_MIXED_READERS_INVALID);
 assert(f->control.phase==PT_EDITOR_MIXED_READERS_SOURCE&&!f->control.source_activated&&!f->control.inputs&&
    before==f->control.guard_count&&!memcmp(guards,f->control.guards,before*sizeof(*guards))&&
    !memcmp(control,&f->control,sizeof(f->control))&&
    !memcmp(&scope,e->source,sizeof(scope))&&!memcmp(&borrow,e->borrow,sizeof(borrow))&&
    f->binding->preparation_context==&f->control&&f->binding->preparation_close&&
    calls==f->masters.calls&&!f->ordinary.calls&&!f->chip.calls&&!f->port.reads&&!f->card->writes&&
    !memcmp(mutable_bytes,e->role_mutable,e->role_capacity)&&!memcmp(immutable_bytes,e->role_immutable,e->role_capacity));
 /* Restore the original future preparation only after the refused transition;
  * exact admitted roles then genuinely activate on the unchanged SAME scope. */
 f->input=original;f->card->cache.reservation=reservation;
 esb_activate(e,1);assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);
 free(guards);free(mutable_bytes);free(immutable_bytes);free(control);esb_drop(e);
}
static void esb_future_unset(void)
{
 struct esb_trial *e=esb_scope(16,0);struct emp_trial *f=&e->base;
 struct pt_editor_mixed_readers_prepare_inputs original=f->input;
 memset(&f->input,0,sizeof(f->input));esb_begin(e);
 assert(e->source->activation_workspace.data==original.activation_workspace&&
    e->source->factory_workspace.data==original.factory_workspace&&
    e->source->backend_parent.data==f->card&&e->source->backend_parent.bytes==sizeof(*f->card));
 f->input=original;esb_promote(e);esb_activate(e,1);
 assert(!pt_editor_mixed_readers_prepare_close(&f->control));esb_release(e);emp_same(f);esb_drop(e);
}
#ifndef PT_EDITOR_MIXED_SOURCE_BORROW_TEST_MAIN
#define PT_EDITOR_MIXED_SOURCE_BORROW_TEST_MAIN main
#endif
int PT_EDITOR_MIXED_SOURCE_BORROW_TEST_MAIN(void)
{
    unsigned i,bits;
    assert(esb_quantized_main()==0);
    assert(PT_EDITOR_MIXED_READERS_EMPTY==0&&PT_EDITOR_MIXED_READERS_ACTIVATION==1&&
        PT_EDITOR_MIXED_READERS_FACTORY==2&&PT_EDITOR_MIXED_READERS_VALIDATION==3&&
        PT_EDITOR_MIXED_READERS_REQUESTS==4&&PT_EDITOR_MIXED_READERS_FAILED==5&&
        PT_EDITOR_MIXED_READERS_FINISHED==6&&PT_EDITOR_MIXED_READERS_SOURCE==7);
    for(i=0;i<18;++i)esb_admission(i);
    for(i=0;i<5;++i)esb_role_admission(i);
    for(i=0;i<16;++i)esb_role_alias(i);
    esb_future_unset();
    for(bits=8;bits<=24;bits+=8)esb_identity(bits);
    puts("EDITOR SOURCE BORROW ADMISSION PASS:18 complete-parent/span/role refusals;5 explicit whole-role refusals and16 later mutable/control/unrelated-immutable/narrowed-role refusals, each followed by genuine SAME-scope exact-role activation; unset future input then exact full-card reuse;3 genuine early8/16/24 bounded promotions and independent current pins; copied control/publisher, stale serial, missing-master activation refusal; unchanged standalone enums/suites once; SOFTWARE_ONLY");
    esb_master_reentry();esb_busy();for(i=0;i<2;++i)esb_stale(i);for(i=0;i<10;++i)esb_alias(i);
    for(i=PT_EDITOR_MIXED_READERS_ACTIVATION;i<=PT_EDITOR_MIXED_READERS_VALIDATION;++i)esb_phase(i);
    puts("EDITOR SOURCE BORROW EXCLUSION PASS:genuine first master allocation editor reentry retains actual job then cancels once; original/copy busy enter/leave/activation;2 descriptor/tag-poisoned SOURCE cleanup groups;10 full mutable/original/promoted/master/table/publisher/parent allocator aliases;3 real construction-phase cancels; SOFTWARE_ONLY");
    esb_paired_pending();esb_maximum();esb_maximum_source();esb_empty_release();
    puts("EDITOR SOURCE BORROW LIFETIME PASS:real paired quantized original publication/copied-only fire; independent C/R then pending source quiet; failure-plus-NULL child consumption retained and never retried; original borrow release then positive final hook clear;4090 extension activation dedup and255-slot full PCM/marker maximum-source refusal preserve every guard; SOFTWARE_ONLY");
    puts("EDITOR SOURCE BORROW PASS:private early fixed-hook ownership seam only; no caller READY/quiet certificate, full song producer/audit integration, new standalone link dependency, timer/PLAY/MMIO/native/device/timing/audio/listening authority");
    return 0;
}
