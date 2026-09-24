#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render_commands.h"
static void dispatch(const struct pt_render_plan *p,struct pt_voice *v,uint32_t g[16][2])
{
    unsigned i;
    for(i=0;i<p->count;++i) {
        const struct pt_render_action *a=p->action+i;const struct pt_voice *s=&a->voice;
        struct pt_voice *d=v+a->channel;
        switch(a->kind) {
        case PT_RENDER_TRIGGER:assert(pt_voice_init(d,s->pcm,s->start,s->end,(enum pt_voice_loop)s->loop,s->loop_start,s->loop_end,s->step,s->linear)==PT_PCM_OK);break;
        case PT_RENDER_SEGMENT:assert(pt_voice_init_segment(d,s->pcm,s->start,s->end,s->loop_start,s->loop_end,s->step,s->linear)==PT_PCM_OK);break;
        case PT_RENDER_REPEAT:assert(pt_voice_set_repeat_source(d,s->repeat_pcm,s->loop_start,s->loop_end)==PT_PCM_OK);break;
        case PT_RENDER_STOP:d->active=0;break;
        case PT_RENDER_CONTROL:d->step=s->step;g[a->channel][0]=a->gain[0];g[a->channel][1]=a->gain[1];break;
        }
    }
}
int main(void)
{
    struct pt_project p={0};struct pt_sample samples[2]={{0}};struct pt_event events[64*16]={{0}};uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_flow f={0};struct pt_pitch pitch={0};struct pt_render_range ranges[16]={{0}};
    struct pt_render_command_state ref,planned;struct pt_render_plan plan;struct pt_voice voices[16]={{0}};uint32_t gains[16][2]={{0}};
    int32_t data[2][8]={{1,91,-713,999,21,37,53,67},{111,201,-301,401,511,601,711,801}};
    unsigned ch,t;uint16_t offsets=0;
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.samples=samples;p.sample_count=2;p.speed=6;p.bpm=125;
    for(ch=0;ch<2;++ch) {samples[ch].pcm=(struct pt_pcm){data[ch],8,8,48000,1,24};samples[ch].volume=64;samples[ch].loop=PT_LOOP_FORWARD;samples[ch].loop_end=8;}
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
        dispatch(&plan,voices,gains);
        {int32_t a[22],b[22],c[22];uint64_t ca,cb,cc;
            struct pt_pcm pa={a,22,11,48000,2,24},pb={b,22,11,48000,2,24},pc={c,22,11,48000,2,24};
            assert(pt_voice_mix(ref.voice,16,ref.gain,&pa,&ca)==PT_PCM_OK);
            assert(pt_voice_mix(planned.voice,16,planned.gain,&pb,&cb)==PT_PCM_OK);
            assert(pt_voice_mix(voices,16,gains,&pc,&cc)==PT_PCM_OK);
            assert(!memcmp(a,b,sizeof(a)) && !memcmp(a,c,sizeof(a)) && ca==cb && ca==cc);
            if(t==0)assert(a[0]!=0);
            if(t==3 || t>=6)assert(a[0]==0);
        }
        offsets=0;
    }
    /* Refuse a failed later channel without exposing earlier partial commands. */
    events[0].kind=events[1].kind=PT_NOTE_PERIOD;events[0].pitch=events[1].pitch=428;
    events[0].instrument=1;events[1].instrument=2;samples[1].pcm.frames=99;
    plan.count=57;
    assert(pt_render_commands_plan(&p,&o,&f,&pitch,ranges,0,&planned,&plan)==PT_RENDER_SAMPLE && plan.count==0);
    puts("render command plan PASS: explicit intents, 16-channel audio equivalence, failure discard");return 0;
}
