#define PT_TEST_EDITOR_CHECKED_EXEC
#include "editor_mixed_checked_test.c"
#include "../src/editor/editor_document_replacement.h"
#include "../src/core/document_retirement_internal.h"
struct replace_fixture {
    struct pt_editor_document_replacement control;
    struct pt_document candidate;
    struct pt_editor_mixed_establish master;
    struct pt_mixed_established fresh;
    struct pt_editor_mixed_bridges bridges;
    struct pt_editor_mixed_bridge_inputs inputs;
    struct pt_playback_format future_format;
    struct fixture bus;
    struct editor_checked_fixture *g;
    unsigned mode,new_calls,new_live,new_releases,old_releases,refusals;
};
static void *replace_allocate(void *context,size_t bytes)
{
    struct replace_fixture *f=context;
    if(f->mode==1&&f->control.phase>=3)return NULL;
    if(f->mode==2&&f->control.phase==4)return NULL;
    if(f->mode==3&&f->control.phase==4){
        assert(pt_document_release_checked(&f->g->doc)==PT_DOCUMENT_RELEASE_REFUSED);
        ++f->refusals;
    }
    return checked_editor_allocate(f->g,bytes);
}
static void replace_release(void *context,void *p)
{struct replace_fixture *f=context;checked_editor_release(f->g,p);}
static void *replacement_document_allocate(void *context,size_t bytes)
{
    struct replace_fixture *f=context;void *p;++f->new_calls;
    if(f->mode==8)return NULL;
    p=malloc(bytes);if(p)++f->new_live;return p;
}
static void replacement_document_release(void *context,void *p)
{struct replace_fixture *f=context;assert(p&&f->new_live);--f->new_live;++f->new_releases;free(p);}
static void replacement_old_release(void *context,void *p)
{
    struct replace_fixture *f=context;++f->old_releases;assert(p);
    if(f->mode==9){
        assert(pt_document_release_checked(&f->candidate)==PT_DOCUMENT_RELEASE_REFUSED);
        assert(pt_document_retirement_callback_active());
        assert(pt_editor_document_replacement_step(&f->control)==PT_DOCUMENT_REPLACE_HOLD);
        ++f->refusals;
    }
    free(p);
}
static struct replace_fixture *replacement_make(unsigned mode)
{
    struct replace_fixture *f=calloc(1,sizeof(*f));size_t bytes,used;uint8_t *encoded;
    struct pt_allocator a;assert(f);f->mode=mode;f->g=checked_editor_make(16);f->g->checked=0;
    f->g->editor->sampler.allocator=(struct pt_allocator){f,replace_allocate,replace_release};
    f->g->doc.allocator.context=f;f->g->doc.allocator.release=replacement_old_release;
    if(mode==5){
        struct pt_sample *original=f->g->doc.project.samples;
        assert(pt_sampler_add_slot(&f->g->editor->sampler,&f->g->doc.project,&f->g->editor->history)==PT_EDIT_OK);
        assert(f->g->editor->sampler.table&&f->g->doc.project.samples!=original&&
            f->g->editor->sampler.table_original==original&&f->g->doc.project.sample_count==4);
        assert(pt_sampler_paula_sync(&f->g->pb)&&pt_sampler_wavetable_sync(&f->g->ab));
    }
    assert(checked_editor_begin(f->g)==PT_MIXED_OWNER_PREPARING);
    assert(pt_project_size(&f->g->doc.project,&bytes)==PT_PROJECT_OK);
    encoded=malloc(bytes);assert(encoded);
    assert(pt_project_encode(&f->g->doc.project,encoded,bytes,&used)==PT_PROJECT_OK&&used==bytes);
    a=(struct pt_allocator){f,replacement_document_allocate,replacement_document_release};
    pt_document_init(&f->candidate,&a);
    if(mode==8){
        struct pt_document old;memcpy(&old,&f->g->doc,sizeof old);
        assert(pt_document_load(&f->candidate,encoded,bytes,SIZE_MAX)==PT_PROJECT_CAPACITY);
        assert(!memcmp(&old,&f->g->doc,sizeof old)&&!f->candidate.loaded&&!f->new_live);
    }else assert(pt_document_load(&f->candidate,encoded,bytes,SIZE_MAX)==PT_PROJECT_OK);
    free(encoded);
    init(&f->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&f->bus.cache,&f->bus.reservation,0,4096,4096,
        &f->bus,bus_owned,bus_write));
    f->future_format=f->g->format;
    f->inputs=(struct pt_editor_mixed_bridge_inputs){&f->bus.cache,&f->g->d,chip_alloc,chip_free,4096,
        f->g->pa,f->g->wa,quiesce,&f->g->d,wave_quiesce,&f->g->wd};
    return f;
}
static enum pt_document_replace_result replacement_begin(struct replace_fixture *f)
{
    return pt_editor_document_replacement_begin(&f->control,&f->g->doc,&f->candidate,
        f->g->binding,f->g->control,&f->g->pv,&f->g->av,&f->master,&f->fresh,&f->bridges,
        &f->inputs,&f->g->options,&f->g->caps,&f->future_format,
        (struct pt_sampler_storage_span){&f->control,sizeof f->control},64);
}
static void replacement_drop(struct replace_fixture *f)
{
    assert(pt_editor_mixed_stop(f->g->binding));
    if(f->bridges.paula.bridge)assert(pt_paula_voices_close(&f->bridges.paula));
    if(f->bridges.amigus.bridge)assert(pt_wavetable_voices_close(&f->bridges.amigus));
    assert(pt_editor_dispose(f->g->editor)&&pt_sampler_lifetime_storage_retired(&f->g->editor->sampler));
    if(f->control.self)assert(pt_editor_document_replacement_finish(&f->control));
    assert(pt_amigus_wavetable_cache_detach(&f->bus.cache)&&pt_amigus_reservation_close(&f->bus.reservation));
    pt_document_release(&f->candidate);
    checked_editor_drop(f->g);assert(!f->new_live);free(f);
}
static enum pt_document_replace_result replacement_until(struct replace_fixture *f)
{
    enum pt_document_replace_result r;unsigned n=0;
    do{r=pt_editor_document_replacement_step(&f->control);assert(++n<10000);}
    while(r==PT_DOCUMENT_REPLACE_PUBLISHED);
    return r;
}
static void replacement_positive(unsigned mode)
{
    struct replace_fixture *f=replacement_make(mode);struct pt_document prepared;
    memcpy(&prepared,&f->candidate,sizeof prepared);
    if(mode==4)f->future_format.bits=7;
    assert(replacement_begin(f)==PT_DOCUMENT_REPLACE_PREPARED);
    assert(pt_document_release_checked(&f->g->doc)==PT_DOCUMENT_RELEASE_REFUSED);
    assert(pt_document_release_checked(&f->candidate)==PT_DOCUMENT_RELEASE_REFUSED);
    assert(!f->old_releases);
    if(mode==0||mode==3){
        assert(replacement_until(f)==PT_DOCUMENT_REPLACE_COMMITTED);
        assert(f->g->binding->owner&&f->g->binding->owner->project==&f->g->doc.project);
        assert(f->fresh.storage.project==&f->g->doc.project&&f->fresh.publisher==&f->g->binding->owner);
        assert(f->bridges.paula_cache.project==&f->g->doc.project&&f->bridges.amigus_cache.project==&f->g->doc.project);
        assert(!f->candidate.loaded&&!f->candidate.allocated_bytes);
        assert(!memcmp(&prepared,&f->g->doc,sizeof prepared));
        if(mode==3)assert(f->refusals==1);
        assert(!pt_editor_document_replacement_finish(&f->control));
    }else{
        assert(replacement_until(f)==PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER);
        assert(!f->g->binding->owner&&!f->g->binding->preparation_context&&!f->g->binding->owner_finish);
        assert(!memcmp(&prepared,&f->g->doc,sizeof prepared)&&!f->candidate.loaded);
        assert(pt_document_release_checked(&f->g->doc)==PT_DOCUMENT_RELEASE_REFUSED);
    }
    assert(f->old_releases==3&&!output_count);replacement_drop(f);
}
static void replacement_refusals(void)
{
    struct replace_fixture *f=replacement_make(0);struct pt_document old,next,alias;
    struct pt_editor_document_replacement copy;struct pt_mixed_established foreign;
    unsigned calls=f->g->calls;
    memcpy(&old,&f->g->doc,sizeof old);memcpy(&next,&f->candidate,sizeof next);
    memcpy(&alias,&old,sizeof alias);
    assert(pt_editor_document_replacement_begin(&f->control,&f->g->doc,&alias,f->g->binding,
        f->g->control,&f->g->pv,&f->g->av,&f->master,&f->fresh,&f->bridges,&f->inputs,
        &f->g->options,&f->g->caps,&f->g->format,(struct pt_sampler_storage_span){&f->control,sizeof f->control},64)
        ==PT_DOCUMENT_REPLACE_INVALID);
    assert(!f->control.self&&f->g->calls==calls&&!f->old_releases);
    memcpy(&foreign,f->g->control,sizeof foreign);
    assert(pt_editor_document_replacement_begin(&f->control,&f->g->doc,&f->candidate,f->g->binding,
        &foreign,&f->g->pv,&f->g->av,&f->master,&f->fresh,&f->bridges,&f->inputs,
        &f->g->options,&f->g->caps,&f->future_format,(struct pt_sampler_storage_span){&f->control,sizeof f->control},64)
        ==PT_DOCUMENT_REPLACE_INVALID);
    assert(!f->control.self&&f->g->calls==calls&&!f->old_releases);
    assert(replacement_begin(f)==PT_DOCUMENT_REPLACE_PREPARED);
    memcpy(&copy,&f->control,sizeof copy);
    assert(pt_editor_document_replacement_step(&copy)==PT_DOCUMENT_REPLACE_INVALID&&!pt_editor_document_replacement_finish(&copy));
    memcpy(&alias,&old,sizeof alias);
    assert(pt_document_release_checked(&alias)==PT_DOCUMENT_RELEASE_REFUSED&&!f->old_releases);
    assert(!memcmp(&old,&f->g->doc,sizeof old)&&!memcmp(&next,&f->candidate,sizeof next));
    assert(pt_editor_document_replacement_finish(&f->control));replacement_drop(f);
}
static void replacement_waiting(void)
{
    struct replace_fixture *f=replacement_make(5);struct pt_document old;struct pt_pcm pcm;
    struct pt_sample_version *pin=NULL;unsigned generation=f->g->editor->sampler.generation;
    memcpy(&old,&f->g->doc,sizeof old);
    assert(pt_sampler_pin(&f->g->editor->sampler,&f->g->doc.project,0,generation,&pcm,&pin)==PT_EDIT_OK&&pin);
    assert(replacement_begin(f)==PT_DOCUMENT_REPLACE_PREPARED);
    f->g->d.quiesce_result=f->g->wd.barrier_result=0;
    assert(pt_editor_document_replacement_step(&f->control)==PT_DOCUMENT_REPLACE_WAITING);
    assert(!memcmp(&old,&f->g->doc,sizeof old)&&!f->old_releases&&f->g->binding->owner);
    f->g->d.quiesce_result=f->g->wd.barrier_result=1;
    assert(pt_editor_document_replacement_step(&f->control)==PT_DOCUMENT_REPLACE_RETIRING);
    assert(!f->old_releases&&!pt_sampler_lifetime_storage_retired(&f->g->editor->sampler));
    assert(!f->g->editor->sampler.table&&!f->g->editor->sampler.table_bytes&&
        f->g->doc.storage.samples==old.storage.samples&&f->control.phase==2&&f->candidate.loaded);
    /* The old expanded table is retired. Do not traverse/encode old.project. */
    pt_sampler_unpin(pin);
    assert(replacement_until(f)==PT_DOCUMENT_REPLACE_COMMITTED&&f->old_releases==3);
    replacement_drop(f);
}
static void replacement_prepare_failure(void)
{
    struct replace_fixture *f=replacement_make(8);assert(!f->old_releases&&f->g->binding->owner);
    assert(replacement_begin(f)==PT_DOCUMENT_REPLACE_INVALID&&!f->control.self);
    replacement_drop(f);
}
static void replacement_finish_requires_closed_bridges(void)
{
    struct replace_fixture *f=replacement_make(2);unsigned calls,releases;
    assert(replacement_begin(f)==PT_DOCUMENT_REPLACE_PREPARED);
    assert(replacement_until(f)==PT_DOCUMENT_REPLACE_COMMITTED_NO_OWNER);
    assert(f->bridges.paula.bridge==&f->bridges.paula_cache&&
        f->bridges.amigus.bridge==&f->bridges.amigus_cache);
    assert(f->bridges.paula_cache.sampler==&f->g->editor->sampler&&
        f->bridges.amigus_cache.sampler==&f->g->editor->sampler);
    assert(pt_editor_mixed_stop(f->g->binding));
    assert(pt_editor_dispose(f->g->editor)&&
        pt_sampler_lifetime_storage_retired(&f->g->editor->sampler));
    calls=f->g->calls;releases=f->new_releases;
    assert(!pt_editor_document_replacement_finish(&f->control)&&f->control.self);
    assert(pt_document_release_checked(&f->g->doc)==PT_DOCUMENT_RELEASE_REFUSED);
    assert(f->g->calls==calls&&f->new_releases==releases&&f->old_releases==3);
    assert(pt_paula_voices_close(&f->bridges.paula));
    assert(!f->bridges.paula.bridge&&!f->bridges.paula_cache.sampler&&
        f->bridges.amigus.bridge);
    assert(!pt_editor_document_replacement_finish(&f->control)&&f->control.self);
    assert(pt_document_release_checked(&f->g->doc)==PT_DOCUMENT_RELEASE_REFUSED);
    assert(pt_wavetable_voices_close(&f->bridges.amigus));
    assert(!f->bridges.amigus.bridge&&!f->bridges.amigus_cache.sampler&&
        !f->bridges.amigus_cache.backend);
    assert(pt_editor_document_replacement_finish(&f->control)&&!f->control.self);
    replacement_drop(f);
}
static void replacement_uncertain_last(void)
{
    struct replace_fixture *f=replacement_make(9);struct pt_document prepared;
    memcpy(&prepared,&f->candidate,sizeof prepared);
    assert(replacement_begin(f)==PT_DOCUMENT_REPLACE_PREPARED);
    assert(pt_editor_document_replacement_step(&f->control)==PT_DOCUMENT_REPLACE_HOLD);
    assert(f->old_releases==1&&f->refusals==1&&!f->g->binding->owner);
    assert(!memcmp(&prepared,&f->candidate,sizeof prepared));
    assert(pt_editor_document_replacement_step(&f->control)==PT_DOCUMENT_REPLACE_HOLD&&f->old_releases==1);
    assert(!pt_editor_document_replacement_finish(&f->control));
    /* Deliberate process-lifetime HOLD. The genuine private registry and core
     * quarantine retain f/all original controls and unreleased allocations.
     * No forced cleanup, second release, rollback or quiet claim follows. */
}
int main(void)
{
    assert(!editor_checked_fixture());
    replacement_refusals();replacement_prepare_failure();replacement_waiting();
    replacement_positive(0);replacement_positive(1);replacement_positive(2);
    replacement_positive(3);replacement_positive(4);
    replacement_finish_requires_closed_bridges();replacement_uncertain_last();
    puts("TYPED DOCUMENT REPLACEMENT PASS:10 focused lifecycle groups; HOST only; final uncertain group retained");return 0;
}
