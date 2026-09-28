#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_capture.h"
static size_t live,calls,fail;
static void *allocate(void *c,size_t n) {void *p;(void)c;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;if(p){assert(live);--live;free(p);}}
static struct pt_allocator allocator={NULL,allocate,release};
static void formats(void)
{
    unsigned bits,channels,i;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        struct pt_capture c={0};int32_t values[514];struct pt_pcm chunk;
        size_t before;const struct pt_pcm *out;
        for(i=0;i<257*channels;++i)values[i]=i%4==0?-(1L<<(bits-1)):i%4==1?(1L<<(bits-1))-1:i%4==2?1:-1;
        assert(pt_capture_open(&c,&allocator,bits,channels,48000,257,257*channels*4-1)==PT_CAPTURE_CAPACITY);
        assert(!c.pcm.data && !live);
        assert(pt_capture_open(&c,&allocator,bits,channels,48000,257,257*channels*4)==PT_CAPTURE_OK);
        assert(c.bytes==257*channels*4 && !pt_capture_pcm(&c));before=calls;
        chunk=(struct pt_pcm){values,256*channels,256,48000,(uint8_t)channels,(uint8_t)bits};
        assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_OK);
        chunk.data=values+256*channels;chunk.frames=1;chunk.capacity=channels;
        assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_OK && calls==before);
        assert(pt_capture_finish(&c)==PT_CAPTURE_OK && pt_capture_finish(&c)==PT_CAPTURE_OK);
        out=pt_capture_pcm(&c);assert(out && out->frames==257 && out->bits==bits && out->channels==channels && out->rate==48000);
        assert(!memcmp(out->data,values,257*channels*4));
        assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_INVALID);
        pt_capture_close(&c);pt_capture_close(&c);assert(!live);
    }
}
static void refusals(void)
{
    struct pt_capture c={0};int32_t values[514]={1,-1};struct pt_pcm p={values,514,1,48000,2,24};unsigned mode;
    assert(pt_capture_open(&c,&allocator,32,2,48000,2,16)==PT_CAPTURE_INVALID);
    assert(pt_capture_open(&c,&allocator,24,3,48000,2,24)==PT_CAPTURE_INVALID);
    assert(pt_capture_open(&c,&allocator,24,2,192001,2,16)==PT_CAPTURE_INVALID);
    assert(pt_capture_open(&c,&allocator,24,2,48000,UINT32_MAX,0)==PT_CAPTURE_CAPACITY);
    fail=calls+1;assert(pt_capture_open(&c,&allocator,24,2,48000,2,16)==PT_CAPTURE_CAPACITY);fail=0;
    for(mode=0;mode<6;++mode) {
        assert(pt_capture_open(&c,&allocator,24,2,48000,2,16)==PT_CAPTURE_OK);
        assert(pt_capture_append(&c,&p)==PT_CAPTURE_OK);
        if(mode==0) {struct pt_pcm bad=p;bad.frames=257;assert(pt_capture_append(&c,&bad)==PT_CAPTURE_INVALID);}
        if(mode==1) {struct pt_pcm bad=p;bad.bits=16;assert(pt_capture_append(&c,&bad)==PT_CAPTURE_INVALID);}
        if(mode==2) {values[0]=8388608;assert(pt_capture_append(&c,&p)==PT_CAPTURE_INVALID);values[0]=1;}
        if(mode==3) {struct pt_pcm bad=p;bad.frames=2;assert(pt_capture_append(&c,&bad)==PT_CAPTURE_OVERRUN);}
        if(mode==4)pt_capture_overrun(&c);
        if(mode==5) {struct pt_pcm bad=p;bad.capacity=1;assert(pt_capture_append(&c,&bad)==PT_CAPTURE_INVALID);}
        assert(c.pcm.frames==1 && c.pcm.data[0]==1 && c.pcm.data[1]==-1);
        assert(pt_capture_finish(&c)==(mode==3 || mode==4?PT_CAPTURE_OVERRUN:PT_CAPTURE_INVALID) && !pt_capture_pcm(&c));
        assert(pt_capture_append(&c,&p)==(mode==3 || mode==4?PT_CAPTURE_OVERRUN:PT_CAPTURE_INVALID));pt_capture_close(&c);assert(!live);
    }
    assert(pt_capture_open(&c,&allocator,24,2,48000,2,16)==PT_CAPTURE_OK);
    assert(pt_capture_finish(&c)==PT_CAPTURE_OK && !pt_capture_pcm(&c));pt_capture_close(&c);
}
static void publication(void)
{
    unsigned attempt,success=0;
    for(attempt=1;attempt<10 && !success;++attempt) {
        struct pt_capture c={0};struct pt_document d,copy;struct pt_sampler s;
        struct pt_pattern_history h;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
        int32_t values[6]={1,257,-513,1025,-8388608,8388607};struct pt_pcm p={values,6,3,48000,2,24};
        size_t n,w,before;uint8_t *encoded,*again;uint32_t revision;enum pt_edit_result result;
        pt_document_init(&d,&allocator);pt_document_init(&copy,&allocator);
        assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);pt_sampler_init(&s,&allocator,1024*1024);
        assert(pt_pattern_history_init(&h,&d.project,commands,4,changes,4)==PT_EDIT_OK);
        assert(pt_capture_open(&c,&allocator,24,2,48000,3,24)==PT_CAPTURE_OK);
        assert(pt_capture_append(&c,&p)==PT_CAPTURE_OK);
        assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"recording")==PT_EDIT_INVALID);
        assert(pt_capture_finish(&c)==PT_CAPTURE_OK);
        before=live;revision=h.revision;fail=calls+attempt;
        result=pt_sampler_capture_append(&s,&d.project,&h,&c,"recording");fail=0;
        if(result!=PT_EDIT_OK) {
            assert(result==PT_EDIT_CAPACITY && d.project.sample_count==31 && h.revision==revision && live==before);
            assert(pt_capture_pcm(&c) && !memcmp(c.pcm.data,values,sizeof(values)));
            assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"recording")==PT_EDIT_OK);
        } else success=1;
        assert(!c.pcm.data && d.project.sample_count==32 && d.project.samples[31].pcm.bits==24);
        assert(!memcmp(d.project.samples[31].pcm.data,values,sizeof(values)) && pt_pattern_dirty(&h));
        assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && d.project.sample_count==31 && !pt_pattern_dirty(&h));
        assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && d.project.sample_count==32);
        assert(!memcmp(d.project.samples[31].pcm.data,values,sizeof(values)));
        assert(pt_project_size(&d.project,&n)==PT_PROJECT_OK);encoded=malloc(n);again=malloc(n);assert(encoded && again);
        assert(pt_project_encode(&d.project,encoded,n,&w)==PT_PROJECT_OK && w==n);
        assert(pt_document_load(&copy,encoded,n,SIZE_MAX)==PT_PROJECT_OK);
        assert(pt_project_encode(&copy.project,again,n,&w)==PT_PROJECT_OK && w==n && !memcmp(encoded,again,n));
        free(encoded);free(again);pt_document_release(&copy);
        pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);assert(!live);
    }
    assert(success);
}
int main(void)
{
    formats();refusals();publication();assert(!live);
    puts("CAPTURE STAGING PASS: bounded exact8/16/24 mono/stereo chunks, explicit overrun, atomic undoable sampler publication, allocation rollback and exact project roundtrip; synthetic input only");return 0;
}
