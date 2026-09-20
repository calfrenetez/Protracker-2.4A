#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler.h"
static unsigned calls,fail,live;
static void *allocate(void *c,size_t n) {(void)c;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p) {assert(live);--live;free(p);}}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d,reopened;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[16];struct pt_event_change changes[16];
    unsigned values_count=argc>1?4096:65536,frames=values_count/2;
    struct pt_sample initial;struct pt_project *p;int32_t *pcm=malloc(values_count*sizeof(*pcm)),*shared;uint32_t markers[2]={0,frames/2};
    char name[32],invalid[32];unsigned i,allocated,revision;size_t n,w,budget;uint8_t *bytes;
    (void)argv;assert(pcm);for(i=0;i<values_count;++i)pcm[i]=(int32_t)(i%1024)-512;
    pt_document_init(&d,&a);pt_document_init(&reopened,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);p=&d.project;
    p->samples[0].pcm=(struct pt_pcm){pcm,values_count,frames,48000,2,24};strcpy(p->samples[0].name,"ORIGINAL");
    p->samples[0].volume=64;p->samples[0].slices=markers;p->samples[0].slice_count=2;
    p->events[0]=(struct pt_event){428,2,PT_NOTE_PERIOD,1,0,0,0,0};initial=p->samples[0];
    assert(pt_pattern_history_init(&h,p,commands,16,changes,16)==PT_EDIT_OK);pt_sampler_init(&s,&a,1024*1024);
    allocated=live;memset(invalid,'x',sizeof(invalid));
    assert(pt_sampler_attributes(&s,p,&h,0,invalid,64,0)==PT_EDIT_INVALID);
    assert(pt_sampler_attributes(&s,p,&h,0,"NAME",65,0)==PT_EDIT_INVALID);
    assert(pt_sampler_attributes(&s,p,&h,0,"NAME",64,-9)==PT_EDIT_INVALID);
    assert(pt_sampler_attributes(&s,p,&h,0,"NAME",64,8)==PT_EDIT_INVALID);
    assert(pt_sampler_attributes(&s,p,&h,0,"ORIGINAL",64,0)==PT_EDIT_OK && live==allocated && !h.count);
    for(i=1;i<=3;++i) {
        fail=calls+i;assert(pt_sampler_attributes(&s,p,&h,0,"BASS",48,-3)==PT_EDIT_CAPACITY);
        assert(live==allocated && !s.bytes && !h.count && !memcmp(&initial,&p->samples[0],sizeof(initial)));
    }
    fail=0;assert(pt_sampler_attributes(&s,p,&h,0,"BASS",48,-3)==PT_EDIT_OK);shared=p->samples[0].pcm.data;
    assert(shared!=pcm && !memcmp(shared,pcm,values_count*sizeof(*pcm)) && p->samples[0].slices[1]==frames/2);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !strcmp(p->samples[0].name,"ORIGINAL"));revision=h.revision;
    budget=s.budget;s.budget=s.bytes;allocated=live;
    assert(pt_sampler_attributes(&s,p,&h,0,"FAIL",0,-8)==PT_EDIT_CAPACITY && h.revision==revision && live==allocated);
    s.budget=budget;assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK && !strcmp(p->samples[0].name,"BASS"));
    for(i=1;i<=2;++i) {
        fail=calls+i;allocated=live;revision=h.revision;
        assert(pt_sampler_attributes(&s,p,&h,0,"FAIL",0,-8)==PT_EDIT_CAPACITY && live==allocated && h.revision==revision);
    }
    fail=0;
    for(i=0;i<200;++i) {
        snprintf(name,sizeof(name),"META %03u",i);
        assert(pt_sampler_attributes(&s,p,&h,0,name,i%65,(int)(i%16)-8)==PT_EDIT_OK);
        assert(p->samples[0].pcm.data==shared && s.bytes<values_count*sizeof(*pcm)+10000);
    }
    assert(!memcmp(shared,pcm,values_count*sizeof(*pcm)) && !strcmp(p->samples[0].name,"META 199"));
    assert(pt_sampler_edit(&s,p,&h,0,PT_PCM_REVERSE,0,frames,0)==PT_EDIT_OK);
    assert(p->samples[0].pcm.data!=shared && shared[0]==pcm[0]);
    assert(pt_sampler_attributes(&s,p,&h,0,"REVERSED",64,7)==PT_EDIT_OK);
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && !strcmp(p->samples[0].name,"META 199"));
    assert(pt_pattern_undo(p,&h,-1)==PT_EDIT_OK && p->samples[0].pcm.data==shared);
    assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK && p->samples[0].pcm.data!=shared);
    assert(pt_pattern_undo(p,&h,1)==PT_EDIT_OK && !strcmp(p->samples[0].name,"REVERSED"));
    assert(pt_project_size(p,&n)==PT_PROJECT_OK);bytes=malloc(n);assert(bytes);
    assert(pt_project_encode(p,bytes,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_document_load(&reopened,bytes,n,SIZE_MAX)==PT_PROJECT_OK);free(bytes);
    assert(reopened.project.samples[0].volume==64 && reopened.project.samples[0].finetune==7 && !strcmp(reopened.project.samples[0].name,"REVERSED"));
    assert(!memcmp(p->samples[0].pcm.data,reopened.project.samples[0].pcm.data,values_count*sizeof(*pcm)) && reopened.project.events[0].slice==2);
    pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);pt_document_release(&d);pt_document_release(&reopened);free(pcm);assert(!live);
    puts("SAMPLE ATTRIBUTES PASS: bounds, every allocation failure, shared 24-bit stereo PCM, bounded metadata history, marker preservation, mixed PCM/metadata undo and exact reopen");return 0;
}
