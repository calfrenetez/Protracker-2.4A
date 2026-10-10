#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
static void *allocate(void *context,size_t bytes) {(void)context;return malloc(bytes);}
static void release(void *context,void *memory) {(void)context;free(memory);}
static void guard(void *context) {++*(unsigned *)context;}
static void start_copy(struct pt_editor *e)
{
    unsigned i;
    e->panel=0;e->playback.active=0;e->sample=1;pt_editor_sample_all(e);
    assert(pt_editor_key(e,0x27,9)==PT_UI_NONE && e->panel==PT_WORKFLOW_TOOLBOX);
    assert(pt_editor_key(e,0x33,0)==PT_UI_NONE);
    for(i=0;i<4000 && !e->workflow.transaction;++i)(void)pt_editor_workflow_idle(e);
    assert(i<4000 && e->workflow.busy && e->workflow.transaction && !e->workflow.scanning);
}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_editor *e=calloc(1,sizeof(*e));struct pt_editor_workflow *work=malloc(sizeof(*work));
    struct pt_project before;struct pt_playback playback;uint32_t revision,generation;
    struct pt_event *events;struct pt_sample samples[PT_PROJECT_SAMPLES];unsigned guards=0;
    size_t owned,event_bytes,sample_bytes;int32_t master[8]={-8388607,7340033,-5,7,9,-11,13,17},original[8];
    assert(e && work);assert(PT_UI_WORKFLOW_STOP_APPLY==24 && PT_UI_RECOVERY_SETTINGS==25);
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){master,8,8,44100,1,24};
    strcpy(d.project.samples[0].name,"original master");d.project.samples[0].volume=37;
    d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;d.project.events[0].instrument=1;
    assert(pt_editor_init(e,&d.project));pt_sampler_init(&e->sampler,&a,32UL*1024*1024);
    pt_document_init(&e->sample_source,&a);pt_song_init(&e->song,&a,8UL*1024*1024);
    pt_editor_change_guard(e,guard,&guards);memcpy(original,master,sizeof(master));
    event_bytes=(size_t)d.project.pattern_count*64*d.project.channels.count*sizeof(*events);
    sample_bytes=(size_t)d.project.sample_count*sizeof(*samples);
    assert(d.project.sample_count<=PT_PROJECT_SAMPLES);events=malloc(event_bytes);assert(events);
    start_copy(e);
    /* A genuinely owned, unpublished transaction, deliberately presented in the
     * default panel to exercise its new hit target. No synthetic busy flag. */
    e->panel=0;guards=0;*work=e->workflow;before=d.project;playback=e->playback;
    memcpy(events,d.project.events,event_bytes);memcpy(samples,d.project.samples,sample_bytes);
    revision=e->history.revision;generation=e->sampler.generation;owned=e->sampler.bytes;
    assert(pt_editor_click(e,500,85)==PT_UI_NONE);
    assert(!memcmp(work,&e->workflow,sizeof(*work)) && !guards && e->panel==0);
    assert(pt_editor_key(e,0x25,9)==PT_UI_NONE);
    assert(pt_editor_key(e,0x25,10)==PT_UI_NONE);
    assert(!memcmp(work,&e->workflow,sizeof(*work)) && !guards && e->panel==0);
    assert(!memcmp(&before,&d.project,sizeof(before)) && !memcmp(samples,d.project.samples,sample_bytes) &&
        !memcmp(events,d.project.events,event_bytes) && !memcmp(master,original,sizeof(master)));
    assert(!memcmp(&playback,&e->playback,sizeof(playback)) && e->history.revision==revision &&
        e->sampler.generation==generation && e->sampler.bytes==owned && !d.project.samples[1].pcm.frames);
    /* Existing Cancel in another panel and Escape in the default panel remain
     * genuine cancellation paths; neither publishes the staged copy. */
    e->panel=PT_WORKFLOW_TOOLBOX;assert(pt_editor_click(e,500,85)==PT_UI_NONE);
    assert(!e->workflow.busy && !e->workflow.transaction && !d.project.samples[1].pcm.frames && !guards);
    start_copy(e);e->panel=0;guards=0;revision=e->history.revision;
    assert(pt_editor_key(e,0x45,0)==PT_UI_NONE);
    assert(!e->workflow.busy && !e->workflow.transaction && e->history.revision==revision &&
        !d.project.samples[1].pcm.frames && !guards && !memcmp(master,original,sizeof(master)));
    /* Idle new actions consume only their own default-panel input. The native
     * frontend must independently refuse active audio/recording before opening. */
    e->panel=0;playback=e->playback;revision=e->history.revision;guards=0;
    assert(pt_editor_click(e,476,78)==PT_UI_RECOVERY_SETTINGS);
    assert(pt_editor_click(e,598,96)==PT_UI_RECOVERY_SETTINGS);
    assert(pt_editor_key(e,0x25,9)==PT_UI_RECOVERY_SETTINGS);
    assert(pt_editor_key(e,0x25,10)==PT_UI_RECOVERY_SETTINGS);
    assert(!guards && e->history.revision==revision && !memcmp(&playback,&e->playback,sizeof(playback)));
    e->panel=2;assert(pt_editor_key(e,0x25,9)==PT_UI_NONE);
    e->panel=0;e->name_entry=1;assert(pt_editor_key(e,0x25,9)==PT_UI_NONE);
    assert(pt_editor_click(e,500,85)==PT_UI_NONE);e->name_entry=0;
    assert(pt_editor_dispose(e));pt_document_release(&d);free(events);free(work);free(e);
    puts("RECOVERY DISPATCH HOST PASS: real busy copy retained; default Settings emits only action; other-panel Cancel/Escape preserved");
    puts("NOT TESTED: native audio ownership gate, requester window/input/redraw, main timer lifecycle, emulator or A1200");
    return 0;
}
