#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render.h"
struct capture {const int32_t *source;uint64_t frames;unsigned calls,silence,start,length,once,bits,muted,fail,gain,slide,zero_after;};
static int receive(void *ctx,const struct pt_pcm *block,uint64_t offset)
{
    struct capture *c=ctx;uint32_t i;unsigned side;
    if(c->fail && c->calls==c->fail)return 0;
    assert(offset==c->frames && block->frames && block->frames<=256 && block->rate==48000 && block->channels==2 && block->bits==c->bits);
    for(i=0;i<block->frames;++i)for(side=0;side<2;++side) {
        uint64_t absolute=offset+i;int32_t expected=0;
        if(!c->muted && absolute>=c->silence && (!c->zero_after || absolute<c->zero_after)) {
            uint64_t frame=absolute-c->silence;
            if(!c->once || frame<c->length)expected=c->source[(c->start+frame%c->length)*2+side];
        }
        {uint32_t gain=c->slide?(64-(unsigned)(absolute/960))*1024:c->gain;
            int64_t n=(int64_t)expected*gain;expected=(int32_t)(n<0?-((-n+32768)/65536):(n+32768)/65536);}
        if(c->bits==16)expected=expected<0?-((-expected+128)/256):(expected+128)/256;
        assert(block->data[i*2+side]==expected);
    }
    c->frames+=block->frames;++c->calls;return 1;
}
static int cancel(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{unsigned mode=*(unsigned *)ctx;(void)ticks;return mode==1?phase!=PT_RENDER_ANALYSE:phase!=PT_RENDER_MIX || frames<512;}
static void reset_capture(struct capture *c,const int32_t *source)
{memset(c,0,sizeof(*c));c->source=source;c->length=7;c->bits=24;c->gain=65536;}
int main(void)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[2*64*4];uint16_t orders[2]={0,1};
    int32_t pcm[14];uint32_t slices[2]={1,4};struct pt_render_options o;
    struct pt_render_report report,before;struct capture c;unsigned i,mode;
    memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    for(i=0;i<7;++i) {pcm[i*2]=(int32_t)i+1;pcm[i*2+1]=-(int32_t)i-1;}
    pt_channels_init(&p.channels);p.channels.track[0].pan=128;p.events=events;p.orders=orders;p.order_count=1;p.pattern_count=2;
    p.samples=&sample;p.sample_count=1;p.speed=1;p.bpm=125;
    sample.pcm.data=pcm;sample.pcm.capacity=14;sample.pcm.frames=7;sample.pcm.rate=48000;sample.pcm.channels=2;sample.pcm.bits=24;
    sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=7;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;events[4].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=1000;o.frame_limit=1000000;
    assert(pt_project_validate(&p,NULL)==PT_PROJECT_OK);reset_capture(&c,pcm);
    assert(pt_render_measure(&p,&o,NULL,NULL,&report)==PT_RENDER_OK && report.frames==960 && report.ticks==2 && report.end==PT_RENDER_F00);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960 && report.frames==960 && !report.clipped);
    o.include_lead_in=1;reset_capture(&c,pcm);c.silence=960;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==1920);o.include_lead_in=0;
    /* A delayed row must keep its voice phase, not retrigger every tick zero. */
    events[0].effect=14;events[0].parameter=0xe2;reset_capture(&c,pcm);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==2880 && report.ticks==4);
    events[0].effect=events[0].parameter=0;
    /* Whole pattern completes only after the final row's interval. */
    events[4].effect=0;reset_capture(&c,pcm);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==64*960 && report.ticks==65 && report.end==PT_RENDER_POSITION_RETURN);
    events[0].effect=11;events[0].parameter=0;reset_capture(&c,pcm);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960 && report.ticks==2 && report.end==PT_RENDER_POSITION_RETURN);
    events[0].effect=0;events[4].effect=15;
    /* Slice 1 spans [1,4), so the whole-sample loop is outside and disabled. */
    sample.slices=slices;sample.slice_count=2;events[0].slice=1;reset_capture(&c,pcm);c.start=1;c.length=3;c.once=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);
    sample.slices=NULL;sample.slice_count=0;events[0].slice=0;
    /* Native and host run the same final 16-bit output expectations. */
    for(i=0;i<7;++i) {pcm[i*2]=128+(int32_t)i*256;pcm[i*2+1]=-128-(int32_t)i*256;}
    o.bits=16;reset_capture(&c,pcm);c.bits=16;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);o.bits=24;
    events[0].effect=12;events[0].parameter=32;reset_capture(&c,pcm);c.gain=32768;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);
    events[0].effect=10;events[0].parameter=1;p.speed=3;reset_capture(&c,pcm);c.slide=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==2880);p.speed=1;
    events[0].effect=events[0].parameter=0;events[0].flags=1;events[0].velocity=64;reset_capture(&c,pcm);c.gain=65536UL*64/127;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);events[0].flags=events[0].velocity=0;
    events[4].effect=0;events[4].kind=PT_NOTE_OFF;events[8].effect=15;reset_capture(&c,pcm);c.zero_after=960;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==1920);
    events[4].effect=15;events[4].kind=PT_NOTE_NONE;events[8].effect=0;
    p.channels.track[0].muted=1;reset_capture(&c,pcm);c.muted=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);p.channels.track[0].muted=0;
    p.channels.track[1].solo=1;reset_capture(&c,pcm);c.muted=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);p.channels.track[1].solo=0;
    /* Other tracks' global commands apply even when their audio is excluded. */
    events[4].effect=0;events[5].effect=15;reset_capture(&c,pcm);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);events[5].effect=0;events[4].effect=15;
    /* Pattern selection does not reject an unsupported unused pattern. */
    memcpy(events+64*4,events,64*4*sizeof(*events));events[0].effect=9;o.pattern_only=1;o.pattern=1;reset_capture(&c,pcm);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==960);o.pattern_only=0;
    memset(&report,0x55,sizeof(report));before=report;reset_capture(&c,pcm);
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_EFFECT && !c.calls && !memcmp(&report,&before,sizeof(report)));events[0].effect=0;
    sample.finetune=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_SAMPLE && !c.calls && !memcmp(&report,&before,sizeof(report)));sample.finetune=0;
    p.channels.track[0].route=PT_MIDI;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_ROUTE && !c.calls && !memcmp(&report,&before,sizeof(report)));p.channels.track[0].route=PT_PAULA;
    events[0].kind=PT_NOTE_NONE;events[0].pitch=0;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_EFFECT && !c.calls);events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;
    o.tick_limit=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_TICK_LIMIT && !c.calls && !memcmp(&report,&before,sizeof(report)));o.tick_limit=1000;
    o.frame_limit=1919;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_FRAME_LIMIT && !c.calls && !memcmp(&report,&before,sizeof(report)));o.frame_limit=1000000;
    mode=1;
    assert(pt_render_stream(&p,&o,receive,&c,cancel,&mode,&report)==PT_RENDER_CANCELLED && !c.calls && !memcmp(&report,&before,sizeof(report)));
    mode=2;
    assert(pt_render_stream(&p,&o,receive,&c,cancel,&mode,&report)==PT_RENDER_CANCELLED && c.frames==512 && c.calls==2 && !memcmp(&report,&before,sizeof(report)));
    reset_capture(&c,pcm);c.fail=1;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_SINK && c.frames==256 && !memcmp(&report,&before,sizeof(report)));
    for(i=0;i<7;++i)assert(pcm[i*2]==128+(int32_t)i*256 && pcm[i*2+1]==-128-(int32_t)i*256);
    puts("RENDER PASS: planned/streamed stereo16/24 PCM, startup trim, delayed voice continuity, pattern/song endings, slice bounds, selection, preflight refusal, cancellation and sink failures");return 0;
}
