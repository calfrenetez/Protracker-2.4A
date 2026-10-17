#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/midi_setup.h"
#include "mod_project.h"

struct allocation {void *pointer;size_t bytes;};
struct fixture {
    struct allocation allocation[128];unsigned live,calls,frees,fail,reenter;
    unsigned hooks,allow,hook_edit,release_reentry,alias;
    unsigned recording_calls,recording_active,recording_edit;
    struct pt_document document;struct pt_editor *editor;struct pt_midi_setup *setup;
};
static void *allocate(void *context,size_t bytes)
{
    struct fixture *f=context;void *p;unsigned i;
    ++f->calls;
    if(f->alias)return f->editor;
    if(f->reenter){
        assert(pt_midi_setup_cancel(f->setup)==PT_MIDI_SETUP_BUSY);
        assert(pt_midi_setup_commit(f->setup)==PT_MIDI_SETUP_BUSY);
    }
    if(f->fail)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<128&&f->allocation[i].pointer;++i){}
    assert(i<128);f->allocation[i]=(struct allocation){p,bytes};++f->live;return p;
}
static void release(void *context,void *pointer)
{
    struct fixture *f=context;unsigned i;
    if(f->release_reentry)assert(pt_midi_setup_cancel(f->setup)==PT_MIDI_SETUP_BUSY);
    for(i=0;i<128&&f->allocation[i].pointer!=pointer;++i){}
    assert(i<128&&f->live);f->allocation[i].pointer=NULL;--f->live;++f->frees;free(pointer);
}
static int guard(void *context)
{
    struct fixture *f=context;++f->hooks;
    if(f->reenter)assert(pt_midi_setup_cancel(f->setup)==PT_MIDI_SETUP_BUSY);
    if(f->hook_edit){
        assert(pt_pattern_title_apply(&f->document.project,&f->editor->history,"OUTSIDE")==PT_EDIT_OK);
        f->hook_edit=0;
    }
    return (int)f->allow;
}
static int recording(void *context)
{
    struct fixture *f=context;++f->recording_calls;
    assert(pt_midi_setup_cancel(f->setup)==PT_MIDI_SETUP_BUSY);
    if(f->recording_edit){
        assert(pt_pattern_title_apply(&f->document.project,&f->editor->history,"CAPTURE CHANGED")==PT_EDIT_OK);
        f->recording_edit=0;
    }
    return (int)f->recording_active;
}
static void start(struct fixture *f,unsigned channels,unsigned commands)
{
    struct pt_allocator a;
    memset(f,0,sizeof(*f));f->allow=1;a=(struct pt_allocator){f,allocate,release};
    pt_document_init(&f->document,&a);
    assert(pt_document_new(&f->document,channels,SIZE_MAX)==PT_PROJECT_OK);
    f->editor=calloc(1,sizeof(*f->editor));f->setup=calloc(1,pt_midi_setup_size());
    assert(f->editor&&f->setup&&pt_editor_init(f->editor,&f->document.project));
    /* A genuine empty sampler may be initialized with the document allocator
       before either the setup owner or journal has retained any allocation. */
    pt_sampler_init(&f->editor->sampler,&a,1024*1024);
    assert(pt_midi_setup_init(f->setup,pt_midi_setup_size(),f->editor,
           commands*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
    assert(pt_editor_change_barrier(f->editor,guard,f));
}
static void finish(struct fixture *f)
{
    f->fail=f->reenter=f->hook_edit=0;f->allow=1;
    assert(pt_editor_dispose(f->editor));
    free(f->editor);f->editor=NULL;
    assert(pt_midi_setup_close(f->setup)==PT_MIDI_SETUP_OK);
    /* close reads only original controller ownership after journal release. */
    free(f->setup);f->setup=NULL;
    f->release_reentry=0;pt_document_release(&f->document);assert(!f->live);
}
static void draft(struct fixture *f,enum pt_midi_setup_target kind,unsigned track,const char *name)
{
    assert(pt_midi_setup_begin(f->setup,kind,track)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(f->setup,name,strlen(name)+1)==PT_MIDI_SETUP_OK);
}
static void put(struct fixture *f,enum pt_midi_setup_target kind,unsigned track,const char *name)
{draft(f,kind,track,name);assert(pt_midi_setup_commit(f->setup)==PT_MIDI_SETUP_OK);}
static struct pt_midi_setup_status status(struct fixture *f)
{
    struct pt_midi_setup_status s;
    assert(pt_midi_setup_get(f->setup,&s)==PT_MIDI_SETUP_OK);return s;
}
static void no_op_invalid_and_cancel_are_silent(void)
{
    struct fixture f;char saved[PT_MIDI_ENDPOINT],bad[64];unsigned calls;
    start(&f,4,4);memset(f.document.project.midi_input,0x5a,64);
    memcpy(f.document.project.midi_input,"keyboard",9);memcpy(saved,f.document.project.midi_input,64);
    calls=f.calls;f.editor->playback.active=1;f.editor->workflow.busy=1;f.allow=0;
    f.editor->workflow.recording_busy=recording;f.editor->workflow.recording_context=&f;f.recording_active=1;
    draft(&f,PT_MIDI_SETUP_INPUT,0,"keyboard");
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_OK);
    assert(!memcmp(saved,f.document.project.midi_input,64));
    assert(!f.hooks&&!f.recording_calls&&f.calls==calls&&!f.editor->history.count&&!pt_editor_dirty(f.editor));
    assert(pt_midi_setup_begin(f.setup,PT_MIDI_SETUP_OUTPUT,4)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_begin(f.setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    memset(bad,'x',64);assert(pt_midi_setup_name(f.setup,bad,64)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_name(f.setup,"a\0b",4)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_name(f.setup,"",0)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_flags(f.setup,1)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    assert(!memcmp(saved,f.document.project.midi_input,64));
    assert(!f.hooks&&f.calls==calls&&!f.editor->history.count);
    f.editor->playback.active=f.editor->workflow.busy=f.recording_active=0;finish(&f);
}
static void changes_require_stopped_and_positive_barrier(void)
{
    struct fixture f;unsigned calls;
    start(&f,4,4);calls=f.calls;draft(&f,PT_MIDI_SETUP_INPUT,0,"keyboard");
    f.editor->playback.active=1;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_ACTIVE);
    assert(f.editor->playback.active&&!f.hooks&&f.calls==calls&&!f.editor->history.count);
    f.editor->playback.active=0;
    f.editor->workflow.busy=1;assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_ACTIVE);f.editor->workflow.busy=0;
    f.editor->workflow.scanning=1;assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_ACTIVE);f.editor->workflow.scanning=0;
    f.editor->workflow.pending_apply=1;assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_ACTIVE);f.editor->workflow.pending_apply=0;
    f.editor->workflow.resolving=1;assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_ACTIVE);f.editor->workflow.resolving=0;
    f.editor->workflow.recording_busy=recording;f.editor->workflow.recording_context=&f;f.recording_active=1;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_ACTIVE);
    assert(f.recording_calls==1&&!f.hooks&&f.calls==calls&&!f.editor->history.count);
    f.recording_active=0;f.allow=0;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_VETO);
    assert(f.hooks==1&&f.calls==calls&&!f.document.project.midi_input[0]&&!f.editor->history.count);
    f.allow=f.reenter=1;assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_OK);
    assert(f.hooks==2&&f.calls==calls+1&&!strcmp(f.document.project.midi_input,"keyboard"));
    f.reenter=0;finish(&f);
}
static void genuine_chronological_journal_and_saved_revision(void)
{
    struct fixture f;struct pt_event_update note={0,{428,0,PT_NOTE_PERIOD,1,0,0,0,0}};
    struct pt_channel ch;uint32_t saved;
    start(&f,16,8);put(&f,PT_MIDI_SETUP_INPUT,0,"keys");
    assert(pt_pattern_apply(&f.document.project,&f.editor->history,&note,1)==PT_EDIT_OK);
    put(&f,PT_MIDI_SETUP_OUTPUT,15,"synth");ch=f.document.project.channels.track[3];ch.muted=1;
    assert(pt_pattern_channel_apply(&f.document.project,&f.editor->history,3,&ch)==PT_EDIT_OK);
    assert(pt_midi_setup_begin(f.setup,PT_MIDI_SETUP_CLOCK,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_flags(f.setup,3)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_OK);
    assert(f.editor->history.count==5&&status(&f).commands==3);
    pt_editor_saved(f.editor);saved=f.editor->history.revision;assert(!pt_editor_dirty(f.editor));
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!f.document.project.midi_flags&&pt_editor_dirty(f.editor));
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!f.document.project.channels.track[3].muted);
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!f.document.project.midi_output[15][0]&&f.document.project.events[0].pitch==428);
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!f.document.project.events[0].kind&&!strcmp(f.document.project.midi_input,"keys"));
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(!f.document.project.midi_input[0]&&status(&f).commands==3);
    for(unsigned i=0;i<5;++i)assert(pt_pattern_undo(&f.document.project,&f.editor->history,1)==PT_EDIT_OK);
    assert(f.editor->history.revision==saved&&!pt_editor_dirty(f.editor));
    assert(!strcmp(f.document.project.midi_output[15],"synth")&&f.document.project.midi_flags==3);
    finish(&f);
}
static void genuine_bounded_redo_ownership(void)
{
    struct fixture f;unsigned hooks,calls,frees;
    start(&f,4,1);put(&f,PT_MIDI_SETUP_INPUT,0,"one");
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    assert(status(&f).commands==1&&status(&f).owned_bytes==pt_midi_setup_command_bytes());
    hooks=f.hooks;calls=f.calls;draft(&f,PT_MIDI_SETUP_INPUT,0,"two");
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_CAPACITY);
    assert(f.hooks==hooks&&f.calls==calls&&f.editor->history.cursor==0&&f.editor->history.count==1);
    assert(!f.document.project.midi_input[0]);assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    /* A genuine non-resource edit actually discards the old redo owner. */
    frees=f.frees;f.release_reentry=1;
    assert(pt_pattern_title_apply(&f.document.project,&f.editor->history,"NEW")==PT_EDIT_OK);
    f.release_reentry=0;assert(f.frees==frees+1&&!status(&f).commands);
    put(&f,PT_MIDI_SETUP_INPUT,0,"two");assert(status(&f).commands==1);finish(&f);
    /* With two record bytes available, new resource apply precedes discard of
       the undone record; both real allocations briefly count against budget. */
    start(&f,4,2);put(&f,PT_MIDI_SETUP_INPUT,0,"one");
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    frees=f.frees;f.release_reentry=1;put(&f,PT_MIDI_SETUP_INPUT,0,"two");
    f.release_reentry=0;assert(f.frees==frees+1&&status(&f).commands==1);
    assert(f.editor->history.count==1&&!strcmp(f.document.project.midi_input,"two"));finish(&f);
}
static void fresh_allocator_failure_preserves_document(void)
{
    struct fixture f;struct pt_project before;unsigned live;
    start(&f,4,2);draft(&f,PT_MIDI_SETUP_OUTPUT,1,"missing destination");
    before=f.document.project;live=f.live;f.fail=1;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_CAPACITY);
    assert(!memcmp(&before,&f.document.project,sizeof(before))&&f.live==live);
    assert(!f.editor->history.count&&!status(&f).owned_bytes&&status(&f).draft);
    f.fail=0;assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_OK);finish(&f);
}
/* An intentionally broken allocator returning an already-live editor is not a
   new allocation. Keep the entire original owner/context reachable until this
   HOST process exits: this fixture does not invent a HELD recovery API. */
static struct fixture held_fixture;
static void held_observation_preserves_output_and_original_owner(void)
{
    struct fixture *f=&held_fixture;struct pt_midi_setup_status out,before;
    struct pt_editor *editor_image;unsigned frees;
    start(f,4,2);draft(f,PT_MIDI_SETUP_INPUT,0,"keys");f->alias=1;frees=f->frees;
    editor_image=malloc(sizeof(*editor_image));assert(editor_image);*editor_image=*f->editor;
    assert(pt_midi_setup_commit(f->setup)==PT_MIDI_SETUP_HELD);
    assert(!memcmp(editor_image,f->editor,sizeof(*editor_image))&&f->frees==frees);
    memset(&out,0xa5,sizeof(out));before=out;
    assert(pt_midi_setup_get(f->setup,&out)==PT_MIDI_SETUP_HELD&&!memcmp(&out,&before,sizeof(out)));
    assert(pt_midi_setup_close(f->setup)==PT_MIDI_SETUP_HELD);
    free(editor_image);
}
static void stale_and_post_hook_drafts_refuse(void)
{
    struct fixture f;unsigned hooks,calls;struct pt_project replacement;
    start(&f,4,3);draft(&f,PT_MIDI_SETUP_INPUT,0,"keys");calls=f.calls;
    assert(pt_pattern_title_apply(&f.document.project,&f.editor->history,"OTHER")==PT_EDIT_OK);
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_STALE);assert(!f.hooks&&f.calls==calls);
    assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    draft(&f,PT_MIDI_SETUP_INPUT,0,"keys");++f.editor->sampler.generation;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_STALE&&!f.hooks);
    assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    draft(&f,PT_MIDI_SETUP_INPUT,0,"keys");replacement=f.document.project;f.editor->project=&replacement;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_STALE&&!f.hooks);
    f.editor->project=&f.document.project;assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    draft(&f,PT_MIDI_SETUP_INPUT,0,"keys");hooks=f.hooks;f.hook_edit=1;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_STALE);
    assert(f.hooks==hooks+1&&f.calls==calls&&!f.document.project.midi_input[0]);
    assert(!strcmp(f.document.project.title,"OUTSIDE")&&f.editor->history.count==2);
    assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);
    draft(&f,PT_MIDI_SETUP_INPUT,0,"keys");f.editor->workflow.recording_busy=recording;
    f.editor->workflow.recording_context=&f;f.recording_edit=1;hooks=f.hooks;
    assert(pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_STALE);
    assert(f.hooks==hooks&&f.calls==calls&&!f.document.project.midi_input[0]);
    assert(pt_midi_setup_cancel(f.setup)==PT_MIDI_SETUP_OK);finish(&f);
}
static void external_metadata_conflict_preserves_cursor(void)
{
    struct fixture f;char owned[64];uint32_t revision;size_t cursor;
    start(&f,4,3);put(&f,PT_MIDI_SETUP_INPUT,0,"keys");memcpy(owned,f.document.project.midi_input,64);
    revision=f.editor->history.revision;cursor=f.editor->history.cursor;
    f.document.project.midi_input[63]=1; /* same visible name, different outside-owned full image */
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_CONFLICT);
    assert(f.editor->history.revision==revision&&f.editor->history.cursor==cursor);
    assert(f.document.project.midi_input[63]==1&&status(&f).commands==1);
    memcpy(f.document.project.midi_input,owned,64);
    f.editor->workflow.recording_busy=recording;f.editor->workflow.recording_context=&f;f.recording_active=1;
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_CONFLICT);
    assert(f.editor->history.revision==revision&&f.editor->history.cursor==cursor);
    assert(!memcmp(f.document.project.midi_input,owned,64));f.recording_active=0;
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    revision=f.editor->history.revision;cursor=f.editor->history.cursor;f.recording_active=1;
    assert(pt_pattern_undo(&f.document.project,&f.editor->history,1)==PT_EDIT_CONFLICT);
    assert(f.editor->history.revision==revision&&f.editor->history.cursor==cursor&&!f.document.project.midi_input[0]);
    f.recording_active=0;assert(pt_pattern_undo(&f.document.project,&f.editor->history,1)==PT_EDIT_OK);
    finish(&f);
}
static void directional_discovery_never_rebinds(void)
{
    struct fixture f;struct pt_midi_setup_discovery d={0};struct pt_midi_setup_view v;
    struct pt_channels channels;unsigned hooks,calls;
    start(&f,16,6);assert(pt_channels_route(&f.document.project.channels,2,PT_MIDI)==PT_CHANNEL_OK);
    assert(pt_midi_setup_view(f.setup,15,&d,&v)==PT_MIDI_SETUP_OK);
    assert(v.input_presence==PT_MIDI_SETUP_UNCONFIGURED&&v.output_presence==PT_MIDI_SETUP_UNCONFIGURED);
    channels=f.document.project.channels;put(&f,PT_MIDI_SETUP_INPUT,0,"shared");
    put(&f,PT_MIDI_SETUP_OUTPUT,15,"shared");hooks=f.hooks;calls=f.calls;
    d.output_known=1;d.output_count=2;strcpy(d.output[0].identity,"other");strcpy(d.output[1].identity,"shared");
    assert(pt_midi_setup_view(f.setup,15,&d,&v)==PT_MIDI_SETUP_OK);
    assert(v.input_presence==PT_MIDI_SETUP_UNKNOWN&&v.output_presence==PT_MIDI_SETUP_PRESENT);
    assert(!strcmp(v.input,"shared")&&!strcmp(v.output,"shared")&&v.route==PT_AMIGUS);
    strcpy(d.output[0].identity,"shared");strcpy(d.output[1].identity,"other");
    assert(pt_midi_setup_view(f.setup,15,&d,&v)==PT_MIDI_SETUP_OK&&v.output_presence==PT_MIDI_SETUP_PRESENT);
    d.output_count=1;strcpy(d.output[0].identity,"Shared");d.input_known=1;d.input_count=1;
    strcpy(d.input[0].identity,"shared");
    assert(pt_midi_setup_view(f.setup,15,&d,&v)==PT_MIDI_SETUP_OK);
    assert(v.input_presence==PT_MIDI_SETUP_PRESENT&&v.output_presence==PT_MIDI_SETUP_MISSING);
    assert(!strcmp(f.document.project.midi_output[15],"shared")&&f.hooks==hooks&&f.calls==calls);
    assert(!memcmp(&channels,&f.document.project.channels,sizeof(channels)));
    d.input_known=0;assert(pt_midi_setup_view(f.setup,15,&d,&v)==PT_MIDI_SETUP_INVALID);
    finish(&f);
}
static void routes_inactive_slots_and_precise_names_are_preserved(void)
{
    struct fixture f;struct pt_channels channels;char inactive[12][64],long_name[64];
    unsigned i;int32_t pcm[3][4]={{-128,127,1,0},{-32768,32767,257,0},{-8388608,8388607,257,0}};
    int32_t saved[3][4];struct pt_sample samples[3];
    start(&f,4,6);assert(pt_channels_route(&f.document.project.channels,1,PT_AMIGUS)==PT_CHANNEL_OK);
    assert(pt_channels_route(&f.document.project.channels,2,PT_MIDI)==PT_CHANNEL_OK);
    channels=f.document.project.channels;memset(f.document.project.midi_output+4,0x7e,sizeof(inactive));
    memcpy(inactive,f.document.project.midi_output+4,sizeof(inactive));
    memcpy(samples,f.document.project.samples,sizeof(samples));memcpy(saved,pcm,sizeof(pcm));
    for(i=0;i<3;++i)f.document.project.samples[i].pcm=(struct pt_pcm){pcm[i],4,3,8287,1,(uint8_t)(8+8*i)};
    for(i=0;i<3;++i)put(&f,PT_MIDI_SETUP_OUTPUT,i,"configured but inactive");
    memset(long_name,'x',63);long_name[31]=(char)0xe9;long_name[63]=0;
    put(&f,PT_MIDI_SETUP_INPUT,0,long_name);assert(!memcmp(long_name,f.document.project.midi_input,64));
    for(i=0;i<4;++i)assert(pt_pattern_undo(&f.document.project,&f.editor->history,-1)==PT_EDIT_OK);
    for(i=0;i<4;++i)assert(pt_pattern_undo(&f.document.project,&f.editor->history,1)==PT_EDIT_OK);
    assert(!memcmp(&channels,&f.document.project.channels,sizeof(channels)));
    assert(!memcmp(inactive,f.document.project.midi_output+4,sizeof(inactive))&&!memcmp(saved,pcm,sizeof(pcm)));
    for(i=0;i<3;++i)assert(f.document.project.samples[i].pcm.data==pcm[i]&&f.document.project.samples[i].pcm.bits==8+8*i);
    memcpy(f.document.project.samples,samples,sizeof(samples));finish(&f);
}
static void genuine_project_roundtrip_and_ordered_disposal(void)
{
    struct fixture f;struct pt_document reopen;struct pt_allocator a;uint8_t *bytes;
    size_t length,written;struct pt_mod_export_report report;struct pt_channels channels;unsigned i;
    start(&f,16,24);channels=f.document.project.channels;put(&f,PT_MIDI_SETUP_INPUT,0,"unavailable keyboard");
    for(i=0;i<16;++i)put(&f,PT_MIDI_SETUP_OUTPUT,i,i==15?"last synth":"synth");
    assert(pt_midi_setup_begin(f.setup,PT_MIDI_SETUP_CLOCK,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_flags(f.setup,4)==PT_MIDI_SETUP_INVALID);
    assert(pt_midi_setup_flags(f.setup,3)==PT_MIDI_SETUP_OK&&pt_midi_setup_commit(f.setup)==PT_MIDI_SETUP_OK);
    assert(pt_project_size(&f.document.project,&length)==PT_PROJECT_OK);bytes=malloc(length);assert(bytes);
    assert(pt_project_encode(&f.document.project,bytes,length,&written)==PT_PROJECT_OK&&written==length);
    a=(struct pt_allocator){&f,allocate,release};pt_document_init(&reopen,&a);
    assert(pt_document_load(&reopen,bytes,length,SIZE_MAX)==PT_PROJECT_OK);
    assert(!memcmp(reopen.project.midi_input,f.document.project.midi_input,64));
    assert(!memcmp(reopen.project.midi_output,f.document.project.midi_output,sizeof(reopen.project.midi_output)));
    assert(reopen.project.midi_flags==3&&!memcmp(&channels,&reopen.project.channels,sizeof(channels)));
    assert(pt_mod_export_analyse(&f.document.project,&report)==PT_PROJECT_OK&&(report.issues&PT_EXPORT_METADATA));
    pt_editor_saved(f.editor);assert(!pt_editor_dirty(f.editor));
    assert(pt_midi_setup_close(f.setup)==PT_MIDI_SETUP_IN_USE);
    pt_document_release(&reopen);free(bytes);f.release_reentry=1;
    finish(&f);
}
int main(void)
{
    no_op_invalid_and_cancel_are_silent();changes_require_stopped_and_positive_barrier();
    genuine_chronological_journal_and_saved_revision();genuine_bounded_redo_ownership();
    fresh_allocator_failure_preserves_document();stale_and_post_hook_drafts_refuse();
    external_metadata_conflict_preserves_cursor();directional_discovery_never_rebinds();
    routes_inactive_slots_and_precise_names_are_preserved();genuine_project_roundtrip_and_ordered_disposal();
    held_observation_preserves_output_and_original_owner();
    puts("MIDI SETUP HOST PASS:11 groups;genuine editor/journal/project;bounded retained records;silent directional configuration;NO CAMD/NATIVE/UI/OUTPUT AUTHORITY");
    return 0;
}
