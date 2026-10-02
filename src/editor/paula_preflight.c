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
struct pt_paula_preflight {
    struct pt_render_plan plan;struct pt_allocator allocator;
    const struct pt_project *project;struct pt_render_options options;
    struct pt_paula_render_caps caps;struct pt_render_sequence *sequence;
    struct pt_render_interval span;struct pt_paula_preflight_report report;
    uint16_t held;uint32_t remaining;unsigned controls,phase;
};
static void report_copy(const struct pt_paula_preflight_report *r,struct pt_paula_preflight_report *out)
{*out=*r;if(r->result!=PT_PAULA_COMPATIBLE)memset(out->samples,0,sizeof(out->samples));}
void pt_paula_preflight_close(struct pt_paula_preflight **work)
{
    struct pt_paula_preflight *w;struct pt_allocator a;
    if(!work || !*work)return;
    w=*work;a=w->allocator;pt_render_sequence_close(w->sequence);
    a.release(a.context,w);*work=NULL;
}
enum pt_paula_capability pt_paula_preflight_begin(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,unsigned controls,
    const struct pt_allocator *a,struct pt_paula_preflight_report *out,struct pt_paula_preflight **work)
{
    struct pt_paula_preflight_report r;struct pt_paula_preflight *w=NULL;
    report_init(&r);if(!out)return PT_PAULA_INVALID;
    if(!work || *work || !p || !o || !a || !a->allocate || !a->release || controls>1 ||
       !pt_paula_render_caps_valid(caps) || pt_project_validate(p,NULL)!=PT_PROJECT_OK ||
       pt_channels_paula_map(&p->channels,previous,r.map)!=PT_CHANNEL_OK)goto done;
    if(o->row_range){r.result=PT_PAULA_RANGE;goto done;}
    w=a->allocate(a->context,sizeof(*w));if(!w){r.result=PT_PAULA_MEMORY;goto done;}
    memset(w,0,sizeof(*w));w->allocator=*a;
    r.render_result=pt_render_sequence_begin(p,o,a,&w->sequence);
    if(r.render_result!=PT_RENDER_OK) {
        r.result=r.render_result==PT_RENDER_MEMORY?PT_PAULA_MEMORY:PT_PAULA_RENDER;
        pt_paula_preflight_close(&w);goto done;
    }
    w->project=p;w->options=*o;w->caps=*caps;w->controls=controls;
    r.result=PT_PAULA_PENDING;w->report=r;*work=w;
done:
    report_copy(&r,out);return r.result;
}
enum pt_paula_capability pt_paula_preflight_step(struct pt_paula_preflight *w,struct pt_paula_preflight_report *out)
{
    struct pt_paula_preflight_report *r,batch;unsigned i,ready;
    if(!w || !out)return PT_PAULA_INVALID;
    r=&w->report;if(r->result!=PT_PAULA_PENDING)goto done;
    switch(w->phase) {
    case 0:
        r->render_result=pt_render_sequence_prepare(w->sequence,256,&ready);
        if(r->render_result==PT_RENDER_OK && ready)w->phase=1;
        break;
    case 1:
        r->render_result=pt_render_sequence_next(w->sequence,&w->span);
        if(r->render_result!=PT_RENDER_OK)break;
        ++r->intervals;w->remaining=w->span.frames;w->phase=w->remaining?2:3;
        break;
    case 2: {
        uint32_t n=w->remaining>256?256:w->remaining;
        r->render_result=pt_render_sequence_consume(w->sequence,n);
        if(r->render_result!=PT_RENDER_OK)break;
        w->remaining-=n;r->frames+=n;if(!w->remaining)w->phase=3;
        break;
    }
    case 3:
        r->render_result=pt_render_sequence_complete(w->sequence,&w->plan);
        if(r->render_result!=PT_RENDER_OK)break;
        r->result=pt_paula_check_plan(w->project,w->options.rate,r->map,&w->plan,&w->caps,w->controls,&w->held,&batch);
        if(r->result!=PT_PAULA_COMPATIBLE) {r->action=batch.action;r->channel=batch.channel;r->kind=batch.kind;break;}
        for(i=0;i<PT_PROJECT_SAMPLES;++i)r->samples[i]|=batch.samples[i];
        if(!w->span.end){r->result=PT_PAULA_PENDING;w->phase=1;}
        break;
    }
    if(r->render_result!=PT_RENDER_OK)r->result=r->render_result==PT_RENDER_MEMORY?PT_PAULA_MEMORY:PT_PAULA_RENDER;
done:
    report_copy(r,out);return r->result;
}
int pt_paula_preflight_transfer(struct pt_paula_preflight *w,struct pt_render_sequence **out)
{
    if(!w || !out || w->report.result!=PT_PAULA_COMPATIBLE || !w->sequence)return 0;
    w->report.render_result=pt_render_sequence_rewind(w->sequence);
    if(w->report.render_result!=PT_RENDER_OK){w->report.result=PT_PAULA_RENDER;return 0;}
    *out=w->sequence;w->sequence=NULL;return 1;
}
static enum pt_paula_capability traverse(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,unsigned controls,
    const struct pt_allocator *a,struct pt_paula_preflight_report *out,struct pt_render_sequence **take)
{
    struct pt_paula_preflight *w=NULL;
    enum pt_paula_capability result=pt_paula_preflight_begin(p,o,previous,caps,controls,a,out,&w);
    while(result==PT_PAULA_PENDING)result=pt_paula_preflight_step(w,out);
    if(result==PT_PAULA_COMPATIBLE && take && !pt_paula_preflight_transfer(w,take))result=pt_paula_preflight_step(w,out);
    pt_paula_preflight_close(&w);return result;
}
enum pt_paula_capability pt_paula_preflight(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,unsigned controls,
    const struct pt_allocator *a,struct pt_paula_preflight_report *out)
{return traverse(p,o,previous,caps,controls,a,out,NULL);}
enum pt_paula_capability pt_paula_preflight_take(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,unsigned controls,
    const struct pt_allocator *a,struct pt_paula_preflight_report *out,struct pt_render_sequence **sequence)
{
    if(!sequence)return PT_PAULA_INVALID;
    return traverse(p,o,previous,caps,controls,a,out,sequence);
}
