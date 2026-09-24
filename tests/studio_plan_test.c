#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render_commands.h"
#include <stdlib.h>
#include "studio_plan.h"
static struct pt_sample *masters;
static unsigned pins,allocations,calls,fail_call;
static void *allocate(void *c,size_t n) {(void)c;++allocations;return malloc(n);}
static void release(void *c,void *p) {(void)c;--allocations;free(p);}
static int acquire(void *c,uint64_t key,uint64_t version,struct pt_pcm *p,void **token)
{
    (void)c;++calls;if(calls==fail_call || key<1 || key>2 || version!=7)return 0;
    *p=masters[key-1].pcm;*token=masters+key-1;++pins;return 1;
}
static void unpin(void *c,void *token) {(void)c;assert(token && pins);--pins;}
int main(void)
{
    struct pt_project p={0};struct pt_sample samples[2];struct pt_event events[64*16]={{0}};uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_flow f={0};struct pt_pitch pitch={0};struct pt_render_range ranges[16]={{0}};
    struct pt_render_command_state ref,planned;struct pt_render_plan plan;struct pt_allocator allocator={NULL,allocate,release};
    struct pt_studio_source provider={NULL,acquire,unpin};struct pt_studio_mix *mix;
    struct pt_studio_binding bindings[2];
    int32_t data[2][8]={{1,91,-713,999,21,37,53,67},{111,201,-301,401,511,601,711,801}};
    unsigned ch,t;uint16_t offsets=0;
    memset(samples,0,sizeof(samples));
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.samples=samples;p.sample_count=2;p.speed=6;p.bpm=125;
    for(ch=0;ch<2;++ch) {samples[ch].pcm=(struct pt_pcm){data[ch],8,8,48000,1,24};samples[ch].volume=64;samples[ch].loop=PT_LOOP_FORWARD;samples[ch].loop_end=8;}
    masters=samples;bindings[0]=(struct pt_studio_binding){&samples[0].pcm,1,7};
    bindings[1]=(struct pt_studio_binding){&samples[1].pcm,2,7};
    mix=pt_studio_open(&allocator,&provider,16);assert(mix);
    o.rate=48000;o.bits=24;o.gain_q16=4096;o.tracks=65535;f.project=&p;f.fresh=1;
    pt_render_commands_init(&ref);pt_render_commands_init(&planned);
    for(t=0;t<8;++t) {
        for(ch=0;ch<16;++ch) {
            struct pt_event *e=events+ch;memset(e,0,sizeof(*e));memset(ranges+ch,0,sizeof(*ranges));
            pitch.channel[ch].output=428;f.effect[ch]=f.parameter[ch]=0;
            if(t==0 || t==1 || t==4) {e->kind=PT_NOTE_PERIOD;e->pitch=428;e->instrument=1;}
            if(t==2)e->instrument=2;
            if(t==3) {e->effect=12;e->parameter=0;}
            if(t==4) {ranges[ch].trigger_start=2;ranges[ch].trigger_length=3;offsets=65535;}
            if(t==5) {ranges[ch].retrigger=1;ranges[ch].trigger_start=1;ranges[ch].trigger_length=2;}
            if(t==6)e->kind=PT_NOTE_OFF;
        }
        assert(pt_render_commands_tick(&p,&o,&f,&pitch,ranges,offsets,&ref)==PT_RENDER_OK);
        pt_render_commands_gains(&p,&o,&ref);
        assert(pt_render_commands_plan(&p,&o,&f,&pitch,ranges,offsets,&planned,&plan)==PT_RENDER_OK);
        assert(plan.count<=PT_RENDER_ACTIONS);
        if(t<=5)assert(plan.count==(t==3?16:32));
        if(t==0 || t==1)assert(plan.action[0].kind==PT_RENDER_TRIGGER);
        if(t==2)assert(plan.action[0].kind==PT_RENDER_REPEAT);
        if(t==4 || t==5)assert(plan.action[0].kind==PT_RENDER_SEGMENT);
        if(t==6)assert(plan.count==16 && plan.action[0].kind==PT_RENDER_STOP);
        if(t==7)assert(plan.count==0);
        assert(pt_studio_dispatch(mix,16,&plan,bindings,2)==PT_PCM_OK);
        {int32_t a[22],b[22],c[22];uint64_t ca,cb,cc;
            struct pt_pcm pa={a,22,11,48000,2,24},pb={b,22,11,48000,2,24},pc={c,22,11,48000,2,24};
            assert(pt_voice_mix(ref.voice,16,ref.gain,&pa,&ca)==PT_PCM_OK);
            assert(pt_voice_mix(planned.voice,16,planned.gain,&pb,&cb)==PT_PCM_OK);
            assert(pt_studio_read(mix,&pc,&cc)==PT_PCM_OK);
            assert(!memcmp(a,b,sizeof(a)) && !memcmp(a,c,sizeof(a)) && ca==cb && ca==cc);
            if(t==0)assert(a[0]!=0);
            if(t==3 || t>=6)assert(a[0]==0);
        }
        offsets=0;
    }
    /* A provider failure after one successful trigger releases all pins. */
    memset(events,0,sizeof(events));
    for(ch=0;ch<16;++ch) {events[ch].kind=PT_NOTE_PERIOD;events[ch].pitch=428;events[ch].instrument=1;}
    assert(pt_render_commands_plan(&p,&o,&f,&pitch,ranges,0,&planned,&plan)==PT_RENDER_OK);
    fail_call=calls+2;
    assert(pt_studio_dispatch(mix,16,&plan,bindings,2)!=PT_PCM_OK && pins==0);
    fail_call=0;
    assert(pt_studio_dispatch(mix,16,&plan,bindings,2)==PT_PCM_OK && pins==16);
    {unsigned before=calls;struct pt_studio_binding bad[2]={bindings[0],bindings[0]};
        assert(pt_studio_dispatch(mix,16,&plan,bad,2)==PT_PCM_INVALID && pins==0 && calls==before);
    }
    assert(pt_studio_dispatch(mix,16,&plan,bindings,2)==PT_PCM_OK && pins==16);
    plan.action[1].channel=16;
    assert(pt_studio_dispatch(mix,16,&plan,bindings,2)==PT_PCM_INVALID && pins==0);
    plan.count=PT_RENDER_ACTIONS+1;
    assert(pt_studio_dispatch(mix,16,&plan,bindings,2)==PT_PCM_INVALID);
    pt_studio_close(mix);assert(!pins && !allocations);
    puts("Studio dispatch PASS: 16-channel true24 audio equivalence, source binding, partial acquisition failure cleanup");return 0;
}
