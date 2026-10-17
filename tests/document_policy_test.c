/* Genuine typed-document HOST cases. No MIDI owner, native API or output. */
#define PT_DOCUMENT_ACTION_TEST_ENTRY original_policy_action_entry
#include "editor_document_action_focus_test.c"

#define POLICY_BUDGET ((size_t)1024*1024)
#define POLICY_SONG_BUDGET ((size_t)128*1024)
enum policy_kind {POLICY_GENERAL,POLICY_SOURCE};
struct policy_fixture;
struct policy_memory {
    struct {void *data;size_t bytes;enum policy_kind kind;} blocks[512];
    struct policy_fixture *fixture;enum policy_kind kind;
    size_t bytes,source_bytes;unsigned calls,frees,live,source_live,source_frees;
};
struct policy_fixture {
    struct action_fixture *action;
    struct policy_memory memory,song;
    struct pt_document reentry_document;
    size_t source_bytes,reduced_budget;
    unsigned source_allocations,loader_calls,main_frees,main_allocations;
    unsigned observe_release,reenter_source,reentries;
};
static struct policy_fixture *policy_retained;
static int policy_allocator_same(struct pt_allocator a,struct pt_allocator b)
{return a.context==b.context&&a.allocate==b.allocate&&a.release==b.release;}
static void *policy_allocate(void *context,size_t bytes)
{
    struct policy_memory *m=context;void *p;unsigned i;++m->calls;
    p=malloc(bytes);if(!p)return NULL;
    for(i=0;i<512&&m->blocks[i].data;++i){}
    assert(i<512&&bytes<=SIZE_MAX-m->bytes);
    m->blocks[i].data=p;m->blocks[i].bytes=bytes;m->blocks[i].kind=m->kind;
    m->bytes+=bytes;++m->live;
    if(m->kind==POLICY_SOURCE){m->source_bytes+=bytes;++m->source_live;}
    return p;
}
static void policy_release(void *context,void *p)
{
    struct policy_memory *m=context;size_t bytes;enum policy_kind kind;unsigned i;
    for(i=0;i<512&&m->blocks[i].data!=p;++i){}
    assert(p&&i<512&&m->live);bytes=m->blocks[i].bytes;kind=m->blocks[i].kind;
    assert(bytes<=m->bytes);m->bytes-=bytes;--m->live;++m->frees;
    memset(m->blocks+i,0,sizeof(m->blocks[i]));
    if(kind==POLICY_SOURCE){assert(m->source_live&&bytes<=m->source_bytes);
        --m->source_live;m->source_bytes-=bytes;++m->source_frees;}
    free(p);
    if(kind==POLICY_SOURCE&&m->fixture->reenter_source){
        struct policy_fixture *f=m->fixture;f->reenter_source=0;++f->reentries;
        /* Actual factory reentry poisons this original checked release. No
         * caller-made receipt or private quarantine/owner mutation is used. */
        assert(pt_document_release_checked(&f->reentry_document)==PT_DOCUMENT_RELEASE_REFUSED);
    }
}
static struct editor_checked_fixture *policy_checked_make(struct policy_memory *memory)
{
    struct editor_checked_fixture *g=calloc(1,sizeof(*g));struct pt_allocator a={NULL,fast_alloc,fast_free};
    struct pt_editor_mixed_establish *master=calloc(1,sizeof(*master));struct pt_sampler_storage_span parent;
    enum pt_establish_result r;unsigned i,n=0;assert(g&&master);
    g->editor=malloc(sizeof(*g->editor));g->binding=calloc(1,sizeof(*g->binding));g->control=calloc(1,sizeof(*g->control));
    assert(g->editor&&g->binding&&g->control);
    for(i=0;i<2048;++i)g->pcm[i]=((int32_t)(i%120)-60)*251;
    init(&g->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&g->bus.cache,&g->bus.reservation,0,4096,4096,&g->bus,bus_owned,bus_write));
    pt_document_init(&g->doc,&a);assert(pt_document_new(&g->doc,16,SIZE_MAX)==PT_PROJECT_OK);
    g->doc.project.sample_count=3;
    for(i=0;i<16;++i)g->doc.project.channels.track[i].route=PT_AMIGUS;
    g->doc.project.channels.track[4].route=PT_PAULA;g->doc.project.channels.track[4].pan=0;
    for(i=0;i<3;++i){g->doc.project.samples[i].pcm=(struct pt_pcm){g->pcm,2048,2048,8000,1,16};g->doc.project.samples[i].volume=64;}
    g->doc.project.speed=1;g->doc.project.events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    g->doc.project.events[7]=g->doc.project.events[4];g->doc.project.events[16+7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    g->doc.project.events[32+15].effect=15;
    assert(pt_editor_init(g->editor,&g->doc.project));
    /* Original sampler policy is selected while genuinely empty, not swapped
     * after versions exist. release_version uses this same current allocator. */
    g->editor->sampler.allocator=(struct pt_allocator){memory,policy_allocate,policy_release};
    g->editor->sampler.budget=POLICY_BUDGET;
    assert(pt_editor_mixed_attach(g->binding,g->editor));parent=(struct pt_sampler_storage_span){g,sizeof(*g)};
    assert(pt_editor_mixed_establish_begin(master,g->binding,&parent,1)==PT_ESTABLISH_PENDING);
    do{r=pt_editor_mixed_establish_step(master,256);assert(++n<10000);}while(r==PT_ESTABLISH_PENDING);
    assert(r==PT_ESTABLISH_READY&&memory->live==4&&!g->live&&!g->binding->owner);
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
    output_count=0;g->checked=0;return g;
}
static void policy_main_release(void *context,void *p)
{
    struct policy_fixture *f=context;struct editor_checked_fixture *g=f->action->f->g;
    if(f->observe_release){
        assert(!g->editor->sample_source.loaded&&!g->editor->sample_source.allocated_bytes&&
            !g->editor->song.current&&!g->editor->song.bytes&&g->editor->sampler.budget==POLICY_BUDGET);
        assert(!f->memory.source_live&&f->memory.source_frees==f->source_allocations);
        assert(f->action->action.replacement.release_attempted);
    }
    assert(p);++f->main_frees;++f->action->f->old_releases;free(p);
}
static enum pt_project_result policy_source(void *context,struct pt_document *next,size_t budget)
{
    struct policy_fixture *f=context;struct pt_allocator a={&f->memory,policy_allocate,policy_release};
    assert(policy_allocator_same(next->allocator,a));++f->loader_calls;
    return pt_document_new(next,4,budget);
}
static void policy_capture(struct action_fixture *a)
{
    size_t used;free(a->encoded);
    assert(pt_project_size(&a->f->g->doc.project,&a->bytes)==PT_PROJECT_OK);
    a->encoded=malloc(a->bytes);assert(a->encoded);
    assert(pt_project_encode(&a->f->g->doc.project,a->encoded,a->bytes,&used)==PT_PROJECT_OK&&used==a->bytes);
}
static struct policy_fixture *policy_make(unsigned song_edit,unsigned overflow)
{
    struct policy_fixture *f=calloc(1,sizeof(*f));struct action_fixture *a;
    struct replace_fixture *r;struct editor_checked_fixture *g;struct pt_allocator song;
    void *owned[7];size_t sizes[7];unsigned i;assert(f);
    f->memory.fixture=f;f->song.fixture=f;a=calloc(1,sizeof(*a));assert(a);f->action=a;
    a->f=r=calloc(1,sizeof(*r));assert(r);r->candidate=calloc(1,sizeof(*r->candidate));assert(r->candidate);
    r->g=g=policy_checked_make(&f->memory);
    song=(struct pt_allocator){&f->song,policy_allocate,policy_release};
    assert(!g->editor->song.current&&!g->editor->song.bytes);
    pt_song_init(&g->editor->song,&song,POLICY_SONG_BUDGET);
    f->memory.kind=POLICY_SOURCE;
    assert(pt_editor_source_load_with(g->editor,policy_source,f)==PT_EDIT_OK);
    f->memory.kind=POLICY_GENERAL;f->source_bytes=g->editor->sample_source.allocated_bytes;
    f->source_allocations=f->memory.source_live;f->reduced_budget=g->editor->sampler.budget;
    assert(f->loader_calls==1&&f->source_bytes&&f->source_allocations&&
        f->memory.source_bytes==f->source_bytes&&f->reduced_budget==POLICY_BUDGET-f->source_bytes);
    if(song_edit){
        assert(pt_editor_prepare_change(g->editor));
        assert(pt_song_append(&g->editor->song,&g->doc.project,&g->editor->history,0,1)==PT_EDIT_OK);
        assert(g->doc.project.pattern_count==2&&g->editor->song.current&&f->song.live);
        assert(pt_pattern_undo(&g->doc.project,&g->editor->history,-1)==PT_EDIT_OK&&g->doc.project.pattern_count==1);
        assert(pt_pattern_undo(&g->doc.project,&g->editor->history,1)==PT_EDIT_OK&&g->doc.project.pattern_count==2);
    }
    if(overflow)g->editor->sampler.budget=SIZE_MAX-f->source_bytes+1;
    assert(!g->binding->owner&&!g->binding->owner_finish&&!g->pv.closing&&!g->av.closing);
    assert(checked_editor_begin(g)==PT_MIXED_OWNER_PREPARING);
    assert(pt_document_owned_storage_spans(&g->doc,owned,sizes));
    for(i=0;i<7;++i)if(owned[i])++f->main_allocations;
    g->doc.allocator.context=f;g->doc.allocator.release=policy_main_release;
    init(&r->bus,PT_AMIGUS_WAVETABLE);
    assert(pt_amigus_wavetable_cache_attach(&r->bus.cache,&r->bus.reservation,0,4096,4096,&r->bus,bus_owned,bus_write));
    r->future_format=g->format;
    r->inputs=(struct pt_editor_mixed_bridge_inputs){&r->bus.cache,&g->d,chip_alloc,chip_free,4096,
        g->pa,g->wa,quiesce,&g->d,wave_quiesce,&g->wd};
    assert(pt_document_release_checked(r->candidate)==PT_DOCUMENT_RELEASED);
    memset(r->candidate,0,sizeof(*r->candidate));
    a->inputs=(struct pt_editor_document_action_inputs){
        .document=&g->doc,.candidate=r->candidate,.binding=g->binding,.old_control=g->control,.new_control=&r->fresh,
        .old_paula=&g->pv,.old_amigus=&g->av,.establish=&r->master,.bridges=&r->bridges,.bridge_inputs=r->inputs,
        .options=g->options,.caps=g->caps,.format=r->future_format,.allocator={a,action_allocate,action_release},
        .contexts={{a,sizeof(*a)},{r,sizeof(*r)},{f,sizeof(*f)}},.context_count=3,.work=64};
    {struct pt_allocator empty={NULL,fast_alloc,fast_free};pt_document_init(&f->reentry_document,&empty);}
    policy_capture(a);return f;
}
static void policy_finish(struct policy_fixture *f)
{
    struct action_fixture *a=f->action;struct replace_fixture *r=a->f;struct editor_checked_fixture *g=r->g;
    assert(!output_count&&!g->d.starts&&!g->wd.starts);
    assert(pt_editor_mixed_stop(g->binding)&&pt_editor_dispose(g->editor));
    if(r->bridges.paula.bridge)assert(pt_paula_voices_close(&r->bridges.paula));
    if(r->bridges.amigus.bridge)assert(pt_wavetable_voices_close(&r->bridges.amigus));
    if(a->action.self)assert(pt_editor_document_action_finish(&a->action));
    assert(!f->memory.source_live&&!f->memory.source_bytes&&f->memory.source_frees==f->source_allocations);
    assert(pt_document_release_checked(&f->reentry_document)==PT_DOCUMENT_RELEASED);
    action_drop(a);
    assert(!f->memory.live&&!f->memory.bytes&&!f->song.live&&!f->song.bytes);free(f);
}
static void policy_main_equal(struct action_fixture *a)
{
    size_t used;uint8_t *saved=malloc(a->bytes);assert(saved);
    assert(pt_project_encode(&a->f->g->doc.project,saved,a->bytes,&used)==PT_PROJECT_OK&&used==a->bytes&&
        !memcmp(saved,a->encoded,a->bytes));free(saved);
}
static void policy_publish(struct policy_fixture *f,unsigned load)
{
    struct action_fixture *a=f->action;struct editor_checked_fixture *g=a->f->g;
    struct pt_allocator sampler={&f->memory,policy_allocate,policy_release},song={&f->song,policy_allocate,policy_release};
    enum pt_document_action_result result;unsigned n=0;
    assert((load?pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX):
        pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX))==PT_DOCUMENT_ACTION_PREPARED);
    assert(g->editor->sampler.budget==f->reduced_budget&&g->editor->sample_source.allocated_bytes==f->source_bytes);
    f->observe_release=1;
    do{result=pt_editor_document_action_step(&a->action);assert(++n<10000);}while(result==PT_DOCUMENT_ACTION_PUBLISHED);
    f->observe_release=0;
    assert(result==PT_DOCUMENT_ACTION_COMMITTED&&f->main_frees==f->main_allocations&&
        !f->memory.source_live&&f->memory.source_frees==f->source_allocations);
    assert(!g->editor->sample_source.loaded&&!g->editor->sample_source.allocated_bytes&&
        g->editor->sampler.budget==POLICY_BUDGET&&policy_allocator_same(g->editor->sampler.allocator,sampler));
    assert(!g->editor->song.current&&!g->editor->song.bytes&&g->editor->song.budget==POLICY_SONG_BUDGET&&
        policy_allocator_same(g->editor->song.allocator,song)&&!f->song.live);
    if(load)policy_main_equal(a);
}
static void policy_preservation(void)
{
    struct policy_fixture *f=policy_make(0,0);struct action_fixture *a=f->action;struct editor_checked_fixture *g=a->f->g;
    struct pt_editor *editor=malloc(sizeof(*editor));struct pt_document source=g->editor->sample_source,doc=g->doc;
    unsigned calls=f->memory.calls,frees=f->memory.frees,song_calls=f->song.calls,song_frees=f->song.frees;assert(editor);
    memcpy(editor,g->editor,sizeof(*editor));a->hook=3;
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_FAILED);
    a->hook=0;
    assert(!memcmp(editor,g->editor,sizeof(*editor))&&!memcmp(&doc,&g->doc,sizeof(doc))&&
        !memcmp(&source,&g->editor->sample_source,sizeof(source)));
    policy_main_equal(a);assert(pt_editor_document_action_reset(&a->action));
    assert(pt_editor_document_action_load(&a->action,&a->inputs,a->encoded,a->bytes,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    assert(pt_editor_document_action_cancel(&a->action)==PT_DOCUMENT_ACTION_CANCELED);
    assert(!memcmp(editor,g->editor,sizeof(*editor))&&!memcmp(&doc,&g->doc,sizeof(doc))&&
        !memcmp(&source,&g->editor->sample_source,sizeof(source))&&g->editor->sampler.budget==f->reduced_budget);
    policy_main_equal(a);
    assert(calls==f->memory.calls&&frees==f->memory.frees&&song_calls==f->song.calls&&song_frees==f->song.frees&&
        !f->main_frees&&!f->memory.source_frees&&f->memory.source_live==f->source_allocations);
    free(editor);policy_finish(f);
}
static void policy_song(void)
{
    struct policy_fixture *f=policy_make(1,0);struct editor_checked_fixture *g=f->action->f->g;unsigned calls;
    assert(f->song.live&&g->editor->song.current&&g->editor->history.count==1&&g->editor->history.cursor==1);
    policy_publish(f,0);calls=f->song.calls;
    /* The new genuine checked owner still holds the project until this actual
     * editor barrier confirms its stop. No edit is made behind an active owner. */
    assert(pt_editor_prepare_change(g->editor)&&!g->binding->owner&&!g->binding->owner_finish);
    assert(pt_song_append(&g->editor->song,&g->doc.project,&g->editor->history,0,1)==PT_EDIT_OK);
    assert(f->song.calls>calls&&f->song.live&&g->editor->song.current&&g->doc.project.pattern_count==2);
    assert(pt_pattern_undo(&g->doc.project,&g->editor->history,-1)==PT_EDIT_OK&&g->doc.project.pattern_count==1);
    assert(pt_pattern_undo(&g->doc.project,&g->editor->history,1)==PT_EDIT_OK&&g->doc.project.pattern_count==2);
    policy_finish(f);
}
static void policy_overflow(void)
{
    struct policy_fixture *f=policy_make(0,1);struct action_fixture *a=f->action;struct replace_fixture *r=a->f;
    struct editor_checked_fixture *g=r->g;unsigned calls,frees,alloc_calls,alloc_frees,barriers,wave_barriers;
    unsigned char *control=malloc(sizeof(a->action));struct pt_document source=g->editor->sample_source,doc=g->doc;
    unsigned char *future=malloc(sizeof(*r)),*editor=malloc(sizeof(*g->editor));
    struct pt_editor_mixed binding=*g->binding;assert(control&&future&&editor);
    /* The candidate factory is genuinely complete BEFORE the no-write/no-stop
     * snapshot. No no-allocation claim covers preparation of this candidate. */
    pt_document_init(r->candidate,&a->inputs.allocator);
    assert(pt_document_new(r->candidate,8,SIZE_MAX)==PT_PROJECT_OK&&r->candidate->loaded&&a->live);
    assert(pt_editor_mixed_checked_binding_current(g->binding,g->control,&g->pv,&g->av));
    /* Raw beforeimages are observations only, never initialized/copied owners. */
    memcpy(control,&a->action,sizeof(a->action));memcpy(future,r,sizeof(*r));memcpy(editor,g->editor,sizeof(*g->editor));
    calls=f->memory.calls;frees=f->memory.frees;
    alloc_calls=a->calls;alloc_frees=a->releases;barriers=g->d.barriers;wave_barriers=g->wd.barriers;
    assert(pt_editor_document_replacement_begin(&a->action.replacement,&g->doc,r->candidate,g->binding,g->control,
        &g->pv,&g->av,&r->master,&r->fresh,&r->bridges,&r->inputs,&g->options,&g->caps,&r->future_format,
        (struct pt_sampler_storage_span){&a->action,sizeof(a->action)},64)==PT_DOCUMENT_REPLACE_INVALID);
    assert(!memcmp(control,&a->action,sizeof(a->action))&&!memcmp(future,r,sizeof(*r))&&
        !memcmp(editor,g->editor,sizeof(*g->editor))&&!memcmp(&binding,g->binding,sizeof(binding))&&
        !memcmp(&source,&g->editor->sample_source,sizeof(source))&&!memcmp(&doc,&g->doc,sizeof(doc))&&
        calls==f->memory.calls&&frees==f->memory.frees&&alloc_calls==a->calls&&alloc_frees==a->releases&&
        barriers==g->d.barriers&&wave_barriers==g->wd.barriers&&!f->main_frees&&!f->memory.source_frees);
    free(control);free(future);free(editor);
    /* Close the original owner under its unchanged original overflow snapshot.
     * Only after the refused new transition may the caller budget be restored
     * for ordinary source cleanup. No HOLD or quarantine is reset. */
    assert(pt_editor_mixed_stop(g->binding));g->editor->sampler.budget=f->reduced_budget;
    policy_finish(f);
}
static void policy_uncertain(void)
{
    struct policy_fixture *f=policy_make(1,0);struct action_fixture *a=f->action;struct editor_checked_fixture *g=a->f->g;
    struct pt_document original=g->doc;unsigned calls,frees,song_calls,song_frees;
    assert(pt_editor_document_action_new(&a->action,&a->inputs,8,SIZE_MAX)==PT_DOCUMENT_ACTION_PREPARED);
    f->reenter_source=1;
    assert(pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_HOLD&&f->reentries==1&&
        f->memory.source_frees==1&&f->memory.source_live==f->source_allocations-1&&f->memory.source_bytes);
    assert(!a->action.replacement.release_attempted&&!f->main_frees&&a->f->candidate->loaded&&
        !memcmp(&original,&g->doc,sizeof(original))&&pt_document_retirement_project_busy(&g->editor->sample_source.project));
    /* Old sample/song data may already be retired. Only numeric/header facts
     * are inspected here: no old project encoding, PCM read or rollback claim. */
    calls=f->memory.calls;frees=f->memory.frees;song_calls=f->song.calls;song_frees=f->song.frees;
    assert(pt_editor_document_action_step(&a->action)==PT_DOCUMENT_ACTION_HOLD&&!pt_editor_document_action_finish(&a->action)&&
        calls==f->memory.calls&&frees==f->memory.frees&&song_calls==f->song.calls&&song_frees==f->song.frees&&
        !a->action.replacement.release_attempted&&!f->main_frees);
    policy_retained=f;
}
int main(int argc,char **argv)
{
    struct policy_fixture *f;if(argc!=2)return 20;
    if(!strcmp(argv[1],"legacy")){char *args[]={argv[0],(char *)"actions",NULL};return original_policy_action_entry(2,args);}
    if(!strcmp(argv[1],"source-new")){f=policy_make(0,0);policy_publish(f,0);policy_finish(f);}
    else if(!strcmp(argv[1],"source-load")){f=policy_make(0,0);policy_publish(f,1);policy_finish(f);}
    else if(!strcmp(argv[1],"preservation"))policy_preservation();
    else if(!strcmp(argv[1],"song-policy"))policy_song();
    else if(!strcmp(argv[1],"overflow"))policy_overflow();
    else if(!strcmp(argv[1],"source-uncertain"))policy_uncertain();
    else return 20;
    if(policy_retained)assert(policy_retained->action&&policy_retained->action->f&&policy_retained->memory.source_live);
    assert(!output_count);
    printf("DOCUMENT POLICY HOST PASS:%s;genuine source/song owners;NO NATIVE AUTHORITY\n",argv[1]);return 0;
}
