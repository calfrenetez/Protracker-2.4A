#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render.h"
struct expected {uint64_t frames,start,change;unsigned initial;};
static int receive(void *context,const struct pt_pcm *pcm,uint64_t offset)
{
    struct expected *e=context;unsigned i;assert(offset==e->frames && pcm->bits==24);
    for(i=0;i<pcm->frames;++i) {
        uint64_t frame=offset+i;int32_t value=0;
        if(frame>=e->start) {
            unsigned volume=frame<e->change?e->initial:32;
            value=(int32_t)(((frame-e->start)%31+1)*65536)*volume/64;
        }
        assert(pcm->data[2*i]==value && pcm->data[2*i+1]==0);
    }
    e->frames+=pcm->frames;return 1;
}
static int counted(void *context,const struct pt_pcm *p,uint64_t n)
{(void)p;(void)n;++*(unsigned *)context;return 1;}
int main(void)
{
    struct pt_project p;struct pt_sample samples[2];struct pt_event events[64*16];
    struct pt_render_options o;struct pt_render_report result,before;struct expected expected;
    int32_t data[31];uint16_t order=0;uint32_t marker=4;unsigned i,calls=0;
    memset(&p,0,sizeof(p));memset(samples,0,sizeof(samples));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);p.channels.track[15].pan=0;
    p.samples=samples;p.sample_count=2;p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;p.speed=1;p.bpm=125;
    for(i=0;i<31;++i)data[i]=(int32_t)(i+1)*65536;
    samples[0].pcm.data=data;samples[0].pcm.frames=samples[0].pcm.capacity=31;samples[0].pcm.bits=24;samples[0].pcm.channels=1;samples[0].pcm.rate=48000;
    samples[0].volume=32;samples[0].loop=PT_LOOP_FORWARD;samples[0].loop_end=31;samples[1]=samples[0];
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=0x8000;o.tick_limit=100;o.frame_limit=100000;
    events[15].kind=PT_NOTE_PERIOD;events[15].pitch=428;events[15].instrument=1;events[15].effect=12;events[15].parameter=16;
    events[31].instrument=1;events[32].effect=15;
    expected=(struct expected){0,0,960,16};
    assert(pt_render_stream(&p,&o,receive,&expected,NULL,NULL,&result)==PT_RENDER_OK && result.frames==1920);
    /* A different sample cannot be silently substituted into a running DMA
       voice; refusal is detected before any sink call or report mutation. */
    memset(&result,0xa5,sizeof(result));before=result;events[31].instrument=2;
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&result)==PT_RENDER_EFFECT && !calls && !memcmp(&result,&before,sizeof(result)));
    events[31].instrument=1;samples[0].slices=&marker;samples[0].slice_count=1;events[15].slice=1;
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&result)==PT_RENDER_EFFECT && !calls && !memcmp(&result,&before,sizeof(result)));
    events[15].slice=0;events[31].slice=1;
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&result)==PT_RENDER_EFFECT && !calls && !memcmp(&result,&before,sizeof(result)));
    samples[0].slices=NULL;samples[0].slice_count=0;memset(events,0,sizeof(events));
    /* Preloading a sample while no note is sounding is valid and stays silent.
       The eventual instrument-zero note uses that sample and stored volume. */
    events[15].instrument=2;events[15].effect=12;events[15].parameter=16;
    events[31].kind=PT_NOTE_PERIOD;events[31].pitch=428;events[47].instrument=2;events[48].effect=15;
    expected=(struct expected){0,960,1920,16};
    assert(pt_render_stream(&p,&o,receive,&expected,NULL,NULL,&result)==PT_RENDER_OK && result.frames==2880);
    puts("INSTRUMENT PASS: track16 true24 phase/volume reload, silent preload, cross-sample and slice refusal before output");return 0;
}
