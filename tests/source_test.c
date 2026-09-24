#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/editor.h"
#include "../src/platform/mod_import.h"
static enum pt_project_result file_source(void *context,struct pt_document *d,size_t budget)
{return pt_mod_file_load(d,(const char *)context,64UL*1024*1024,budget);}
static enum pt_project_result failed_source(void *context,struct pt_document *d,size_t budget)
{enum pt_project_result r=file_source(context,d,budget);return r==PT_PROJECT_OK?PT_PROJECT_INVALID:r;}
static unsigned calls,fail,live;
static void *allocate(void *c,size_t n) {(void)c;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;assert(live);--live;free(p);}
int main(int argc,char **argv)
{
    FILE *f;long n;uint8_t *bytes;struct pt_document d,source_before;struct pt_editor *e;struct pt_allocator a={NULL,allocate,release};
    size_t budget,available;unsigned i,allocated;struct pt_sample *sample;struct pt_event original[1024];struct pt_channels channels;
    assert(argc==2);f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>0);rewind(f);bytes=malloc(n);assert(bytes && fread(bytes,1,n,f)==(size_t)n && !fclose(f));
    pt_document_init(&d,&a);assert(pt_document_new(&d,16,SIZE_MAX)==PT_PROJECT_OK);strcpy(d.project.title,"KEEP THIS SONG");
    memcpy(original,d.project.events,sizeof(original));channels=d.project.channels;
    e=malloc(sizeof(*e));assert(e && pt_editor_init(e,&d.project));e->sampler.allocator=a;e->sample=3;budget=e->sampler.budget;
    allocated=live;e->sampler.budget=1;
    assert(pt_editor_source_load_with(e,file_source,argv[1])==PT_EDIT_CAPACITY && live==allocated);e->sampler.budget=budget;
    for(i=1;i<=6;++i) {
        fail=calls+i;assert(pt_editor_source_load_with(e,file_source,argv[1])==PT_EDIT_CAPACITY);
        assert(live==allocated && !e->sample_source.loaded && e->sampler.budget==budget && !e->history.revision);
    }
    fail=0;assert(pt_editor_source_load(e,bytes,n)==PT_EDIT_OK && e->panel==11 && e->source_selected==1 && !e->history.revision);
    assert(e->sampler.budget+e->sample_source.allocated_bytes==budget && !strcmp(d.project.title,"KEEP THIS SONG"));
    {
        uint8_t title[20];memcpy(title,bytes,20);memcpy(bytes,"PT24G SOURCE CHECK",17);
        assert(pt_editor_source_load(e,bytes,n)==PT_EDIT_OK);
        assert(!strncmp(e->sample_source.project.title,"PT24G SOURCE CHECK",17));
        memcpy(bytes,title,20);
        assert(pt_editor_source_load(e,bytes,n)==PT_EDIT_OK);
    }
    assert(pt_editor_source_load_with(e,file_source,argv[1])==PT_EDIT_OK);
    source_before=e->sample_source;available=e->sampler.budget;allocated=live;
    assert(pt_editor_source_load_with(e,failed_source,argv[1])==PT_EDIT_UNSUPPORTED);
    assert(!memcmp(&source_before,&e->sample_source,sizeof(source_before)) && live==allocated && e->sampler.budget==available && !e->history.revision);

    assert(pt_editor_source_load(e,(const uint8_t *)"PT24G",5)==PT_EDIT_UNSUPPORTED);
    assert(pt_editor_source_load(e,bytes,n-1)==PT_EDIT_UNSUPPORTED && !memcmp(&source_before,&e->sample_source,sizeof(source_before)));
    fail=calls+2;assert(pt_editor_source_load_with(e,file_source,argv[1])==PT_EDIT_CAPACITY && live==allocated && e->sampler.budget==available);fail=0;
    assert(pt_editor_key(e,0x21,8)==PT_UI_SAVE && !e->history.revision);
    assert(pt_editor_key(e,0x28,0)==PT_UI_SOURCE_LOAD);
    pt_editor_click(e,520,30);assert(e->source_selected==2);
    pt_editor_key(e,0x4e,0);assert(e->source_selected==3);pt_editor_key(e,0x44,0);assert(!e->history.revision && strstr(e->status,"NONEMPTY"));
    pt_editor_key(e,0x4f,0);assert(e->source_selected==2);sample=&d.project.samples[2];
    for(i=1;i<=3;++i) {
        fail=calls+i;pt_editor_key(e,0x44,0);assert(!e->history.revision && !sample->pcm.frames && live==allocated && strstr(e->status,"MEMORY"));
    }
    fail=0;pt_editor_click(e,250,85);assert(e->history.revision==1 && sample->pcm.frames==32 && sample->volume==48 && sample->finetune==-3);
    assert(sample->loop==PT_LOOP_FORWARD && sample->loop_start==8 && sample->loop_end==24 && !strcmp(sample->name,"SOURCE TWO"));
    assert(sample->pcm.data!=e->sample_source.project.samples[1].pcm.data);
    for(i=0;i<32;++i)assert(sample->pcm.data[i]==(int)(i*7%256)-128);
    assert(!memcmp(original,d.project.events,sizeof(original)) && !memcmp(&channels,&d.project.channels,sizeof(channels)) && !strcmp(d.project.title,"KEEP THIS SONG"));
    {
        uint32_t marker=0;struct pt_event_update event={0};
        assert(pt_sampler_slices(&e->sampler,&d.project,&e->history,2,&marker,1)==PT_EDIT_OK);
        event.event.kind=PT_NOTE_PERIOD;event.event.pitch=428;event.event.instrument=3;event.event.slice=1;
        assert(pt_pattern_apply(&d.project,&e->history,&event,1)==PT_EDIT_OK);i=e->history.revision;
        pt_editor_key(e,0x44,0);assert(e->history.revision==i && strstr(e->status,"UNSUPPORTED"));
        pt_editor_key(e,0x31,8);pt_editor_key(e,0x31,8);assert(e->history.revision==1);
    }
    pt_editor_key(e,0x31,8);assert(!sample->pcm.frames && !e->history.revision);
    pt_editor_key(e,0x31,9);assert(sample->pcm.frames==32 && e->history.revision==1);
    pt_editor_key(e,0x45,0);assert(e->panel==5 && !e->sample_source.loaded && e->sampler.budget==budget);
    assert(sample->pcm.data[0]==-128 && sample->finetune==-3);pt_editor_key(e,0x31,8);pt_editor_key(e,0x31,9);assert(sample->pcm.frames==32);
    assert(pt_editor_source_load(e,bytes,n)==PT_EDIT_OK);pt_editor_key(e,0x42,0);assert(e->sampler.budget==budget && e->history.revision==1);
    assert(pt_editor_key(e,0x28,1)==PT_UI_SOURCE_LOAD);
    pt_editor_dispose(e);free(e);pt_document_release(&d);free(bytes);assert(!live);
    puts("MOD SOURCE PASS: separate preview, source/destination selection, failed allocation/budget preservation, exact metadata/PCM import, shared undo, retained song and source release");return 0;
}
