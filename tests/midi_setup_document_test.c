/* Focused HOST composition only. Existing fixtures supply genuine engines,
 * documents, allocators, history and checked publication; no native authority. */
#define PT_DOCUMENT_ACTION_TEST_ENTRY original_action_entry
#include "editor_document_action_focus_test.c"
#include "../src/editor/midi_setup_owner.h"

enum midi_kind {MIDI_OTHER,MIDI_WORKSPACE,MIDI_RECORD,MIDI_CANDIDATE};
struct midi_rig;
struct midi_memory {
    struct {void *data;size_t bytes;enum midi_kind kind;} block[512];
    struct midi_rig *rig;
    void *workspace,*alias;enum midi_kind kind;
    unsigned calls,releases,live,records,record_releases,workspace_releases;
    unsigned borrow_on_last_record;
};
struct midi_rig {
    struct action_fixture *action;
    struct midi_memory *memory;
    struct pt_midi_setup_owner *owner;size_t capacity;
    struct pt_midi_setup_owner_borrow ticket;
    unsigned release_observations,record_busy_refusals,verify_transition;
};
static struct midi_rig *retained;
static void *midi_allocate(void *context,size_t bytes)
{
    struct midi_memory *m=context;void *p;unsigned i;++m->calls;
    if(m->alias)return m->alias;
    p=malloc(bytes);if(!p)return NULL;
    for(i=0;i<512&&m->block[i].data;++i){}
    assert(i<512);m->block[i].data=p;m->block[i].bytes=bytes;m->block[i].kind=m->kind;++m->live;
    if(m->kind==MIDI_WORKSPACE){assert(!m->workspace);m->workspace=p;}
    if(m->kind==MIDI_RECORD)++m->records;
    return p;
}
static void midi_release(void *context,void *p)
{
    struct midi_memory *m=context;unsigned i;enum midi_kind kind;
    for(i=0;i<512&&m->block[i].data!=p;++i){}
    assert(p&&i<512&&m->live&&p!=m->alias);kind=m->block[i].kind;
    memset(&m->block[i],0,sizeof(m->block[i]));--m->live;++m->releases;
    if(kind==MIDI_RECORD){assert(m->records);--m->records;++m->record_releases;}
    if(kind==MIDI_WORKSPACE){assert(p==m->workspace);++m->workspace_releases;m->workspace=NULL;}
    free(p);
    /* The genuine child keeps its busy guard across original release. A real
     * attempt to borrow here must refuse, leave the ticket untouched and allow
     * the genuine discard to complete; no artificial close-refusal is injected. */
    if(kind==MIDI_RECORD&&!m->records&&m->borrow_on_last_record){
        m->borrow_on_last_record=0;
        struct pt_midi_setup_owner_borrow before=m->rig->ticket;
        assert(pt_midi_setup_owner_borrow(m->rig->owner,&m->rig->ticket)==PT_MIDI_SETUP_BUSY&&
            !memcmp(&before,&m->rig->ticket,sizeof(before)));
        ++m->rig->record_busy_refusals;
    }
}
static void midi_old_release(void *context,void *p)
{
    struct midi_rig *r=context;struct pt_midi_setup_owner_status s;
    if(r->verify_transition){
        assert(pt_midi_setup_owner_get(r->owner,&s)==PT_MIDI_SETUP_OK&&s.state==PT_MIDI_OWNER_CLOSED&&
            !s.commands&&!s.metadata_bytes&&!s.modal&&!r->memory->records);
        assert(r->action->action.replacement.midi_close_attempted&&
            !r->action->action.replacement.midi_rebind_attempted);
        ++r->release_observations;
    }
    ++r->action->f->old_releases;free(p);
}
static void recapture(struct action_fixture *a)
{
    size_t used;free(a->encoded);assert(pt_project_size(&a->f->g->doc.project,&a->bytes)==PT_PROJECT_OK);
    a->encoded=malloc(a->bytes);assert(a->encoded);
    assert(pt_project_encode(&a->f->g->doc.project,a->encoded,a->bytes,&used)==PT_PROJECT_OK&&used==a->bytes);
}
static void midi_put(struct midi_rig *r,const char *name)
{
    struct pt_midi_setup_owner_borrow ticket;struct midi_memory *m=r->memory;
    assert(pt_midi_setup_owner_borrow(r->owner,&ticket)==PT_MIDI_SETUP_OK);
    assert(ticket.setup==m->workspace&&pt_midi_setup_begin(ticket.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(ticket.setup,name,strlen(name)+1)==PT_MIDI_SETUP_OK);
    m->kind=MIDI_RECORD;assert(pt_midi_setup_commit(ticket.setup)==PT_MIDI_SETUP_OK);m->kind=MIDI_OTHER;
    assert(pt_midi_setup_owner_return(r->owner,&ticket)==PT_MIDI_SETUP_OK);
}
/* Exact genuine checked-editor base construction, with ONE intentional policy
 * selection: install original m on the EMPTY initialized sampler before any
 * master allocation. Every actual master/workspace/record remains on that same
 * m ledger through retirement; the document allocator stays separate. */
static struct editor_checked_fixture *midi_checked_make(unsigned bits,struct midi_memory *memory)
{
    struct editor_checked_fixture *g=calloc(1,sizeof(*g));struct pt_allocator a={NULL,fast_alloc,fast_free};
    struct pt_editor_mixed_establish *master=calloc(1,sizeof(*master));struct pt_sampler_storage_span parent;
    enum pt_establish_result r;unsigned i,n=0;assert(g&&master);
    g->editor=malloc(sizeof(*g->editor));g->binding=calloc(1,sizeof(*g->binding));g->control=calloc(1,sizeof(*g->control));
    assert(g->editor&&g->binding&&g->control);
    for(i=0;i<2048;++i)g->pcm[i]=((int32_t)(i%120)-60)*(bits==8?1:bits==16?251:65537);
    init(&g->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&g->bus.cache,&g->bus.reservation,0,4096,4096,&g->bus,bus_owned,bus_write));
    pt_document_init(&g->doc,&a);assert(pt_document_new(&g->doc,16,SIZE_MAX)==PT_PROJECT_OK);
    PT_CHECKED_TRACE("initial-document-created");
    g->doc.project.sample_count=3;
    for(i=0;i<16;++i)g->doc.project.channels.track[i].route=PT_AMIGUS;
    g->doc.project.channels.track[4].route=PT_PAULA;g->doc.project.channels.track[4].pan=0;
    for(i=0;i<3;++i){g->doc.project.samples[i].pcm=(struct pt_pcm){g->pcm,2048,2048,8000,1,(uint8_t)bits};g->doc.project.samples[i].volume=64;}
    g->doc.project.speed=1;g->doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    g->doc.project.events[7]=g->doc.project.events[4];g->doc.project.events[16+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    g->doc.project.events[32+15].effect=15;
    assert(pt_editor_init(g->editor,&g->doc.project));
    PT_CHECKED_TRACE("initial-editor-created");
    g->editor->sampler.allocator=(struct pt_allocator){memory,midi_allocate,midi_release};g->editor->sampler.budget=1024*1024;
    assert(pt_editor_mixed_attach(g->binding,g->editor));parent=(struct pt_sampler_storage_span){g,sizeof(*g)};
    assert(pt_editor_mixed_establish_begin(master,g->binding,&parent,1)==PT_ESTABLISH_PENDING);
    PT_CHECKED_TRACE("initial-masters-begun");
    do{r=pt_editor_mixed_establish_step(master,256);assert(++n<10000);}while(r==PT_ESTABLISH_PENDING);
    assert(r==PT_ESTABLISH_READY&&memory->live==4&&!g->live&&!g->binding->owner);
    PT_CHECKED_TRACE("initial-masters-ready");
    assert(pt_editor_mixed_stop(g->binding)&&memory->live==3&&!g->live);free(master);
    assert(pt_sampler_paula_bind(&g->pb,&g->editor->sampler,&g->doc.project,&g->d,chip_alloc,chip_free,4096));
    assert(pt_sampler_wavetable_bind(&g->ab,&g->editor->sampler,&g->doc.project,&g->bus.cache));
    g->pa=(struct pt_paula_voice_api){&g->d,mixed_start,stop,mixed_control};
    g->wa=(struct pt_wavetable_voice_api){&g->wd,wave_start,wave_stop,wave_control,NULL};
    assert(pt_paula_voices_bind(&g->pv,&g->pb,&g->pa)&&pt_wavetable_voices_bind(&g->av,&g->ab,&g->wa));
    assert(pt_paula_voices_bind_quiesce(&g->pv,quiesce,&g->d)&&pt_wavetable_voices_bind_quiesce(&g->av,wave_quiesce,&g->wd));
    g->d.start_result=g->d.control_result=g->d.quiesce_result=g->wd.start_result=g->wd.stop_result=g->wd.barrier_result=1;
    for(i=0;i<4;++i)g->d.stop_result[i]=1;
    g->options=(struct pt_render_options){100000,100,48000,65536,(1U<<4)|(1U<<7),0,0,24,0,0,0,0,0};
    g->caps=(struct pt_paula_render_caps){3546895,124,65535};g->format=(struct pt_playback_format){8,0,0,0};
    g->api=(struct pt_mixed_timer_api){&g->timer,pump_read,pump_poll,pump_arm,pump_alarm_close,pump_counter_close,pump_signal};
    output_count=0;g->checked=1;return g;
}

/* Test-only construction using the same genuine base helper, but without
 * replacement_make's early checked-owner begin. MIDI allocator/metadata is
 * established before the FIRST old-owner begin. The untouched original helper
 * and all thirteen default action cases retain their existing construction. */
static struct action_fixture *midi_action_make(struct midi_memory *memory)
{
    struct action_fixture *a=calloc(1,sizeof(*a));struct replace_fixture *f;
    assert(a);a->f=f=calloc(1,sizeof(*f));assert(f);
    f->candidate=calloc(1,sizeof(*f->candidate));assert(f->candidate);
    f->g=midi_checked_make(16,memory);f->g->checked=0;
    assert(!f->g->binding->owner&&!f->g->binding->owner_finish&&!f->g->binding->transport&&
        !f->g->pv.closing&&!f->g->av.closing);
    f->g->doc.allocator.context=f;f->g->doc.allocator.release=action_old_release;
    init(&f->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&f->bus.cache,&f->bus.reservation,0,4096,4096,
        &f->bus,bus_owned,bus_write));
    f->future_format=f->g->format;
    f->inputs=(struct pt_editor_mixed_bridge_inputs){&f->bus.cache,&f->g->d,chip_alloc,chip_free,4096,
        f->g->pa,f->g->wa,quiesce,&f->g->d,wave_quiesce,&f->g->wd};
    assert(pt_document_release_checked(f->candidate)==PT_DOCUMENT_RELEASED);
    memset(f->candidate,0,sizeof(*f->candidate));
    a->inputs=(struct pt_editor_document_action_inputs){
        .document=&f->g->doc,.candidate=f->candidate,.binding=f->g->binding,
        .old_control=f->g->control,.new_control=&f->fresh,.old_paula=&f->g->pv,.old_amigus=&f->g->av,
        .establish=&f->master,.bridges=&f->bridges,.bridge_inputs=f->inputs,
        .options=f->g->options,.caps=f->g->caps,.format=f->future_format,
        .allocator={a,action_allocate,action_release},
        .contexts={{a,sizeof(*a)},{f,sizeof(*f)}},.context_count=2,.work=64};
    return a;
}
static struct midi_rig *midi_make(unsigned shared)
{
    struct midi_rig *r=calloc(1,sizeof(*r));struct action_fixture *a;struct editor_checked_fixture *g;
    struct pt_allocator allocator;assert(r);r->memory=calloc(1,sizeof(*r->memory));assert(r->memory);
    r->memory->rig=r;r->action=a=midi_action_make(r->memory);g=a->f->g;
    /* m was installed on the empty sampler BEFORE initial master establishment.
     * No live sampler allocator is changed: release_version uses its CURRENT
     * owner sampler allocator, so the same original m/context must survive. */
    assert(pt_editor_mixed_stop(g->binding));allocator=(struct pt_allocator){r->memory,midi_allocate,midi_release};
    assert(g->editor->sampler.allocator.context==allocator.context&&
        g->editor->sampler.allocator.allocate==allocator.allocate&&g->editor->sampler.allocator.release==allocator.release);
    r->capacity=pt_midi_setup_owner_size()+16;r->owner=calloc(1,r->capacity);assert(r->owner);
    memset((unsigned char *)r->owner+pt_midi_setup_owner_size(),0x7b,16);
    r->memory->kind=MIDI_WORKSPACE;
    assert(pt_midi_setup_owner_init(r->owner,r->capacity,g->editor,
        (struct pt_midi_setup_owner_span){r->memory,sizeof(*r->memory)},4096)==PT_MIDI_SETUP_OK);
    r->memory->kind=MIDI_OTHER;
    midi_put(r,"source-one");midi_put(r,"source-two");
    assert(pt_pattern_undo(&g->doc.project,&g->editor->history,-1)==PT_EDIT_OK);
    assert(g->editor->history.count==2&&g->editor->history.cursor==1&&r->memory->records==2&&
        !strcmp(g->doc.project.midi_input,"source-one"));
    assert(!g->binding->owner&&!g->binding->owner_finish&&!g->pv.closing&&!g->av.closing);
    assert(checked_editor_begin(g)==PT_MIXED_OWNER_PREPARING);
    g->doc.allocator.context=r;g->doc.allocator.release=midi_old_release;
    a->inputs.contexts[2]=(struct pt_sampler_storage_span){r,sizeof(*r)};a->inputs.context_count=3;
    a->inputs.midi_setup=r->owner;a->inputs.midi_capacity=r->capacity;
    if(shared){
        a->inputs.allocator=allocator;
        a->inputs.contexts[3]=(struct pt_sampler_storage_span){r->memory,sizeof(*r->memory)};
        a->inputs.context_count=4;
    }
    recapture(a);return r;
}
static void midi_finish(struct midi_rig *r)
{
    struct action_fixture *a=r->action;struct replace_fixture *f=a->f;unsigned i;
    assert(pt_editor_mixed_stop(f->g->binding));
    assert(pt_editor_dispose(f->g->editor));
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK);
    if(f->bridges.paula.bridge)assert(pt_paula_voices_close(&f->bridges.paula));
    if(f->bridges.amigus.bridge)assert(pt_wavetable_voices_close(&f->bridges.amigus));
    if(a->action.self)assert(pt_editor_document_action_finish(&a->action));
    assert(pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_OK);
    assert(!r->memory->records&&r->memory->workspace_releases==1);
    /* The actual current document keeps its genuinely captured candidate/old
     * allocator through existing checked document retirement. */
    action_drop(a);
    assert(!r->memory->live);
    for(i=0;i<16;++i)assert(((unsigned char *)r->owner)[pt_midi_setup_owner_size()+i]==0x7b);
    free(r->owner);free(r->memory);free(r);
}
static void preserved(struct midi_rig *r,const struct pt_editor *e,const struct pt_document *d,unsigned calls,unsigned releases)
{
    struct action_fixture *a=r->action;size_t used;uint8_t *bytes=malloc(a->bytes);assert(bytes);
    assert(!memcmp(e,a->f->g->editor,sizeof(*e))&&!memcmp(d,&a->f->g->doc,sizeof(*d)));
    assert(a->f->g->editor->history.count==2&&a->f->g->editor->history.cursor==1&&
        r->memory->records==2&&!r->memory->record_releases&&!r->release_observations);
    assert(pt_project_encode(&a->f->g->doc.project,bytes,a->bytes,&used)==PT_PROJECT_OK&&used==a->bytes&&
        !memcmp(bytes,a->encoded,a->bytes));free(bytes);
    assert(r->memory->calls==calls&&r->memory->releases==releases);
}
static void admission_case(void)
{
    struct midi_rig *r=midi_make(0);struct pt_editor *e=r->action->f->g->editor;
    struct pt_midi_setup_owner_transition_span s;struct pt_midi_setup_owner_borrow t;
    unsigned calls=r->memory->calls,releases=r->memory->releases;void *before=malloc(r->capacity);
    struct pt_document d;struct pt_editor *other=malloc(sizeof(*other));struct pt_allocator a={NULL,fast_alloc,fast_free};
    assert(before&&other);memcpy(before,r->owner,r->capacity);
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK&&pt_editor_init(other,&d.project));
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,other,NULL,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity-1,e,NULL,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity+1,e,NULL,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,NULL,0)==PT_MIDI_SETUP_OK);
    s=(struct pt_midi_setup_owner_transition_span){r->owner,r->capacity,PT_MIDI_OWNER_TRANSITION_STORAGE};
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,&s,1)==PT_MIDI_SETUP_INVALID);
    s.data=r->memory->workspace;s.bytes=pt_midi_setup_size();
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,&s,1)==PT_MIDI_SETUP_INVALID);
    s=(struct pt_midi_setup_owner_transition_span){r->memory,sizeof(*r->memory),PT_MIDI_OWNER_TRANSITION_STORAGE};
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,&s,1)==PT_MIDI_SETUP_INVALID);
    s.role=PT_MIDI_OWNER_TRANSITION_CONTEXT_INPUT;
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,&s,1)==PT_MIDI_SETUP_OK);
    --s.bytes;assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,&s,1)==PT_MIDI_SETUP_INVALID);
    s.bytes+=2;assert(pt_midi_setup_owner_transition_admit(r->owner,r->capacity,e,&s,1)==PT_MIDI_SETUP_INVALID);
    assert(!pt_midi_setup_owner_storage_disjoint(r->owner,r->capacity,r->memory,sizeof(*r->memory)));
    assert(!memcmp(before,r->owner,r->capacity)&&calls==r->memory->calls&&releases==r->memory->releases);
    assert(pt_midi_setup_owner_borrow(r->owner,&t)==PT_MIDI_SETUP_OK);
    assert(pt_editor_document_action_new(&r->action->action,&r->action->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_INVALID&&
        !r->action->action.self&&!r->action->calls);
    assert(pt_midi_setup_owner_return(r->owner,&t)==PT_MIDI_SETUP_OK);
    assert(pt_editor_dispose(other)&&pt_document_release_checked(&d)==PT_DOCUMENT_RELEASED);free(other);free(before);
    midi_finish(r);
}
static void preservation_case(void)
{
    struct midi_rig *r=midi_make(0);struct action_fixture *a=r->action;
    struct pt_editor *e=malloc(sizeof(*e));struct pt_document d=a->f->g->doc;unsigned calls=r->memory->calls,releases=r->memory->releases;
    assert(e);memcpy(e,a->f->g->editor,sizeof(*e));a->hook=3;
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_FAILED);
    preserved(r,e,&d,calls,releases);a->hook=0;assert(pt_editor_document_action_reset(&a->action));
    assert(pt_editor_document_action_load(&a->action,&a->inputs,(const uint8_t *)"invalid",7,SIZE_MAX)==PT_DOCUMENT_ACTION_FAILED);
    preserved(r,e,&d,calls,releases);assert(pt_editor_document_action_reset(&a->action));
    assert(pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_CANCELED);
    preserved(r,e,&d,calls,releases);free(e);midi_finish(r);
}
static void publication_case(unsigned load,unsigned shared)
{
    struct midi_rig *r=midi_make(shared);struct action_fixture *a=r->action;struct editor_checked_fixture *g=a->f->g;
    struct pt_midi_setup_owner_status s;enum pt_document_action_result result;unsigned n=0;size_t bytes,used;uint8_t *encoded;
    r->memory->kind=MIDI_CANDIDATE;
    assert((load?pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX):
        pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX))==PT_DOCUMENT_ACTION_PREPARED);
    r->memory->kind=MIDI_OTHER;
    assert(r->memory->records==2&&!r->memory->record_releases&&g->editor->history.cursor==1);
    r->verify_transition=1;
    do{result=pt_editor_document_action_step(&a->action);assert(++n<10000);}while(result==PT_DOCUMENT_ACTION_PUBLISHED);
    r->verify_transition=0;
    assert(result==PT_DOCUMENT_ACTION_COMMITTED&&r->release_observations==3&&r->memory->record_releases==2&&
        !r->memory->records&&a->action.replacement.midi_close_attempted==1&&a->action.replacement.midi_rebind_attempted==1);
    assert(g->editor->project==&g->doc.project&&g->editor->sampler.allocator.context==r->memory&&
        g->editor->sampler.allocator.allocate==midi_allocate&&g->editor->sampler.allocator.release==midi_release);
    assert(pt_midi_setup_owner_get(r->owner,&s)==PT_MIDI_SETUP_OK&&s.state==PT_MIDI_OWNER_LIVE&&!s.commands);
    if(load){
        assert(pt_project_size(&g->doc.project,&bytes)==PT_PROJECT_OK&&bytes==a->bytes);encoded=malloc(bytes);assert(encoded);
        assert(pt_project_encode(&g->doc.project,encoded,bytes,&used)==PT_PROJECT_OK&&used==bytes&&!memcmp(encoded,a->encoded,bytes));free(encoded);
    }else assert(!g->doc.project.midi_input[0]&&g->doc.project.channels.count==8);
    midi_put(r,"final-editor");assert(g->editor->history.count==1&&r->memory->records==1);
    midi_finish(r);
}
static void waiting_modal_case(void)
{
    struct midi_rig *r=midi_make(0);struct action_fixture *a=r->action;unsigned calls,releases;
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    a->f->g->d.quiesce_result=a->f->g->wd.barrier_result=0;
    assert(pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_WAITING);
    assert(pt_midi_setup_owner_borrow(r->owner,&r->ticket)==PT_MIDI_SETUP_OK);
    calls=r->memory->calls;releases=r->memory->releases;a->f->g->d.quiesce_result=a->f->g->wd.barrier_result=1;
    assert(pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_HOLD&&
        a->f->g->editor->history.count==2&&!r->memory->record_releases&&!r->release_observations&&
        !a->action.replacement.midi_close_attempted&&a->f->candidate->loaded);
    assert(pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_HOLD&&
        !pt_editor_document_action_finish(&a->action)&&calls==r->memory->calls&&releases==r->memory->releases);
    retained=r; /* Whole original modal/editor/document/allocator/action remain reachable. */
}
static void record_reentry_case(void)
{
    struct midi_rig *r=midi_make(0);struct action_fixture *a=r->action;enum pt_document_action_result result;unsigned n=0;
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    r->memory->borrow_on_last_record=1;r->verify_transition=1;
    do{result=pt_editor_document_action_step(&a->action);assert(++n<10000);}while(result==PT_DOCUMENT_ACTION_PUBLISHED);
    r->verify_transition=0;
    assert(result==PT_DOCUMENT_ACTION_COMMITTED&&r->record_busy_refusals==1&&
        r->memory->record_releases==2&&r->release_observations==3&&
        a->action.replacement.midi_close_attempted==1&&a->action.replacement.midi_rebind_attempted==1);
    midi_finish(r);
}
static void held_admission_case(void)
{
    struct midi_rig *r=midi_make(0);struct action_fixture *a=r->action;struct pt_midi_setup_owner_borrow t;
    assert(pt_midi_setup_owner_borrow(r->owner,&t)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_begin(t.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK&&
        pt_midi_setup_name(t.setup,"retained",9)==PT_MIDI_SETUP_OK);
    r->memory->alias=a->f->g->editor;
    assert(pt_midi_setup_commit(t.setup)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_return(r->owner,&t)==PT_MIDI_SETUP_HELD);
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_INVALID&&
        !a->action.self&&!a->calls&&!r->release_observations);
    retained=r; /* Alias was never initialized/freed; all genuine sources survive. */
}
#ifndef PT_MIDI_SETUP_DOCUMENT_TEST_ENTRY
#define PT_MIDI_SETUP_DOCUMENT_TEST_ENTRY main
#endif
int PT_MIDI_SETUP_DOCUMENT_TEST_ENTRY(int argc,char **argv)
{
    if(argc!=2)return 20;
    if(!strcmp(argv[1],"null-regression")){
        char *args[]={argv[0],(char *)"actions",NULL};return original_action_entry(2,args);
    }
    if(!strcmp(argv[1],"admission"))admission_case();
    else if(!strcmp(argv[1],"preservation"))preservation_case();
    else if(!strcmp(argv[1],"publish-new"))publication_case(0,0);
    else if(!strcmp(argv[1],"publish-load"))publication_case(1,0);
    else if(!strcmp(argv[1],"shared-load"))publication_case(1,1);
    else if(!strcmp(argv[1],"waiting-modal"))waiting_modal_case();
    else if(!strcmp(argv[1],"record-reentry"))record_reentry_case();
    else if(!strcmp(argv[1],"held-admission"))held_admission_case();
    else return 20;
    if(retained)assert(retained->action&&retained->memory&&retained->owner);
    printf("MIDI DOCUMENT HOST PASS:%s;genuine action/journal/owner;NO NATIVE AUTHORITY\n",argv[1]);return 0;
}
