#include <limits.h>
#include <string.h>
#include "paula_preflight.h"
static void report_init(struct pt_paula_preflight_report *r)
{
    unsigned i;memset(r,0,sizeof(*r));r->result=PT_PAULA_INVALID;r->render_result=PT_RENDER_OK;
    r->action=r->channel=UINT_MAX;for(i=0;i<PT_CHANNEL_LIMIT;++i)r->map[i]=-1;
}
static int resolve(const struct pt_project *p,const struct pt_pcm *pcm,unsigned *slot)
{unsigned i;for(i=0;i<p->sample_count;++i)if(pcm==&p->samples[i].pcm){*slot=i;return 1;}return 0;}
enum pt_paula_capability pt_paula_check_plan(const struct pt_project *p,unsigned rate,
    const int8_t map[PT_CHANNEL_LIMIT],const struct pt_render_plan *plan,
    const struct pt_paula_render_caps *caps,unsigned controls,uint16_t *state,struct pt_paula_preflight_report *out)
{
    struct pt_paula_preflight_report r;struct pt_paula_render_plan prepared;
    uint16_t held,allowed=0;unsigned i,sample;uint16_t period;uint8_t volume;int8_t expected[PT_CHANNEL_LIMIT];
    report_init(&r);
    if(!out)return PT_PAULA_INVALID;
    if(!p || !p->samples || !p->sample_count || p->sample_count>PT_PROJECT_SAMPLES || !map || !plan ||
       plan->count>PT_RENDER_ACTIONS || !state || controls>1 || !pt_paula_render_caps_valid(caps) ||
       (rate!=44100 && rate!=48000) || pt_channels_paula_map(&p->channels,map,expected)!=PT_CHANNEL_OK ||
       memcmp(map,expected,sizeof(expected)))goto done;
    memcpy(r.map,map,sizeof(r.map));held=*state;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)if(map[i]>=0)allowed|=(uint16_t)(1U<<i);
    if(held&(uint16_t)~allowed)goto done;
    for(i=0;i<plan->count;++i) {
        const struct pt_render_action *a=&plan->action[i];r.action=i;r.channel=a->channel;r.kind=a->kind;
        if(a->channel>=p->channels.count){r.result=PT_PAULA_CHANNEL;goto done;}
        if(a->kind<PT_RENDER_TRIGGER || a->kind>PT_RENDER_CONTROL){r.result=PT_PAULA_OPERATION;goto done;}
        if(map[a->channel]<0)continue;
        switch(a->kind) {
        case PT_RENDER_TRIGGER:
            if(!resolve(p,a->voice.pcm,&sample)){r.result=PT_PAULA_SOURCE;goto done;}
            if(!pt_paula_render_voice(&a->voice,rate,a->gain,(unsigned)map[a->channel],caps,&prepared)) {
                r.result=PT_PAULA_GEOMETRY;goto done;
            }
            r.samples[sample]=1;held|=(uint16_t)(1U<<a->channel);break;
        case PT_RENDER_CONTROL:
            if(!controls || !(held&(1U<<a->channel)) ||
               !pt_paula_render_control(a->voice.step,rate,a->gain,(unsigned)map[a->channel],caps,&period,&volume)) {
                r.result=PT_PAULA_CONTROL;goto done;
            }
            break;
        case PT_RENDER_STOP:held&=(uint16_t)~(1U<<a->channel);break;
        default:r.result=PT_PAULA_OPERATION;goto done;
        }
    }
    *state=held;r.action=r.channel=UINT_MAX;r.result=PT_PAULA_COMPATIBLE;
done:
    if(r.result!=PT_PAULA_COMPATIBLE)memset(r.samples,0,sizeof(r.samples));
    *out=r;return r.result;
}
enum pt_paula_capability pt_paula_preflight(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,unsigned controls,
    const struct pt_allocator *allocator,struct pt_paula_preflight_report *out)
{
    struct pt_paula_preflight_report r,batch;struct pt_render_sequence *sequence=NULL;
    struct pt_render_plan *plan=NULL;struct pt_render_interval span;struct pt_allocator a;
    uint16_t held=0;uint32_t remaining,block;unsigned i;
    report_init(&r);if(!out)return PT_PAULA_INVALID;
    if(!p || !o || !allocator || !allocator->allocate || !allocator->release || controls>1 ||
       !pt_paula_render_caps_valid(caps) || pt_project_validate(p,NULL)!=PT_PROJECT_OK ||
       pt_channels_paula_map(&p->channels,previous,r.map)!=PT_CHANNEL_OK)goto done;
    if(o->row_range){r.result=PT_PAULA_RANGE;goto done;}
    a=*allocator;plan=a.allocate(a.context,sizeof(*plan));
    if(!plan){r.result=PT_PAULA_MEMORY;goto done;}
    r.render_result=pt_render_sequence_open(p,o,&a,&sequence);
    if(r.render_result!=PT_RENDER_OK)goto render_error;
    do {
        r.render_result=pt_render_sequence_next(sequence,&span);if(r.render_result!=PT_RENDER_OK)goto render_error;
        ++r.intervals;remaining=span.frames;
        while(remaining) {
            block=remaining>256?256:remaining;
            r.render_result=pt_render_sequence_consume(sequence,block);if(r.render_result!=PT_RENDER_OK)goto render_error;
            remaining-=block;r.frames+=block;
        }
        r.render_result=pt_render_sequence_complete(sequence,plan);if(r.render_result!=PT_RENDER_OK)goto render_error;
        r.result=pt_paula_check_plan(p,o->rate,r.map,plan,caps,controls,&held,&batch);
        if(r.result!=PT_PAULA_COMPATIBLE) {r.action=batch.action;r.channel=batch.channel;r.kind=batch.kind;goto done;}
        for(i=0;i<PT_PROJECT_SAMPLES;++i)r.samples[i]|=batch.samples[i];
    }while(!span.end);
    goto done;
render_error:r.result=r.render_result==PT_RENDER_MEMORY?PT_PAULA_MEMORY:PT_PAULA_RENDER;
done:
    pt_render_sequence_close(sequence);if(plan)a.release(a.context,plan);
    if(r.result!=PT_PAULA_COMPATIBLE)memset(r.samples,0,sizeof(r.samples));
    *out=r;return r.result;
}
