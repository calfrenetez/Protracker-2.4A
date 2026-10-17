#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/native/midi_requester.h"

struct allocation {void *pointer;};
struct fixture {
    struct allocation blocks[128];unsigned live,calls,frees,hooks,allow,fail,alias,reenter;
    struct pt_document document;struct pt_editor *editor;struct pt_midi_setup *setup;
    struct pt_midi_requester_model *model;
};
static void reentry(struct fixture *f)
{
    struct pt_midi_requester_model before=*f->model;
    assert(pt_midi_requester_model_apply(f->model)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_requester_model_cancel(f->model)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_requester_model_select(f->model,PT_MIDI_SETUP_CLOCK)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_requester_model_key(f->model,PT_MIDI_KEY_ESCAPE,0)==PT_MIDI_EVENT_NONE);
    assert(!memcmp(&before,f->model,sizeof(before)));
}
static void *allocate(void *context,size_t bytes)
{
    struct fixture *f=context;unsigned i;void *p;++f->calls;
    if(f->reenter)reentry(f);
    if(f->alias)return f->editor;
    if(f->fail)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<128&&f->blocks[i].pointer;++i){}
    assert(i<128);f->blocks[i].pointer=p;++f->live;return p;
}
static void release(void *context,void *p)
{
    struct fixture *f=context;unsigned i;
    for(i=0;i<128&&f->blocks[i].pointer!=p;++i){}
    assert(i<128&&f->live);f->blocks[i].pointer=NULL;--f->live;++f->frees;free(p);
}
static int ready(void *context)
{struct fixture *f=context;++f->hooks;if(f->reenter)reentry(f);return (int)f->allow;}
static void start(struct fixture *f,unsigned tracks)
{
    struct pt_allocator allocator;
    memset(f,0,sizeof(*f));f->allow=1;allocator=(struct pt_allocator){f,allocate,release};
    pt_document_init(&f->document,&allocator);
    assert(pt_document_new(&f->document,tracks,SIZE_MAX)==PT_PROJECT_OK);
    f->editor=calloc(1,sizeof(*f->editor));f->setup=calloc(1,pt_midi_setup_size());
    f->model=calloc(1,sizeof(*f->model));
    assert(f->editor&&f->setup&&f->model&&pt_editor_init(f->editor,&f->document.project));
    /* A fresh independent model object cannot overlap the live allocator/guard
       context f; explicitly observe that complete context-placement boundary. */
    assert((uintptr_t)f->model>=(uintptr_t)f+sizeof(*f)||
        (uintptr_t)f>=(uintptr_t)f->model+sizeof(*f->model));
    pt_sampler_init(&f->editor->sampler,&allocator,1024*1024);
    assert(pt_midi_setup_init(f->setup,pt_midi_setup_size(),f->editor,
        8*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
    assert(pt_editor_change_barrier(f->editor,ready,f));
}
static void finish(struct fixture *f)
{
    f->fail=f->reenter=0;f->allow=1;
    if(f->model->self)assert(pt_midi_requester_model_cancel(f->model)==PT_MIDI_SETUP_OK);
    assert(pt_editor_dispose(f->editor));free(f->editor);f->editor=NULL;
    assert(pt_midi_setup_close(f->setup)==PT_MIDI_SETUP_OK);free(f->setup);f->setup=NULL;
    pt_document_release(&f->document);assert(!f->live);
    free(f->model);f->model=NULL;
}
static void open_model(struct fixture *f,unsigned track)
{assert(pt_midi_requester_model_open(f->model,f->setup,track)==PT_MIDI_SETUP_OK);}
static void select_field(struct fixture *f,enum pt_midi_setup_target target)
{assert(pt_midi_requester_model_select(f->model,target)==PT_MIDI_SETUP_OK);}
static void input(struct fixture *f,const char *text)
{
    const unsigned char *p=(const unsigned char *)text;
    assert(pt_midi_requester_model_focus(f->model,PT_MIDI_FOCUS_NAME));
    assert(pt_midi_requester_model_key(f->model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    while(*p)assert(pt_midi_requester_model_key(f->model,PT_MIDI_KEY_CHARACTER,*p++)==PT_MIDI_EVENT_CHANGED);
}
static void chooser_and_noop_are_observational(void)
{
    struct fixture f;struct pt_project before;struct pt_midi_requester_model zero={0},saved;
    struct pt_midi_setup_status status;unsigned calls;
    start(&f,4);before=f.document.project;calls=f.calls;open_model(&f,1);saved=*f.model;
    assert(f.model->stage==PT_MIDI_REQUESTER_CHOOSE&&f.model->track==1);
    assert(!memcmp(&before,&f.document.project,sizeof(before))&&!f.hooks&&f.calls==calls);
    assert(pt_midi_setup_get(f.setup,&status)==PT_MIDI_SETUP_OK&&!status.draft);
    assert(pt_midi_requester_model_open(f.model,f.setup,1)==PT_MIDI_SETUP_INVALID);
    assert(!memcmp(&saved,f.model,sizeof(saved)));
    assert(pt_midi_requester_model_cancel(f.model)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_begin(f.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_open(&zero,f.setup,1)==PT_MIDI_SETUP_IN_USE&&!zero.self);
    assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    open_model(&f,1);select_field(&f,PT_MIDI_SETUP_INPUT);f.editor->playback.active=1;f.allow=0;
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK&&!f.model->self);
    assert(f.editor->playback.active&&!f.hooks&&f.calls==calls&&!f.editor->history.count);
    assert(!memcmp(&before,&f.document.project,sizeof(before)));f.editor->playback.active=0;finish(&f);
}
static void focus_hits_and_cancellation(void)
{
    struct fixture f;struct pt_midi_setup_status status;unsigned i,calls;
    start(&f,4);calls=f.calls;open_model(&f,2);
    assert(pt_midi_requester_hit(f.model,14,52)==PT_MIDI_FOCUS_INPUT);
    assert(pt_midi_requester_hit(f.model,537,75)==PT_MIDI_FOCUS_INPUT);
    assert(pt_midi_requester_hit(f.model,538,75)==PT_MIDI_FOCUS_COUNT);
    assert(pt_midi_requester_hit(f.model,537,76)==PT_MIDI_FOCUS_COUNT);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_BACKTAB,0)==PT_MIDI_EVENT_CHANGED);
    assert(f.model->focus==PT_MIDI_FOCUS_CANCEL);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_TAB,0)==PT_MIDI_EVENT_CHANGED);
    assert(f.model->focus==PT_MIDI_FOCUS_INPUT);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,' ')==PT_MIDI_EVENT_CHANGED);
    assert(f.model->stage==PT_MIDI_REQUESTER_FIELD&&f.model->focus==PT_MIDI_FOCUS_NAME);
    assert(pt_midi_requester_hit(f.model,14,52)==PT_MIDI_FOCUS_COUNT);
    assert(!pt_midi_requester_model_focus(f.model,PT_MIDI_FOCUS_SEND));
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    assert(f.model->focus==PT_MIDI_FOCUS_APPLY);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_APPLY);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_ESCAPE,0)==PT_MIDI_EVENT_CANCEL);
    assert(pt_midi_requester_model_cancel(f.model)==PT_MIDI_SETUP_OK);
    for(i=PT_MIDI_SETUP_INPUT;i<=PT_MIDI_SETUP_CLOCK;++i){
        open_model(&f,2);select_field(&f,(enum pt_midi_setup_target)i);
        assert(pt_midi_requester_model_focus(f.model,PT_MIDI_FOCUS_CANCEL));
        assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CANCEL);
        assert(pt_midi_requester_model_cancel(f.model)==PT_MIDI_SETUP_OK);
    }
    assert(pt_midi_setup_get(f.setup,&status)==PT_MIDI_SETUP_OK&&!status.draft&&!status.commands);
    assert(!f.hooks&&f.calls==calls&&!f.editor->history.count);finish(&f);
}
static void exact_text_capacity_editing_and_empty_clear(void)
{
    struct fixture f;struct pt_midi_requester_model before;char expected[64];unsigned i;
    start(&f,4);open_model(&f,0);select_field(&f,PT_MIDI_SETUP_INPUT);input(&f,"AC");
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_HOME,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_RIGHT,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,'B')==PT_MIDI_EVENT_CHANGED);
    assert(!strcmp(f.model->name,"ABC"));
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_BACKSPACE,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_DELETE,0)==PT_MIDI_EVENT_CHANGED);
    assert(!strcmp(f.model->name,"A"));
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_END,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,0xe9)==PT_MIDI_EVENT_CHANGED);
    assert((unsigned char)f.model->name[1]==0xe9);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    for(i=0;i<63;++i)assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,i==31?0xe9:'x')==PT_MIDI_EVENT_CHANGED);
    memcpy(expected,f.model->name,64);before=*f.model;
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,'y')==PT_MIDI_EVENT_NONE);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,0)==PT_MIDI_EVENT_NONE);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,127)==PT_MIDI_EVENT_NONE);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,256)==PT_MIDI_EVENT_NONE);
    assert(!memcmp(&before,f.model,sizeof(before)));
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK);
    assert(!memcmp(f.document.project.midi_input,expected,64)&&f.editor->history.count==1);
    open_model(&f,0);select_field(&f,PT_MIDI_SETUP_INPUT);input(&f,"");
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK&&!f.document.project.midi_input[0]);
    assert(f.editor->history.count==2);
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!memcmp(f.document.project.midi_input,expected,64));finish(&f);
}
static void one_captured_output_preserves_other_routes_and_slots(void)
{
    struct fixture f;struct pt_channels channels;char before[PT_CHANNEL_LIMIT][64],expected[PT_CHANNEL_LIMIT][64];
    start(&f,4);assert(pt_channels_route(&f.document.project.channels,1,PT_AMIGUS)==PT_CHANNEL_OK);
    assert(pt_channels_route(&f.document.project.channels,2,PT_MIDI)==PT_CHANNEL_OK);
    memset(f.document.project.midi_output+4,0x7e,12*64);strcpy(f.document.project.midi_input,"keys");
    f.document.project.channels.selected=2;open_model(&f,2);f.document.project.channels.selected=0;
    channels=f.document.project.channels;memcpy(before,f.document.project.midi_output,sizeof(before));
    memcpy(expected,before,sizeof(expected));memset(expected[2],0,64);strcpy(expected[2],"missing synth");
    select_field(&f,PT_MIDI_SETUP_OUTPUT);input(&f,"missing synth");
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK);
    assert(!memcmp(expected,f.document.project.midi_output,sizeof(expected)));
    assert(!memcmp(&channels,&f.document.project.channels,sizeof(channels))&&!strcmp(f.document.project.midi_input,"keys"));
    assert(!f.document.project.midi_flags&&f.editor->history.count==1);
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!memcmp(before,f.document.project.midi_output,sizeof(before)));finish(&f);
}
static void clock_flags_are_one_transaction(void)
{
    struct fixture f;char input_before[64],output_before[PT_CHANNEL_LIMIT][64];
    start(&f,4);memcpy(input_before,f.document.project.midi_input,64);memcpy(output_before,f.document.project.midi_output,sizeof(output_before));
    open_model(&f,3);select_field(&f,PT_MIDI_SETUP_CLOCK);
    assert(pt_midi_requester_hit(f.model,14,72)==PT_MIDI_FOCUS_SEND);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,' ')==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_TAB,0)==PT_MIDI_EVENT_CHANGED);
    assert(f.model->focus==PT_MIDI_FOCUS_RECEIVE);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    assert(f.model->flags==3&&pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK);
    assert(f.document.project.midi_flags==3&&f.editor->history.count==1);
    assert(!memcmp(input_before,f.document.project.midi_input,64)&&!memcmp(output_before,f.document.project.midi_output,sizeof(output_before)));
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK&&!f.document.project.midi_flags);
    open_model(&f,3);select_field(&f,PT_MIDI_SETUP_CLOCK);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_CHARACTER,'x')==PT_MIDI_EVENT_NONE);
    assert(pt_midi_requester_model_key(f.model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_cancel(f.model)==PT_MIDI_SETUP_OK&&!f.document.project.midi_flags);
    assert(f.editor->history.count==1&&f.editor->history.cursor==0);finish(&f);
}
static void refusals_retain_only_the_single_draft(void)
{
    struct fixture f;unsigned calls;
    start(&f,4);calls=f.calls;open_model(&f,0);select_field(&f,PT_MIDI_SETUP_INPUT);input(&f,"keys");
    f.editor->playback.active=1;
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_ACTIVE);
    assert(f.model->self&&f.model->last_result==PT_MIDI_SETUP_ACTIVE&&f.editor->playback.active&&!f.hooks&&f.calls==calls);
    f.editor->playback.active=0;f.allow=0;
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_VETO&&f.model->self);
    assert(f.calls==calls&&!f.document.project.midi_input[0]&&!f.editor->history.count);
    f.allow=1;f.fail=1;
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_CAPACITY&&f.model->self);
    assert(!f.document.project.midi_input[0]&&!f.editor->history.count);f.fail=0;
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK&&!f.model->self);
    assert(!strcmp(f.document.project.midi_input,"keys")&&f.editor->history.count==1);finish(&f);
}
static void stale_drafts_and_callback_reentry(void)
{
    struct fixture f;unsigned hooks,calls;
    start(&f,4);open_model(&f,0);select_field(&f,PT_MIDI_SETUP_INPUT);input(&f,"keys");hooks=f.hooks;calls=f.calls;
    assert(pt_pattern_title_apply(&f.document.project,&f.editor->history,"OTHER")==PT_EDIT_OK);
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_STALE&&f.model->self);
    assert(f.hooks==hooks&&f.calls==calls&&!f.document.project.midi_input[0]);
    assert(pt_midi_requester_model_cancel(f.model)==PT_MIDI_SETUP_OK);
    open_model(&f,0);select_field(&f,PT_MIDI_SETUP_OUTPUT);input(&f,"synth");f.reenter=1;
    assert(pt_midi_requester_model_apply(f.model)==PT_MIDI_SETUP_OK&&!f.model->self);
    assert(!strcmp(f.document.project.midi_output[0],"synth")&&f.editor->history.count==2);
    f.reenter=0;finish(&f);
}
/* Actual broken-allocator alias enters the controller's original retained HELD
 * state. The full original rig and model stay reachable until HOST process exit;
 * closing this UI is explicitly not controller/resource cleanup. */
static struct fixture retained;
static void held_closes_only_the_ui_and_retains_original_controller(void)
{
    struct fixture *f=&retained;struct pt_midi_setup_status out,before;unsigned frees;
    start(f,4);open_model(f,0);select_field(f,PT_MIDI_SETUP_INPUT);input(f,"keys");
    f->alias=1;frees=f->frees;
    assert(pt_midi_requester_model_apply(f->model)==PT_MIDI_SETUP_HELD&&f->model->self&&f->model->held);
    assert(pt_midi_requester_model_key(f->model,PT_MIDI_KEY_ESCAPE,0)==PT_MIDI_EVENT_CANCEL);
    assert(pt_midi_requester_model_cancel(f->model)==PT_MIDI_SETUP_HELD&&!f->model->self);
    memset(&out,0x5a,sizeof(out));before=out;
    assert(pt_midi_setup_get(f->setup,&out)==PT_MIDI_SETUP_HELD&&!memcmp(&out,&before,sizeof(out)));
    assert(pt_midi_setup_close(f->setup)==PT_MIDI_SETUP_HELD&&f->frees==frees&&!f->document.project.midi_input[0]);
}
int main(void)
{
    chooser_and_noop_are_observational();focus_hits_and_cancellation();
    exact_text_capacity_editing_and_empty_clear();one_captured_output_preserves_other_routes_and_slots();
    clock_flags_are_one_transaction();refusals_retain_only_the_single_draft();
    stale_drafts_and_callback_reentry();held_closes_only_the_ui_and_retains_original_controller();
    puts("MIDI REQUESTER MODEL HOST PASS:8 groups;single-field genuine journal;silent chooser/text/clock/cancel;retained controller;NO NATIVE WINDOW/CAMD/OUTPUT AUTHORITY");
    return 0;
}
