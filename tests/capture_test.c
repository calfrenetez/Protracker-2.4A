#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/sampler_capture.h"
static size_t live,calls,fail,max_request;
static void *allocate(void *c,size_t n) {void *p;(void)c;if(++calls==fail || (max_request && n>max_request))return NULL;p=malloc(n);if(p)++live;return p;}
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
static void transfer_ownership(void)
{
    struct pt_capture c={0};struct pt_document d;struct pt_sampler s;
    struct pt_pattern_history h;struct pt_pattern_command commands[4];struct pt_event_change changes[4];
    struct pt_sample_version *pin;struct pt_pcm pinned;int32_t values[256],*original;unsigned i;
    struct pt_pcm chunk={values,256,256,48000,1,24};size_t before,charged;uint32_t revision;
    for(i=0;i<256;++i)values[i]=(int32_t)i*257-32769;
    pt_document_init(&d,&allocator);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    pt_sampler_init(&s,&allocator,1024*1024);
    assert(pt_pattern_history_init(&h,&d.project,commands,4,changes,4)==PT_EDIT_OK);
    assert(pt_capture_open(&c,&allocator,24,1,48000,65536,262144)==PT_CAPTURE_OK);
    for(i=0;i<128;++i)assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_OK);
    assert(pt_capture_finish(&c)==PT_CAPTURE_OK);original=c.pcm.data;before=live;revision=h.revision;
    /* Enough for a compact 128 KiB master but not the owned 256 KiB capacity. */
    s.budget=200000;
    assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"owned")==PT_EDIT_CAPACITY);
    assert(c.pcm.data==original && live==before && s.bytes==0 && h.revision==revision && d.project.sample_count==31);
    s.budget=1024*1024;
    h.next_revision=UINT32_MAX;
    assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"owned")==PT_EDIT_CAPACITY);
    assert(c.pcm.data==original && live==before && s.bytes==0 && h.revision==revision);
    h.next_revision=revision+1;
    /* Reject every allocation large enough for a second full PCM copy. */
    max_request=65536;
    assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"owned")==PT_EDIT_OK);
    assert(!c.pcm.data && d.project.samples[31].pcm.data==original);
    assert(s.bytes>=262144 && d.project.samples[31].pcm.capacity==65536);
    assert(pt_sampler_attributes(&s,&d.project,&h,31,"renamed",32,0)==PT_EDIT_OK);
    assert(d.project.samples[31].pcm.data==original);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && d.project.sample_count==31);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK);
    assert(pt_sampler_pin(&s,&d.project,31,s.generation,&pinned,&pin)==PT_EDIT_OK && pinned.data==original);
    pt_pattern_history_release(&h);pt_sampler_release(&s);
    charged=s.bytes;assert(charged>=262144 && !memcmp(pinned.data,values,sizeof(values)));
    pt_sampler_unpin(pin);assert(s.bytes==0);max_request=0;
    pt_capture_close(&c);pt_document_release(&d);assert(!live);
}
static void allocator_fallback(void)
{
    unsigned attempt,success=0;int other_context=0;
    struct pt_allocator other={&other_context,allocate,release};
    for(attempt=1;attempt<8 && !success;++attempt) {
        struct pt_capture c={0};struct pt_document d;struct pt_sampler s;
        struct pt_pattern_history h;struct pt_pattern_command commands[2];struct pt_event_change changes[2];
        int32_t values[2]={257,-513},*original;struct pt_pcm chunk={values,2,2,48000,1,24};
        size_t before;enum pt_edit_result result;
        pt_document_init(&d,&allocator);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        pt_sampler_init(&s,&allocator,1024*1024);
        assert(pt_pattern_history_init(&h,&d.project,commands,2,changes,2)==PT_EDIT_OK);
        assert(pt_capture_open(&c,&other,24,1,48000,2,8)==PT_CAPTURE_OK);
        assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_OK && pt_capture_finish(&c)==PT_CAPTURE_OK);
        original=c.pcm.data;before=live;
        assert(pt_sampler_append_owned(&s,&d.project,&h,&c.pcm,&other,"refuse")==PT_EDIT_INVALID);
        assert(c.pcm.data==original && live==before);
        fail=calls+attempt;result=pt_sampler_capture_append(&s,&d.project,&h,&c,"copy");fail=0;
        if(result!=PT_EDIT_OK) {
            assert(result==PT_EDIT_CAPACITY && c.pcm.data==original && live==before && !h.cursor && s.bytes==0);
            assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"copy")==PT_EDIT_OK);
        } else success=1;
        assert(!c.pcm.data && d.project.samples[31].pcm.data!=original);
        assert(!memcmp(d.project.samples[31].pcm.data,values,sizeof(values)));
        pt_pattern_history_release(&h);pt_sampler_release(&s);pt_document_release(&d);assert(!live);
    }
    assert(success);
}
static void removed_recording_pin(void)
{
    unsigned bits;
    for(bits=8;bits<=24;bits+=8) {
        struct pt_capture c={0};struct pt_document d;struct pt_sampler s;
        struct pt_pattern_history h;struct pt_pattern_command commands[1];struct pt_event_change changes[1];
        struct pt_sample_version *old_pin,*new_pin;struct pt_pcm old_pcm,new_pcm;
        int32_t values[2]={(1L<<(bits-2))+1,-1};struct pt_pcm chunk={values,2,2,48000,1,(uint8_t)bits};
        unsigned generation;size_t before;
        pt_document_init(&d,&allocator);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        pt_sampler_init(&s,&allocator,1024*1024);
        assert(pt_pattern_history_init(&h,&d.project,commands,1,changes,1)==PT_EDIT_OK);
        assert(pt_capture_open(&c,&allocator,bits,1,48000,8,32)==PT_CAPTURE_OK);
        assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_OK && pt_capture_finish(&c)==PT_CAPTURE_OK);
        assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"first")==PT_EDIT_OK);
        generation=s.generation;
        assert(pt_sampler_pin(&s,&d.project,31,generation,&old_pcm,&old_pin)==PT_EDIT_OK);
        assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && d.project.sample_count==31);
        values[0]=-values[0];values[1]=1;
        assert(pt_capture_open(&c,&allocator,bits,1,48000,8,32)==PT_CAPTURE_OK);
        assert(pt_capture_append(&c,&chunk)==PT_CAPTURE_OK && pt_capture_finish(&c)==PT_CAPTURE_OK);
        /* Reuse the removed slot and discard the old recording's redo entry. */
        assert(pt_sampler_capture_append(&s,&d.project,&h,&c,"second")==PT_EDIT_OK);
        assert(d.project.sample_count==32 && d.project.samples[31].pcm.data!=old_pcm.data);
        assert(pt_sampler_pin(&s,&d.project,31,generation,&new_pcm,&new_pin)==PT_EDIT_CONFLICT);
        assert(pt_sampler_pin(&s,&d.project,31,s.generation,&new_pcm,&new_pin)==PT_EDIT_OK);
        assert(old_pcm.data[0]==-values[0] && old_pcm.data[1]==-1);
        assert(new_pcm.data[0]==values[0] && new_pcm.data[1]==1);
        before=s.bytes;pt_sampler_unpin(old_pin);assert(before-s.bytes>=32);
        /* Evict the new append command while current and playback still own it. */
        assert(pt_pattern_title_apply(&d.project,&h,"evict")==PT_EDIT_OK);
        assert(!memcmp(new_pcm.data,values,sizeof(values)));
        pt_pattern_history_release(&h);pt_sampler_release(&s);
        assert(s.bytes>=32 && !memcmp(new_pcm.data,values,sizeof(values)));
        pt_sampler_unpin(new_pin);assert(!s.bytes);pt_document_release(&d);assert(!live);
    }
}
int main(void)
{
    formats();refusals();publication();transfer_ownership();allocator_fallback();removed_recording_pin();assert(!live);
    puts("CAPTURE STAGING PASS: bounded exact8/16/24 mono/stereo chunks, explicit overrun, atomic owned-buffer publication, capacity charging, pinned metadata/undo lifetime, removed-slot reuse and redo eviction, copy fallback, allocation rollback and exact project roundtrip; synthetic input only");return 0;
}
