#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
#include "mod_project.h"
static unsigned calls,fail,live;
static void *allocate(void *c,size_t n) {(void)c;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p) {assert(live);--live;free(p);}}
static struct pt_pattern_command commands[128];
static struct pt_event_change changes[2048];
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,copy;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_project *p;struct pt_sample *original;struct pt_event_update u;
    int32_t pcm[8]={-8388608,8388607,-17,19,33,-39,1048577,-1048579};uint32_t slices[2]={0,2};
    unsigned i,allocated,revision;size_t budget,n,w;uint8_t *encoded,*again;struct pt_mod_export_report report;
    struct pt_editor *e;
    pt_document_init(&d,&a);pt_document_init(&copy,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);p=&d.project;
    p->samples[0].pcm=(struct pt_pcm){pcm,8,4,48000,2,24};strcpy(p->samples[0].name,"STEREO SOURCE");p->samples[0].volume=48;p->samples[0].finetune=-1;
    p->samples[0].slices=slices;p->samples[0].slice_count=2;p->samples[0].loop=PT_LOOP_FORWARD;p->samples[0].loop_end=4;
    original=p->samples;pt_sampler_init(&s,&a,1024*1024);assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);allocated=live;
    for(i=1;i<=2;++i) {
        fail=calls+i;assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_CAPACITY && !s.table && !s.bytes && live==allocated && !h.count && p->samples==original && p->sample_count==31);
    }
    fail=0;budget=s.budget;s.budget=1;assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_CAPACITY && live==allocated);s.budget=budget;
    h.next_revision=UINT32_MAX;assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_CAPACITY && !s.table && !s.bytes);h.next_revision=1;
    h.bound_patterns=7;assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_INVALID && !s.table && !s.bytes);h.bound_patterns=1;
    assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_OK && p->sample_count==32 && p->samples==s.table && p->samples!=original);
    assert(p->samples[0].pcm.data==pcm && p->samples[31].pcm.bits==8 && p->samples[31].pcm.rate==PT_CLASSIC_RATE && !p->samples[31].pcm.frames);
    revision=h.revision;p->samples[31].volume=1;
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_CONFLICT && h.revision==revision && p->sample_count==32);p->samples[31].volume=0;
    p->events[0].instrument=32;assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_CONFLICT);p->events[0].instrument=0;
    assert(pt_sampler_import_slot(&s,p,&h,31,p,0)==PT_EDIT_OK && p->samples[31].pcm.data!=pcm && !memcmp(pcm,p->samples[31].pcm.data,sizeof(pcm)));
    u=(struct pt_event_update){0,{428,2,PT_NOTE_PERIOD,32,0,0,0,0}};assert(pt_pattern_apply(p,&h,&u,1)==PT_EDIT_OK);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !p->events[0].instrument);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !p->samples[31].pcm.frames);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->sample_count==31 && p->samples==original && !pt_pattern_dirty(&h));
    for(i=0;i<3;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
    assert(p->events[0].instrument==32 && p->samples[31].slice_count==2 && !memcmp(pcm,p->samples[31].pcm.data,sizeof(pcm)));
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);encoded=malloc(n);again=malloc(n);assert(encoded && again);
    assert(pt_project_encode(p,encoded,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&copy,encoded,n,SIZE_MAX)==PT_PROJECT_OK && copy.project.sample_count==32);
    assert(pt_project_encode(&copy.project,again,n,&w)==PT_PROJECT_OK && !memcmp(encoded,again,n));free(encoded);free(again);
    assert(pt_mod_export_analyse(p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_LIMITS));
    for(i=0;i<3;++i)assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
    allocated=live;revision=h.revision;fail=calls+1;
    assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_CAPACITY && live==allocated && h.revision==revision && p->sample_count==31);fail=0;
    assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK && pt_pattern_undo(p,&h,1)==PT_EDIT_OK && p->samples[31].pcm.frames==4);
    /* Replace original storage before releasing the history/table owner. */
    assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);
    for(i=31;i<255;++i)assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_OK && p->sample_count==i+1 && s.bytes<65536);
    assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_CAPACITY);
    for(i=0;i<128;++i)assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK);
    assert(p->sample_count==127 && pt_pattern_undo(p,&h,-1)==PT_EDIT_END);
    for(i=0;i<128;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
    assert(p->sample_count==255 && pt_project_validate(p,NULL)==PT_PROJECT_OK);
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);p->sample_count=0;p->samples=NULL;
    assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);
    assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_OK && p->sample_count==1);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !p->sample_count && !p->samples);
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    /* Earlier slot edits remain owned when table undo returns to document storage. */
    original=p->samples;assert(pt_pattern_history_init(&h,p,commands,128,changes,2048)==PT_EDIT_OK);
    assert(pt_sampler_attributes(&s,p,&h,0,"HELD",64,0)==PT_EDIT_OK);
    assert(pt_sampler_add_slot(&s,p,&h)==PT_EDIT_OK);
    assert(pt_sampler_attributes(&s,p,&h,0,"NEXT",32,0)==PT_EDIT_OK);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !strcmp(p->samples[0].name,"HELD"));
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->samples==original && p->samples[0].volume==64 && !strcmp(p->samples[0].name,"HELD"));
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !p->samples[0].name[0] && !p->samples[0].volume);
    for(i=0;i<3;++i)assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK);
    assert(p->sample_count==32 && !strcmp(p->samples[0].name,"NEXT") && p->samples[0].volume==32);
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);
    assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    e=calloc(1,sizeof(*e));assert(e && pt_editor_init(e,p));pt_editor_key(e,0x28,8);
    pt_editor_click(e,615,87);assert(p->sample_count==32 && e->sample==32);
    pt_editor_key(e,0x0c,9);assert(p->sample_count==33 && e->sample==33);
    pt_editor_key(e,0x31,8);assert(p->sample_count==32 && e->sample==32);
    pt_editor_key(e,0x31,8);assert(p->sample_count==31 && e->sample==31);
    pt_editor_key(e,0x31,9);assert(p->sample_count==32 && e->sample<=32);
    pt_editor_dispose(e);free(e);pt_document_release(&d);pt_document_release(&copy);assert(!live);
    puts("SLOTS PASS: 0..255 slots, allocation rollback, metadata/PCM/reference preservation, mixed undo, eviction, strict MOD refusal, exact PTG, selection clamp and complete release");return 0;
}
