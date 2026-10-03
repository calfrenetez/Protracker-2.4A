#ifndef PT_INVERT_PREPARE_EMBEDDED
#include <assert.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_invert.h"
#include "document.h"
struct preparation_memory {unsigned calls,fail,live,refuse;void *allocation[5];};
static void *preparation_allocate(void *ctx,size_t bytes)
{
    struct preparation_memory *m=ctx;void *p;
    if(m->refuse || ++m->calls==m->fail)return NULL;
    p=malloc(bytes);if(p){assert(m->live<5);m->allocation[m->live++]=p;}return p;
}
static void preparation_release(void *ctx,void *p)
{
    struct preparation_memory *m=ctx;unsigned i;
    for(i=0;i<m->live;++i)if(m->allocation[i]==p)break;
    assert(i<m->live);m->allocation[i]=m->allocation[--m->live];free(p);
}
static void invert_preparation_fixture(void)
{
    struct preparation_memory memory={0};struct pt_allocator allocator={&memory,preparation_allocate,preparation_release};
    struct pt_project project={0};struct pt_sample samples[2]={0};struct pt_event events[64*4]={0};
    uint16_t orders[5]={0,0,0,0,0};struct pt_render_options options={0};
    static int32_t master[8192];int32_t second[8]={17,-93,30,40,1,99,-77,27},original[8];
    struct pt_render_invert_session *session=NULL;const struct pt_pcm *block;
    unsigned i,phase,ready,done,steps,total=0;
    for(i=0;i<8192;++i)master[i]=(int)(i%256)-128;
    memcpy(original,second,sizeof(second));pt_channels_init(&project.channels);
    project.samples=samples;project.sample_count=2;project.events=events;project.orders=orders;
    project.order_count=5;project.pattern_count=1;project.speed=1;project.bpm=125;
    samples[0].pcm=(struct pt_pcm){master,8192,8192,48000,1,8};samples[0].volume=64;
    samples[0].loop=PT_LOOP_FORWARD;samples[0].loop_end=8192;
    samples[1]=samples[0];samples[1].pcm=(struct pt_pcm){second,8,8,48000,1,8};samples[1].loop=PT_LOOP_NONE;samples[1].loop_end=0;
    events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,14,0xff,0,0};
    events[1]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,14,0xff,0,0};
    options.rate=48000;options.bits=24;options.tracks=3;options.gain_q16=65536;
    options.tick_limit=1000;options.frame_limit=1000000;
    /* Eight bounded copies of the large sample, one of the short one, and two
     * measurement batches. Exercise close at every pending/completed phase. */
    for(phase=0;phase<=14;++phase) {
        memory.calls=0;ready=0;
        assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,&session)==PT_RENDER_OK);
        assert(memory.live==5 && memory.calls==5);memory.refuse=1;
        for(steps=0;steps<phase;++steps) {
            block=(const struct pt_pcm *)1;done=77;
            assert(pt_render_invert_pull(session,256,&block,&done)==PT_RENDER_INVALID && !block && !done);
            assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_OK);
            assert(ready==(steps==13) && memory.live==5 && memory.calls==5);
        }
        if(phase==14)assert(ready);
        assert(!memcmp(second,original,sizeof(second)));
        for(i=0;i<8192;++i)assert(master[i]==(int)(i%256)-128);
        pt_render_invert_close(session);session=NULL;memory.refuse=0;assert(!memory.live);
    }
    assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,&session)==PT_RENDER_OK);
    assert(pt_render_invert_prepare(session,(unsigned *)master)==PT_RENDER_INVALID && master[0]==-128);
    assert(pt_render_invert_prepare(session,(unsigned *)second)==PT_RENDER_INVALID && second[0]==17);
    assert(pt_render_invert_prepare(session,(unsigned *)&project)==PT_RENDER_INVALID);
    assert(pt_render_invert_prepare(session,(unsigned *)samples)==PT_RENDER_INVALID);
    assert(pt_render_invert_prepare(session,(unsigned *)events)==PT_RENDER_INVALID);
    assert(pt_render_invert_prepare(session,(unsigned *)orders)==PT_RENDER_INVALID);
    for(i=0;i<memory.live;++i) {
        unsigned char saved[sizeof(unsigned)];memcpy(saved,memory.allocation[i],sizeof(saved));
        assert(!pt_render_invert_output_disjoint(session,memory.allocation[i],sizeof(unsigned)));
        assert(pt_render_invert_prepare(session,memory.allocation[i])==PT_RENDER_INVALID);
        assert(!memcmp(saved,memory.allocation[i],sizeof(saved)));
    }
    assert(!pt_render_invert_output_disjoint(NULL,&ready,sizeof(ready)));
    ready=77;assert(pt_render_invert_prepare(NULL,&ready)==PT_RENDER_INVALID && ready==77);
    for(steps=0,ready=0;!ready;++steps){assert(steps<14);assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_OK);}
    assert(steps==14);assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_OK && ready);
    pt_render_invert_stop(session);assert(memory.live==1);
    ready=77;assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_INVALID && !ready);
    block=(const struct pt_pcm *)1;done=77;
    assert(pt_render_invert_pull(session,256,&block,&done)==PT_RENDER_OK && !block && done);
    pt_render_invert_close(session);session=NULL;assert(!memory.live);
    for(i=1;i<=5;++i) {
        memory.calls=0;memory.fail=i;
        assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,&session)==PT_RENDER_MEMORY);
        assert(!session && !memory.live);
    }
    memory.fail=0;assert(pt_render_invert_begin(&project,&options,1,&allocator,&session)==PT_RENDER_MEMORY && !session && !memory.live);
    assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,(struct pt_render_invert_session **)master)==PT_RENDER_INVALID && !memory.live && master[0]==-128);
    assert(pt_render_invert_open(&project,&options,SIZE_MAX,&allocator,(struct pt_render_invert_session **)second)==PT_RENDER_INVALID && !memory.live && second[0]==17);
    /* A late measurement refusal must never make the copied bank playable. */
    options.tick_limit=10;
    assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,&session)==PT_RENDER_OK);
    for(steps=0;steps<12;++steps)assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_OK && !ready);
    ready=77;assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_TICK_LIMIT && !ready && memory.live==1);
    assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_TICK_LIMIT && !ready);
    block=(const struct pt_pcm *)1;done=0;
    assert(pt_render_invert_pull(session,256,&block,&done)==PT_RENDER_TICK_LIMIT && !block && done);
    pt_render_invert_close(session);session=NULL;assert(!memory.live);
    /* Descriptor staleness during copying releases the unpublished bank. */
    options.tick_limit=1000;
    assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,&session)==PT_RENDER_OK);
    assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_OK && !ready);
    samples[0].pcm.rate=44100;
    assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_SAMPLE && !ready && memory.live==1);
    samples[0].pcm.rate=48000;
    assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_SAMPLE && !ready);
    pt_render_invert_close(session);session=NULL;assert(!memory.live);
    /* A short actual pull remains usable after incremental preparation. The
     * full mixed first-word/offline parity oracle lives in the session test. */
    project.order_count=1;events[8].effect=15;events[8].parameter=0;
    assert(pt_render_invert_begin(&project,&options,SIZE_MAX,&allocator,&session)==PT_RENDER_OK);
    for(steps=0,ready=0;!ready;++steps){assert(steps<14);assert(pt_render_invert_prepare(session,&ready)==PT_RENDER_OK);}
    assert(steps==13);done=0;memory.refuse=1;
    for(steps=0;!done;++steps) {
        assert(steps<64);
        assert(pt_render_invert_pull(session,256,&block,&done)==PT_RENDER_OK);
        if(block){assert(block->frames<=256 && block->bits==24);total+=block->frames;}
    }
    memory.refuse=0;assert(total && memory.live==1);pt_render_invert_close(session);assert(!memory.live);
    assert(!memcmp(second,original,sizeof(second)));
    for(i=0;i<8192;++i)assert(master[i]==(int)(i%256)-128);
}
#ifndef PT_INVERT_PREPARE_EMBEDDED
int main(void)
{invert_preparation_fixture();puts("INVERT PREPARATION PASS: bounded copy/measurement, all-phase cancellation, aliases, late refusal and immutable masters");return 0;}
#endif
