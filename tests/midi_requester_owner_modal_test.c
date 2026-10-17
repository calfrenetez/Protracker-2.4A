#include "../src/native/midi_requester.h"
#define main original_owner_main
#include "midi_setup_owner_test.c"
#undef main

struct modal_fixture {struct rig *rig;struct pt_midi_requester_modal *modal;unsigned recording,reenter,queries;};
static struct modal_fixture *modal_retained;
static unsigned modal_completed;
static int modal_apart(const void *a,size_t an,const void *b,size_t bn)
{return (uintptr_t)a>=(uintptr_t)b+bn||(uintptr_t)b>=(uintptr_t)a+an;}
static struct modal_fixture *modal_create(void)
{
    struct modal_fixture *f=calloc(1,sizeof(*f));assert(f);f->rig=create();
    f->modal=calloc(1,sizeof(*f->modal));assert(f->modal);
    assert(modal_apart(f->modal,sizeof(*f->modal),f,sizeof(*f))&&
        modal_apart(f->modal,sizeof(*f->modal),f->rig->context,sizeof(*f->rig->context))&&
        modal_apart(f->modal,sizeof(*f->modal),f->rig->owner,f->rig->owner_capacity)&&
        modal_apart(f->modal,sizeof(*f->modal),f->rig->editor,sizeof(*f->rig->editor))&&
        modal_apart(f->modal,sizeof(*f->modal),f->rig->document,sizeof(*f->rig->document)));
    assert(pt_channels_route(&f->rig->document->project.channels,1,PT_AMIGUS)==PT_CHANNEL_OK);
    assert(pt_channels_route(&f->rig->document->project.channels,2,PT_MIDI)==PT_CHANNEL_OK);
    memset(f->rig->document->project.midi_output+4,0x7e,12*PT_MIDI_ENDPOINT);
    assert(initialize(f->rig,8*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
    return f;
}
static void modal_open(struct modal_fixture *f,unsigned track)
{
    assert(pt_midi_requester_modal_begin(f->modal,f->rig->owner,f->rig->owner_capacity,f->rig->editor,track)==PT_MIDI_SETUP_OK);
    assert(f->modal->self==f->modal&&f->modal->ticket.owner==f->rig->owner&&
        (void *)f->modal->ticket.setup==f->rig->context->workspace&&
        f->modal->model.setup==f->modal->ticket.setup&&f->modal->model.track==track);
}
static void modal_text(struct modal_fixture *f,enum pt_midi_setup_target target,const char *name)
{
    const unsigned char *p=(const unsigned char *)name;
    assert(pt_midi_requester_model_select(&f->modal->model,target)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_key(&f->modal->model,PT_MIDI_KEY_CLEAR,0)==PT_MIDI_EVENT_CHANGED);
    while(*p)assert(pt_midi_requester_model_key(&f->modal->model,PT_MIDI_KEY_CHARACTER,*p++)==PT_MIDI_EVENT_CHANGED);
}
static void modal_apply(struct modal_fixture *f,enum pt_midi_setup_target target,unsigned track,const char *name)
{
    modal_open(f,track);modal_text(f,target,name);
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_OK&&!f->modal->model.self);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&!f->modal->self&&!status(f->rig).modal);
}
static void modal_finish(struct modal_fixture *f)
{
    assert(!f->modal->self);free(f->modal);finish(f->rig,0,0);free(f);++modal_completed;
}
static void modal_admission_and_failed_open(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;struct pt_midi_requester_modal zero={0};
    struct pt_editor *other=calloc(1,sizeof(*other));unsigned calls=r->context->calls,hooks=r->context->hooks;
    unsigned char *owner_before=malloc(r->owner_capacity),*context_before=malloc(sizeof(*r->context));
    assert(other&&owner_before&&context_before);memcpy(owner_before,r->owner,r->owner_capacity);memcpy(context_before,r->context,sizeof(*r->context));
    assert(pt_midi_requester_modal_begin(f->modal,r->owner,r->owner_capacity-1,r->editor,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_requester_modal_begin(f->modal,r->owner,r->owner_capacity+1,r->editor,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_requester_modal_begin(f->modal,r->owner,r->owner_capacity,other,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_requester_modal_begin((struct pt_midi_requester_modal *)r->owner,r->owner,r->owner_capacity,r->editor,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_requester_modal_begin((struct pt_midi_requester_modal *)r->context,r->owner,r->owner_capacity,r->editor,0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_requester_modal_begin((struct pt_midi_requester_modal *)r->context->workspace,r->owner,r->owner_capacity,r->editor,0)==PT_MIDI_SETUP_INVALID);
    assert(!memcmp(owner_before,r->owner,r->owner_capacity)&&!memcmp(context_before,r->context,sizeof(*r->context))&&
        !memcmp(f->modal,&zero,sizeof(zero))&&r->context->calls==calls&&r->context->hooks==hooks);
    free(owner_before);free(context_before);free(other);
    assert(pt_midi_requester_modal_begin(f->modal,r->owner,r->owner_capacity,r->editor,99)==PT_MIDI_SETUP_INVALID);
    assert(!memcmp(f->modal,&zero,sizeof(zero))&&!status(r).modal&&r->context->calls==calls&&r->context->hooks==hooks);
    modal_open(f,2);
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->owner_capacity,r->editor,NULL,0)==PT_MIDI_SETUP_BUSY);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&!memcmp(f->modal,&zero,sizeof(zero)));
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_owner_transition_admit(r->owner,r->owner_capacity,r->editor,NULL,0)==PT_MIDI_SETUP_OK);
    modal_open(f,1);assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK);modal_finish(f);
}
static void modal_chronology_persistence_and_redo(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;struct pt_project *p=&r->document->project;
    struct pt_channels channels;struct pt_midi_setup_owner_status old_status;struct pt_document decoded;
    struct pt_event_update note={0,{428,0,PT_NOTE_PERIOD,1,0,0,0,0}};
    struct pt_allocator a={r->context,allocate,release};uint8_t *encoded;size_t bytes,written;uint32_t revision;
    char inactive[12][PT_MIDI_ENDPOINT];memcpy(inactive,p->midi_output+4,sizeof(inactive));
    modal_apply(f,PT_MIDI_SETUP_INPUT,0,"keys");
    assert(pt_pattern_apply(p,&r->editor->history,&note,1)==PT_EDIT_OK);
    p->channels.selected=2;modal_open(f,2);p->channels.selected=0;channels=p->channels;
    modal_text(f,PT_MIDI_SETUP_OUTPUT,"synth");assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK);
    assert(!strcmp(p->midi_input,"keys")&&!strcmp(p->midi_output[2],"synth")&&
        !memcmp(&p->channels,&channels,sizeof(channels))&&!memcmp(p->midi_output+4,inactive,sizeof(inactive)));
    pt_editor_saved(r->editor);revision=r->editor->history.revision;assert(!pt_editor_dirty(r->editor));
    assert(pt_pattern_undo(p,&r->editor->history,-1)==PT_EDIT_OK&&!p->midi_output[2][0]&&pt_editor_dirty(r->editor));
    old_status=status(r);assert(old_status.commands==2&&old_status.metadata_bytes==2*pt_midi_setup_command_bytes());
    modal_open(f,0);assert(pt_midi_requester_model_select(&f->modal->model,PT_MIDI_SETUP_CLOCK)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_key(&f->modal->model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&!p->midi_flags);
    assert(r->editor->history.count==3&&r->editor->history.cursor==2&&status(r).commands==old_status.commands);
    assert(pt_pattern_undo(p,&r->editor->history,1)==PT_EDIT_OK&&!strcmp(p->midi_output[2],"synth"));
    assert(r->editor->history.revision==revision&&!pt_editor_dirty(r->editor));
    assert(pt_project_size(p,&bytes)==PT_PROJECT_OK);encoded=malloc(bytes);assert(encoded);
    assert(pt_project_encode(p,encoded,bytes,&written)==PT_PROJECT_OK&&written==bytes);
    pt_document_init(&decoded,&a);assert(pt_document_load(&decoded,encoded,bytes,LIMIT)==PT_PROJECT_OK);
    assert(!strcmp(decoded.project.midi_input,"keys")&&!strcmp(decoded.project.midi_output[2],"synth")&&
        !memcmp(&decoded.project.channels,&p->channels,sizeof(p->channels)));
    assert(pt_document_release_checked(&decoded)==PT_DOCUMENT_RELEASED);free(encoded);modal_finish(f);
}
static int modal_recording(void *context)
{
    struct modal_fixture *f=context;++f->queries;
    if(f->reenter){
        struct pt_midi_requester_modal before=*f->modal;
        assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_BUSY);
        assert(pt_midi_requester_modal_begin(f->modal,f->rig->owner,f->rig->owner_capacity,f->rig->editor,0)==PT_MIDI_SETUP_BUSY);
        assert(!memcmp(&before,f->modal,sizeof(before)));
    }
    return (int)f->recording;
}
static void modal_active_noop_and_capacity(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;unsigned calls=r->context->calls,hooks=r->context->hooks;
    r->editor->workflow.recording_busy=modal_recording;r->editor->workflow.recording_context=f;
    modal_open(f,0);modal_text(f,PT_MIDI_SETUP_INPUT,"keys");f->recording=1;
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_ACTIVE);
    f->recording=0;r->editor->playback.active=1;
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_ACTIVE&&r->editor->playback.active);
    r->editor->playback.active=0;r->editor->workflow.busy=1;
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_ACTIVE);r->editor->workflow.busy=0;
    assert(r->context->calls==calls&&r->context->hooks==hooks&&!r->editor->history.count);
    r->context->fail=1;assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_CAPACITY);r->context->fail=0;
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&!r->document->project.midi_input[0]);
    calls=r->context->calls;hooks=r->context->hooks;modal_open(f,0);
    assert(pt_midi_requester_model_select(&f->modal->model,PT_MIDI_SETUP_INPUT)==PT_MIDI_SETUP_OK);
    r->editor->playback.active=1;f->recording=1;
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_OK&&r->editor->playback.active);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&r->context->calls==calls&&r->context->hooks==hooks);
    r->editor->playback.active=0;f->recording=0;r->editor->workflow.recording_busy=NULL;r->editor->workflow.recording_context=NULL;modal_finish(f);
}
static void modal_actual_callback_reentry(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;unsigned queries;
    modal_open(f,0);modal_text(f,PT_MIDI_SETUP_INPUT,"keys");
    r->editor->workflow.recording_busy=modal_recording;r->editor->workflow.recording_context=f;
    f->reenter=1;r->context->reenter=1;queries=f->queries;
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_OK&&!f->modal->model.self);
    assert(f->queries>queries&&r->context->reentries==1);
    f->reenter=0;r->context->reenter=0;r->editor->workflow.recording_busy=NULL;r->editor->workflow.recording_context=NULL;
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&!strcmp(r->document->project.midi_input,"keys"));modal_finish(f);
}
static void modal_stale_draft_and_cancel(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;
    struct pt_event_update note={0,{428,0,PT_NOTE_PERIOD,1,0,0,0,0}};
    modal_open(f,0);modal_text(f,PT_MIDI_SETUP_INPUT,"keys");
    assert(pt_pattern_apply(&r->document->project,&r->editor->history,&note,1)==PT_EDIT_OK);
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_STALE);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&!r->document->project.midi_input[0]&&r->editor->history.count==1);
    modal_finish(f);
}
static void modal_positive_dispose_rebind_new_address(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;struct pt_editor *old=r->editor,*next;
    struct pt_allocator a={r->context,allocate,release};unsigned frees;
    modal_apply(f,PT_MIDI_SETUP_INPUT,0,"keys");
    assert(pt_pattern_undo(&r->document->project,&r->editor->history,-1)==PT_EDIT_OK);
    assert(status(r).commands==1&&pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_IN_USE);
    frees=r->context->frees;dispose_editor(r);assert(r->context->frees==frees+1);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_OK&&!status(r).commands);
    next=calloc(1,sizeof(*next));assert(next&&pt_editor_init(next,&r->document->project));
    pt_sampler_init(&next->sampler,&a,LIMIT);pt_document_init(&next->sample_source,&a);pt_song_init(&next->song,&a,LIMIT);
    assert(pt_editor_change_barrier(next,barrier,r->context));
    assert(pt_midi_setup_owner_rebind(r->owner,next)==PT_MIDI_SETUP_OK);r->editor=next;r->context->editor=next;free(old);
    modal_open(f,3);assert(pt_midi_requester_model_select(&f->modal->model,PT_MIDI_SETUP_CLOCK)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_model_key(&f->modal->model,PT_MIDI_KEY_ACTIVATE,0)==PT_MIDI_EVENT_CHANGED);
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_OK);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_OK&&r->document->project.midi_flags==1);modal_finish(f);
}
static void modal_genuine_held_is_retained(void)
{
    struct modal_fixture *f=modal_create();struct rig *r=f->rig;struct pt_midi_requester_modal saved;unsigned frees;
    modal_open(f,0);modal_text(f,PT_MIDI_SETUP_INPUT,"keys");r->context->alias=1;frees=r->context->frees;
    assert(pt_midi_requester_model_apply(&f->modal->model)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_HELD&&f->modal->self&&f->modal->held&&f->modal->return_attempted);
    saved=*f->modal;
    assert(pt_midi_requester_modal_return(f->modal)==PT_MIDI_SETUP_HELD&&!memcmp(&saved,f->modal,sizeof(saved)));
    assert(pt_midi_requester_modal_begin(f->modal,r->owner,r->owner_capacity,r->editor,0)==PT_MIDI_SETUP_HELD);
    assert(pt_midi_setup_owner_close_controller(r->owner)==PT_MIDI_SETUP_HELD&&
        pt_midi_setup_owner_release_workspace(r->owner)==PT_MIDI_SETUP_HELD&&r->context->frees==frees);
    assert(!r->document->project.midi_input[0]&&!r->editor->history.count&&!r->context->workspace_frees);modal_retained=f;
}
int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"owner-legacy"))return original_owner_main();
    assert(argc==1);
    modal_admission_and_failed_open();modal_chronology_persistence_and_redo();modal_active_noop_and_capacity();
    modal_actual_callback_reentry();modal_stale_draft_and_cancel();modal_positive_dispose_rebind_new_address();modal_genuine_held_is_retained();
    assert(modal_completed==6&&modal_retained&&modal_retained->modal&&modal_retained->rig->context);
    puts("MIDI REQUESTER OWNER MODAL HOST PASS:7 groups;6 complete/1 retained;real borrow/model/return;journal/redo/save/rebind;NO NATIVE/CAMD AUTHORITY");return 0;
}
