#include "flow.h"
#include <string.h>

#define FLOW_PREPARATION_INITIALIZED 0x46505231U
static int span(const void *p,size_t n)
{return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p);}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!span(a,an) || !span(b,bn))return 1;
    return an && bn && x<y+bn && y<x+an;
}
/* Bounded metadata only, never active PCM or reserved padding values. */
static int source_disjoint(const struct pt_project *p,const void *out,size_t bytes)
{
    unsigned i;
    if(!p || !span(p,sizeof(*p)) || !span(out,bytes) ||
       !p->order_count || p->order_count>PT_PROJECT_ORDERS ||
       !p->pattern_count || p->pattern_count>PT_PROJECT_PATTERNS ||
       !p->channels.count || p->channels.count>PT_CHANNEL_LIMIT ||
       p->sample_count>PT_PROJECT_SAMPLES || p->extension_count>4090 ||
       overlap(p,sizeof(*p),out,bytes) ||
       overlap(p->orders,p->order_count*sizeof(*p->orders),out,bytes) ||
       overlap(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events),out,bytes) ||
       overlap(p->samples,p->sample_count*sizeof(*p->samples),out,bytes) ||
       overlap(p->extensions,p->extension_count*sizeof(*p->extensions),out,bytes))return 0;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *sample=p->samples+i;
        if(sample->pcm.capacity>SIZE_MAX/sizeof(*sample->pcm.data) ||
           sample->slice_count>PT_PROJECT_SLICES ||
           overlap(sample->pcm.data,sample->pcm.capacity*sizeof(*sample->pcm.data),out,bytes) ||
           overlap(sample->slices,sample->slice_count*sizeof(*sample->slices),out,bytes))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(overlap(p->extensions[i].data,p->extensions[i].length,out,bytes))return 0;
    return 1;
}
static int reset_arguments(const struct pt_project *p,enum pt_flow_mode mode,unsigned start,uint32_t limit)
{
    return p && limit && (mode==PT_FLOW_CLASSIC128 || mode==PT_FLOW_EXTENDED256) &&
        start<p->order_count && (mode!=PT_FLOW_CLASSIC128 ||
        (p->channels.count==4 && p->order_count<=128 && p->speed==6 && p->bpm==125));
}
/* File-private reset: every caller must complete its own checked validation. */
static void reset(struct pt_flow *out,const struct pt_project *p,enum pt_flow_mode mode,unsigned start,uint32_t limit)
{
    struct pt_flow next;
    memset(&next,0,sizeof(next));next.project=p;next.mode=(uint8_t)mode;
    next.order=next.played_order=(uint16_t)start;next.speed=p->speed;next.bpm=p->bpm;
    next.limit=limit;next.active=1;*out=next;
}
enum pt_flow_result pt_flow_init(struct pt_flow *out,const struct pt_project *p,
                               enum pt_flow_mode mode,unsigned start,uint32_t limit)
{
    if(!out || !p || !limit || (mode!=PT_FLOW_CLASSIC128 && mode!=PT_FLOW_EXTENDED256) ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || !reset_arguments(p,mode,start,limit) ||
       !source_disjoint(p,out,sizeof(*out)))return PT_FLOW_INVALID;
    reset(out,p,mode,start,limit);return PT_FLOW_TICK;
}
enum pt_flow_result pt_flow_begin(struct pt_flow_preparation *out,const struct pt_project *p,
    enum pt_flow_mode mode,unsigned start,uint32_t limit,uint32_t revision,uint32_t generation)
{
    struct pt_flow_preparation next;enum pt_project_result result;
    if(!out || !span(out,sizeof(*out)) || !p || !span(p,sizeof(*p)) ||
       !reset_arguments(p,mode,start,limit) || !source_disjoint(p,out,sizeof(*out)))return PT_FLOW_INVALID;
    memset(&next,0,sizeof(next));
    result=pt_project_validation_begin(&next.validation,p,revision,generation);
    if(result!=PT_PROJECT_OK)return PT_FLOW_INVALID;
    next.initialized=FLOW_PREPARATION_INITIALIZED;next.mode=(unsigned)mode;next.start_order=start;
    next.revision=revision;next.generation=generation;next.limit=limit;*out=next;return PT_FLOW_PENDING;
}
static enum pt_flow_result validation_result(enum pt_project_result result)
{
    return result==PT_PROJECT_OK?PT_FLOW_TICK:result==PT_PROJECT_PENDING?PT_FLOW_PENDING:
        result==PT_PROJECT_STALE?PT_FLOW_STALE:PT_FLOW_INVALID;
}
enum pt_flow_result pt_flow_prepare(struct pt_flow_preparation *prep,
    uint32_t revision,uint32_t generation,unsigned work)
{
    enum pt_project_result result;
    if(!prep || !span(prep,sizeof(*prep)) || prep->initialized!=FLOW_PREPARATION_INITIALIZED ||
       !work || work>PT_PROJECT_VALIDATION_WORK_MAX)return PT_FLOW_INVALID;
    if(prep->revision!=revision || prep->generation!=generation)return PT_FLOW_STALE;
    result=prep->ready?pt_project_validation_get(&prep->validation,revision,generation,NULL):
        pt_project_validation_step(&prep->validation,revision,generation,work);
    if(result!=PT_PROJECT_OK)return validation_result(result);
    if(!prep->ready) {
        /* Recheck completed/current provenance immediately before reset. */
        result=pt_project_validation_get(&prep->validation,revision,generation,NULL);
        if(result!=PT_PROJECT_OK)return validation_result(result);
        reset(&prep->initial,prep->validation.project,(enum pt_flow_mode)prep->mode,prep->start_order,prep->limit);
        prep->ready=1;
    }
    return PT_FLOW_TICK;
}
enum pt_flow_result pt_flow_get(const struct pt_flow_preparation *prep,
    uint32_t revision,uint32_t generation,struct pt_flow *out)
{
    enum pt_project_result result;
    if(!prep || !span(prep,sizeof(*prep)) || prep->initialized!=FLOW_PREPARATION_INITIALIZED ||
       !out || overlap(prep,sizeof(*prep),out,sizeof(*out)))return PT_FLOW_INVALID;
    if(prep->revision!=revision || prep->generation!=generation)return PT_FLOW_STALE;
    result=pt_project_validation_get(&prep->validation,revision,generation,NULL);
    if(result!=PT_PROJECT_OK && result!=PT_PROJECT_PENDING)return validation_result(result);
    if(!source_disjoint(prep->validation.project,out,sizeof(*out)))return PT_FLOW_INVALID;
    if(result==PT_PROJECT_PENDING || !prep->ready)return PT_FLOW_PENDING;
    *out=prep->initial;return PT_FLOW_TICK;
}
void pt_flow_cancel(struct pt_flow_preparation *prep)
{
    if(prep && span(prep,sizeof(*prep)))memset(prep,0,sizeof(*prep));
}

static void extended_effect(struct pt_flow *s,unsigned ch)
{
    unsigned parameter=s->parameter[ch],value=parameter&15;
    if(s->counter)return;
    if((parameter>>4)==6) {
        if(!value)s->loop_start[ch]=(uint8_t)(s->row&63);
        else {
            if(!s->loop_count[ch])s->loop_count[ch]=(uint8_t)value;
            else if(!--s->loop_count[ch])return;
            s->break_row=s->loop_start[ch];s->loop_break=1;
        }
    } else if((parameter>>4)==14 && !s->delay)s->pending_delay=(uint8_t)(value+1);
}

static void new_effect(struct pt_flow *s,unsigned ch)
{
    unsigned parameter=s->parameter[ch],row;
    switch(s->effect[ch]) {
    case 11:s->order=(uint8_t)(parameter-1);s->break_row=0;s->jump=1;break;
    case 13:
        row=(parameter>>4)*10+(parameter&15);
        s->break_row=(uint8_t)(row>63?0:row);s->jump=1;break;
    case 14:extended_effect(s,ch);break;
    case 15:
        if(!parameter)s->active=0;
        else if(parameter<32) {s->counter=0;s->speed=(uint8_t)parameter;}
        else s->bpm=(uint16_t)parameter;
        break;
    default:break; /* Audio/voice effects belong to the subsequent voice core. */
    }
}

static void next_position(struct pt_flow *s,unsigned from)
{
    s->row=s->break_row;s->break_row=s->jump=0;
    s->order=(uint16_t)((s->order+1)&(s->mode==PT_FLOW_CLASSIC128?127:255));
    if(s->order>=s->project->order_count)s->order=0;
    ++s->positions;if(s->order<=from)++s->returns;
}

enum pt_flow_result pt_flow_tick(struct pt_flow *s)
{
    const struct pt_project *p;unsigned ch,from;int row_tick;
    if(!s || !s->project || !s->limit)return PT_FLOW_INVALID;
    if(!s->active)return PT_FLOW_STOPPED;
    if(s->ticks>=s->limit)return PT_FLOW_LIMIT;
    p=s->project;from=s->order;++s->ticks;s->fresh=s->delayed=0;
    ++s->counter;row_tick=s->counter>=s->speed;
    if(row_tick)s->counter=0;
    if(row_tick && !s->delay) {
        const struct pt_event *events=p->events+((size_t)p->orders[s->order]*64+s->row)*p->channels.count;
        s->played_order=s->order;s->played_row=s->row;s->fresh=1;++s->fetches;
        for(ch=0;ch<p->channels.count;++ch) {
            s->effect[ch]=events[ch].effect;s->parameter[ch]=events[ch].parameter;
            new_effect(s,ch);
        }
    } else {
        s->delayed=(uint8_t)row_tick;
        for(ch=0;ch<p->channels.count;++ch)if(s->effect[ch]==14)extended_effect(s,ch);
    }
    if(row_tick) {
        ++s->row;
        if(s->pending_delay) {s->delay=s->pending_delay;s->pending_delay=0;}
        if(s->delay && --s->delay)--s->row;
        if(s->loop_break) {s->loop_break=0;s->row=s->break_row;s->break_row=0;}
        if(s->row>=64)next_position(s,from);
    }
    if(s->jump)next_position(s,from);
    return PT_FLOW_TICK;
}
