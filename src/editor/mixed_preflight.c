#include "mixed_preflight.h"
#include <limits.h>
#include <string.h>
struct workspace {struct pt_render_plan all,paula,amigus;unsigned index[2][PT_RENDER_ACTIONS];};
struct pt_mixed_preflight {
    struct workspace batch;struct pt_allocator allocator;
    const struct pt_project *project;struct pt_render_options options;
    struct pt_paula_render_caps caps;struct pt_playback_format format;
    struct pt_render_sequence *sequence;struct pt_render_interval span;
    struct pt_mixed_report report;uint16_t held[2];uint32_t remaining;
    unsigned pc,ac,phase;
    struct pt_render_sequence_setup *startup;
    struct pt_project setup_header;
    const struct pt_paula_render_caps *caps_source;
    const struct pt_playback_format *format_source;
    const int8_t *previous_source;int8_t previous[PT_CHANNEL_LIMIT];
    unsigned setup_busy,setup_failed,setup_mode,checked_setup;
    unsigned *setup_callback_failure;
};
static int mixed_setup_apart(const void *,size_t,const void *,size_t);
static int mixed_checked_output(const struct pt_mixed_preflight *,const void *,size_t);
static int mixed_setup_control_apart(const struct pt_mixed_preflight *,const void *,size_t);
static void init(struct pt_mixed_report *r)
{unsigned i;memset(r,0,sizeof(*r));r->result=PT_MIXED_INVALID;r->action=r->channel=UINT_MAX;r->paula=PT_PAULA_INVALID;r->amigus=PT_WAVETABLE_INVALID;for(i=0;i<PT_CHANNEL_LIMIT;++i)r->map[i]=-1;}
static void report(const struct pt_mixed_report *r,struct pt_mixed_report *out)
{*out=*r;if(r->result!=PT_MIXED_OK)memset(out->samples,0,sizeof(out->samples));}
void pt_mixed_preflight_close(struct pt_mixed_preflight **work)
{
    struct pt_mixed_preflight *w;struct pt_allocator a;unsigned checked;
    if(!work || !*work)return;
    w=*work;checked=w->checked_setup;
    if(checked&&w->setup_busy){w->setup_failed=1;if(w->setup_callback_failure)*w->setup_callback_failure=1;return;}
    if(checked&&(w->setup_mode||!mixed_setup_control_apart(w,work,sizeof(*work))||
       (w->sequence&&!pt_render_sequence_control_output_disjoint(w->sequence,work,sizeof(*work)))))return;
    if(checked)w->setup_busy=1;
    a=w->allocator;pt_render_sequence_close(w->sequence);
    a.release(a.context,w);
    if(!checked||*work==w)*work=NULL;
}
static enum pt_mixed_result batch(const struct pt_project *p,unsigned rate,struct workspace *w,
    const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,unsigned pc,unsigned ac,
    uint16_t held[2],struct pt_mixed_report *r)
{
    struct pt_paula_preflight_report pr;struct pt_wavetable_preflight_report ar;
    uint16_t next[2]={held[0],held[1]};unsigned i,k,pi=UINT_MAX,ai=UINT_MAX;
    w->paula.count=w->amigus.count=0;
    for(i=0;i<w->all.count;++i) {
        const struct pt_render_action *a=&w->all.action[i];unsigned route;
        if(a->channel>=p->channels.count){r->action=i;r->channel=a->channel;return PT_MIXED_ROUTE;}
        route=p->channels.track[a->channel].route;
        if(route!=PT_PAULA && route!=PT_AMIGUS){r->action=i;r->channel=a->channel;r->kind=a->kind;return PT_MIXED_ROUTE;}
        k=route==PT_PAULA?0:1;
        if(k){w->index[k][w->amigus.count]=i;w->amigus.action[w->amigus.count++]=*a;}
        else {w->index[k][w->paula.count]=i;w->paula.action[w->paula.count++]=*a;}
    }
    r->paula=pt_paula_check_plan(p,rate,r->map,&w->paula,caps,pc,&next[0],&pr);
    r->amigus=pt_wavetable_check_plan(p,rate,&w->amigus,f,ac,&next[1],&ar);
    if(pr.action<w->paula.count)pi=w->index[0][pr.action];
    if(ar.action<w->amigus.count)ai=w->index[1][ar.action];
    if(r->paula!=PT_PAULA_COMPATIBLE || r->amigus!=PT_WAVETABLE_COMPATIBLE) {
        unsigned backend=r->paula!=PT_PAULA_COMPATIBLE && (r->amigus==PT_WAVETABLE_COMPATIBLE || pi<=ai)?0:1;
        r->action=backend?ai:pi;
        if(r->action<w->all.count){r->channel=w->all.action[r->action].channel;r->kind=w->all.action[r->action].kind;}
        return backend?PT_MIXED_AMIGUS:PT_MIXED_PAULA;
    }
    for(i=0;i<PT_PROJECT_SAMPLES;++i){r->samples[0][i]|=pr.samples[i];r->samples[1][i]|=ar.samples[i];}
    held[0]=next[0];held[1]=next[1];return PT_MIXED_OK;
}
enum pt_mixed_result pt_mixed_preflight_begin(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,
    unsigned pc,unsigned ac,const struct pt_allocator *a,struct pt_mixed_report *out,struct pt_mixed_preflight **work)
{
    struct pt_mixed_report r;struct pt_mixed_preflight *w=NULL;unsigned i;
    init(&r);if(!out)return PT_MIXED_INVALID;
    if(!work || *work || !p || !o || !a || !a->allocate || !a->release || pc>1 || ac>1 || (o->rate!=44100 && o->rate!=48000) ||
       !pt_paula_render_caps_valid(caps) || !f || (f->bits!=8 && f->bits!=16) ||
       f->channel || f->word_pad || f->little_endian>1 || pt_project_validate(p,NULL)!=PT_PROJECT_OK ||
       pt_channels_paula_map(&p->channels,previous,r.map)!=PT_CHANNEL_OK)goto done;
    if(o->row_range){r.result=PT_MIXED_RANGE;goto done;}
    for(i=0;i<p->channels.count;++i)if((o->tracks&(1U<<i)) &&
       p->channels.track[i].route!=PT_PAULA && p->channels.track[i].route!=PT_AMIGUS){r.result=PT_MIXED_ROUTE;r.channel=i;goto done;}
    w=a->allocate(a->context,sizeof(*w));if(!w){r.result=PT_MIXED_MEMORY;goto done;}
    memset(w,0,sizeof(*w));w->allocator=*a;
    r.render_result=pt_render_sequence_begin(p,o,a,&w->sequence);
    if(r.render_result!=PT_RENDER_OK) {
        r.result=r.render_result==PT_RENDER_MEMORY?PT_MIXED_MEMORY:PT_MIXED_RENDER;
        pt_mixed_preflight_close(&w);goto done;
    }
    w->project=p;w->options=*o;w->caps=*caps;w->format=*f;w->pc=pc;w->ac=ac;
    r.result=PT_MIXED_PENDING;w->report=r;*work=w;
done:
    report(&r,out);return r.result;
}
enum pt_mixed_result pt_mixed_preflight_step(struct pt_mixed_preflight *w,struct pt_mixed_report *out)
{
    struct pt_mixed_report *r;unsigned ready;
    if(!w || !out)return PT_MIXED_INVALID;
    if(w->checked_setup&&w->setup_busy){w->setup_failed=1;if(w->setup_callback_failure)*w->setup_callback_failure=1;return PT_MIXED_INVALID;}
    if(w->checked_setup&&(w->setup_mode||w->setup_failed||!mixed_checked_output(w,out,sizeof(*out))))return PT_MIXED_INVALID;
    r=&w->report;if(r->result!=PT_MIXED_PENDING)goto done;
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
        r->render_result=pt_render_sequence_complete(w->sequence,&w->batch.all);
        if(r->render_result!=PT_RENDER_OK)break;
        r->result=batch(w->project,w->options.rate,&w->batch,&w->caps,&w->format,w->pc,w->ac,w->held,r);
        if(r->result==PT_MIXED_OK && !w->span.end){r->result=PT_MIXED_PENDING;w->phase=1;}
        break;
    }
    if(r->render_result!=PT_RENDER_OK)r->result=r->render_result==PT_RENDER_MEMORY?PT_MIXED_MEMORY:PT_MIXED_RENDER;
done:
    report(r,out);return r->result;
}
int pt_mixed_preflight_take(struct pt_mixed_preflight *w,struct pt_render_sequence **out)
{
    if(w&&w->checked_setup&&w->setup_busy){w->setup_failed=1;if(w->setup_callback_failure)*w->setup_callback_failure=1;return 0;}
    if(!w || !out || w->report.result!=PT_MIXED_OK || !w->sequence)return 0;
    if(w->checked_setup&&(w->setup_mode||w->setup_failed||!mixed_checked_output(w,out,sizeof(*out))))return 0;
    w->report.render_result=pt_render_sequence_rewind(w->sequence);
    if(w->report.render_result!=PT_RENDER_OK){w->report.result=PT_MIXED_RENDER;return 0;}
    *out=w->sequence;w->sequence=NULL;return 1;
}
enum pt_mixed_result pt_mixed_preflight(const struct pt_project *p,const struct pt_render_options *o,
    const int8_t *previous,const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,
    unsigned pc,unsigned ac,const struct pt_allocator *a,struct pt_mixed_report *out,struct pt_render_sequence **take)
{
    struct pt_mixed_preflight *w=NULL;
    enum pt_mixed_result result=pt_mixed_preflight_begin(p,o,previous,caps,f,pc,ac,a,out,&w);
    while(result==PT_MIXED_PENDING)result=pt_mixed_preflight_step(w,out);
    if(result==PT_MIXED_OK && take && !pt_mixed_preflight_take(w,take))result=pt_mixed_preflight_step(w,out);
    pt_mixed_preflight_close(&w);return result;
}

/* Initial setup owns the real opaque renderer startup, never an exposed
 * validator/flow certificate. This allocation becomes the actual audit owner. */
static int mixed_setup_span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int mixed_setup_apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return mixed_setup_span(a,an)&&mixed_setup_span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);
}
/* Fixed captures only: usable for closing after sequence ownership was moved,
 * or source headers changed, without traversing former source descriptors. */
static int mixed_setup_control_apart(const struct pt_mixed_preflight *w,const void *out,size_t n)
{
    const struct pt_project *p=&w->setup_header;
    return mixed_setup_apart(w,sizeof(*w),out,n)&&
        mixed_setup_apart(w->project,sizeof(*w->project),out,n)&&
        mixed_setup_apart(p->orders,p->order_count*sizeof(*p->orders),out,n)&&
        mixed_setup_apart(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events),out,n)&&
        mixed_setup_apart(p->samples,p->sample_count*sizeof(*p->samples),out,n)&&
        mixed_setup_apart(p->extensions,p->extension_count*sizeof(*p->extensions),out,n)&&
        mixed_setup_apart(w->caps_source,w->caps_source?sizeof(*w->caps_source):0,out,n)&&
        mixed_setup_apart(w->format_source,w->format_source?sizeof(*w->format_source):0,out,n)&&
        mixed_setup_apart(w->previous_source,w->previous_source?sizeof(w->previous):0,out,n);
}
static int mixed_checked_output(const struct pt_mixed_preflight *w,const void *out,size_t n)
{
    return mixed_setup_apart(w,sizeof(*w),out,n)&&
        pt_render_sequence_output_disjoint(w->sequence,out,n);
}
static enum pt_render_setup_result mixed_setup_enter(struct pt_mixed_preflight *w)
{
    if(!mixed_setup_span(w,sizeof(*w))||!w->setup_mode)return PT_RENDER_SETUP_INVALID;
    if(w->setup_busy){w->setup_failed=1;if(w->setup_callback_failure)*w->setup_callback_failure=1;return PT_RENDER_SETUP_BUSY;}
    return PT_RENDER_SETUP_PENDING;
}
static enum pt_render_setup_result mixed_setup_current(struct pt_mixed_preflight *w,uint32_t revision,uint32_t generation)
{
    if(w->setup_failed)return PT_RENDER_SETUP_FAILED;
    if(memcmp(w->caps_source,&w->caps,sizeof(w->caps))||
       memcmp(w->format_source,&w->format,sizeof(w->format))||
       (w->previous_source&&memcmp(w->previous_source,w->previous,sizeof(w->previous))))return PT_RENDER_SETUP_STALE;
    return pt_render_sequence_setup_get(w->startup,revision,generation,NULL);
}
static int mixed_setup_output_apart(struct pt_mixed_preflight *w,uint32_t revision,uint32_t generation,const void *out,size_t n)
{
    return mixed_setup_apart(w,sizeof(*w),out,n)&&
        mixed_setup_apart(w->caps_source,sizeof(*w->caps_source),out,n)&&
        mixed_setup_apart(w->format_source,sizeof(*w->format_source),out,n)&&
        mixed_setup_apart(w->previous_source,w->previous_source?sizeof(w->previous):0,out,n)&&
        pt_render_sequence_setup_output_disjoint(w->startup,revision,generation,out,n);
}
enum pt_render_setup_result pt_mixed_preflight_setup_begin(const struct pt_project *p,
    const struct pt_render_options *o,const int8_t *previous,const struct pt_paula_render_caps *caps,
    const struct pt_playback_format *f,unsigned pc,unsigned ac,const struct pt_allocator *a,
    uint32_t revision,uint32_t generation,
    struct pt_mixed_preflight_setup **out)
{
    struct pt_render_sequence_setup *startup=NULL;struct pt_mixed_preflight *w=NULL;
    struct pt_paula_render_caps saved_caps;struct pt_playback_format saved_format;
    int8_t saved_map[PT_CHANNEL_LIMIT]={0};struct pt_allocator saved_allocator;
    struct pt_render_setup_guard guards[8];enum pt_render_setup_result r;
    if(!mixed_setup_span(out,sizeof(*out))||!mixed_setup_span(caps,sizeof(*caps))||
       !mixed_setup_span(f,sizeof(*f))||!mixed_setup_span(a,sizeof(*a))||
       !mixed_setup_span(previous,previous?sizeof(saved_map):0)||pc>1||ac>1||
       !a->allocate||!a->release||!pt_paula_render_caps_valid(caps)||
       (f->bits!=8&&f->bits!=16)||f->channel||f->word_pad||f->little_endian>1)return PT_RENDER_SETUP_INVALID;
    memcpy(&saved_caps,caps,sizeof(saved_caps));memcpy(&saved_format,f,sizeof(saved_format));
    memcpy(&saved_allocator,a,sizeof(saved_allocator));
    if(previous)memcpy(saved_map,previous,sizeof(saved_map));
    guards[0]=(struct pt_render_setup_guard){caps,sizeof(*caps)};
    guards[1]=(struct pt_render_setup_guard){f,sizeof(*f)};
    guards[2]=(struct pt_render_setup_guard){previous,previous?sizeof(saved_map):0};
    guards[3]=(struct pt_render_setup_guard){out,sizeof(*out)};
    guards[4]=(struct pt_render_setup_guard){&saved_caps,sizeof(saved_caps)};
    guards[5]=(struct pt_render_setup_guard){&saved_format,sizeof(saved_format)};
    guards[6]=(struct pt_render_setup_guard){saved_map,sizeof(saved_map)};
    guards[7]=(struct pt_render_setup_guard){&saved_allocator,sizeof(saved_allocator)};
    r=pt_render_sequence_setup_begin(p,o,a,revision,generation,guards,8,&startup);
    if(r!=PT_RENDER_SETUP_PENDING)return r;
    if(!pt_render_sequence_setup_output_disjoint(startup,revision,generation,out,sizeof(*out))||
       !mixed_setup_apart(out,sizeof(*out),caps,sizeof(*caps))||
       !mixed_setup_apart(out,sizeof(*out),f,sizeof(*f))||
       !mixed_setup_apart(out,sizeof(*out),previous,previous?sizeof(saved_map):0)){
        pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_ALIAS;
    }
    if(*out){pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_INVALID;}
    if(memcmp(caps,&saved_caps,sizeof(saved_caps))||memcmp(f,&saved_format,sizeof(saved_format))||
       (previous&&memcmp(previous,saved_map,sizeof(saved_map)))){
        pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_STALE;
    }
    w=saved_allocator.allocate(saved_allocator.context,sizeof(*w));
    /* Classify against the actual retained startup and original fixed extents
     * BEFORE stale cleanup. An overlapping return never granted fresh ownership:
     * releasing it could free startup/source and make cancellation use freed data. */
    if(w&&(!pt_render_sequence_setup_control_output_disjoint(startup,w,sizeof(*w))||
       !mixed_setup_apart(w,sizeof(*w),caps,sizeof(*caps))||
       !mixed_setup_apart(w,sizeof(*w),f,sizeof(*f))||
       !mixed_setup_apart(w,sizeof(*w),previous,previous?sizeof(saved_map):0)||
       !mixed_setup_apart(w,sizeof(*w),out,sizeof(*out))||
       !mixed_setup_apart(w,sizeof(*w),&saved_caps,sizeof(saved_caps))||
       !mixed_setup_apart(w,sizeof(*w),&saved_format,sizeof(saved_format))||
       !mixed_setup_apart(w,sizeof(*w),saved_map,sizeof(saved_map))||
       !mixed_setup_apart(w,sizeof(*w),&saved_allocator,sizeof(saved_allocator))||
       !mixed_setup_apart(w,sizeof(*w),guards,sizeof(guards))||
       !mixed_setup_apart(w,sizeof(*w),&startup,sizeof(startup))||
       !mixed_setup_apart(w,sizeof(*w),&w,sizeof(w))||!mixed_setup_apart(w,sizeof(*w),&r,sizeof(r)))){
        pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_ALIAS;
    }
    r=pt_render_sequence_setup_get(startup,revision,generation,NULL);
    if(r!=PT_RENDER_SETUP_PENDING||memcmp(caps,&saved_caps,sizeof(saved_caps))||
       memcmp(f,&saved_format,sizeof(saved_format))||
       (previous&&memcmp(previous,saved_map,sizeof(saved_map)))||*out){
        if(w)saved_allocator.release(saved_allocator.context,w);
        pt_render_sequence_setup_cancel(&startup);return r==PT_RENDER_SETUP_FAILED?r:PT_RENDER_SETUP_STALE;
    }
    if(!w){pt_render_sequence_setup_cancel(&startup);return PT_RENDER_SETUP_CAPACITY;}
    if(!pt_render_sequence_setup_output_disjoint(startup,revision,generation,w,sizeof(*w))||
       !mixed_setup_apart(w,sizeof(*w),caps,sizeof(*caps))||
       !mixed_setup_apart(w,sizeof(*w),f,sizeof(*f))||
       !mixed_setup_apart(w,sizeof(*w),previous,previous?sizeof(saved_map):0)||
       !mixed_setup_apart(w,sizeof(*w),out,sizeof(*out))||
       !pt_render_sequence_setup_output_disjoint(startup,revision,generation,out,sizeof(*out))){
        pt_render_sequence_setup_cancel(&startup);
        return PT_RENDER_SETUP_ALIAS;
    }
    memset(w,0,sizeof(*w));w->allocator=saved_allocator;w->project=p;memcpy(&w->options,o,sizeof(*o));
    memcpy(&w->setup_header,p,sizeof(w->setup_header));
    memcpy(&w->caps,&saved_caps,sizeof(w->caps));memcpy(&w->format,&saved_format,sizeof(w->format));
    w->caps_source=caps;w->format_source=f;w->previous_source=previous;
    if(previous)memcpy(w->previous,saved_map,sizeof(saved_map));
    w->startup=startup;w->pc=pc;w->ac=ac;w->setup_mode=1;w->checked_setup=1;init(&w->report);w->report.result=PT_MIXED_PENDING;
    *out=(struct pt_mixed_preflight_setup *)w;return PT_RENDER_SETUP_PENDING;
}
enum pt_render_setup_result pt_mixed_preflight_setup_step(struct pt_mixed_preflight_setup *setup,
    uint32_t revision,uint32_t generation,unsigned work)
{
    struct pt_mixed_preflight *w=(struct pt_mixed_preflight *)setup;enum pt_render_setup_result r=mixed_setup_enter(w);
    if(r!=PT_RENDER_SETUP_PENDING)return r;
    if(!work||work>PT_PROJECT_VALIDATION_WORK_MAX)return PT_RENDER_SETUP_INVALID;
    r=mixed_setup_current(w,revision,generation);if(r!=PT_RENDER_SETUP_PENDING)return r;
    return pt_render_sequence_setup_step(w->startup,revision,generation,work);
}
enum pt_render_setup_result pt_mixed_preflight_setup_get(struct pt_mixed_preflight_setup *setup,
    uint32_t revision,uint32_t generation,struct pt_render_setup_report *out)
{
    struct pt_mixed_preflight *w=(struct pt_mixed_preflight *)setup;enum pt_render_setup_result r=mixed_setup_enter(w);
    if(r!=PT_RENDER_SETUP_PENDING)return r;
    r=mixed_setup_current(w,revision,generation);if(r==PT_RENDER_SETUP_STALE||r==PT_RENDER_SETUP_FAILED)return r;
    if(out&&!mixed_setup_output_apart(w,revision,generation,out,sizeof(*out)))return PT_RENDER_SETUP_ALIAS;
    return pt_render_sequence_setup_get(w->startup,revision,generation,out);
}
enum pt_render_setup_result pt_mixed_preflight_setup_transfer(struct pt_mixed_preflight_setup **owner,
    uint32_t revision,uint32_t generation,struct pt_mixed_preflight **out)
{
    struct pt_mixed_preflight *w;enum pt_render_setup_result r;struct pt_render_setup_guard guards[6];int8_t map[PT_CHANNEL_LIMIT];unsigned i;
    struct pt_mixed_preflight *original_out;
    if(!mixed_setup_span(owner,sizeof(*owner))||!mixed_setup_span(out,sizeof(*out))||
       !mixed_setup_apart(owner,sizeof(*owner),out,sizeof(*out)))return PT_RENDER_SETUP_INVALID;
    w=(struct pt_mixed_preflight *)*owner;r=mixed_setup_enter(w);if(r!=PT_RENDER_SETUP_PENDING)return r;
    r=mixed_setup_current(w,revision,generation);if(r!=PT_RENDER_SETUP_READY)return r;
    if(!mixed_setup_output_apart(w,revision,generation,owner,sizeof(*owner))||
       !mixed_setup_output_apart(w,revision,generation,out,sizeof(*out)))return PT_RENDER_SETUP_ALIAS;
    if((w->options.rate!=44100&&w->options.rate!=48000)||w->options.row_range||
       pt_channels_paula_map(&w->project->channels,w->previous_source,map)!=PT_CHANNEL_OK)
        {w->setup_failed=1;return PT_RENDER_SETUP_FAILED;}
    for(i=0;i<w->project->channels.count;++i)if((w->options.tracks&(1U<<i))&&
        w->project->channels.track[i].route!=PT_PAULA&&w->project->channels.track[i].route!=PT_AMIGUS)
        {w->setup_failed=1;return PT_RENDER_SETUP_FAILED;}
    guards[0]=(struct pt_render_setup_guard){w,sizeof(*w)};
    guards[1]=(struct pt_render_setup_guard){w->caps_source,sizeof(*w->caps_source)};
    guards[2]=(struct pt_render_setup_guard){w->format_source,sizeof(*w->format_source)};
    guards[3]=(struct pt_render_setup_guard){w->previous_source,w->previous_source?sizeof(w->previous):0};
    guards[4]=(struct pt_render_setup_guard){owner,sizeof(*owner)};
    guards[5]=(struct pt_render_setup_guard){out,sizeof(*out)};
    original_out=*out;
    w->setup_busy=1;r=pt_render_sequence_setup_take(&w->startup,revision,generation,guards,6,&w->sequence);
    w->setup_busy=0;
    if(r!=PT_RENDER_SETUP_READY){
        if(!w->startup)w->setup_failed=1;
        return w->setup_failed?PT_RENDER_SETUP_FAILED:r;
    }
    if(w->setup_failed||memcmp(w->caps_source,&w->caps,sizeof(w->caps))||
       memcmp(w->format_source,&w->format,sizeof(w->format))||
       (w->previous_source&&memcmp(w->previous_source,w->previous,sizeof(w->previous)))||
       *owner!=(struct pt_mixed_preflight_setup *)w||*out!=original_out||
       !mixed_setup_apart(w,sizeof(*w),out,sizeof(*out))||
       !pt_render_sequence_output_disjoint(w->sequence,out,sizeof(*out))){w->setup_failed=1;return PT_RENDER_SETUP_FAILED;}
    memcpy(w->report.map,map,sizeof(map));w->setup_mode=0;
    /* Actual audit owns these copies; original setup input controls are no longer
     * borrowed and may be reused after transfer. Project/source remains borrowed. */
    w->caps_source=NULL;w->format_source=NULL;w->previous_source=NULL;
    *owner=NULL;*out=w;return PT_RENDER_SETUP_READY;
}
enum pt_render_setup_result pt_mixed_preflight_setup_cancel(struct pt_mixed_preflight_setup **owner)
{
    struct pt_mixed_preflight *w;struct pt_allocator a;enum pt_render_setup_result r;unsigned callback_failure=0;
    if(!mixed_setup_span(owner,sizeof(*owner)))return PT_RENDER_SETUP_INVALID;
    if(!*owner)return PT_RENDER_SETUP_READY;
    w=(struct pt_mixed_preflight *)*owner;r=mixed_setup_enter(w);if(r!=PT_RENDER_SETUP_PENDING)return r;
    /* Genuine external handle storage, fixed captures only; never former tables. */
    if(!mixed_setup_control_apart(w,owner,sizeof(*owner)))
        return PT_RENDER_SETUP_ALIAS;
    a=w->allocator;w->setup_busy=1;w->setup_callback_failure=&callback_failure;
    r=pt_render_sequence_setup_cancel(&w->startup);if(r!=PT_RENDER_SETUP_READY&&w->startup){w->setup_busy=0;w->setup_callback_failure=NULL;return r;}
    pt_render_sequence_close(w->sequence);a.release(a.context,w);
    if(*owner==(struct pt_mixed_preflight_setup *)w)*owner=NULL;else callback_failure=1;
    return callback_failure||r!=PT_RENDER_SETUP_READY?PT_RENDER_SETUP_FAILED:PT_RENDER_SETUP_READY;
}
