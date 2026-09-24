#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_invert.h"
#include "document.h"
struct state {unsigned calls,fail,live,sinks,cancel;size_t sample_bytes;uint64_t frames;};
static void *allocate(void *ctx,size_t n){struct state *s=ctx;void *p;++s->calls;if(s->calls<=3)s->sample_bytes+=n;if(s->calls==s->fail)return NULL;p=malloc(n);if(p)++s->live;return p;}
static void release(void *ctx,void *p){struct state *s=ctx;assert(s->live);--s->live;free(p);}
static int receive(void *ctx,const struct pt_pcm *p,uint64_t offset)
{
    struct state *s=ctx;unsigned i;assert(offset==s->frames);
    for(i=0;i<p->frames;++i){uint64_t frame=offset+i;unsigned tick=(unsigned)(frame/960),index=(unsigned)(frame%4);int value=(int)(index+1)*10;
        if(index && index<=tick+1)value=-1-value;
        assert(p->data[i*2]==value*65536 && p->data[i*2+1]==0);
    }
    ++s->sinks;s->frames+=p->frames;return s->cancel!=3;
}
static int progress(void *ctx,enum pt_render_phase phase,uint32_t ticks,uint64_t frames)
{struct state *s=ctx;(void)ticks;return !(s->cancel==1 && phase==PT_RENDER_ANALYSE) && !(s->cancel==2 && phase==PT_RENDER_MIX && frames>=256);}
int main(void)
{
    struct pt_project p={0};struct pt_sample sample={0};struct pt_event events[256]={0};uint16_t order=0;
    int32_t pcm[4]={10,20,30,40},original[4];struct pt_render_options o={0};struct pt_render_report r,before;
    struct state s={0};struct pt_allocator a={&s,allocate,release};unsigned i;size_t budget;
    pt_channels_init(&p.channels);p.channels.track[0].pan=0;p.events=events;p.orders=&order;p.order_count=p.pattern_count=p.sample_count=1;p.samples=&sample;p.speed=3;p.bpm=125;
    sample.pcm.data=pcm;sample.pcm.capacity=sample.pcm.frames=4;sample.pcm.rate=48000;sample.pcm.bits=8;sample.pcm.channels=1;sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=4;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;events[0].effect=14;events[0].parameter=255;
    events[4].effect=14;events[4].parameter=240;events[8].effect=15;
    o.rate=48000;o.bits=24;o.tracks=15;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    memcpy(original,pcm,sizeof(pcm));memset(&r,0x55,sizeof(r));before=r;
    assert(pt_render_invert_stream(&p,&o,receive,&s,progress,&s,&r,SIZE_MAX,&a)==PT_RENDER_OK);
    assert(s.calls==4 && !s.live && r.frames==5760 && s.frames==5760 && !memcmp(pcm,original,sizeof(pcm)));budget=s.sample_bytes;
    for(i=1;i<=4;++i){memset(&s,0,sizeof(s));s.fail=i;r=before;
        assert(pt_render_invert_stream(&p,&o,receive,&s,progress,&s,&r,budget,&a)==PT_RENDER_MEMORY);
        assert(!s.live && !s.sinks && !memcmp(&r,&before,sizeof(r)) && !memcmp(pcm,original,sizeof(pcm)));
    }
    memset(&s,0,sizeof(s));r=before;
    assert(pt_render_invert_stream(&p,&o,receive,&s,progress,&s,&r,budget-1,&a)==PT_RENDER_MEMORY && !s.live && !s.sinks);
    for(i=1;i<=3;++i){memset(&s,0,sizeof(s));s.cancel=i;r=before;
        assert(pt_render_invert_stream(&p,&o,receive,&s,progress,&s,&r,budget,&a)==(i==3?PT_RENDER_SINK:PT_RENDER_CANCELLED));
        assert(!s.live && !memcmp(&r,&before,sizeof(r)) && !memcmp(pcm,original,sizeof(pcm)));
    }
    memset(&s,0,sizeof(s));assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_OK && s.frames==5760 && !s.live);
    o.tracks=1;memset(&s,0,sizeof(s));assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_EFFECT && !s.calls);o.tracks=15;
    sample.pcm.bits=24;assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_SAMPLE && !s.calls);
    sample.pcm.bits=8;o.tracks=15;events[0].parameter=0;memset(&s,0,sizeof(s));r=before;
    assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_EFFECT && !s.sinks && !s.live && !memcmp(&r,&before,sizeof(r)));
    puts("INVERT render PASS: exact PCM, immutable master, repeatability, budgets, allocation failures, cancellation, sink failure");return 0;
}
