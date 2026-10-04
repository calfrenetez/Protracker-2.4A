#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
#include "../src/editor/view.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static void idle(struct pt_editor *e)
{
    unsigned i;
    for(i=0;i<4000;++i) {
        (void)pt_editor_workflow_idle(e);
        if(!e->workflow.scanning && !e->workflow.busy && !e->workflow.resolving && !e->workflow.wave_building)return;
    }
    assert(!"workflow exceeded test bound");
}
static void guarded(void *c) {++*(unsigned *)c;}
static int capturing(void *c) {return *(unsigned *)c!=0;}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;
    struct pt_editor *e=malloc(sizeof(*e));unsigned guards=0,i,recording=0;uint32_t revision;struct pt_playback before;
    int32_t stereo[16]={-8388607,7340033,-5,7,9,-11,13,17,21,-23,29,31,37,-41,43,47},unused[4]={3,5,7,11};
    struct pt_sample original;struct pt_sample_range range;size_t owned;
    assert(e);
#ifdef PT_WORKFLOW_NATIVE
    assert((TypeOfMem(stereo)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
    assert((TypeOfMem(unused)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
#endif
    memset(e,0,sizeof(*e));pt_document_init(&d,&a);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){stereo,16,8,44100,2,24};
    strcpy(d.project.samples[0].name,"stereo master");d.project.samples[0].volume=37;d.project.samples[0].finetune=-2;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_start=2;d.project.samples[0].loop_end=6;
    strcpy(d.project.samples[1].name,"configured empty");
    d.project.samples[2].pcm=(struct pt_pcm){unused,4,4,22050,1,16};strcpy(d.project.samples[2].name,"unused");
    d.project.events[0].instrument=1;d.project.events[0].kind=PT_NOTE_PERIOD;d.project.events[0].pitch=428;
    d.project.events[16].kind=PT_NOTE_PERIOD;d.project.events[16].pitch=404;
    d.project.events[2*16+15].instrument=2; /* Last track, instrument-only protects empty. */
    assert(pt_editor_init(e,&d.project));
    pt_sampler_init(&e->sampler,&a,32UL*1024*1024);pt_document_init(&e->sample_source,&a);pt_song_init(&e->song,&a,8UL*1024*1024);
    pt_editor_change_guard(e,guarded,&guards);original=d.project.samples[0];revision=e->history.revision;
    assert(pt_editor_key(e,0x28,9)==PT_UI_NONE && e->panel==PT_WORKFLOW_MANAGER);idle(e);
    assert(e->workflow.usage_valid && e->workflow.usage.rows[0].references==1 && e->workflow.usage.rows[1].references==1);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!e->workflow.usage.selected[i]);
    assert(pt_editor_key(e,0x21,0)==PT_UI_NONE && e->workflow.view[e->workflow.highlight]+1U==e->sample);
    assert(pt_editor_key(e,0x21,0)==PT_UI_NONE && e->workflow.view[e->workflow.highlight]+1U==e->sample);
    assert(pt_editor_key(e,0x21,0)==PT_UI_NONE);
    e->workflow.recording_busy=capturing;e->workflow.recording_context=&recording;recording=1;
    assert(pt_editor_key(e,0x57,0)==PT_UI_NONE && !guards);recording=0;
    e->playback.active=1;before=e->playback;owned=e->sampler.bytes;
    assert(pt_editor_click(e,50,304)==PT_UI_NONE && e->sample==3); /* List row2 is silent. */
    assert(pt_editor_key(e,0x57,0)==PT_UI_NONE && !memcmp(&before,&e->playback,sizeof(before)));
    assert(e->history.revision==revision && e->sampler.bytes==owned && !guards);
    assert(pt_editor_key(e,0x28,0)==PT_UI_NONE && e->panel==5 && !guards);
    e->playback.active=0;e->sample=1;pt_editor_sample_all(e);
    e->sample_start=1;e->sample_end=7;
    assert(pt_editor_key(e,0x27,9)==PT_UI_NONE && e->panel==PT_WORKFLOW_TOOLBOX);
    assert(e->sample_start==1 && e->sample_end==7); /* Opening toolbox preserves actual selection. */
    assert(pt_editor_key(e,0x16,0)==PT_UI_NONE && e->sample_start==2 && e->sample_end==6 && !guards);
    e->wave_start=0;e->wave_end=3;e->wave_slot=1;e->wave_frames=8;
    range=(struct pt_sample_range){e->sample_start,e->sample_end};
    assert(pt_editor_key(e,0x12,0)==PT_UI_NONE && e->wave_start==5 && e->wave_end==8);
    assert(e->sample_start==range.start && e->sample_end==range.end && !memcmp(&original,&d.project.samples[0],sizeof(original)) && !guards);
    /* Pending copy authorization cannot rebase onto a different visible range. */
    e->playback.active=1;e->sample_start=2;e->sample_end=6;revision=e->history.revision;
    assert(pt_editor_key(e,0x33,0)==PT_UI_NONE && e->workflow.pending_apply==2);
    e->sample_start=3;assert(pt_editor_key(e,0x19,0)==PT_UI_WORKFLOW_STOP_APPLY);
    e->playback.active=0;pt_editor_workflow_apply(e);idle(e);
    assert(e->history.revision==revision && !d.project.samples[3].pcm.frames);
    /* Current selection copy bypasses clipboard and skips named/referenced slots. */
    e->sample_start=1;e->sample_end=7;e->clipboard.rows=1;e->clipboard.events[0].instrument=31;
    assert(pt_editor_key(e,0x33,0)==PT_UI_NONE);idle(e);
    assert(e->sample==4 && e->history.count==1 && e->sampler.generation==1);
    assert(d.project.samples[3].pcm.frames==6 && d.project.samples[3].pcm.bits==24 && d.project.samples[3].pcm.channels==2);
    assert(d.project.samples[3].volume==37 && d.project.samples[3].finetune==-2 && d.project.samples[3].loop_start==1 && d.project.samples[3].loop_end==5);
    assert(d.project.samples[3].pcm.data!=stereo && !memcmp(d.project.samples[3].pcm.data,stereo+2,12*sizeof(int32_t)));
    assert(!memcmp(&original,&d.project.samples[0],sizeof(original)));
    assert(pt_editor_key(e,0x31,8)==PT_UI_NONE && !d.project.samples[3].pcm.frames);
    assert(pt_editor_key(e,0x31,9)==PT_UI_NONE && d.project.samples[3].pcm.frames==6);
    /* Selected-only cleanup preview refuses active mutation; explicit Stop+Apply. */
    assert(pt_editor_key(e,0x28,9)==PT_UI_NONE);idle(e);
    assert(pt_editor_click(e,8,304)==PT_UI_NONE && e->workflow.usage.selected[2]);
    e->playback.active=1;revision=e->history.revision;
    assert(pt_editor_key(e,0x22,0)==PT_UI_NONE && e->workflow.pending_apply==1 && !e->workflow.transaction);
    assert(e->history.revision==revision && d.project.samples[2].pcm.frames==4);
    assert(pt_editor_key(e,0x19,0)==PT_UI_WORKFLOW_STOP_APPLY);
    e->playback.active=0;pt_editor_workflow_apply(e);idle(e);
    assert(!d.project.samples[2].pcm.frames && d.project.samples[0].pcm.frames==8 && !strcmp(d.project.samples[1].name,"configured empty"));
    assert(d.project.samples[3].pcm.frames==6 && e->history.count==2 && e->workflow.stats.master_bytes_released==0);
    /* Existing native project save/restore preserves retained music and 24-bit masters. */
    {size_t bytes,written=0;uint8_t *encoded;struct pt_document restored;
     assert(pt_project_size(&d.project,&bytes)==PT_PROJECT_OK);encoded=allocate(NULL,bytes);assert(encoded);
     assert(pt_project_encode(&d.project,encoded,bytes,&written)==PT_PROJECT_OK && written==bytes);
     pt_document_init(&restored,&a);assert(pt_document_load(&restored,encoded,bytes,SIZE_MAX)==PT_PROJECT_OK);
     assert(restored.project.sample_count==d.project.sample_count && restored.project.channels.count==16);
     assert(!memcmp(restored.project.events,d.project.events,64*16*sizeof(*d.project.events)));
     assert(restored.project.samples[0].pcm.bits==24 && restored.project.samples[0].pcm.channels==2);
     assert(!memcmp(restored.project.samples[0].pcm.data,stereo,sizeof(stereo)) && !restored.project.samples[2].pcm.frames);
     assert(!memcmp(restored.project.samples[3].pcm.data,stereo+2,12*sizeof(int32_t)));
     pt_document_release(&restored);release(NULL,encoded);}

    assert(pt_editor_key(e,0x31,8)==PT_UI_NONE && d.project.samples[2].pcm.frames==4);
    /* Explicit and inherited navigation consume input, leave music/transport alone. */
    e->panel=0;e->sample=3;e->field=4;e->row=0;e->pattern=0;e->position=0;d.project.channels.selected=0;revision=e->history.revision;before=e->playback;guards=0;
    assert(pt_editor_key(e,0x44,8)==PT_UI_NONE && e->sample==1 && e->panel==0);
    assert(!guards && e->history.revision==revision && !memcmp(&before,&e->playback,sizeof(before)));
    e->row=1;e->sample=3;assert(pt_editor_key(e,0x44,9)==PT_UI_NONE);idle(e);
    assert(e->sample==1 && e->panel==5 && e->workflow.resource.state==PT_EVENT_RESOURCE_RESOLVED_INHERITED);
    e->row=9;assert(pt_editor_key(e,0x41,0x10)==PT_UI_NONE && e->row==1 && e->field==4 && e->panel==1 && !guards);
    /* Resolver must not substitute unrelated selected sample for detached event. */
    e->position=PT_PROJECT_ORDERS;e->sample=3;e->row=1;
    assert(pt_editor_key(e,0x44,9)==PT_UI_NONE && e->sample==3 && e->workflow.resource.state==PT_EVENT_RESOURCE_AMBIGUOUS);
    /* Summary is built once per viewport, then selection/status redraw needs no reads. */
    e->panel=5;e->sample=1;pt_editor_sample_all(e);idle(e);
    assert(pt_wave_summary_current(&e->workflow.wave,e->project,e->sampler.generation));
    {struct pt_wave_summary summary=e->workflow.wave;unsigned reads=e->workflow.wave_job.last_values;
     pt_editor_click(e,50,300);pt_editor_click(e,120,300);idle(e);
     assert(!memcmp(&summary,&e->workflow.wave,sizeof(summary)) && reads==e->workflow.wave_job.last_values);}
    printf("WORKFLOW HOST PASS: silent UI, conservative preview, exact current copy, one undo, explicit stop gate, event origin, summary reuse; editor=%lu workflow=%lu bytes\n",(unsigned long)sizeof(*e),(unsigned long)sizeof(e->workflow));
    assert(pt_editor_dispose(e));assert(!e->sampler.bytes);pt_document_release(&d);free(e);return 0;
}
