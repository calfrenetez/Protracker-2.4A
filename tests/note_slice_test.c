#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
#include "wav.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_editor *e=calloc(1,sizeof(*e));
    struct pt_event_update change={0};struct pt_event original;struct pt_project *p;
    int32_t values[8]={-100,80,20,-30,100,-80,40,-50};struct pt_pcm pcm={values,8,8,8287,1,8};
    uint8_t wav[100],*bytes;uint32_t markers[3]={0,2,6},retarget[3]={0,1,6};size_t length,n,w;unsigned revision;
    assert(e);pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);p=&d.project;
    assert(pt_editor_init(e,p));assert(pt_wav_encode(&pcm,wav,sizeof(wav),&length)==PT_WAV_OK);
    assert(pt_sampler_import(&e->sampler,p,&e->history,0,wav,length,"slices")==PT_EDIT_OK);
    assert(pt_sampler_slices(&e->sampler,p,&e->history,0,markers,3)==PT_EDIT_OK);
    change.event=(struct pt_event){428,0,PT_NOTE_PERIOD,1,12,32,64,1};
    assert(pt_pattern_apply(p,&e->history,&change,1)==PT_EDIT_OK);original=p->events[0];
    change.index=4;change.event.instrument=0;assert(pt_pattern_apply(p,&e->history,&change,1)==PT_EDIT_OK);
    pt_editor_saved(e);revision=e->history.revision;
    if(argc==2) {
        FILE *file;assert(pt_project_size(p,&n)==PT_PROJECT_OK);bytes=malloc(n);assert(bytes);
        assert(pt_project_encode(p,bytes,n,&w)==PT_PROJECT_OK && w==n);file=fopen(argv[1],"wb");assert(file);
        assert(fwrite(bytes,1,n,file)==n && !fclose(file));free(bytes);
    }
    pt_editor_key(e,0x17,8);assert(e->panel==1 && e->note_details && !pt_editor_dirty(e));
    pt_editor_key(e,0x0b,0);assert(e->history.revision==revision);
    pt_editor_key(e,0x0c,0);assert(p->events[0].slice==1);
    pt_editor_key(e,0x21,0);pt_editor_key(e,3,0);pt_editor_key(e,0x44,0);assert(p->events[0].slice==3 && !e->number_field);
    pt_editor_key(e,0x31,8);assert(p->events[0].slice==1);revision=e->history.revision;
    pt_editor_click(e,400,30);pt_editor_key(e,4,0);pt_editor_key(e,0x44,0);
    assert(e->number_field==8 && e->history.revision==revision && p->events[0].slice==1);
    pt_editor_key(e,0x4d,0);pt_editor_click(e,220,300);assert(!e->row && !p->channels.selected);
    pt_editor_key(e,0x45,0);pt_editor_key(e,0x31,9);assert(p->events[0].slice==3);
    pt_editor_click(e,400,50);assert(!p->events[0].slice);pt_editor_key(e,0x31,8);assert(p->events[0].slice==3);
    pt_editor_key(e,0x0b,0);assert(p->events[0].slice==2);
    assert(pt_sampler_slices(&e->sampler,p,&e->history,0,retarget,3)==PT_EDIT_UNSUPPORTED);
    revision=e->history.revision;pt_editor_key(e,0x1b,0);assert(e->sample==2);pt_editor_click(e,250,50);
    assert(e->history.revision==revision && p->events[0].instrument==1 && p->events[0].slice==2);
    pt_editor_key(e,0x1a,0);pt_editor_key(e,0x16,0);assert(e->history.revision==revision);
    pt_editor_key(e,0x31,0);assert(!e->row && p->events[0].pitch==428); /* No note entry in settings. */
    pt_editor_key(e,0x4d,0);pt_editor_key(e,0x0c,0);pt_editor_key(e,0x16,0);
    assert(e->row==1 && !p->events[16].instrument && !p->events[16].slice && e->history.revision==revision);
    pt_editor_key(e,0x4c,0);pt_editor_key(e,0x42,0);pt_editor_key(e,0x0c,0);assert(!p->events[1].slice);
    pt_editor_key(e,0x42,1);assert(!p->channels.selected);
    pt_editor_key(e,0x28,0);assert(e->panel==5 && e->sample==1);pt_editor_key(e,0x17,8);assert(e->note_details && e->panel==1);
    pt_editor_key(e,0x33,0);assert(!p->events[0].slice);
    assert(pt_sampler_slices(&e->sampler,p,&e->history,0,retarget,3)==PT_EDIT_OK);
    pt_editor_key(e,0x31,8);assert(p->samples[0].slices[1]==2 && !p->events[0].slice);
    pt_editor_key(e,0x31,8);assert(p->events[0].slice==2 && p->samples[0].slices[1]==2);
    original.slice=2;assert(!memcmp(&p->events[0],&original,sizeof(original)));
    assert(!memcmp(p->samples[0].pcm.data,values,sizeof(values)));
    pt_editor_saved(e);assert(pt_project_size(p,&n)==PT_PROJECT_OK);bytes=malloc(n);assert(bytes);
    assert(pt_project_encode(p,bytes,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&reopened,bytes,n,SIZE_MAX)==PT_PROJECT_OK);free(bytes);
    assert(!memcmp(&reopened.project.events[0],&original,sizeof(original)) && reopened.project.samples[0].slices[1]==2);
    /* USE SMP explicitly attaches a sample to an existing instrument-zero note. */
    change.index=16;change.event=original;change.event.slice=0;change.event.instrument=0;
    assert(pt_pattern_apply(p,&e->history,&change,1)==PT_EDIT_OK);pt_editor_key(e,0x4d,0);pt_editor_key(e,0x16,0);
    assert(p->events[16].instrument==1 && !p->events[16].slice);pt_editor_key(e,0x0c,0);assert(p->events[16].slice==1);
    change.index=32;change.event=(struct pt_event){60,0,PT_NOTE_MIDI,1,0,0,0,0};assert(pt_pattern_apply(p,&e->history,&change,1)==PT_EDIT_OK);
    pt_editor_key(e,0x4d,0);revision=e->history.revision;pt_editor_key(e,0x0c,0);pt_editor_key(e,0x16,0);assert(e->history.revision==revision);
    pt_editor_key(e,0x45,0);assert(e->panel==1 && !e->note_details);pt_editor_click(e,520,87);assert(e->note_details);
    pt_editor_click(e,520,87);assert(!e->note_details);pt_editor_click(e,400,87);assert(!e->panel);
    pt_editor_dispose(e);free(e);pt_document_release(&d);pt_document_release(&reopened);
    puts("NOTE SLICE PASS: explicit ordinal mapping, sample attachment, bounds, modal isolation, shared note/sample undo, marker protection, PCM preservation and PTG reopen");return 0;
}
