#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_invert.h"
#include "document.h"
#ifdef __amigaos__
/* Diagnostic failures must return to the harness, not open a fatal requester. */
static void test_failure(const char *condition,unsigned line)
{fprintf(stderr,"INVERT render FAIL: line=%u condition=%s\n",line,condition);exit(20);}
#undef assert
#define assert(condition) ((condition)?(void)0:test_failure(#condition,__LINE__))
#endif

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
/* Independent frame oracle: reference mt_Init zeros word zero; mt_FunkIt
   flips byte 1 at tick zero, then non-fresh ticks alternate bytes 0 and 1.
   Fresh rows do not advance an inherited EFx clock; EF0 preserves the bytes. */
struct one_shot_output {uint64_t frames;unsigned mode;};
static int receive_one_shot(void *ctx,const struct pt_pcm *p,uint64_t offset)
{
    static const int word[9][2]={{0,-1},{-1,-1},{-1,0},{-1,0},{0,0},{0,-1},{0,-1},{0,-1},{0,-1}};
    struct one_shot_output *s=ctx;unsigned i;
    assert(offset==s->frames);
    for(i=0;i<p->frames;++i) {
        uint64_t frame=offset+i;unsigned tick=(unsigned)(frame/960),within=(unsigned)(frame%960);
        int restart=tick==0 || (s->mode==1 && tick==3) ||
            (s->mode==2 && tick>=3 && tick<=5) || (s->mode==3 && tick==4);
        int left=(restart && within>=2 && within<4)?(int)(within+1)*10:word[tick][frame%2];
        assert(tick<9);
        if(p->data[i*2]!=left*65536)fprintf(stderr,"oneshot mode=%u frame=%lu got=%ld expected=%d\n",s->mode,(unsigned long)frame,(long)p->data[i*2],left*65536);
        assert(p->data[i*2]==left*65536);
        assert(p->data[i*2+1]==(int)((frame%4+1)*10)*65536);
    }
    s->frames+=p->frames;return 1;
}
static void one_shots(void)
{
    struct pt_project p={0};struct pt_sample samples[2];struct pt_event events[256]={0};uint16_t order=0;
    int32_t pcm[2][4]={{17,-93,30,40},{10,20,30,40}},original[2][4];
    struct pt_render_options o={0};struct pt_render_report r;struct state state={0};
    struct pt_allocator a={&state,allocate,release};unsigned mode,i;size_t budget;
    memset(samples,0,sizeof(samples));
    pt_channels_init(&p.channels);p.channels.track[0].pan=0;p.channels.track[1].pan=255;
    p.samples=samples;p.sample_count=2;p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;p.speed=3;p.bpm=125;
    for(i=0;i<2;++i) {
        samples[i].pcm.data=pcm[i];samples[i].pcm.capacity=samples[i].pcm.frames=4;
        samples[i].pcm.rate=48000;samples[i].pcm.bits=8;samples[i].pcm.channels=1;samples[i].volume=64;
    }
    samples[1].loop=PT_LOOP_FORWARD;samples[1].loop_end=4;
    o.rate=48000;o.bits=24;o.tracks=3;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    memcpy(original,pcm,sizeof(pcm));
    for(mode=0;mode<5;++mode) {
        struct one_shot_output out={0,mode};memset(events,0,sizeof(events));memset(&state,0,sizeof(state));
        events[0].kind=events[1].kind=PT_NOTE_PERIOD;events[0].pitch=events[1].pitch=428;
        events[0].instrument=1;events[1].instrument=2;
        events[mode==4?2:0].effect=14;events[mode==4?2:0].parameter=255;
        if(mode==4) {events[2].instrument=1;p.channels.track[2].muted=1;}
        if(mode==1 || mode==3) {events[4].kind=PT_NOTE_PERIOD;events[4].pitch=428;}
        if(mode==2 || mode==3) {events[4].effect=14;events[4].parameter=mode==2?0x91:0xd1;}
        events[mode==4?10:8].effect=14;events[mode==4?10:8].parameter=240;events[12].effect=15;
        assert(pt_render_invert_stream(&p,&o,receive_one_shot,&out,NULL,NULL,&r,SIZE_MAX,&a)==PT_RENDER_OK);
        assert(out.frames==8640 && r.frames==8640 && !state.live && !memcmp(pcm,original,sizeof(pcm)));
        budget=state.sample_bytes;
        for(i=1;i<=4;++i) {
            memset(&state,0,sizeof(state));state.fail=i;out.frames=0;
            assert(pt_render_invert_stream(&p,&o,receive_one_shot,&out,NULL,NULL,&r,budget,&a)==PT_RENDER_MEMORY);
            assert(!state.live && !out.frames && !memcmp(pcm,original,sizeof(pcm)));
        }
        memset(&state,0,sizeof(state));out.frames=0;
        assert(pt_render_invert_stream(&p,&o,receive_one_shot,&out,NULL,NULL,&r,budget-1,&a)==PT_RENDER_MEMORY);
        assert(!state.live && !out.frames && !memcmp(pcm,original,sizeof(pcm)));
    }
}
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
    o.tracks=1;memset(&s,0,sizeof(s));assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_OK && s.frames==5760 && !s.live);
    /* Only the unselected channel mutates the shared sample. Mute and solo
       must not suppress its clock or change the selected channel's PCM. */
    events[0].effect=events[0].parameter=0;events[4].effect=events[4].parameter=0;
    events[1].instrument=1;events[1].effect=14;events[1].parameter=255;
    events[5].effect=14;events[5].parameter=240;
    for(i=0;i<3;++i) {
        p.channels.track[1].muted=(i==1);p.channels.track[0].solo=(i==2);
        memset(&s,0,sizeof(s));assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_OK && s.frames==5760 && !s.live);
        assert(!memcmp(pcm,original,sizeof(pcm)));
    }
    p.channels.track[1].muted=p.channels.track[0].solo=0;
    memset(&s,0,sizeof(s));o.tracks=15;
    sample.pcm.bits=24;assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_SAMPLE && !s.calls);
    sample.pcm.bits=8;o.tracks=15;events[0].effect=14;events[0].parameter=0;memset(&s,0,sizeof(s));r=before;
    assert(pt_render_invert_stream(&p,&o,receive,&s,NULL,NULL,&r,budget,&a)==PT_RENDER_EFFECT && !s.sinks && !s.live && !memcmp(&r,&before,sizeof(r)));
    one_shots();
    puts("INVERT render PASS: mixed loop/one-shot, retrigger/delay, exact PCM, immutable master, repeatability, budgets, allocation failures, cancellation, sink failure");return 0;
}
