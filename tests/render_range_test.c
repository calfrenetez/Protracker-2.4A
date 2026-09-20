#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render.h"
struct capture {int32_t data[40000];uint64_t frames;unsigned calls;};
static struct capture full,part;
static int receive(void *ctx,const struct pt_pcm *p,uint64_t offset)
{
    struct capture *c=ctx;assert(offset==c->frames && (offset+p->frames)*2<=40000);
    memcpy(c->data+offset*2,p->data,p->frames*2*sizeof(int32_t));c->frames+=p->frames;++c->calls;return 1;
}
static int cancel(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{(void)ctx;return !(phase==PT_RENDER_MIX && ticks>2 && !frames);}
int main(void)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[256];uint16_t order=0;
    int32_t data[5]={1000000,-2000000,3000000,-4000000,5000000};struct pt_render_options o;
    struct pt_render_report report,before;unsigned i;
    memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    pt_channels_init(&p.channels);p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;p.samples=&sample;p.sample_count=1;p.speed=2;p.bpm=125;
    sample.pcm.data=data;sample.pcm.frames=sample.pcm.capacity=5;sample.pcm.bits=24;sample.pcm.channels=1;sample.pcm.rate=48000;
    sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=5;sample.interpolation=1;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[4].kind=PT_NOTE_PERIOD;events[4].pitch=214;events[4].effect=3;events[4].parameter=5;
    events[8].effect=0;events[8].parameter=0x37;events[13].effect=14;events[13].parameter=0xe1;
    events[16].effect=15;
    o.pattern_only=1;o.rate=48000;o.bits=24;o.gain_q16=32768;o.tracks=1;o.tick_limit=200;o.frame_limit=100000;
    assert(pt_render_stream(&p,&o,receive,&full,NULL,NULL,&report)==PT_RENDER_OK && full.frames==9600);
    o.row_range=1;o.row_first=1;o.row_end=4;
    assert(pt_render_stream(&p,&o,receive,&part,NULL,NULL,&report)==PT_RENDER_OK && part.frames==7680 && report.end==PT_RENDER_F00);
    assert(!memcmp(part.data,full.data+1920*2,part.frames*2*sizeof(int32_t)));
    memset(&part,0,sizeof(part));o.row_end=3;
    assert(pt_render_stream(&p,&o,receive,&part,NULL,NULL,&report)==PT_RENDER_OK && part.frames==3840 && report.end==PT_RENDER_ROW_EXIT);
    assert(!memcmp(part.data,full.data+1920*2,part.frames*2*sizeof(int32_t)));
    before=report;memset(&part,0,sizeof(part));o.row_first=5;o.row_end=6;
    assert(pt_render_stream(&p,&o,receive,&part,NULL,NULL,&report)==PT_RENDER_EMPTY_RANGE && !part.calls && !memcmp(&report,&before,sizeof(report)));
    o.row_first=2;o.row_end=4;
    assert(pt_render_stream(&p,&o,receive,&part,cancel,NULL,&report)==PT_RENDER_CANCELLED && !part.calls && !memcmp(&report,&before,sizeof(report)));
    for(i=0;i<4;++i) {
        struct pt_render_options bad=o;
        if(i==0)bad.pattern_only=0;else if(i==1)bad.include_lead_in=1;else if(i==2)bad.row_end=bad.row_first;else bad.row_end=65;
        assert(pt_render_measure(&p,&bad,NULL,NULL,&report)==PT_RENDER_INVALID && !memcmp(&report,&before,sizeof(report)));
    }
    /* Native row loops wholly inside selection stay; leaving below it ends. */
    memset(events+4,0,12*sizeof(*events));events[5].effect=14;events[5].parameter=0x60;events[9].effect=14;events[9].parameter=0x62;
    o.row_first=1;o.row_end=3;
    assert(pt_render_measure(&p,&o,NULL,NULL,&report)==PT_RENDER_OK && report.frames==11520 && report.end==PT_RENDER_ROW_EXIT);
    events[5].effect=events[5].parameter=0;events[1].effect=14;events[1].parameter=0x60;events[9].parameter=0x61;
    assert(pt_render_measure(&p,&o,NULL,NULL,&report)==PT_RENDER_OK && report.frames==3840 && report.end==PT_RENDER_ROW_EXIT);
    /* Full-pattern bounds preserve ordinary output exactly. */
    memset(events+1,0,15*sizeof(*events));o.row_first=0;o.row_end=64;memset(&part,0,sizeof(part));memset(&full,0,sizeof(full));
    assert(pt_render_stream(&p,&o,receive,&part,NULL,NULL,&report)==PT_RENDER_OK);
    o.row_range=0;assert(pt_render_stream(&p,&o,receive,&full,NULL,NULL,&report)==PT_RENDER_OK);
    assert(part.frames==full.frames && !memcmp(part.data,full.data,full.frames*2*sizeof(int32_t)));
    puts("RANGE PASS: exact full-render crop with pre-roll pitch/phase, delays, loop boundaries, cancellation, empty/invalid refusal and full-range identity");return 0;
}
