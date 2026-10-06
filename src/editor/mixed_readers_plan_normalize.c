#include <limits.h>
#include <stddef.h>
#include <string.h>
#include "mixed_readers_plan_normalize.h"
#include "../core/amigus_render_voice.h"
#include "../core/render_storage_internal.h"

#define PLAN_MAGIC UINT64_C(0x50544d504c414e31)
#define PLAN_EXTENSIONS 4090U
/* 5 project/header tables +510 sample extents +4090 payloads +6 named
 * inputs/vectors +32 extra context spans +1 known owner slot =4644. */
#define PLAN_GUARDS 4656U
struct pt_mixed_plan_normalizer {
    uint64_t magic;size_t capacity;
    struct pt_mixed_plan_normalizer **owner;unsigned owner_guard;
    const struct pt_mixed_plan_inputs *inputs;
    struct pt_mixed_plan_inputs saved;
    struct pt_project header;
    struct pt_render_plan plan;
    struct pt_sample samples[PT_PROJECT_SAMPLES];
    struct pt_extension extensions[PLAN_EXTENSIONS];
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS];
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_mixed_plan_span contexts[PT_MIXED_PLAN_CONTEXTS];
    struct pt_mixed_plan_span guards[PLAN_GUARDS];unsigned guard_count;
    struct pt_mixed_plan_batch batch;
    struct pt_mixed_plan_report report;
    int first[PT_MIXED_PLAN_RECORDS],control[PT_MIXED_PLAN_RECORDS];
    int8_t slots[PT_MIXED_PLAN_RECORDS];
    unsigned index,track,source,candidate,busy;
};
/* Always disjoint local outputs for the canonical converters. Never pass an
 * embedded job field as their writable result. The whole scratch is guarded
 * BEFORE initialization, writable job entry or a stale/failure latch. */
struct scratch {
    struct pt_voice voice;uint32_t gains[2];
    struct pt_paula_render_plan paula;
    struct pt_amigus_voice_plan card;
    struct pt_amigus_voice_request request;
};
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);
}
static int source_shape(const struct pt_project *p)
{
    unsigned i;
    if(!span(p,sizeof(*p))||(uintptr_t)p%_Alignof(struct pt_project)||
       !p->channels.count||p->channels.count>PT_CHANNEL_LIMIT||
       !p->order_count||p->order_count>PT_PROJECT_ORDERS||
       !p->pattern_count||p->pattern_count>PT_PROJECT_PATTERNS||
       p->sample_count>PT_PROJECT_SAMPLES||p->extension_count>PLAN_EXTENSIONS||
       !span(p->orders,p->order_count*sizeof(*p->orders))||
       (uintptr_t)p->orders%_Alignof(uint16_t)||
       !span(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events))||
       (uintptr_t)p->events%_Alignof(struct pt_event)||
       !span(p->samples,p->sample_count*sizeof(*p->samples))||
       (uintptr_t)p->samples%_Alignof(struct pt_sample)||
       !span(p->extensions,p->extension_count*sizeof(*p->extensions))||
       (uintptr_t)p->extensions%_Alignof(struct pt_extension))return 0;
    for(i=0;i<p->sample_count;++i){
        const struct pt_sample *s=p->samples+i;
        if(s->pcm.capacity>SIZE_MAX/sizeof(*s->pcm.data)||s->slice_count>PT_PROJECT_SLICES||
           !span(s->pcm.data,s->pcm.capacity*sizeof(*s->pcm.data))||
           (uintptr_t)s->pcm.data%_Alignof(int32_t)||
           !span(s->slices,s->slice_count*sizeof(*s->slices))||
           (uintptr_t)s->slices%_Alignof(uint32_t))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(!span(p->extensions[i].data,p->extensions[i].length))return 0;
    return 1;
}
static int initial_apart(const struct pt_mixed_plan_inputs *v,const void *out,size_t n)
{
    unsigned i;
    if(!span(v,sizeof(*v))||(uintptr_t)v%_Alignof(struct pt_mixed_plan_inputs)||
       !span(out,n)||!span(v->plan,sizeof(*v->plan))||
       (uintptr_t)v->plan%_Alignof(struct pt_render_plan)||
       !span(v->origins,PT_MIXED_PLAN_RECORDS*sizeof(*v->origins))||
       (uintptr_t)v->origins%_Alignof(struct pt_mixed_plan_origin)||
       !span(v->caps,sizeof(*v->caps))||!span(v->format,sizeof(*v->format))||
       (uintptr_t)v->caps%_Alignof(struct pt_paula_render_caps)||
       (uintptr_t)v->format%_Alignof(struct pt_playback_format)||
       v->context_count>PT_MIXED_PLAN_CONTEXTS||
       !span(v->contexts,v->context_count*sizeof(*v->contexts))||
       (uintptr_t)v->contexts%_Alignof(struct pt_mixed_plan_span)||
       !apart(v,sizeof(*v),out,n)||!apart(v->plan,sizeof(*v->plan),out,n)||
       !apart(v->origins,PT_MIXED_PLAN_RECORDS*sizeof(*v->origins),out,n)||
       !apart(v->caps,sizeof(*v->caps),out,n)||!apart(v->format,sizeof(*v->format),out,n)||
       !apart(v->contexts,v->context_count*sizeof(*v->contexts),out,n)||!source_shape(v->project)||
       !pt_render_project_storage_output_disjoint(v->project,out,n))return 0;
    for(i=0;i<v->context_count;++i)
        if(!apart(v->contexts[i].data,v->contexts[i].bytes,out,n))return 0;
    return 1;
}
static int valid(const struct pt_mixed_plan_normalizer *j)
{return span(j,sizeof(*j))&&!((uintptr_t)j%_Alignof(struct pt_mixed_plan_normalizer))&&
    j->magic==PLAN_MAGIC&&j->capacity>=sizeof(*j)&&span(j,j->capacity);}
static int initial_local_apart(const struct pt_mixed_plan_inputs *v,uintptr_t address,size_t n)
{return initial_apart(v,(const void *)address,n);}
static int output_apart(const struct pt_mixed_plan_normalizer *j,const void *out,size_t n)
{
    unsigned i;
    if(!span(out,n)||!apart(j,j->capacity,out,n))return 0;
    for(i=0;i<j->guard_count;++i)
        if(!apart(j->guards[i].data,j->guards[i].bytes,out,n))return 0;
    return 1;
}
static int local_apart(const struct pt_mixed_plan_normalizer *j,uintptr_t address,size_t n)
{return output_apart(j,(const void *)address,n);}
static int close_apart(const struct pt_mixed_plan_normalizer *j,
    struct pt_mixed_plan_normalizer **owner)
{
    unsigned i;
    if(owner!=j->owner||!apart(j,j->capacity,owner,sizeof(*owner))||
       j->owner_guard>=j->guard_count||j->guards[j->owner_guard].data!=owner||
       j->guards[j->owner_guard].bytes!=sizeof(*owner))return 0;
    /* Only this exact captured noncopyable owner slot may be consumed. The
     * workspace and EVERY other captured span retain their complete guards. */
    for(i=0;i<j->guard_count;++i)
        if(i!=j->owner_guard&&!apart(j->guards[i].data,j->guards[i].bytes,
                owner,sizeof(*owner)))return 0;
    return 1;
}
static void capture(struct pt_mixed_plan_normalizer *j,const void *p,size_t n)
{
    /* Called only after complete initial guards. The documented metadata/count
     * bounds prove this fixed vector's maximum; no fallible partial adoption. */
    if(n){j->guards[j->guard_count].data=p;j->guards[j->guard_count++].bytes=n;}
}
static void sources(struct pt_mixed_plan_normalizer *j)
{
    const struct pt_project *p=j->saved.project;unsigned i;
    capture(j,j->inputs,sizeof(*j->inputs));capture(j,p,sizeof(*p));
    capture(j,p->orders,p->order_count*sizeof(*p->orders));
    capture(j,p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events));
    capture(j,p->samples,p->sample_count*sizeof(*p->samples));
    capture(j,p->extensions,p->extension_count*sizeof(*p->extensions));
    for(i=0;i<p->sample_count;++i){
        const struct pt_sample *s=p->samples+i;
        capture(j,s->pcm.data,s->pcm.capacity*sizeof(*s->pcm.data));
        capture(j,s->slices,s->slice_count*sizeof(*s->slices));
    }
    for(i=0;i<p->extension_count;++i)capture(j,p->extensions[i].data,p->extensions[i].length);
    capture(j,j->saved.plan,sizeof(*j->saved.plan));
    capture(j,j->saved.origins,sizeof(j->origins));capture(j,j->saved.caps,sizeof(j->caps));
    capture(j,j->saved.format,sizeof(j->format));
    capture(j,j->saved.contexts,j->saved.context_count*sizeof(*j->saved.contexts));
    for(i=0;i<j->saved.context_count;++i)capture(j,j->contexts[i].data,j->contexts[i].bytes);
    j->owner_guard=j->guard_count;capture(j,j->owner,sizeof(*j->owner));
}
static int current(const struct pt_mixed_plan_normalizer *j,uint32_t revision,uint32_t generation)
{
    const struct pt_project *p=j->saved.project;size_t k;
    if(revision!=j->saved.revision||generation!=j->saved.generation||
       memcmp(j->inputs,&j->saved,sizeof(j->saved)))return 0;
    /* These original fixed controls remain readable. No former table walk
     * before actual source tags and pointer/count/header identity match. */
    k=offsetof(struct pt_project,channels)+offsetof(struct pt_channels,selected);
    if(p->channels.selected>=j->header.channels.count||memcmp(p,&j->header,k)||
       memcmp((const uint8_t *)p+k+sizeof(p->channels.selected),
              (const uint8_t *)&j->header+k+sizeof(p->channels.selected),
              sizeof(*p)-k-sizeof(p->channels.selected))||
       memcmp(j->saved.plan,&j->plan,sizeof(j->plan))||
       memcmp(j->saved.origins,j->origins,sizeof(j->origins))||
       memcmp(j->saved.caps,&j->caps,sizeof(j->caps))||
       memcmp(j->saved.format,&j->format,sizeof(j->format))||
       (j->saved.context_count&&memcmp(j->saved.contexts,j->contexts,
            j->saved.context_count*sizeof(*j->saved.contexts))))return 0;
    return (!p->sample_count||!memcmp(p->samples,j->samples,p->sample_count*sizeof(*p->samples)))&&
        (!p->extension_count||!memcmp(p->extensions,j->extensions,p->extension_count*sizeof(*p->extensions)));
}
size_t pt_mixed_plan_normalizer_workspace_size(void){return sizeof(struct pt_mixed_plan_normalizer);}
size_t pt_mixed_plan_normalizer_workspace_alignment(void){return _Alignof(struct pt_mixed_plan_normalizer);}
enum pt_mixed_plan_result pt_mixed_plan_normalizer_begin_in_workspace(
    void *storage,size_t capacity,const struct pt_mixed_plan_inputs *v,struct pt_mixed_plan_normalizer **out)
{
    struct pt_mixed_plan_normalizer *j=storage;int8_t map[PT_MIXED_PLAN_RECORDS];
    const uint8_t *bytes=storage;size_t i;unsigned card=0;
    if(!span(storage,capacity)||capacity<sizeof(*j)||
       (uintptr_t)storage%_Alignof(struct pt_mixed_plan_normalizer)||
       !span(v,sizeof(*v))||(uintptr_t)v%_Alignof(struct pt_mixed_plan_inputs)||
       !span(out,sizeof(*out))||(uintptr_t)out%_Alignof(struct pt_mixed_plan_normalizer *))return PT_MIXED_PLAN_INVALID;
    /* Genuine existing workspace reuse is refused with numeric capture guards;
     * no incoming source walk, zeroing or mutation of an adopted job. Only the
     * exact original owner slot preserves INVALID after all other guards. */
    if(j->magic==PLAN_MAGIC){
        if(!valid(j)||!apart(j,j->capacity,v,sizeof(*v))||
           (out==j->owner?!close_apart(j,out):!output_apart(j,out,sizeof(*out))))
            return PT_MIXED_PLAN_ALIAS;
        return PT_MIXED_PLAN_INVALID;
    }
    if(!apart(storage,capacity,out,sizeof(*out))||
       !initial_apart(v,storage,capacity)||!initial_apart(v,out,sizeof(*out))||
       !initial_local_apart(v,(uintptr_t)&map,sizeof(map))||
       !apart(storage,capacity,(const void *)(uintptr_t)&map,sizeof(map))||
       !apart(out,sizeof(*out),(const void *)(uintptr_t)&map,sizeof(map)))return PT_MIXED_PLAN_ALIAS;
    if(*out||v->frame==UINT64_MAX||(v->rate!=44100&&v->rate!=48000)||
       !pt_paula_render_caps_valid(v->caps)||
       (v->format->bits!=8&&v->format->bits!=16)||v->format->channel||
       v->format->word_pad||v->format->little_endian>1||
       pt_channels_paula_map(&v->project->channels,NULL,map)!=PT_CHANNEL_OK)return PT_MIXED_PLAN_INVALID;
    for(i=0;i<sizeof(*j);++i)if(bytes[i])return PT_MIXED_PLAN_INVALID;
    memset(j,0,sizeof(*j));j->capacity=capacity;j->owner=out;
    j->inputs=v;j->saved=*v;j->header=*v->project;
    memcpy(&j->plan,v->plan,sizeof(j->plan));memcpy(j->origins,v->origins,sizeof(j->origins));
    j->caps=*v->caps;j->format=*v->format;
    if(v->project->sample_count)memcpy(j->samples,v->project->samples,v->project->sample_count*sizeof(*j->samples));
    if(v->project->extension_count)memcpy(j->extensions,v->project->extensions,v->project->extension_count*sizeof(*j->extensions));
    if(v->context_count)memcpy(j->contexts,v->contexts,v->context_count*sizeof(*j->contexts));
    memcpy(j->batch.next,j->origins,sizeof(j->origins));j->batch.frame=v->frame;
    for(i=0;i<PT_MIXED_PLAN_RECORDS;++i){j->first[i]=j->control[i]=-1;j->slots[i]=map[i];
        if(i<v->project->channels.count&&v->project->channels.track[i].route==PT_AMIGUS)
            j->slots[i]=(int8_t)card++;
    }
    sources(j);j->report.result=PT_MIXED_PLAN_PENDING;j->report.phase=PT_MIXED_PLAN_SHAPES;
    j->report.action=j->report.track=j->report.kind=UINT_MAX;j->magic=PLAN_MAGIC;
    *out=j;return PT_MIXED_PLAN_PENDING;
}
static enum pt_mixed_plan_result refuse(struct pt_mixed_plan_normalizer *j,enum pt_mixed_plan_reason reason)
{j->report.reason=reason;j->report.result=PT_MIXED_PLAN_REFUSED;return j->report.result;}
static int same_voice(const struct pt_voice *a,const struct pt_voice *b)
{
    return a->pcm==b->pcm&&a->repeat_pcm==b->repeat_pcm&&a->phase==b->phase&&a->cycle==b->cycle&&
        a->start==b->start&&a->end==b->end&&a->loop_start==b->loop_start&&a->loop_end==b->loop_end&&
        a->loop==b->loop&&a->looped==b->looped&&a->linear==b->linear&&
        a->active==b->active&&a->segment==b->segment;
}
static int same_image(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{
    return a->start==b->start&&a->loop==b->loop&&a->end_exclusive==b->end_exclusive&&
        a->rate==b->rate&&a->control==b->control&&a->left==b->left&&a->right==b->right;
}
static int same_bounds(const struct pt_amigus_voice_plan *a,const struct pt_amigus_voice_plan *b)
{
    return a->start==b->start&&a->loop==b->loop&&a->end_exclusive==b->end_exclusive&&
        a->rate==b->rate&&a->control==b->control;
}
static void effective(struct pt_mixed_plan_normalizer *j,struct scratch *s)
{
    const struct pt_render_action *a=j->plan.action+j->first[j->track];
    s->voice=a->voice;s->gains[0]=a->gain[0];s->gains[1]=a->gain[1];
    if(j->control[j->track]>=0){a=j->plan.action+j->control[j->track];
        s->voice.step=a->voice.step;s->gains[0]=a->gain[0];s->gains[1]=a->gain[1];}
}
static void accepted(struct pt_mixed_plan_normalizer *j)
{
    struct pt_mixed_plan_record *r=j->batch.record+j->batch.count;
    if(r->kind==PT_MIXED_PLAN_TRIGGER){
        j->batch.next[r->track]=(struct pt_mixed_plan_origin){1,r->sample,r->channel,j->saved.frame};
        j->batch.samples[r->route==PT_PAULA?0:1][r->sample]=1;
    }else if(r->kind==PT_MIXED_PLAN_STOP)memset(j->batch.next+r->track,0,sizeof(*j->batch.next));
    ++j->batch.count;++j->track;j->report.phase=PT_MIXED_PLAN_RECORD;
}
static void one(struct pt_mixed_plan_normalizer *j,struct scratch *s)
{
    struct pt_mixed_plan_record *r=j->batch.record+j->batch.count;
    const struct pt_render_action *a;const struct pt_sample *sample;
    const struct pt_mixed_plan_origin *origin;unsigned k;uint64_t bytes,numerator;
    switch(j->report.phase){
    case PT_MIXED_PLAN_SHAPES:
        if(j->plan.count>PT_RENDER_ACTIONS){refuse(j,PT_MIXED_PLAN_REASON_COUNT);break;}
        if(j->index==j->plan.count){j->index=0;j->report.phase=PT_MIXED_PLAN_ORIGINS;break;}
        a=j->plan.action+j->index;j->report.action=j->index;j->report.track=a->channel;
        j->report.kind=(unsigned)a->kind;++j->report.actions_scanned;
        if(a->channel>=j->header.channels.count||
           (j->header.channels.track[a->channel].route!=PT_PAULA&&j->header.channels.track[a->channel].route!=PT_AMIGUS)){
            refuse(j,PT_MIXED_PLAN_REASON_ROUTE);break;}
        if(a->kind!=PT_RENDER_TRIGGER&&a->kind!=PT_RENDER_CONTROL&&a->kind!=PT_RENDER_STOP){
            refuse(j,PT_MIXED_PLAN_REASON_KIND);break;}
        k=a->channel;
        if(j->first[k]<0)j->first[k]=(int)j->index;
        else if(j->plan.action[j->first[k]].kind!=PT_RENDER_TRIGGER||
                a->kind!=PT_RENDER_CONTROL||j->control[k]>=0){refuse(j,PT_MIXED_PLAN_REASON_DUPLICATE);break;}
        else if(!same_voice(&j->plan.action[j->first[k]].voice,&a->voice)){refuse(j,PT_MIXED_PLAN_REASON_FOLD);break;}
        else j->control[k]=(int)j->index;
        ++j->index;break;
    case PT_MIXED_PLAN_ORIGINS:
        if(j->index==PT_MIXED_PLAN_RECORDS){j->report.phase=PT_MIXED_PLAN_RECORD;break;}
        origin=j->origins+j->index;j->report.track=j->index;j->report.action=j->report.kind=UINT_MAX;
        if(origin->present>1||(!origin->present&&(origin->sample||origin->channel||origin->frame))||
           (origin->present&&(j->index>=j->header.channels.count||origin->sample>=j->header.sample_count||
            origin->channel||origin->frame>=j->saved.frame||
            (j->header.channels.track[j->index].route!=PT_PAULA&&j->header.channels.track[j->index].route!=PT_AMIGUS)))){
            refuse(j,PT_MIXED_PLAN_REASON_ORIGIN);break;}
        ++j->index;break;
    case PT_MIXED_PLAN_RECORD:
        if(j->track==PT_MIXED_PLAN_RECORDS){j->report.phase=PT_MIXED_PLAN_COMPLETE;
            j->report.result=PT_MIXED_PLAN_READY;j->report.action=j->report.track=j->report.kind=UINT_MAX;break;}
        if(j->first[j->track]<0){++j->track;break;}
        a=j->plan.action+j->first[j->track];j->report.action=(unsigned)j->first[j->track];
        j->report.track=j->track;j->report.kind=(unsigned)a->kind;
        r->first_action=j->report.action;r->control_action=j->control[j->track]<0?UINT_MAX:(unsigned)j->control[j->track];
        r->track=j->track;r->route=j->header.channels.track[j->track].route;r->slot=(unsigned)j->slots[j->track];
        r->kind=a->kind==PT_RENDER_TRIGGER?PT_MIXED_PLAN_TRIGGER:
            a->kind==PT_RENDER_CONTROL?PT_MIXED_PLAN_CONTROL:PT_MIXED_PLAN_STOP;
        if(r->kind==PT_MIXED_PLAN_TRIGGER){j->source=0;j->report.phase=PT_MIXED_PLAN_SOURCE;break;}
        origin=j->origins+j->track;
        if(!origin->present){refuse(j,PT_MIXED_PLAN_REASON_ORIGIN);break;}
        r->sample=origin->sample;r->channel=origin->channel;
        if(r->kind==PT_MIXED_PLAN_STOP){accepted(j);break;}
        if(a->voice.pcm!=&j->saved.project->samples[r->sample].pcm||a->voice.active!=1||
           a->voice.segment||a->voice.repeat_pcm){refuse(j,PT_MIXED_PLAN_REASON_SOURCE);break;}
        j->report.phase=PT_MIXED_PLAN_GEOMETRY;break;
    case PT_MIXED_PLAN_SOURCE:
        a=j->plan.action+j->first[j->track];
        /* Pointer identity first. The action's foreign PCM is never dereferenced. */
        if(j->source==j->header.sample_count){refuse(j,PT_MIXED_PLAN_REASON_SOURCE);break;}
        if(a->voice.pcm==&j->saved.project->samples[j->source].pcm){
            r->sample=j->source;r->channel=0;j->report.phase=PT_MIXED_PLAN_GEOMETRY;
        }else ++j->source;
        break;
    case PT_MIXED_PLAN_GEOMETRY:
        effective(j,s);sample=j->samples+r->sample;
        if(r->kind==PT_MIXED_PLAN_CONTROL){
            if(r->route==PT_PAULA){
                if(!pt_paula_render_control(s->voice.step,j->saved.rate,s->gains,r->slot,&j->caps,
                        &s->paula.period,&s->paula.volume)){refuse(j,PT_MIXED_PLAN_REASON_PAULA);break;}
                r->geometry.paula.period=s->paula.period;r->geometry.paula.volume=s->paula.volume;
            }else{
                if(!pt_amigus_render_control(s->voice.step,j->saved.rate,s->gains,&s->card.rate,
                        &s->card.left,&s->card.right)){refuse(j,PT_MIXED_PLAN_REASON_AMIGUS);break;}
                r->geometry.amigus.rate=s->card.rate;r->geometry.amigus.left=s->card.left;
                r->geometry.amigus.right=s->card.right;
            }
            accepted(j);break;
        }
        if(r->route==PT_PAULA){
            if(sample->loop!=PT_LOOP_NONE||sample->loop_start||sample->loop_end||sample->crossfade||
               sample->interpolation||!pt_paula_render_voice(&s->voice,j->saved.rate,s->gains,r->slot,&j->caps,&s->paula)||
               s->paula.offset||s->paula.length!=sample->pcm.frames){refuse(j,PT_MIXED_PLAN_REASON_PAULA);break;}
            r->geometry.paula.period=s->paula.period;r->geometry.paula.volume=s->paula.volume;accepted(j);break;
        }
        bytes=(uint64_t)sample->pcm.frames*(j->format.bits/8);
        if(bytes>UINT32_MAX||!pt_amigus_render_voice(&s->voice,j->saved.rate,s->gains,&j->format,0,
                (uint32_t)bytes,&s->card)){refuse(j,PT_MIXED_PLAN_REASON_AMIGUS);break;}
        r->image=s->card;r->geometry.amigus.bits=j->format.bits;
        r->geometry.amigus.little_endian=j->format.little_endian;
        numerator=((uint64_t)s->card.rate*375+127)/128;
        if(numerator>UINT32_MAX){refuse(j,PT_MIXED_PLAN_REASON_AMIGUS);break;}
        r->geometry.amigus.trigger.rate_numerator=(uint32_t)numerator;
        r->geometry.amigus.trigger.rate_denominator=16384;
        r->geometry.amigus.trigger.offset=s->voice.start;
        s->request=r->geometry.amigus.trigger;s->request.volume=0;s->request.pan=0;
        if(!pt_amigus_voice_plan_prepare(sample,&j->format,&s->request,0,(uint32_t)bytes,&s->card)||
           !same_bounds(&r->image,&s->card)){refuse(j,PT_MIXED_PLAN_REASON_AMIGUS);break;}
        j->candidate=0;j->report.phase=PT_MIXED_PLAN_GAINS;break;
    case PT_MIXED_PLAN_GAINS:
        if(j->candidate==PT_MIXED_PLAN_GAIN_CANDIDATES){refuse(j,PT_MIXED_PLAN_REASON_GAINS);break;}
        sample=j->samples+r->sample;s->request=r->geometry.amigus.trigger;
        if(!j->candidate){s->request.volume=0;s->request.pan=0;}
        else{s->request.volume=1+(j->candidate-1)/257;s->request.pan=(j->candidate-1)%257;}
        ++j->candidate;++j->report.candidates;
        bytes=(uint64_t)sample->pcm.frames*(j->format.bits/8);
        if(pt_amigus_voice_plan_prepare(sample,&j->format,&s->request,0,(uint32_t)bytes,&s->card)&&
           same_image(&r->image,&s->card)){r->geometry.amigus.trigger=s->request;accepted(j);}
        break;
    default:break;
    }
}
enum pt_mixed_plan_result pt_mixed_plan_normalizer_step(
    struct pt_mixed_plan_normalizer *j,uint32_t revision,uint32_t generation,unsigned work)
{
    struct scratch local;unsigned i;
    if(!valid(j)||!work||work>PT_MIXED_PLAN_WORK_MAX)return PT_MIXED_PLAN_INVALID;
    if(!local_apart(j,(uintptr_t)&local,sizeof(local)))return PT_MIXED_PLAN_ALIAS;
    if(j->busy)return PT_MIXED_PLAN_INVALID;
    if(j->report.result==PT_MIXED_PLAN_REFUSED||j->report.result==PT_MIXED_PLAN_STALE)return j->report.result;
    if(!current(j,revision,generation)){j->report.result=PT_MIXED_PLAN_STALE;return j->report.result;}
    if(j->report.result!=PT_MIXED_PLAN_PENDING)return j->report.result;
    memset(&local,0,sizeof(local));j->busy=1;j->report.last_work=0;
    for(i=0;i<work&&j->report.result==PT_MIXED_PLAN_PENDING;++i){
        one(j,&local);++j->report.last_work;++j->report.work;
    }
    j->busy=0;return j->report.result;
}
enum pt_mixed_plan_result pt_mixed_plan_normalizer_get(
    struct pt_mixed_plan_normalizer *j,uint32_t revision,uint32_t generation,struct pt_mixed_plan_batch *out)
{
    if(!valid(j))return PT_MIXED_PLAN_INVALID;
    if(out&&((uintptr_t)out%_Alignof(struct pt_mixed_plan_batch)||!output_apart(j,out,sizeof(*out))))return PT_MIXED_PLAN_ALIAS;
    if(j->busy)return PT_MIXED_PLAN_INVALID;
    if(j->report.result==PT_MIXED_PLAN_REFUSED||j->report.result==PT_MIXED_PLAN_STALE)return j->report.result;
    if(!current(j,revision,generation)){j->report.result=PT_MIXED_PLAN_STALE;return j->report.result;}
    if(j->report.result==PT_MIXED_PLAN_READY&&out)memcpy(out,&j->batch,sizeof(*out));
    return j->report.result;
}
enum pt_mixed_plan_result pt_mixed_plan_normalizer_report(
    struct pt_mixed_plan_normalizer *j,struct pt_mixed_plan_report *out)
{
    if(!valid(j))return PT_MIXED_PLAN_INVALID;
    if(out&&((uintptr_t)out%_Alignof(struct pt_mixed_plan_report)||!output_apart(j,out,sizeof(*out))))return PT_MIXED_PLAN_ALIAS;
    if(j->busy)return PT_MIXED_PLAN_INVALID;
    if(out)*out=j->report;
    return j->report.result;
}
int pt_mixed_plan_normalizer_close(struct pt_mixed_plan_normalizer **owner)
{
    struct pt_mixed_plan_normalizer *j;
    if(!span(owner,sizeof(*owner))||(uintptr_t)owner%_Alignof(struct pt_mixed_plan_normalizer *))return 0;
    j=*owner;if(!j)return 1;
    if(!valid(j)||!close_apart(j,owner)||j->busy)return 0;
    /* Exact local consumption; no current()/former descriptor/source reads. */
    memset(j,0,sizeof(*j));*owner=NULL;return 1;
}
