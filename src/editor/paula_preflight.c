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
    struct pt_render_sequence_setup *startup;
    struct pt_project setup_header;
    const struct pt_paula_render_caps *caps_source;const int8_t *previous_source;
    int8_t previous[PT_CHANNEL_LIMIT];unsigned setup_busy,setup_failed,setup_mode,checked_setup;
    unsigned *setup_callback_failure;
};
static int paula_setup_apart(const void *,size_t,const void *,size_t);
static int checked_preflight_output(const struct pt_paula_preflight *,const void *,size_t);
static int paula_setup_control_apart(const struct pt_paula_preflight *,const void *,size_t);
static void report_copy(const struct pt_paula_preflight_report *r,struct pt_paula_preflight_report *out)
{*out=*r;if(r->result!=PT_PAULA_COMPATIBLE)memset(out->samples,0,sizeof(out->samples));}
void pt_paula_preflight_close(struct pt_paula_preflight **work)
{
    struct pt_paula_preflight *w;struct pt_allocator a;unsigned checked;
    if(!work || !*work)return;
    w=*work;checked=w->checked_setup;
    if(w->checked_setup&&w->setup_busy){w->setup_failed=1;return;}
    if(w->checked_setup&&(!paula_setup_control_apart(w,work,sizeof(*work))||
       (w->sequence&&!pt_render_sequence_control_output_disjoint(w->sequence,work,sizeof(*work)))))return;
    if(w->checked_setup)w->setup_busy=1;
    a=w->allocator;pt_render_sequence_close(w->sequence);
    a.release(a.context,w);
    if(!checked||*work==w)*work=NULL;
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
    if(w->checked_setup&&w->setup_busy){w->setup_failed=1;return PT_PAULA_INVALID;}
    if(w->checked_setup&&!checked_preflight_output(w,out,sizeof(*out)))return PT_PAULA_INVALID;
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
    if(w&&w->checked_setup&&w->setup_busy){w->setup_failed=1;return 0;}
    if(!w || !out || w->report.result!=PT_PAULA_COMPATIBLE || !w->sequence)return 0;
    if(w->checked_setup&&!checked_preflight_output(w,out,sizeof(*out)))return 0;
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

/* Initial setup owns the real opaque renderer startup, never an exposed
 * validator/flow certificate. This allocation becomes the actual audit owner. */
static int paula_setup_span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int paula_setup_apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return paula_setup_span(a,an)&&paula_setup_span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);
}
/* Fixed captures only: usable for closing after sequence ownership was moved,
 * or source headers changed, without traversing former source descriptors. */
static int paula_setup_control_apart(const struct pt_paula_preflight *w,const void *out,size_t n)
{
    const struct pt_project *p=&w->setup_header;
    return paula_setup_apart(w,sizeof(*w),out,n)&&
        paula_setup_apart(w->project,sizeof(*w->project),out,n)&&
        paula_setup_apart(p->orders,p->order_count*sizeof(*p->orders),out,n)&&
        paula_setup_apart(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events),out,n)&&
        paula_setup_apart(p->samples,p->sample_count*sizeof(*p->samples),out,n)&&
        paula_setup_apart(p->extensions,p->extension_count*sizeof(*p->extensions),out,n)&&
        paula_setup_apart(w->caps_source,w->caps_source?sizeof(*w->caps_source):0,out,n)&&
        paula_setup_apart(w->previous_source,w->previous_source?sizeof(w->previous):0,out,n);
}
static int checked_preflight_output(const struct pt_paula_preflight *w,const void *out,size_t n)
{
    return paula_setup_apart(w,sizeof(*w),out,n)&&
        pt_render_sequence_output_disjoint(w->sequence,out,n);
}
static enum pt_render_setup_result paula_setup_enter(struct pt_paula_preflight *w)
{
    if(!paula_setup_span(w,sizeof(*w))||!w->setup_mode)return PT_RENDER_SETUP_INVALID;
    if(w->setup_busy){w->setup_failed=1;if(w->setup_callback_failure)*w->setup_callback_failure=1;return PT_RENDER_SETUP_BUSY;}
    return PT_RENDER_SETUP_PENDING;
}
static enum pt_render_setup_result paula_setup_current(struct pt_paula_preflight *w,uint32_t revision,uint32_t generation)
{
    if(w->setup_failed)return PT_RENDER_SETUP_FAILED;
    if(memcmp(w->caps_source,&w->caps,sizeof(w->caps))||
       (w->previous_source&&memcmp(w->previous_source,w->previous,sizeof(w->previous))))return PT_RENDER_SETUP_STALE;
    return pt_render_sequence_setup_get(w->startup,revision,generation,NULL);
}
static int paula_setup_output_apart(struct pt_paula_preflight *w,uint32_t revision,uint32_t generation,const void *out,size_t n)
{
    return paula_setup_apart(w,sizeof(*w),out,n)&&
        paula_setup_apart(w->caps_source,sizeof(*w->caps_source),out,n)&&
        paula_setup_apart(w->previous_source,w->previous_source?sizeof(w->previous):0,out,n)&&
        pt_render_sequence_setup_output_disjoint(w->startup,revision,generation,out,n);
}
enum pt_render_setup_result pt_paula_preflight_setup_begin(const struct pt_project *p,
    const struct pt_render_options *o,const int8_t *previous,const struct pt_paula_render_caps *caps,
    unsigned controls,const struct pt_allocator *a,uint32_t revision,uint32_t generation,
    struct pt_paula_preflight_setup **out)
{
    struct pt_render_sequence_setup *startup=NULL;struct pt_paula_preflight *w;
    struct pt_paula_render_caps saved_caps;int8_t saved_map[PT_CHANNEL_LIMIT];struct pt_allocator saved_allocator;
    struct pt_render_setup_guard guards[3];enum pt_render_setup_result r;
    if(!paula_setup_span(out,sizeof(*out))||!paula_setup_span(caps,sizeof(*caps))||
       !paula_setup_span(a,sizeof(*a))||!paula_setup_span(previous,previous?sizeof(saved_map):0)||
       controls>1||!a->allocate||!a->release||!pt_paula_render_caps_valid(caps))return PT_RENDER_SETUP_INVALID;
    memcpy(&saved_caps,caps,sizeof(saved_caps));memcpy(&saved_allocator,a,sizeof(saved_allocator));
    if(previous)memcpy(saved_map,previous,sizeof(saved_map));
    guards[0]=(struct pt_render_setup_guard){caps,sizeof(*caps)};
    guards[1]=(struct pt_render_setup_guard){previous,previous?sizeof(saved_map):0};
    guards[2]=(struct pt_render_setup_guard){out,sizeof(*out)};
    r=pt_render_sequence_setup_begin(p,o,a,revision,generation,guards,3,&startup);
    if(r!=PT_RENDER_SETUP_PENDING)return r;
    if(!pt_render_sequence_setup_output_disjoint(startup,revision,generation,out,sizeof(*out))||
       !paula_setup_apart(out,sizeof(*out),caps,sizeof(*caps))||
       !paula_setup_apart(out,sizeof(*out),previous,previous?sizeof(saved_map):0)){
        pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_ALIAS;
    }
    if(*out){pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_INVALID;}
    if(memcmp(caps,&saved_caps,sizeof(saved_caps))||(previous&&memcmp(previous,saved_map,sizeof(saved_map)))){
        pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_STALE;
    }
    w=saved_allocator.allocate(saved_allocator.context,sizeof(*w));
    /* Classify against the actual retained startup and original fixed extents
     * BEFORE stale cleanup. An overlapping return never granted fresh ownership:
     * releasing it could free startup/source and make cancellation use freed data. */
    if(w&&(!pt_render_sequence_setup_control_output_disjoint(startup,w,sizeof(*w))||
       !paula_setup_apart(w,sizeof(*w),caps,sizeof(*caps))||
       !paula_setup_apart(w,sizeof(*w),previous,previous?sizeof(saved_map):0)||
       !paula_setup_apart(w,sizeof(*w),out,sizeof(*out))||
       !paula_setup_apart(w,sizeof(*w),&saved_caps,sizeof(saved_caps))||
       !paula_setup_apart(w,sizeof(*w),saved_map,sizeof(saved_map))||
       !paula_setup_apart(w,sizeof(*w),&saved_allocator,sizeof(saved_allocator))||
       !paula_setup_apart(w,sizeof(*w),guards,sizeof(guards))||
       !paula_setup_apart(w,sizeof(*w),&startup,sizeof(startup)))){
        pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_ALIAS;
    }
    r=pt_render_sequence_setup_get(startup,revision,generation,NULL);
    if(r!=PT_RENDER_SETUP_PENDING||memcmp(caps,&saved_caps,sizeof(saved_caps))||
       (previous&&memcmp(previous,saved_map,sizeof(saved_map)))||*out){
        if(w)saved_allocator.release(saved_allocator.context,w);
        pt_render_sequence_setup_cancel(&startup);return r==PT_RENDER_SETUP_FAILED?r:PT_RENDER_SETUP_STALE;
    }
    if(!w){pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_CAPACITY;}
    if(!pt_render_sequence_setup_output_disjoint(startup,revision,generation,w,sizeof(*w))||
       !paula_setup_apart(w,sizeof(*w),caps,sizeof(*caps))||
       !paula_setup_apart(w,sizeof(*w),previous,previous?sizeof(saved_map):0)||
       !paula_setup_apart(w,sizeof(*w),out,sizeof(*out))||
       !pt_render_sequence_setup_output_disjoint(startup,revision,generation,out,sizeof(*out))){
        pt_render_sequence_setup_cancel(&startup);
        return PT_RENDER_SETUP_ALIAS;
    }
    memset(w,0,sizeof(*w));w->allocator=saved_allocator;w->project=p;memcpy(&w->options,o,sizeof(*o));
    memcpy(&w->setup_header,p,sizeof(w->setup_header));
    memcpy(&w->caps,&saved_caps,sizeof(w->caps));w->caps_source=caps;w->previous_source=previous;
    if(previous)memcpy(w->previous,saved_map,sizeof(saved_map));
    w->startup=startup;w->controls=controls;w->setup_mode=1;w->checked_setup=1;report_init(&w->report);w->report.result=PT_PAULA_PENDING;
    *out=(struct pt_paula_preflight_setup *)w;return PT_RENDER_SETUP_PENDING;
}
enum pt_render_setup_result pt_paula_preflight_setup_step(struct pt_paula_preflight_setup *setup,
    uint32_t revision,uint32_t generation,unsigned work)
{
    struct pt_paula_preflight *w=(struct pt_paula_preflight *)setup;enum pt_render_setup_result r=paula_setup_enter(w);
    if(r!=PT_RENDER_SETUP_PENDING)return r;
    if(!work||work>PT_PROJECT_VALIDATION_WORK_MAX)return PT_RENDER_SETUP_INVALID;
    r=paula_setup_current(w,revision,generation);if(r!=PT_RENDER_SETUP_PENDING)return r;
    return pt_render_sequence_setup_step(w->startup,revision,generation,work);
}
enum pt_render_setup_result pt_paula_preflight_setup_get(struct pt_paula_preflight_setup *setup,
    uint32_t revision,uint32_t generation,struct pt_render_setup_report *out)
{
    struct pt_paula_preflight *w=(struct pt_paula_preflight *)setup;enum pt_render_setup_result r=paula_setup_enter(w);
    if(r!=PT_RENDER_SETUP_PENDING)return r;
    r=paula_setup_current(w,revision,generation);if(r==PT_RENDER_SETUP_STALE||r==PT_RENDER_SETUP_FAILED)return r;
    if(out&&!paula_setup_output_apart(w,revision,generation,out,sizeof(*out)))return PT_RENDER_SETUP_ALIAS;
    return pt_render_sequence_setup_get(w->startup,revision,generation,out);
}
enum pt_render_setup_result pt_paula_preflight_setup_transfer(struct pt_paula_preflight_setup **owner,
    uint32_t revision,uint32_t generation,struct pt_paula_preflight **out)
{
    struct pt_paula_preflight *w;enum pt_render_setup_result r;struct pt_render_setup_guard guards[5];int8_t map[PT_CHANNEL_LIMIT];
    struct pt_paula_preflight *original_out;
    if(!paula_setup_span(owner,sizeof(*owner))||!paula_setup_span(out,sizeof(*out))||
       !paula_setup_apart(owner,sizeof(*owner),out,sizeof(*out)))return PT_RENDER_SETUP_INVALID;
    w=(struct pt_paula_preflight *)*owner;r=paula_setup_enter(w);if(r!=PT_RENDER_SETUP_PENDING)return r;
    r=paula_setup_current(w,revision,generation);if(r!=PT_RENDER_SETUP_READY)return r;
    if(!paula_setup_output_apart(w,revision,generation,owner,sizeof(*owner))||
       !paula_setup_output_apart(w,revision,generation,out,sizeof(*out)))return PT_RENDER_SETUP_ALIAS;
    if(w->options.row_range||pt_channels_paula_map(&w->project->channels,w->previous_source,map)!=PT_CHANNEL_OK)
        {w->setup_failed=1;return PT_RENDER_SETUP_FAILED;}
    guards[0]=(struct pt_render_setup_guard){w,sizeof(*w)};
    guards[1]=(struct pt_render_setup_guard){w->caps_source,sizeof(*w->caps_source)};
    guards[2]=(struct pt_render_setup_guard){w->previous_source,w->previous_source?sizeof(w->previous):0};
    guards[3]=(struct pt_render_setup_guard){owner,sizeof(*owner)};
    guards[4]=(struct pt_render_setup_guard){out,sizeof(*out)};
    original_out=*out;
    w->setup_busy=1;r=pt_render_sequence_setup_take(&w->startup,revision,generation,guards,5,&w->sequence);
    w->setup_busy=0;
    if(r!=PT_RENDER_SETUP_READY){
        if(!w->startup)w->setup_failed=1;
        return w->setup_failed?PT_RENDER_SETUP_FAILED:r;
    }
    if(w->setup_failed||memcmp(w->caps_source,&w->caps,sizeof(w->caps))||
       (w->previous_source&&memcmp(w->previous_source,w->previous,sizeof(w->previous)))||
       *owner!=(struct pt_paula_preflight_setup *)w||*out!=original_out||
       !paula_setup_apart(w,sizeof(*w),out,sizeof(*out))||
       !pt_render_sequence_output_disjoint(w->sequence,out,sizeof(*out))){w->setup_failed=1;return PT_RENDER_SETUP_FAILED;}
    memcpy(w->report.map,map,sizeof(map));w->setup_mode=0;
    /* Actual audit owns these copies; original setup input controls are no longer
     * borrowed and may be reused after transfer. Project/source remains borrowed. */
    w->caps_source=NULL;w->previous_source=NULL;
    *owner=NULL;*out=w;return PT_RENDER_SETUP_READY;
}
enum pt_render_setup_result pt_paula_preflight_setup_cancel(struct pt_paula_preflight_setup **owner)
{
    struct pt_paula_preflight *w;struct pt_allocator a;enum pt_render_setup_result r;unsigned callback_failure=0;
    if(!paula_setup_span(owner,sizeof(*owner)))return PT_RENDER_SETUP_INVALID;
    if(!*owner)return PT_RENDER_SETUP_READY;
    w=(struct pt_paula_preflight *)*owner;r=paula_setup_enter(w);if(r!=PT_RENDER_SETUP_PENDING)return r;
    /* Genuine external handle storage, fixed captures only; never former tables. */
    if(!paula_setup_control_apart(w,owner,sizeof(*owner)))
        return PT_RENDER_SETUP_ALIAS;
    a=w->allocator;w->setup_busy=1;w->setup_callback_failure=&callback_failure;
    r=pt_render_sequence_setup_cancel(&w->startup);if(r!=PT_RENDER_SETUP_READY&&w->startup){w->setup_busy=0;w->setup_callback_failure=NULL;return r;}
    pt_render_sequence_close(w->sequence);a.release(a.context,w);
    if(*owner==(struct pt_paula_preflight_setup *)w)*owner=NULL;else callback_failure=1;
    return callback_failure||r!=PT_RENDER_SETUP_READY?PT_RENDER_SETUP_FAILED:PT_RENDER_SETUP_READY;
}
