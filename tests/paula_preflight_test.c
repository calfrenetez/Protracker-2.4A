#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/paula_preflight.h"
static unsigned live,calls,fail;
static void *allocate(void *context,size_t bytes)
{void *p;(void)context;if(++calls==fail)return NULL;p=malloc(bytes);if(p)++live;return p;}
static void release(void *context,void *p) {(void)context;assert(p && live);--live;free(p);}
static void controls(void)
{
    struct pt_paula_render_caps caps={3546895,124,65535};uint16_t period=99;uint8_t volume=99;
    uint32_t gains[2]={65536,0};unsigned slot;struct pt_pcm pcm={(int32_t *)(uintptr_t)1,16,16,8000,1,24};
    struct pt_voice v={0};struct pt_paula_render_plan plan={99,99,99,99},before;
    /* Borrowed sentinel data proves this conversion never reads sample values. */
    v.pcm=&pcm;v.active=1;v.step=((uint64_t)8000<<32)/48000;v.end=16;
    for(slot=0;slot<4;++slot) {
        unsigned side=slot==0 || slot==3?0:1;gains[side]=65536;gains[1-side]=0;
        assert(pt_paula_render_control(v.step,48000,gains,slot,&caps,&period,&volume));
        assert(period==443 && volume==64);
        assert(pt_paula_render_voice(&v,48000,gains,slot,&caps,&plan) && plan.offset==0 && plan.length==16);
    }
    gains[0]=32768;gains[1]=0;
    assert(pt_paula_render_control(v.step,48000,gains,0,&caps,&period,&volume) && volume==32);
    period=volume=99;gains[0]=1;
    assert(!pt_paula_render_control(v.step,48000,gains,0,&caps,&period,&volume) && period==99 && volume==99);
    gains[0]=0;gains[1]=65536;
    assert(!pt_paula_render_control(v.step,48000,gains,0,&caps,&period,&volume));
    gains[0]=gains[1]=0;assert(pt_paula_render_control(v.step,48000,gains,0,&caps,&period,&volume) && volume==0);
    assert(!pt_paula_render_control(UINT64_MAX,48000,gains,0,&caps,&period,&volume));
    assert(!pt_paula_render_control(0,48000,gains,0,&caps,&period,&volume));
    assert(!pt_paula_render_control(1ULL<<32,48000,gains,0,&caps,&period,&volume)); /* No period clamp. */
    assert(!pt_paula_render_control(1,48000,gains,0,&caps,&period,&volume));
    assert(!pt_paula_render_control(v.step,22050,gains,0,&caps,&period,&volume));
    caps.clock_hz=72000;caps.minimum_period=1; /* Exact quotient1.5: tie rounds upward. */
    assert(pt_paula_render_control(1ULL<<32,48000,gains,0,&caps,&period,&volume) && period==2);
    caps.clock_hz=3546895;caps.minimum_period=124;
    before=plan;v.phase=1;
    assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan) && !memcmp(&plan,&before,sizeof(plan)));
    v.phase=0;v.end=15;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
    v.end=16;v.linear=1;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
    v.linear=0;v.repeat_pcm=&pcm;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
    v.repeat_pcm=NULL;v.loop=PT_VOICE_FORWARD;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
    v.loop=0;pcm.channels=2;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
    pcm.channels=1;pcm.bits=32;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
    pcm.bits=8;pcm.frames=pcm.capacity=v.end=131070;
    assert(pt_paula_render_voice(&v,48000,gains,0,&caps,&plan) && plan.length==131070);
    pcm.frames=pcm.capacity=v.end=131072;assert(!pt_paula_render_voice(&v,48000,gains,0,&caps,&plan));
}
static void fixture(unsigned bits)
{
    struct pt_project p={0};struct pt_sample samples[2];struct pt_event events[64*16]={{0}};
    uint16_t order=0;int32_t pcm[16]={1,257,-513,799},stereo[32]={1,2,3,4};
    struct pt_render_options o={0};struct pt_render_report measured;
    struct pt_paula_render_caps caps={3546895,124,65535};struct pt_allocator a={NULL,allocate,release};
    struct pt_paula_preflight_report report,batch;struct pt_render_plan plan={0};int8_t map[16];
    uint16_t held=0;unsigned i,first_calls;struct pt_voice voice;
    memset(samples,0,sizeof(samples));for(i=0;i<16;++i){if(bits==8)pcm[i]=(int32_t)i+1;}
    pt_channels_init(&p.channels);p.channels.count=16;
    for(i=0;i<16;++i)p.channels.track[i].route=PT_AMIGUS;
    for(i=4;i<11;i+=3)p.channels.track[i].route=PT_PAULA;
    p.channels.track[14].route=PT_PAULA;
    p.channels.track[4].pan=p.channels.track[14].pan=0;p.channels.track[7].pan=p.channels.track[10].pan=255;
    p.samples=samples;p.sample_count=2;p.orders=&order;p.order_count=p.pattern_count=1;p.events=events;p.speed=3;p.bpm=125;
    samples[0].pcm=(struct pt_pcm){pcm,16,16,8000,1,(uint8_t)bits};samples[0].volume=64;
    samples[1].pcm=(struct pt_pcm){stereo,32,16,8000,2,(uint8_t)bits};samples[1].volume=64;
    events[4]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    events[16+15].effect=15;events[16+15].parameter=150; /* Non-Paula global tempo. */
    events[16*2+15].effect=14;events[16*2+15].parameter=0xe1; /* Global delay on another backend. */
    events[16*3+15].effect=15;events[16*3+15].parameter=0; /* Non-Paula F00 end. */
    o.rate=48000;o.bits=24;o.tracks=(1U<<4);o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    first_calls=calls;
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_COMPATIBLE);
    assert(calls==first_calls+2 && !live && report.frames==measured.frames && report.samples[0] && !report.samples[1]);
    assert(report.map[4]==0 && report.map[7]==1 && report.map[10]==2 && report.map[14]==3);
    /* A different global tempo changes the full shared timeline. */
    events[16+15].parameter=125;
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&batch)==PT_PAULA_COMPATIBLE && batch.frames>report.frames);
    events[16+15].parameter=150;
    o.include_lead_in=1;assert(pt_render_measure(&p,&o,NULL,NULL,&measured)==PT_RENDER_OK);
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_COMPATIBLE && report.frames==measured.frames);
    o.include_lead_in=0;
    /* Later stereo trigger rejects the complete timeline, never marks samples. */
    events[16*2+4]=(struct pt_event){428,0,PT_NOTE_PERIOD,2,0,0,0,0};
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_GEOMETRY && report.channel==4 && report.intervals>1);
    assert(!report.samples[0] && !report.samples[1] && !live);
    events[16*2+4]=(struct pt_event){0};
    p.channels.track[4].pan=255;
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_CONTROL && !live);
    p.channels.track[4].pan=0;
    assert(pt_paula_preflight(&p,&o,NULL,&caps,0,&a,&report)==PT_PAULA_CONTROL && !live);
    samples[0].loop=PT_LOOP_FORWARD;samples[0].loop_end=16;
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_GEOMETRY && !live);
    samples[0].loop=samples[0].loop_end=0;
    first_calls=calls;o.row_range=1;
    assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_RANGE && calls==first_calls);
    o.row_range=0;
    for(i=1;i<=2;++i) {
        fail=calls+i;
        assert(pt_paula_preflight(&p,&o,NULL,&caps,1,&a,&report)==PT_PAULA_MEMORY && !live);
        fail=0;
    }
    /* Pure batch gate resolves identity before dereferencing a foreign pointer. */
    assert(pt_channels_paula_map(&p.channels,NULL,map)==PT_CHANNEL_OK);
    assert(pt_voice_init(&voice,&samples[0].pcm,0,16,PT_VOICE_ONCE,0,0,((uint64_t)8000<<32)/48000,0)==PT_PCM_OK);
    plan.count=2;plan.action[0]=(struct pt_render_action){PT_RENDER_TRIGGER,4,voice,{0,0}};
    plan.action[1]=plan.action[0];plan.action[1].voice.pcm=(const struct pt_pcm *)(uintptr_t)1;
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_SOURCE && !held && !batch.samples[0]);
    plan.action[1].channel=1; /* Non-Paula sources are another backend's responsibility. */
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_COMPATIBLE && held==(1U<<4));
    plan.action[0].kind=PT_RENDER_REPEAT;
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_OPERATION && held==(1U<<4));
    plan.action[0].kind=PT_RENDER_STOP;plan.count=1;
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_COMPATIBLE && !held);
    plan.action[0].channel=16;
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_CHANNEL && !held);
    plan.action[0].channel=4;plan.action[0].kind=(enum pt_render_action_kind)99;
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_OPERATION && !held);
    plan.action[0].kind=PT_RENDER_STOP;held=1;
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_INVALID && held==1);
    held=0;map[4]=3; /* Duplicate/missing physical assignment. */
    assert(pt_paula_check_plan(&p,o.rate,map,&plan,&caps,1,&held,&batch)==PT_PAULA_INVALID && !held);
    assert(!live && samples[0].pcm.bits==bits && pcm[0]==1);
}
int main(void){controls();fixture(8);fixture(16);fixture(24);puts("PAULA PREFLIGHT PASS: full shared timeline, explicit clock/stereo/volume/geometry, late refusal and allocation cleanup; no dispatch");return 0;}
