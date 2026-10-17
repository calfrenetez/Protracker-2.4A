#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor_midi_recording.h"
#include "../src/editor/midi_setup.h"
#include "mod_project.h"

#define ROW(n) ((uint64_t)(n)<<16)
#define BUDGET (1024U*1024U)
struct allocation {void *pointer;size_t bytes;};
struct fixture {
    struct allocation blocks[128];unsigned live,calls,frees,reentry,probes;
    size_t used;
    struct pt_document document;
    struct pt_editor *editor;
    struct pt_editor_midi_recording *take;
    struct pt_midi_setup *setup;
    size_t take_capacity;
    struct pt_sample_version *pin;
    struct pt_pcm master;
    struct pt_sample sample;
    int32_t master_values[4];
    size_t master_bytes;
    unsigned generation;
};
static struct fixture *retained[2];
static void *allocate(void *context,size_t bytes)
{
    struct fixture *f=context;void *p;unsigned i;
    ++f->calls;
    if(bytes>BUDGET-f->used)return NULL;
    p=malloc(bytes);assert(p);
    for(i=0;i<128&&f->blocks[i].pointer;++i){}
    assert(i<128);f->blocks[i]=(struct allocation){p,bytes};
    f->used+=bytes;++f->live;return p;
}
static void release(void *context,void *p)
{
    struct fixture *f=context;unsigned i;
    if(f->reentry){
        struct pt_editor_midi_record_status s,before;
        const uint8_t message[3]={0x9f,60,100};
        void *owner_copy=malloc(f->take_capacity);assert(owner_copy);
        memcpy(owner_copy,f->take,f->take_capacity);
        memset(&s,0x65,sizeof(s));memcpy(&before,&s,sizeof(s));
        assert(pt_editor_midi_recording_message(f->take,0,message,3)==PT_EDITOR_RECORD_BUSY);
        assert(pt_editor_midi_recording_stop(f->take,0)==PT_EDITOR_RECORD_BUSY);
        assert(pt_editor_midi_recording_cancel(f->take)==PT_EDITOR_RECORD_BUSY);
        assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_BUSY);
        assert(pt_editor_midi_recording_get(f->take,&s)==PT_EDITOR_RECORD_BUSY);
        assert(pt_editor_midi_recording_get(f->take,(void *)f->take)==PT_EDITOR_RECORD_ALIAS);
        assert(!memcmp(&s,&before,sizeof(s))&&!memcmp(owner_copy,f->take,f->take_capacity));
        free(owner_copy);++f->probes;
    }
    for(i=0;i<128&&f->blocks[i].pointer!=p;++i){}
    assert(i<128&&f->live&&f->used>=f->blocks[i].bytes);
    f->used-=f->blocks[i].bytes;f->blocks[i].pointer=NULL;--f->live;++f->frees;free(p);
}
static struct fixture *make(void)
{
    static const uint8_t raw[12]={0x7f,0xff,0xff,0,0,1,0xff,0xff,0xff,0x80,0,0};
    const struct pt_raw_format format={48000,24,1,0,0};
    struct fixture *f=calloc(1,sizeof(*f));struct pt_allocator a;
    struct pt_channel channel;struct pt_event_update u;
    assert(f);a=(struct pt_allocator){f,allocate,release};
    pt_document_init(&f->document,&a);
    assert(pt_document_new(&f->document,16,BUDGET)==PT_PROJECT_OK);
    f->editor=calloc(1,sizeof(*f->editor));assert(f->editor);
    assert(pt_editor_init(f->editor,&f->document.project));
    /* Rebind only still-empty original owners, before any retained resource. */
    pt_sampler_init(&f->editor->sampler,&a,BUDGET);
    pt_song_init(&f->editor->song,&a,BUDGET);
    pt_document_init(&f->editor->sample_source,&a);
    assert(pt_sampler_import_raw(&f->editor->sampler,&f->document.project,
        &f->editor->history,0,raw,sizeof(raw),"Original24",&format)==PT_EDIT_OK);
    assert(pt_song_append(&f->editor->song,&f->document.project,&f->editor->history,0,1)==PT_EDIT_OK);
    channel=f->document.project.channels.track[15];channel.route=PT_MIDI;
    channel.midi_channel=3;strcpy(channel.name,"Input target");
    assert(pt_pattern_channel_apply(&f->document.project,&f->editor->history,15,&channel)==PT_EDIT_OK);
    f->document.project.channels.selected=15;
    u.index=63*16+15;u.event=f->document.project.events[u.index];
    u.event.instrument=1;u.event.effect=14;u.event.parameter=0xc3;
    assert(pt_pattern_apply(&f->document.project,&f->editor->history,&u,1)==PT_EDIT_OK);
    assert(pt_sampler_pin(&f->editor->sampler,&f->document.project,0,
        f->editor->sampler.generation,&f->master,&f->pin)==PT_EDIT_OK);
    assert(f->pin&&f->master.frames==4&&f->master.bits==24&&f->master.channels==1);
    assert(f->master.data[0]==8388607&&f->master.data[1]==1&&
           f->master.data[2]==-1&&f->master.data[3]==-8388608);
    memcpy(f->master_values,f->master.data,sizeof(f->master_values));
    f->sample=f->document.project.samples[0];f->generation=f->editor->sampler.generation;
    f->master_bytes=f->editor->sampler.bytes;
    f->take_capacity=pt_editor_midi_recording_size()+32;
    f->take=calloc(1,f->take_capacity);f->setup=calloc(1,pt_midi_setup_size());
    assert(f->take&&f->setup);
    memset((uint8_t *)f->take+pt_editor_midi_recording_size(),0xa5,32);
    assert(pt_midi_setup_init(f->setup,pt_midi_setup_size(),f->editor,
        8*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
    assert(pt_project_validate(&f->document.project,NULL)==PT_PROJECT_OK);
    pt_editor_saved(f->editor);return f;
}
static void master_unchanged(const struct fixture *f)
{
    const struct pt_sample *s=&f->document.project.samples[0];
    assert(f->editor->sampler.current[0]==f->pin);
    assert(f->editor->sampler.generation==f->generation&&f->editor->sampler.bytes==f->master_bytes);
    assert(s->pcm.data==f->master.data&&s->pcm.capacity==f->master.capacity&&
        s->pcm.frames==f->master.frames&&s->pcm.rate==48000&&s->pcm.channels==1&&s->pcm.bits==24);
    assert(!memcmp(s,&f->sample,sizeof(*s)));
    assert(!memcmp(s->pcm.data,f->master_values,sizeof(f->master_values)));
}
static uint8_t *saved(struct fixture *f,size_t *size)
{
    uint8_t *b;size_t written;
    assert(pt_project_size(&f->document.project,size)==PT_PROJECT_OK);b=malloc(*size);assert(b);
    assert(pt_project_encode(&f->document.project,b,*size,&written)==PT_PROJECT_OK&&written==*size);return b;
}
static void same_save(struct fixture *f,const uint8_t *before,size_t bytes)
{size_t n;uint8_t *b=saved(f,&n);assert(n==bytes&&!memcmp(b,before,n));free(b);master_unchanged(f);}
static enum pt_editor_midi_record_result begin(struct fixture *f,uint32_t first,uint32_t limit,unsigned hold)
{
    const struct pt_record_settings settings={first,limit,1,(uint8_t)hold,1,0};
    return pt_editor_midi_recording_begin(f->take,f->take_capacity,f->editor,sizeof(*f),&settings,0,15,16);
}
static enum pt_editor_midi_record_result send(struct fixture *f,uint64_t position,unsigned status,unsigned note,unsigned velocity)
{
    const uint8_t b[3]={(uint8_t)status,(uint8_t)note,(uint8_t)velocity};
    return pt_editor_midi_recording_message(f->take,position,b,sizeof(b));
}
static struct pt_editor_midi_record_status state(struct fixture *f)
{struct pt_editor_midi_record_status s;assert(pt_editor_midi_recording_get(f->take,&s)==PT_EDITOR_RECORD_OK);return s;}
static void finish(struct fixture *f)
{
    struct pt_editor_midi_recording *take=f->take;
    struct pt_editor_midi_record_status s=state(f);assert(!s.attached&&!s.armed&&!s.active);
    master_unchanged(f);pt_sampler_unpin(f->pin);f->pin=NULL;
    assert(pt_editor_dispose(f->editor));free(f->editor);f->editor=NULL;
    assert(pt_midi_setup_close(f->setup)==PT_MIDI_SETUP_OK);free(f->setup);f->setup=NULL;
    pt_document_release(&f->document);assert(!f->live&&!f->used);
    free(f);
    /* The original source/editor AND allocator context have ended. This only
       reads the detached owner, whose separate original workspace stays live. */
    assert(pt_editor_midi_recording_get(take,&s)==PT_EDITOR_RECORD_OK&&!s.attached);
    free(take);
}
static int foreign(void *context){return context!=NULL;}
static void admission_refusals(void)
{
    struct fixture *f=make();const struct pt_record_settings settings={0,128,1,1,1,0};
    unsigned char *before=malloc(f->take_capacity);size_t bytes;uint8_t *gold=saved(f,&bytes);
    assert(before);memcpy(before,f->take,f->take_capacity);
    assert(pt_editor_midi_recording_begin(f->take,f->take_capacity,f->editor,sizeof(*f),&settings,0,15,0)==PT_EDITOR_RECORD_INVALID);
    assert(pt_editor_midi_recording_begin(f->take,f->take_capacity,f->editor,sizeof(*f),&settings,0,0,16)==PT_EDITOR_RECORD_INVALID);
    assert(!memcmp(before,f->take,f->take_capacity));
    f->editor->workflow.recording_context=f;
    assert(begin(f,0,128,1)==PT_EDITOR_RECORD_CONFLICT);f->editor->workflow.recording_context=NULL;
    f->editor->before_change_context=f;
    assert(begin(f,0,128,1)==PT_EDITOR_RECORD_CONFLICT);f->editor->before_change_context=NULL;
    assert(pt_editor_change_barrier(f->editor,foreign,f));assert(begin(f,0,128,1)==PT_EDITOR_RECORD_CONFLICT);
    assert(f->editor->change_ready==foreign&&f->editor->before_change_context==f);
    assert(pt_editor_change_barrier(f->editor,NULL,NULL));
    f->editor->workflow.recording_busy=foreign;f->editor->workflow.recording_context=f;
    assert(begin(f,0,128,1)==PT_EDITOR_RECORD_CONFLICT);
    f->editor->workflow.recording_busy=NULL;f->editor->workflow.recording_context=NULL;
    f->editor->playback.active=1;assert(begin(f,0,128,1)==PT_EDITOR_RECORD_CONFLICT);f->editor->playback.active=0;
    assert(pt_editor_midi_recording_begin((struct pt_editor_midi_recording *)f,sizeof(*f),f->editor,sizeof(*f),&settings,0,15,16)==PT_EDITOR_RECORD_ALIAS);
    assert(pt_editor_midi_recording_begin((struct pt_editor_midi_recording *)f->editor,sizeof(*f->editor),f->editor,sizeof(*f),&settings,0,15,16)==PT_EDITOR_RECORD_ALIAS);
    assert(!memcmp(before,f->take,f->take_capacity));same_save(f,gold,bytes);free(gold);free(before);
    assert(begin(f,0,128,1)==PT_EDITOR_RECORD_OK);
    assert(pt_editor_midi_recording_cancel(f->take)==PT_EDITOR_RECORD_OK);
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_OK);finish(f);
}
static void check_alias(struct fixture *f,void *out,size_t bytes)
{
    uint8_t *before=malloc(bytes);assert(before);memcpy(before,out,bytes);
    assert(pt_editor_midi_recording_get(f->take,out)==PT_EDITOR_RECORD_ALIAS);
    assert(!memcmp(before,out,bytes));free(before);
}
static void hold_veto_and_workflow(void)
{
    struct fixture *f=make();size_t bytes;uint8_t *gold=saved(f,&bytes);
    size_t cursor=f->editor->history.cursor;unsigned calls=f->calls;enum pt_editor_action action;
    struct pt_editor_midi_record_status s;const uint8_t invalid[3]={0x9f,60,128};
    void *copy=malloc(f->take_capacity);assert(copy&&begin(f,63,128,1)==PT_EDITOR_RECORD_OK);
    memcpy(copy,f->take,f->take_capacity);
    assert(pt_editor_midi_recording_message(f->take,UINT64_MAX,invalid,3)==PT_EDITOR_RECORD_INVALID);
    assert(send(f,UINT64_MAX,0x90,60,100)==PT_EDITOR_RECORD_IGNORED);
    assert(!memcmp(copy,f->take,f->take_capacity));free(copy);
    assert(f->editor->workflow.recording_busy(f->editor->workflow.recording_context)==1);
    assert(!pt_editor_prepare_change(f->editor)&&!pt_editor_dispose(f->editor));
    f->editor->panel=0;f->editor->editing=1;f->editor->field=0;f->editor->row=0;
    (void)pt_editor_key(f->editor,0x41,0); /* Real Backspace/OFF edit admission. */
    (void)pt_editor_key(f->editor,0x31,8); /* Real editor Undo admission. */
    assert(f->editor->history.cursor==cursor);
    assert(pt_midi_setup_begin(f->setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(f->setup,"keys",5)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_commit(f->setup)==PT_MIDI_SETUP_ACTIVE);
    assert(pt_midi_setup_cancel(f->setup)==PT_MIDI_SETUP_OK);
    pt_editor_workflow_apply(f->editor);assert(!f->editor->workflow.transaction&&!f->editor->workflow.busy);
    assert(pt_editor_workflow_key(f->editor,0x28,11,&action)&&action==PT_UI_NONE);
    assert(f->editor->workflow.scanning);
    assert(pt_editor_workflow_key(f->editor,0x22,0,&action)&&action==PT_UI_NONE);
    assert(f->editor->workflow.pending_apply==1&&!f->editor->workflow.transaction);
    assert(f->calls==calls);same_save(f,gold,bytes);free(gold);
    check_alias(f,f->take,sizeof(s));
    check_alias(f,(uint8_t *)f->take+f->take_capacity-sizeof(s),sizeof(s));
    check_alias(f,f->editor,sizeof(s));check_alias(f,f,sizeof(s));
    check_alias(f,f->editor->history.commands,sizeof(s));check_alias(f,f->editor->history.changes,sizeof(s));
    /* Use an output spanning the live tiny PCM range; sizeof(status) exceeds
       it, so no test reads that beyond-allocation output before alias refusal. */
    assert(pt_editor_midi_recording_get(f->take,(void *)f->master.data)==PT_EDITOR_RECORD_ALIAS);
    master_unchanged(f);
    assert(send(f,ROW(100),0x9f,60,90)==PT_EDITOR_RECORD_OK);
    assert(pt_editor_workflow_idle(f->editor));
    assert(!f->editor->workflow.scanning&&!f->editor->workflow.pending_apply&&!f->editor->workflow.transaction);
    /* A fresh read-only scan must actually complete while the take is armed.
       Stop's real Off command then invalidates this completed preview. */
    assert(pt_editor_workflow_key(f->editor,0x28,11,&action)&&action==PT_UI_NONE);
    {unsigned steps;
     for(steps=0;steps<1024&&f->editor->workflow.scanning;++steps)
         (void)pt_editor_workflow_idle(f->editor);
     assert(steps<1024&&!f->editor->workflow.scanning&&f->editor->workflow.usage_valid);}
    assert(pt_sample_usage_current(&f->editor->workflow.usage,f->editor->project,
        f->editor->history.revision,f->editor->sampler.generation));
    assert(pt_editor_workflow_key(f->editor,0x22,0,&action)&&action==PT_UI_NONE);
    assert(f->editor->workflow.pending_apply==1&&!f->editor->workflow.transaction&&!f->editor->workflow.busy);
    master_unchanged(f);
    cursor=f->editor->history.cursor;
    assert(pt_editor_midi_recording_stop(f->take,ROW(100)+1)==PT_EDITOR_RECORD_OK);
    assert(f->editor->history.cursor==cursor+1&&f->editor->workflow.usage_valid&&
        !pt_sample_usage_current(&f->editor->workflow.usage,f->editor->project,
            f->editor->history.revision,f->editor->sampler.generation));
    s=state(f);assert(s.attached&&!s.armed&&!s.active&&!pt_editor_prepare_change(f->editor));
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_OK);
    assert(pt_editor_prepare_change(f->editor));
    gold=saved(f,&bytes);cursor=f->editor->history.cursor;calls=f->calls;
    pt_editor_workflow_apply(f->editor);
    assert(!strcmp(f->editor->status,"CLEANUP PREVIEW STALE - REFRESH BEFORE APPLY"));
    assert(!f->editor->workflow.pending_apply&&!f->editor->workflow.transaction&&!f->editor->workflow.busy);
    assert(f->editor->history.cursor==cursor&&f->calls==calls);
    same_save(f,gold,bytes);free(gold);finish(f);
}
static void real_record_save_undo(void)
{
    struct fixture *f=make();struct pt_event *on,*off;size_t baseline=f->editor->history.cursor,n;
    uint8_t *b;struct pt_project_requirements need;struct pt_document decoded;
    struct pt_allocator a={f,allocate,release};
    assert(begin(f,63,128,1)==PT_EDITOR_RECORD_OK);
    /* UI cursor changes do not change the take's fixed target or input filter. */
    f->document.project.channels.selected=0;
    assert(send(f,ROW(104),0x8f,60,127)==PT_EDITOR_RECORD_IGNORED);
    assert(!state(f).started&&state(f).last_position==ROW(104));
    assert(send(f,ROW(105),0x9f,60,91)==PT_EDITOR_RECORD_OK);
    assert(send(f,ROW(105)+1,0x9f,60,0)==PT_EDITOR_RECORD_OK);
    on=&f->document.project.events[63*16+15];off=&f->document.project.events[64*16+15];
    assert(on->kind==PT_NOTE_MIDI&&on->pitch==60&&on->velocity==91&&on->instrument==1&&on->effect==14&&on->parameter==0xc3);
    assert(off->kind==PT_NOTE_OFF&&!off->pitch&&f->editor->history.cursor==baseline+2);
    assert(pt_editor_midi_recording_stop(f->take,ROW(106))==PT_EDITOR_RECORD_OK);
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_OK);master_unchanged(f);
    b=saved(f,&n);assert(pt_project_probe(b,n,&need)==PT_PROJECT_OK);
    pt_document_init(&decoded,&a);assert(pt_document_load(&decoded,b,n,BUDGET)==PT_PROJECT_OK);
    assert(decoded.project.samples[0].pcm.bits==24&&!memcmp(decoded.project.samples[0].pcm.data,f->master_values,sizeof(f->master_values)));
    {size_t bytes,written;uint8_t *again;assert(pt_project_size(&decoded.project,&bytes)==PT_PROJECT_OK&&bytes==n);
     again=malloc(n);assert(again&&pt_project_encode(&decoded.project,again,n,&written)==PT_PROJECT_OK&&written==n);
     assert(!memcmp(again,b,n));free(again);}
    pt_document_release(&decoded);free(b);pt_editor_saved(f->editor);
    assert(pt_pattern_undo(&f->document.project,&f->editor->history,-1)==PT_EDIT_OK&&off->kind==PT_NOTE_NONE);
    assert(pt_pattern_undo(&f->document.project,&f->editor->history,-1)==PT_EDIT_OK&&on->kind==PT_NOTE_NONE);
    assert(on->effect==14&&on->parameter==0xc3&&on->instrument==1);
    assert(pt_pattern_undo(&f->document.project,&f->editor->history,1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&f->document.project,&f->editor->history,1)==PT_EDIT_OK&&!pt_editor_dirty(f->editor));
    master_unchanged(f);finish(f);
}
static void failed_stop_explicit_cancel(void)
{
    struct fixture *f=make();struct pt_editor_midi_record_status s;size_t cursor;
    assert(begin(f,127,128,0)==PT_EDITOR_RECORD_OK);
    assert(send(f,0,0x9f,127,100)==PT_EDITOR_RECORD_OK);cursor=f->editor->history.cursor;
    assert(pt_editor_midi_recording_stop(f->take,ROW(1))==PT_EDITOR_RECORD_CAPACITY);
    s=state(f);assert(s.armed&&s.active==1&&s.attached);
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_CONFLICT&&!pt_editor_dispose(f->editor));
    assert(pt_editor_midi_recording_cancel(f->take)==PT_EDITOR_RECORD_OK);
    assert(f->editor->history.cursor==cursor&&f->document.project.events[127*16+15].kind==PT_NOTE_MIDI);
    assert(!pt_editor_prepare_change(f->editor));
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_OK);finish(f);
}
static void genuine_discard_reentry(void)
{
    struct fixture *f=make();unsigned probes;
    /* Actual empty rebind after genuine history release, while no take/draft
       exists, creates a one-command journal. No live owner is copied/reset. */
    pt_pattern_history_release(&f->editor->history);
    /* Original sample-change journal metadata was genuinely retired. Capture
       the actual new accounting baseline; the held current master/pin survived. */
    f->master_bytes=f->editor->sampler.bytes;
    assert(pt_pattern_history_init(&f->editor->history,&f->document.project,f->editor->commands,1,f->editor->changes,2048)==PT_EDIT_OK);
    /* Rebind the empty MIDI settings controller to that genuine new journal. */
    assert(pt_midi_setup_close(f->setup)==PT_MIDI_SETUP_OK);free(f->setup);
    f->setup=calloc(1,pt_midi_setup_size());assert(f->setup);
    assert(pt_midi_setup_init(f->setup,pt_midi_setup_size(),f->editor,2*pt_midi_setup_command_bytes())==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_begin(f->setup,PT_MIDI_SETUP_INPUT,0)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_name(f->setup,"real record",12)==PT_MIDI_SETUP_OK);
    assert(pt_midi_setup_commit(f->setup)==PT_MIDI_SETUP_OK);
    assert(begin(f,0,128,0)==PT_EDITOR_RECORD_OK);f->reentry=1;probes=f->probes;
    assert(send(f,0,0x9f,60,100)==PT_EDITOR_RECORD_OK&&f->probes==probes+1);
    f->reentry=0;
    {struct pt_midi_setup_status s;assert(pt_midi_setup_get(f->setup,&s)==PT_MIDI_SETUP_OK&&!s.commands&&!s.owned_bytes);}
    assert(pt_editor_midi_recording_stop(f->take,ROW(1))==PT_EDITOR_RECORD_OK);
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_OK);finish(f);
}
static void retained_callback_drift(void)
{
    struct fixture *f=make();size_t bytes;uint8_t *before;
    assert(begin(f,0,128,1)==PT_EDITOR_RECORD_OK);
    /* Deliberate forbidden outside mutation is defensive coverage, not a
       supported caller path or a manufactured resource/quiet receipt. */
    f->editor->workflow.recording_context=f;
    bytes=f->take_capacity;before=malloc(bytes);assert(before);memcpy(before,f->take,bytes);
    assert(pt_editor_midi_recording_get(f->take,(void *)f->take)==PT_EDITOR_RECORD_ALIAS);
    assert(!memcmp(before,f->take,bytes));free(before);
    {struct pt_editor_midi_record_status s,b;memset(&s,0x31,sizeof(s));memcpy(&b,&s,sizeof(s));
     assert(pt_editor_midi_recording_get(f->take,&s)==PT_EDITOR_RECORD_HELD&&!memcmp(&s,&b,sizeof(s)));}
    assert(pt_editor_midi_recording_cancel(f->take)==PT_EDITOR_RECORD_HELD);
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_HELD);
    master_unchanged(f);retained[0]=f;
}
static void retained_journal_drift(void)
{
    struct fixture *f=make();assert(begin(f,0,128,1)==PT_EDITOR_RECORD_OK);
    /* Genuine lower-core edit bypasses the editor gate, deliberately violating
       enrollment's caller restriction. Detect its actual journal revision. */
    assert(pt_pattern_title_apply(&f->document.project,&f->editor->history,"Outside edit")==PT_EDIT_OK);
    assert(send(f,0,0x9f,60,100)==PT_EDITOR_RECORD_HELD);
    assert(pt_editor_midi_recording_stop(f->take,ROW(1))==PT_EDITOR_RECORD_HELD);
    assert(pt_editor_midi_recording_detach(f->take)==PT_EDITOR_RECORD_HELD);
    master_unchanged(f);retained[1]=f;
}
int main(void)
{
    admission_refusals();hold_veto_and_workflow();real_record_save_undo();
    failed_stop_explicit_cancel();genuine_discard_reentry();
    retained_callback_drift();retained_journal_drift();assert(retained[0]&&retained[1]);
    puts("EDITOR MIDI RECORD HOST PASS:7 groups;5 complete;2 retained;real recorder/editor/journal;Hold/veto;workflow staleness;master/save/undo;NO INPUT/CAMD/TIMING/OUTPUT");
    return 0;
}
