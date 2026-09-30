#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/mixed_preflight.h"
static unsigned live,calls,fail;
static void *allocate(void *c,size_t n){void *p;(void)c;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p){(void)c;assert(p && live);--live;free(p);}
static void fixture(unsigned bits,unsigned cache_bits)
{
    struct pt_project p={0};struct pt_sample samples[3];
    struct pt_event events[64*16]={{0}};uint16_t order=0;
    int32_t pcm[16]={1,2,3,4},stereo[32]={0};
    struct pt_render_options o={0};struct pt_render_report measured;
    struct pt_paula_render_caps caps={3546895,124,65535};
    struct pt_playback_format format={0};struct pt_allocator a={NULL,allocate,release};
    struct pt_mixed_report r;struct pt_render_sequence *s=NULL,*sentinel=(void *)(uintptr_t)1;
    struct pt_render_interval span;struct pt_render_plan *plan;uint64_t frames;
    unsigned i,before,pass;uint16_t held=0;struct pt_wavetable_preflight_report wr;struct pt_voice v;
    memset(samples,0,sizeof(samples));pt_channels_init(&p.channels);p.channels.count=16;
    for(i=0;i<16;++i)p.channels.track[i].route=PT_AMIGUS;
    p.channels.track[4].route=PT_PAULA;p.channels.track[4].pan=0;
    p.samples=samples;p.sample_count=3;p.orders=&order;p.order_count=p.pattern_count=1;
    p.events=events;p.speed=1;p.bpm=125;
    for(i=0;i<3;++i){samples[i].pcm=(struct pt_pcm){pcm,16,16,8000,1,(uint8_t)bits};samples[i].volume=64;}
    samples[2].pcm=(struct pt_pcm){stereo,32,16,8000,2,(uint8_t)bits};
    events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    events[7]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    events[16+15].effect=15;events[16+15].parameter=150;
    events[32+15].effect=14;events[32+15].parameter=0xe1;
    events[48+15].effect=15; /* Unselected global F00 ends both backend tracks. */
    o.rate=48000;o.bits=24;o.tracks=(1U<<4)|(1U<<7);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    format.bits=cache_bits;
    assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    before=calls;
    assert(pt_mixed_preflight(&p,&o,NULL,&caps,&format,1,1,&a,&r,&s)==PT_MIXED_OK);
    assert(calls==before+2 && live==1 && s && r.frames==measured.frames && r.intervals>1);
    assert(r.map[4]==0 && r.samples[0][0] && !r.samples[0][1] && r.samples[1][1] && !r.samples[1][0]);
    plan=malloc(sizeof(*plan));assert(plan);
    for(pass=0;pass<2;++pass){frames=0;do {
        assert(pt_render_sequence_next(s,&span)==PT_RENDER_OK);
        {uint32_t left=span.frames;while(left){uint32_t n=left>256?256:left;assert(pt_render_sequence_consume(s,n)==PT_RENDER_OK);left-=n;}frames+=span.frames;}
        assert(pt_render_sequence_complete(s,plan)==PT_RENDER_OK);
        for(i=0;i<plan->count;++i)assert(plan->action[i].channel==4 || plan->action[i].channel==7);
    }while(!span.end);assert(frames==measured.frames);assert(pt_render_sequence_rewind(s)==PT_RENDER_OK);}
    pt_render_sequence_close(s);assert(!live);
    events[16+15].parameter=125;
    assert(pt_mixed_preflight(&p,&o,NULL,&caps,&format,1,1,&a,&r,NULL)==PT_MIXED_OK && r.frames>measured.frames);
    events[16+15].parameter=150;
    for(pass=0;pass<2;++pass){unsigned ch=pass?7:4;s=sentinel;
        events[32+ch]=(struct pt_event){428,0,PT_NOTE_PERIOD,3,0,0,0,0};
        assert(pt_mixed_preflight(&p,&o,NULL,&caps,&format,1,1,&a,&r,&s)==(pass?PT_MIXED_AMIGUS:PT_MIXED_PAULA));
        assert(r.channel==ch && r.kind==PT_RENDER_TRIGGER && r.intervals>1 && s==sentinel && !live);
        for(i=0;i<PT_PROJECT_SAMPLES;++i)assert(!r.samples[0][i] && !r.samples[1][i]);
        events[32+ch]=(struct pt_event){0};
    }
    before=calls;o.row_range=1;
    assert(pt_mixed_preflight(&p,&o,NULL,&caps,&format,1,1,&a,&r,NULL)==PT_MIXED_RANGE && calls==before);
    o.row_range=0;p.channels.track[7].route=PT_MIDI;
    assert(pt_mixed_preflight(&p,&o,NULL,&caps,&format,1,1,&a,&r,NULL)==PT_MIXED_ROUTE && calls==before);
    p.channels.track[7].route=PT_AMIGUS;
    for(i=1;i<=2;++i){fail=calls+i;s=sentinel;
        assert(pt_mixed_preflight(&p,&o,NULL,&caps,&format,1,1,&a,&r,&s)==PT_MIXED_MEMORY && !live && s==sentinel);fail=0;}
    /* Public AmiGUS batch gate rejects a foreign descriptor before reading it,
     * and never commits the first trigger's held bit or source mask on refusal. */
    assert(pt_voice_init(&v,&samples[1].pcm,0,16,PT_VOICE_ONCE,0,0,((uint64_t)8000<<32)/48000,0)==PT_PCM_OK);
    plan->count=2;plan->action[0]=(struct pt_render_action){PT_RENDER_TRIGGER,7,v,{32768,32768}};
    plan->action[1]=plan->action[0];plan->action[1].voice.pcm=(void *)(uintptr_t)1;
    assert(pt_wavetable_check_plan(&p,48000,plan,&format,1,&held,&wr)==PT_WAVETABLE_SOURCE && !held && !wr.samples[1]);
    plan->count=1;assert(pt_wavetable_check_plan(&p,48000,plan,&format,1,&held,&wr)==PT_WAVETABLE_COMPATIBLE && held==(1U<<7));
    free(plan);assert(!live && samples[0].pcm.bits==bits && samples[1].pcm.bits==bits && pcm[0]==1);
}
int main(void){fixture(8,8);fixture(16,16);fixture(24,8);fixture(24,16);puts("MIXED PREFLIGHT PASS: one global timeline, separate source masks, late backend refusal, transfer and allocation cleanup; no dispatch");return 0;}
