#define PT_TEST_EDITOR_CHECKED_EXEC
#include "editor_mixed_checked_test.c"
#include "../src/editor/editor_document_action.h"
#include "../src/editor/sampler_lifetime_internal.h"
#include "../src/core/document_retirement_internal.h"
struct replace_fixture {
    struct pt_document *candidate;
    struct pt_editor_mixed_establish master;
    struct pt_mixed_established fresh;
    struct pt_editor_mixed_bridges bridges;
    struct pt_editor_mixed_bridge_inputs inputs;
    struct pt_playback_format future_format;
    struct fixture bus;
    struct editor_checked_fixture *g;
    unsigned old_releases;
};
static void action_old_release(void *context,void *p)
{struct replace_fixture *f=context;assert(p);++f->old_releases;free(p);}
static struct replace_fixture *replacement_make(unsigned mode)
{
    struct replace_fixture *f=calloc(1,sizeof(*f));assert(f&&mode<=1);
    f->candidate=calloc(1,sizeof(*f->candidate));assert(f->candidate);
    f->g=checked_editor_make(16);f->g->checked=0;
    f->g->doc.allocator.context=f;f->g->doc.allocator.release=action_old_release;
    if(mode){
        assert(pt_sampler_add_slot(&f->g->editor->sampler,&f->g->doc.project,&f->g->editor->history)==PT_EDIT_OK);
        assert(pt_sampler_paula_sync(&f->g->pb)&&pt_sampler_wavetable_sync(&f->g->ab));
        assert(f->g->editor->sampler.table&&f->g->doc.project.samples==f->g->editor->sampler.table);
    }
    assert(checked_editor_begin(f->g)==PT_MIXED_OWNER_PREPARING);
    init(&f->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&f->bus.cache,&f->bus.reservation,0,4096,4096,
        &f->bus,bus_owned,bus_write));
    f->future_format=f->g->format;
    f->inputs=(struct pt_editor_mixed_bridge_inputs){&f->bus.cache,&f->g->d,chip_alloc,chip_free,4096,
        f->g->pa,f->g->wa,quiesce,&f->g->d,wave_quiesce,&f->g->wd};
    return f;
}
static void replacement_drop(struct replace_fixture *f)
{
    assert(pt_editor_mixed_stop(f->g->binding));
    if(f->bridges.paula.bridge)assert(pt_paula_voices_close(&f->bridges.paula));
    if(f->bridges.amigus.bridge)assert(pt_wavetable_voices_close(&f->bridges.amigus));
    assert(pt_editor_dispose(f->g->editor)&&pt_sampler_lifetime_storage_retired(&f->g->editor->sampler));
    assert(pt_amigus_wavetable_cache_detach(&f->bus.cache)&&pt_amigus_reservation_close(&f->bus.reservation));
    assert(pt_document_release_checked(f->candidate)==PT_DOCUMENT_RELEASED);
    checked_editor_drop(f->g);free(f->candidate);free(f);
}
struct action_fixture {
    struct replace_fixture *f;
    struct pt_editor_document_action action;
    struct pt_editor_document_action_inputs inputs;
    uint8_t *encoded;size_t bytes;
    void *alias;unsigned hook,calls,live,releases,alias_releases;
};
static void *action_allocate(void *context,size_t bytes)
{
    struct action_fixture *a=context;void *p;++a->calls;
    if(a->hook==1)assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_HOLD);
    if(a->hook==2)++a->f->g->editor->row;
    if(a->alias&&(a->hook!=8||a->calls>1))return a->alias;
    if(a->hook==3)return NULL;
    p=malloc(bytes);if(p)++a->live;return p;
}
static void action_release(void *context,void *p)
{
    struct action_fixture *a=context;
    if(p==a->alias){++a->alias_releases;return;}
    assert(p&&a->live);--a->live;++a->releases;free(p);
    if(a->hook==4)assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_HOLD);
}
static struct action_fixture *action_make(unsigned expanded)
{
    struct action_fixture *a=calloc(1,sizeof(*a));struct replace_fixture *f;size_t used;
    assert(a);a->f=f=replacement_make(expanded);
    assert(pt_document_release_checked(f->candidate)==PT_DOCUMENT_RELEASED);
    memset(f->candidate,0,sizeof(*f->candidate));
    assert(pt_project_size(&f->g->doc.project,&a->bytes)==PT_PROJECT_OK);
    a->encoded=malloc(a->bytes);assert(a->encoded);
    assert(pt_project_encode(&f->g->doc.project,a->encoded,a->bytes,&used)==PT_PROJECT_OK&&used==a->bytes);
    a->inputs=(struct pt_editor_document_action_inputs){
        .document=&f->g->doc,.candidate=f->candidate,.binding=f->g->binding,
        .old_control=f->g->control,.new_control=&f->fresh,.old_paula=&f->g->pv,.old_amigus=&f->g->av,
        .establish=&f->master,.bridges=&f->bridges,.bridge_inputs=f->inputs,
        .options=f->g->options,.caps=f->g->caps,.format=f->future_format,
        .allocator={a,action_allocate,action_release},
        .contexts={{a,sizeof(*a)},{f,sizeof(*f)}},.context_count=2,.work=64};
    return a;
}
static void action_drop(struct action_fixture *a)
{
    assert(!a->action.self);a->alias=NULL;a->hook=0;
    replacement_drop(a->f);assert(!a->live&&!a->alias_releases);free(a->encoded);free(a);
}
static void old_exact(struct action_fixture *a,const struct pt_document *d,
    const struct pt_editor *e,const struct pt_editor_mixed *b,unsigned calls,unsigned releases)
{
    struct replace_fixture *f=a->f;size_t used;uint8_t *encoded=malloc(a->bytes);assert(encoded);
    assert(pt_project_encode(&f->g->doc.project,encoded,a->bytes,&used)==PT_PROJECT_OK&&used==a->bytes&&
        !memcmp(encoded,a->encoded,a->bytes));free(encoded);
    assert(!memcmp(d,&f->g->doc,sizeof(*d))&&!memcmp(e,f->g->editor,sizeof(*e))&&
        !memcmp(b,f->g->binding,sizeof(*b)));
    assert(pt_editor_mixed_checked_binding_current(f->g->binding,f->g->control,&f->g->pv,&f->g->av));
    assert(f->g->calls==calls&&!f->old_releases&&a->releases==releases&&!output_count);
}
static void action_preservation(unsigned mode)
{
    struct action_fixture *a=action_make(0);struct replace_fixture *f=a->f;
    struct pt_document d=f->g->doc;struct pt_editor *e=malloc(sizeof(*e));
    struct pt_editor_mixed b=*f->g->binding;unsigned calls=f->g->calls,releases=a->releases;
    enum pt_document_action_result r;assert(e);memcpy(e,f->g->editor,sizeof(*e));
    if(mode==0){a->hook=3;r=pt_editor_document_action_new(&a->action,&a->inputs,4,SIZE_MAX);}
    else if(mode==1)r=pt_editor_document_action_load(&a->action,&a->inputs,(const uint8_t *)"invalid",7,SIZE_MAX);
    else if(mode==2)r=pt_editor_document_action_new(&a->action,&a->inputs,4,1);
    else r=pt_editor_document_action_new(&a->action,&a->inputs,17,SIZE_MAX);
    assert(r==PT_DOCUMENT_ACTION_FAILED&&!a->action.self&&!a->live&&a->action.result==PT_DOCUMENT_ACTION_FAILED);
    assert(a->action.preparation_result==((mode==0||mode==2)?PT_PROJECT_CAPACITY:PT_PROJECT_INVALID));
    old_exact(a,&d,e,&b,calls,releases);
    if(mode==0){
        a->hook=0;assert(pt_editor_document_action_reset(&a->action));
        assert(pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
        assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_CANCELED&&!a->live);
        old_exact(a,&d,e,&b,calls,a->releases);
    }
    free(e);action_drop(a);
}
static void action_cancel_case(unsigned load)
{
    struct action_fixture *a=action_make(0);struct replace_fixture *f=a->f;
    struct pt_document d=f->g->doc;struct pt_editor *e=malloc(sizeof(*e));
    struct pt_editor_mixed b=*f->g->binding;struct pt_editor_document_action *copy=malloc(sizeof(*copy));
    unsigned calls=f->g->calls,releases=a->releases,allocated;
    assert(e&&copy);memcpy(e,f->g->editor,sizeof(*e));
    assert((load?pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX):
        pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX))==PT_DOCUMENT_ACTION_PREPARED);
    allocated=a->live;assert(allocated&&a->action.replacement.phase==0&&a->action.replacement.self);
    old_exact(a,&d,e,&b,calls,releases);
    memcpy(copy,&a->action,sizeof(*copy));
    assert(pt_editor_document_action_step(copy)==PT_DOCUMENT_ACTION_INVALID&&
        pt_editor_document_action_cancel(copy)==PT_DOCUMENT_ACTION_INVALID&&!pt_editor_document_action_finish(copy));
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_CANCELED);
    assert(!a->action.self&&!a->live&&a->releases==releases+allocated&&
        a->action.result==PT_DOCUMENT_ACTION_CANCELED&&a->action.preparation_result==PT_PROJECT_OK);
    old_exact(a,&d,e,&b,calls,releases+allocated);
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_INVALID&&
        pt_editor_document_action_finish(&a->action)&&a->releases==releases+allocated);
    free(e);free(copy);action_drop(a);
}
static void action_publish(unsigned load,unsigned waiting,unsigned no_owner)
{
    struct action_fixture *a=action_make(0);struct replace_fixture *f=a->f;
    enum pt_document_action_result r;unsigned n=0;
    if(no_owner)a->inputs.format.bits=7;
    assert((load?pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX):
        pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX))==PT_DOCUMENT_ACTION_PREPARED);
    if(waiting){
        f->g->d.quiesce_result=f->g->wd.barrier_result=0;
        assert(pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_WAITING);
        assert(!f->old_releases&&f->candidate->loaded&&!a->releases&&f->g->binding->owner);
        assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_INVALID&&
            !pt_editor_document_action_finish(&a->action));
        f->g->d.quiesce_result=f->g->wd.barrier_result=1;
    }
    do{r=pt_editor_document_action_step(&a->action);assert(++n<10000);}while(r==PT_DOCUMENT_ACTION_PUBLISHED);
    assert(f->old_releases==3&&!f->candidate->loaded);
    if(no_owner)assert(r==PT_DOCUMENT_ACTION_COMMITTED_NO_OWNER&&!f->g->binding->owner&&
        !f->g->binding->owner_finish&&!f->g->binding->preparation_context);
    else{
        assert(r==PT_DOCUMENT_ACTION_COMMITTED&&f->g->binding->owner&&
            f->g->binding->owner->project==&f->g->doc.project);
        assert(f->fresh.storage.project==&f->g->doc.project&&f->fresh.publisher==&f->g->binding->owner);
    }
    assert(f->g->doc.project.channels.count==(load?16:8));
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_INVALID);
    assert(!pt_editor_document_action_finish(&a->action));
    assert(pt_editor_mixed_stop(f->g->binding)&&pt_editor_dispose(f->g->editor));
    assert(!pt_editor_document_action_finish(&a->action));
    assert(pt_paula_voices_close(&f->bridges.paula));
    assert(!pt_editor_document_action_finish(&a->action));
    assert(pt_wavetable_voices_close(&f->bridges.amigus));
    assert(pt_editor_document_action_finish(&a->action));action_drop(a);
}
static void action_alias(unsigned mode)
{
    struct action_fixture *a=action_make(mode==16||mode==17);struct replace_fixture *f=a->f;unsigned j;
    struct pt_document d=f->g->doc;struct pt_editor *e=malloc(sizeof(*e));
    struct pt_editor_mixed b=*f->g->binding;unsigned calls=f->g->calls;
    assert(e);memcpy(e,f->g->editor,sizeof(*e));
    switch(mode){
    case 0:a->alias=&f->g->doc;break;
    case 1:a->alias=f->g->doc.storage.orders;break;
    case 2:a->alias=(uint8_t *)f->g->doc.storage.samples+f->g->doc.storage.sample_capacity*sizeof(struct pt_sample)-1;break;
    case 3:a->alias=f->g->editor;break;
    case 4:a->alias=f->g->editor->sampler.current[0];break;
    case 5:a->alias=f->g->binding->owner;break;
    case 6:a->alias=f->g->control;break;
    case 7:a->alias=&f->g->pv;break;
    case 8:a->alias=&f->bus.cache;break;
    case 9:a->alias=&f->fresh;break;
    case 10:a->alias=&a->action;break;
    case 11:a->alias=a->encoded+a->bytes-1;break;
    case 13:a->alias=f->candidate;break;
    case 14:a->alias=(uint8_t *)a+sizeof(*a)-1;break;
    case 15:a->alias=(uint8_t *)&f->fresh+sizeof(f->fresh)-1;break;
    case 16:a->alias=(uint8_t *)f->g->editor->sampler.table+f->g->editor->sampler.table_bytes-1;break;
    case 18:a->alias=(void *)UINTPTR_MAX;a->hook=8;break;
    case 17:{
        struct pt_sampler_live *live=f->g->editor->sampler.live_head;
        while(live&&live->kind!=PT_LIVE_TABLE)live=live->next;
        assert(live&&live->spans[0].data&&live->spans[0].bytes);
        a->alias=(void *)live->spans[0].data;break;
    }
    default:
        a->alias=NULL;
        for(j=0;j<PT_SAMPLER_PREPARE_BLOCKS;++j)if(f->g->control->storage.memory.block[j].data){
            a->alias=(uint8_t *)f->g->control->storage.memory.block[j].data+
                f->g->control->storage.memory.block[j].bytes-1;break;}
        assert(a->alias);break;
    }
    if(mode==11||mode==13){
        assert(pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX)==PT_DOCUMENT_ACTION_FAILED);
        assert(!a->action.self&&a->calls==1&&!a->live&&!a->releases&&!a->alias_releases);
        old_exact(a,&d,e,&b,calls,0);free(e);action_drop(a);return;
    }
    assert(pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX)==PT_DOCUMENT_ACTION_HOLD);
    assert(!pt_editor_document_action_reset(&a->action));
    assert(a->action.self&&a->action.result==PT_DOCUMENT_ACTION_HOLD&&
        a->calls==(mode==18?2U:1U)&&a->live==(mode==18?1U:0U)&&!a->releases&&!a->alias_releases&&
        a->action.cleanup_attempted&&a->action.preparation_result==PT_PROJECT_ALIAS);
    if(mode==18){
        /* Numeric span rejection occurs in core before the action output hook.
         * One earlier genuine staged child and the unproven return remain in
         * the private core quarantine. Attempted checked cleanup cannot free it. */
        assert(pt_document_retirement_project_busy(&f->candidate->project));
        assert(pt_document_release_checked(f->candidate)==PT_DOCUMENT_RELEASE_REFUSED&&
            a->live==1&&!a->releases);
    }
    old_exact(a,&d,e,&b,calls,0);
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_HOLD&&
        pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_HOLD&&!pt_editor_document_action_finish(&a->action));
    free(e);/* Genuine action/core quarantine deliberately retain the fixture. */
}
static void action_reset_alias(void)
{
    struct action_fixture *a=action_make(0);struct replace_fixture *f=a->f;
    struct pt_editor_document_action *before=malloc(sizeof(*before));struct pt_document candidate;
    struct pt_editor_document_action *interior=(void *)&a->action.old_document.project;
    assert(before&&pt_editor_document_action_new(&a->action,&a->inputs,4,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    memcpy(before,&a->action,sizeof(*before));candidate=*f->candidate;
    assert(!interior->self&&!((struct pt_editor_document_action *)(void *)f->candidate)->self);
    assert(!pt_editor_document_action_reset(interior)&&
        !pt_editor_document_action_reset((void *)f->candidate)&&
        !memcmp(before,&a->action,sizeof(*before))&&!memcmp(&candidate,f->candidate,sizeof(candidate)));
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_CANCELED);
    free(before);action_drop(a);
}
static void action_admission_alias(unsigned mode)
{
    struct action_fixture *a=action_make(0);struct replace_fixture *f=a->f;
    struct pt_editor_document_action_inputs inputs=a->inputs;
    struct pt_editor_document_action *before=calloc(1,sizeof(*before));
    struct pt_document d=f->g->doc;struct pt_editor *e=malloc(sizeof(*e));
    struct pt_editor_mixed b=*f->g->binding;unsigned calls=f->g->calls;assert(before&&e);
    memcpy(e,f->g->editor,sizeof(*e));
    if(mode==0)inputs.candidate=(struct pt_document *)(void *)&a->action.inputs;
    else{
        inputs.allocator.context=f->candidate;
        inputs.contexts[0]=(struct pt_sampler_storage_span){f->candidate,sizeof(*f->candidate)};
    }
    assert(pt_editor_document_action_new(&a->action,&inputs,4,SIZE_MAX)==PT_DOCUMENT_ACTION_INVALID);
    assert(!memcmp(before,&a->action,sizeof(*before))&&!a->calls&&!a->releases&&!a->live);
    old_exact(a,&d,e,&b,calls,0);free(before);free(e);action_drop(a);
}
static void action_uncertain(unsigned hook)
{
    struct action_fixture *a=action_make(0);struct replace_fixture *f=a->f;
    struct pt_mixed_owner *owner=f->g->binding->owner;unsigned calls=f->g->calls;
    if(hook==4){
        assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
        a->hook=4;
        assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_HOLD&&a->releases==1&&a->live==2);
        assert(a->action.cleanup_attempted);
    }else{
        a->hook=hook;
        assert(pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX)==PT_DOCUMENT_ACTION_HOLD);
        assert(a->calls==1&&a->live==1&&!a->releases);
    }
    assert(f->g->binding->owner==owner&&f->g->calls==calls&&!f->old_releases&&!output_count);
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_HOLD&&
        pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_HOLD&&!pt_editor_document_action_finish(&a->action));
    /* No forced cleanup or retry: original controls and known obligations live. */
}
int main(int argc,char **argv)
{
    unsigned n;
    if(argc==3&&!strcmp(argv[1],"alias")){action_alias((unsigned)atoi(argv[2]));puts("DOCUMENT ACTION ALIAS REFUSAL PASS");return 0;}
    if(argc==3&&!strcmp(argv[1],"uncertain")){action_uncertain((unsigned)atoi(argv[2]));puts("DOCUMENT ACTION UNCERTAIN HOLD PASS");return 0;}
    assert(!editor_checked_fixture());
    action_admission_alias(0);action_admission_alias(1);action_reset_alias();
    for(n=0;n<4;++n)action_preservation(n);
    action_cancel_case(0);action_cancel_case(1);
    action_publish(0,0,0);action_publish(1,0,0);action_publish(0,1,0);action_publish(1,0,1);
    puts("DOCUMENT ACTION PASS:13 prewrite admission/reset, NEW/load preservation, cancel, WAITING and final-address publication groups; HOST only");
    return 0;
}
