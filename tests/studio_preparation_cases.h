#ifndef PT_STUDIO_PREPARATION_CASES_H
#define PT_STUDIO_PREPARATION_CASES_H
#include <assert.h>
#include <string.h>
#include "../src/editor/sampler_song.h"
struct studio_reference {int32_t *values;size_t count,capacity;};
static int studio_capture(void *context,const struct pt_pcm *pcm,uint64_t offset)
{
    struct studio_reference *r=context;size_t n=pcm->frames*2;
    assert(offset*2==r->count && n<=r->capacity-r->count);
    memcpy(r->values+r->count,pcm->data,n*sizeof(*pcm->data));r->count+=n;return 1;
}
static void studio_preparation_fixture(const struct pt_allocator *a)
{
    unsigned mode;
    for(mode=0;mode<10;++mode) {
        struct pt_document d;struct pt_sampler sampler;struct pt_sampler_song *song=NULL;
        struct pt_render_options o={0};struct pt_render_report report;struct studio_reference reference;
        int32_t *data=a->allocate(a->context,4200*sizeof(*data));
        unsigned ready=0,done=0,i,steps=0;const struct pt_pcm *out=NULL;size_t bytes,emitted=0;
        assert(data);for(i=0;i<4200;++i)data[i]=mode==8?(int32_t)(i%127):(int32_t)(257+i*3);
        pt_document_init(&d,a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        pt_sampler_init(&sampler,a,mode==5?1:1024*1024);
        d.project.speed=1;d.project.channels.track[0].pan=0;
        d.project.samples[0].pcm=(struct pt_pcm){data,4200,2100,48000,mode==8?1:2,mode==8?8:24};
        d.project.samples[0].volume=64;d.project.samples[0].loop=mode==8?PT_LOOP_FORWARD:PT_LOOP_PINGPONG;
        d.project.samples[0].loop_start=2;d.project.samples[0].loop_end=2100;
        d.project.samples[1]=d.project.samples[0]; /* Valid but normally unused. */
        d.project.samples[2]=d.project.samples[0]; /* Always unused. */
        d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
        d.project.events[4].instrument=mode==8?2:0;d.project.events[8].effect=15;
        o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=10000;
        o.pattern_only=o.row_range=mode==7;o.row_first=1;o.row_end=2;
        if(mode==9) {static uint32_t slices[2]={0,100};d.project.samples[0].slices=slices;
            d.project.samples[0].slice_count=2;d.project.events[0].slice=2;}
        assert(pt_sampler_song_begin(&sampler,&d.project,&o,a,&song)==PT_RENDER_OK);
        assert(!sampler.bytes && d.project.samples[0].pcm.data==data);
        if(mode==0)goto cancelled;
        /* Preparation cannot publish any audio, including during source copy. */
        while(!sampler.bytes) {
            enum pt_render_result result=pt_sampler_song_pull(song,256,&out,&done);++steps;
            if(mode==5) {
                if(result!=PT_RENDER_OK) {assert(result==PT_RENDER_MEMORY && !out && done);break;}
            }else assert(result==PT_RENDER_OK);
            assert(!out && steps<1000);
        }
        if(mode==5) {assert(!sampler.bytes && d.project.samples[0].pcm.data==data);goto cancelled;}
        assert(!done && d.project.samples[0].pcm.data==data);bytes=sampler.bytes;
        if(mode==1)goto cancelled;
        assert(pt_sampler_song_prepare(song,&ready)==PT_RENDER_OK && !ready);
        assert(sampler.bytes==bytes && d.project.samples[0].pcm.data==data);
        if(mode==2)goto cancelled;
        if(mode==3 || mode==4) {
            if(mode==3)++sampler.generation;else ++d.project.bpm;
            assert(pt_sampler_song_prepare(song,&ready)==PT_RENDER_INVALID && !ready && !sampler.bytes);
            assert(pt_sampler_song_pull(song,256,&out,&done)==PT_RENDER_INVALID && !out && done);
            goto cancelled;
        }
        while(!ready) {assert(pt_sampler_song_prepare(song,&ready)==PT_RENDER_OK);assert(++steps<1000);}
        assert(d.project.samples[0].pcm.data!=data && d.project.samples[0].pcm.bits==(mode==8?8:24));
        assert(!memcmp(d.project.samples[0].pcm.data,data,(mode==8?2100:4200)*sizeof(*data)));
        assert((d.project.samples[1].pcm.data!=data)==(mode==8));assert(d.project.samples[2].pcm.data==data);
        /* Source pins prevent first-use promotion; playback needs no allocation. */
        bytes=sampler.bytes;sampler.budget=bytes;
        reference.capacity=20000;reference.count=0;reference.values=a->allocate(a->context,reference.capacity*sizeof(int32_t));assert(reference.values);
        assert(pt_render_stream(&d.project,&o,studio_capture,&reference,NULL,NULL,&report)==PT_RENDER_OK);
        d.project.channels.selected=1; /* Navigation is not a source mutation. */
        while(!done) {
            assert(pt_sampler_song_pull(song,256,&out,&done)==PT_RENDER_OK && sampler.bytes==bytes);
            if(out) {size_t n=out->frames*2;assert(n<=reference.count-emitted);
                assert(!memcmp(out->data,reference.values+emitted,n*sizeof(int32_t)));emitted+=n;}
            assert(++steps<1000);
        }
        assert(emitted==reference.count && emitted==report.frames*2);a->release(a->context,reference.values);
    cancelled:
        pt_sampler_song_stop(song);pt_sampler_song_stop(song);
        if(mode<6)assert(!sampler.bytes && d.project.samples[0].pcm.data==data);
        pt_sampler_release(&sampler);pt_document_release(&d);assert(!sampler.bytes);
        assert(pt_sampler_song_pull(song,256,&out,&done)==(mode==3 || mode==4?PT_RENDER_INVALID:mode==5?PT_RENDER_MEMORY:PT_RENDER_OK));
        assert(!out && done);pt_sampler_song_close(song);a->release(a->context,data);
    }
    puts("STUDIO PREPARATION PASS: bounded copy/cancel/stale/budget, selective pre-roll/repeat pins, stereo24/pingpong/slice parity and stop ownership");
}
#endif
