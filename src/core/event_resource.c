#include "event_resource.h"
#include "pcm_internal.h"
#include <string.h>

#define RESOURCE_INITIALIZED 0x45565231U
static int span(const void *p,size_t n)
{return !n || (p && n<=UINTPTR_MAX-(uintptr_t)p);}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!span(a,an) || !span(b,bn))return 1;
    return an && bn && x<y+bn && y<x+an;
}
static int geometry(const struct pt_project *p)
{
    unsigned i;
    if(!p || !span(p,sizeof(*p)) || pt_channels_validate(&p->channels)!=PT_CHANNEL_OK ||
       !p->order_count || p->order_count>PT_PROJECT_ORDERS ||
       !p->pattern_count || p->pattern_count>PT_PROJECT_PATTERNS ||
       p->sample_count>PT_PROJECT_SAMPLES || p->extension_count>4090 ||
       !span(p->orders,p->order_count*sizeof(*p->orders)) ||
       !span(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events)) ||
       !span(p->samples,p->sample_count*sizeof(*p->samples)) ||
       !span(p->extensions,p->extension_count*sizeof(*p->extensions)))return 0;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=p->samples+i;
        if(pt_pcm_shape(&s->pcm)!=PT_PCM_OK || s->pcm.capacity>SIZE_MAX/sizeof(int32_t) ||
           !span(s->pcm.data,s->pcm.capacity*sizeof(int32_t)) || s->slice_count>PT_PROJECT_SLICES ||
           !span(s->slices,s->slice_count*sizeof(*s->slices)))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(!span(p->extensions[i].data,p->extensions[i].length))return 0;
    return 1;
}
static int disjoint(const struct pt_project *p,const void *out,size_t n)
{
    unsigned i;
    if(!span(out,n) || overlap(p,sizeof(*p),out,n) ||
       overlap(p->orders,p->order_count*sizeof(*p->orders),out,n) ||
       overlap(p->events,(size_t)p->pattern_count*64*p->channels.count*sizeof(*p->events),out,n) ||
       overlap(p->samples,p->sample_count*sizeof(*p->samples),out,n) ||
       overlap(p->extensions,p->extension_count*sizeof(*p->extensions),out,n))return 0;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=p->samples+i;
        if(s->pcm.capacity>SIZE_MAX/sizeof(int32_t) ||
           overlap(s->pcm.data,s->pcm.capacity*sizeof(int32_t),out,n) ||
           overlap(s->slices,s->slice_count*sizeof(*s->slices),out,n))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(overlap(p->extensions[i].data,p->extensions[i].length,out,n))return 0;
    return 1;
}
static int current(const struct pt_event_resource_job *j,uint32_t revision,uint32_t generation)
{
    struct pt_project now;
    if(j->initialized!=RESOURCE_INITIALIZED || !j->project ||
       j->revision!=revision || j->generation!=generation)return 0;
    memcpy(&now,j->project,sizeof(now));
    if(now.channels.selected>=now.channels.count)return 0;
    now.channels.selected=j->snapshot.channels.selected;
    return !memcmp(&now,&j->snapshot,sizeof(now));
}
static void normalize(struct pt_flow *out,const struct pt_flow *in)
{
    memcpy(out,in,sizeof(*out));
    out->ticks=out->fetches=out->positions=out->returns=0;out->limit=0;
}
static void checkpoint(struct pt_event_resource_job *j)
{
    normalize(&j->checkpoint,&j->flow);
    j->checkpoint_instrument=j->pitch.channel[j->result.origin.track].instrument;
}
static int repeated(const struct pt_event_resource_job *j)
{
    struct pt_flow now;
    normalize(&now,&j->flow);
    return !memcmp(&now,&j->checkpoint,sizeof(now)) &&
        j->checkpoint_instrument==j->pitch.channel[j->result.origin.track].instrument;
}
static void destination(struct pt_event_resource_result *r)
{
    r->destination=r->route==PT_MIDI?PT_EVENT_RESOURCE_MIDI_ROUTING:PT_EVENT_RESOURCE_AUDIO_MASTER;
}
static void complete(struct pt_event_resource_job *j)
{
    j->ready=1;j->result.ticks=j->flow.ticks;
    if(!j->seen) {
        j->result.state=PT_EVENT_RESOURCE_UNRESOLVED;j->result.reason=PT_EVENT_RESOURCE_UNREACHABLE;
    } else if(!j->selection) {
        j->result.state=PT_EVENT_RESOURCE_UNRESOLVED;j->result.reason=PT_EVENT_RESOURCE_NO_CARRY;
    } else {
        j->result.state=PT_EVENT_RESOURCE_RESOLVED_INHERITED;
        j->result.reason=PT_EVENT_RESOURCE_COMPLETE_HISTORY;j->result.instrument=j->selection;
        destination(&j->result);
    }
}
static int same_origin(const struct pt_event_resource_origin *a,const struct pt_event_resource_origin *b)
{
    return a->pattern==b->pattern && a->row==b->row && a->track==b->track &&
        a->order==b->order && a->order_known==b->order_known && a->start_order==b->start_order && a->flow_mode==b->flow_mode;
}
enum pt_event_resource_status pt_event_resource_begin(struct pt_event_resource_job *j,
    const struct pt_project *p,const struct pt_event_resource_origin *origin,
    uint32_t revision,uint32_t generation,uint32_t limit)
{
    struct pt_event_resource_origin o;struct pt_event event;
    struct pt_flow initial;unsigned reuse=0;
    if(!j || !span(j,sizeof(*j)) || !origin || !span(origin,sizeof(*origin)) || !limit ||
       !geometry(p) || !disjoint(p,j,sizeof(*j)) || overlap(j,sizeof(*j),origin,sizeof(*origin)))return PT_EVENT_RESOURCE_INVALID;
    o=*origin;
    if(o.pattern>=p->pattern_count || o.row>=64 || o.track>=p->channels.count ||
       o.order_known>1 || (o.flow_mode!=PT_FLOW_CLASSIC128 && o.flow_mode!=PT_FLOW_EXTENDED256) || (o.order_known && (o.order>=p->order_count ||
       o.start_order>=p->order_count || p->orders[o.order]!=o.pattern)))return PT_EVENT_RESOURCE_INVALID;
    event=p->events[((size_t)o.pattern*64+o.row)*p->channels.count+o.track];
    if(!pt_project_event_valid(p,&event))return PT_EVENT_RESOURCE_INVALID;
    /* Only a genuine same-version audited reset state may skip validation.
       A caller must clear this workspace before releasing/replacing its project. */
    if(j->project==p && current(j,revision,generation) && j->validated &&
       j->initial.order==o.start_order && j->initial.mode==o.flow_mode) {
        initial=j->initial;initial.limit=limit;reuse=1;
    }
    memset(j,0,sizeof(*j));j->project=p;memcpy(&j->snapshot,p,sizeof(*p));
    j->revision=revision;j->generation=generation;j->limit=limit;j->initialized=RESOURCE_INITIALIZED;
    j->result.origin=o;j->result.source=o;j->result.route=p->channels.track[o.track].route;
    j->result.midi_channel=p->channels.track[o.track].midi_channel;
    j->result.state=PT_EVENT_RESOURCE_UNRESOLVED;j->result.destination=PT_EVENT_RESOURCE_NONE;
    j->result.source_unique=0;j->power=1;
    if(reuse) {j->initial=initial;j->flow=initial;j->validated=1;pt_pitch_init(&j->pitch);checkpoint(j);}
    if(event.instrument) {
        j->ready=1;j->result.state=PT_EVENT_RESOURCE_EXPLICIT;j->result.reason=PT_EVENT_RESOURCE_DIRECT;
        j->result.instrument=event.instrument;j->result.source_unique=1;destination(&j->result);
    } else if(event.kind==PT_NOTE_NONE && !event.effect && !event.parameter && !event.slice && !event.flags) {
        j->ready=1;j->result.state=PT_EVENT_RESOURCE_NO_RESOURCE;j->result.reason=PT_EVENT_RESOURCE_EMPTY_EVENT;
    } else if(!o.order_known) {
        j->ready=1;j->result.state=PT_EVENT_RESOURCE_AMBIGUOUS;j->result.reason=PT_EVENT_RESOURCE_DETACHED_PATTERN;
    }
    return PT_EVENT_RESOURCE_OK;
}
enum pt_event_resource_status pt_event_resource_step(struct pt_event_resource_job *j,
    uint32_t revision,uint32_t generation,unsigned ticks,unsigned *ready)
{
    unsigned i,ch;enum pt_flow_result status;
    if(!j || !span(j,sizeof(*j)) || !ready || !ticks || ticks>256 ||
       overlap(j,sizeof(*j),ready,sizeof(*ready)))return PT_EVENT_RESOURCE_INVALID;
    if(!current(j,revision,generation))return PT_EVENT_RESOURCE_STALE;
    if(!geometry(j->project) || !disjoint(j->project,ready,sizeof(*ready)))return PT_EVENT_RESOURCE_INVALID;
    if(j->ready) {*ready=1;return PT_EVENT_RESOURCE_OK;}
    if(!j->validated) {
        if(!j->preparing) {
            status=pt_flow_begin(&j->preparation,j->project,j->result.origin.flow_mode,
                j->result.origin.start_order,j->limit,revision,generation);
            if(status!=PT_FLOW_PENDING)return status==PT_FLOW_STALE?PT_EVENT_RESOURCE_STALE:PT_EVENT_RESOURCE_INVALID;
            j->preparing=1;
        } else {
            status=pt_flow_prepare(&j->preparation,revision,generation,PT_PROJECT_VALIDATION_WORK_MAX);
            if(status!=PT_FLOW_PENDING && status!=PT_FLOW_TICK)
                return status==PT_FLOW_STALE?PT_EVENT_RESOURCE_STALE:PT_EVENT_RESOURCE_INVALID;
            if(status==PT_FLOW_TICK) {
                status=pt_flow_get(&j->preparation,revision,generation,&j->initial);
                if(status!=PT_FLOW_TICK)return status==PT_FLOW_STALE?PT_EVENT_RESOURCE_STALE:PT_EVENT_RESOURCE_INVALID;
                j->flow=j->initial;j->validated=1;pt_pitch_init(&j->pitch);checkpoint(j);
            }
        }
        *ready=0;return PT_EVENT_RESOURCE_OK;
    }
    ch=j->result.origin.track;
    for(i=0;i<ticks;++i) {
        status=pt_flow_tick(&j->flow);
        if(status==PT_FLOW_STOPPED) {complete(j);break;}
        if(status==PT_FLOW_LIMIT) {
            j->ready=1;j->result.ticks=j->flow.ticks;
            j->result.state=PT_EVENT_RESOURCE_UNRESOLVED;j->result.reason=PT_EVENT_RESOURCE_TICK_BUDGET;break;
        }
        if(status!=PT_FLOW_TICK)return PT_EVENT_RESOURCE_INVALID;
        pt_pitch_tick(&j->pitch,&j->flow,(uint16_t)(1U<<ch));
        if(j->flow.fresh) {
            const struct pt_event *event=j->project->events+
                ((size_t)j->project->orders[j->flow.played_order]*64+j->flow.played_row)*j->project->channels.count+ch;
            if(event->instrument) {
                j->last_source=(struct pt_event_resource_origin){j->project->orders[j->flow.played_order],
                    j->flow.played_row,ch,j->flow.played_order,1,j->result.origin.start_order,j->result.origin.flow_mode};
            }
            if(j->flow.played_order==j->result.origin.order && j->flow.played_row==j->result.origin.row) {
                unsigned instrument=j->pitch.channel[ch].instrument;
                ++j->result.visits;
                if(!j->seen) {
                    j->seen=1;j->selection=instrument;j->result.source=j->last_source;
                    j->result.source_unique=instrument!=0;
                } else if(j->selection!=instrument) {
                    j->ready=1;j->result.state=PT_EVENT_RESOURCE_AMBIGUOUS;
                    j->result.reason=PT_EVENT_RESOURCE_MULTIPLE_SELECTIONS;j->result.source_unique=0;
                    j->result.ticks=j->flow.ticks;break;
                } else if(!same_origin(&j->result.source,&j->last_source))j->result.source_unique=0;
            }
        }
        if(!j->flow.active || repeated(j)) {complete(j);break;}
        ++j->length;
        if(j->length==j->power) {checkpoint(j);j->power*=2;j->length=0;}
    }
    *ready=j->ready;return PT_EVENT_RESOURCE_OK;
}
void pt_event_resource_cancel(struct pt_event_resource_job *j)
{
    if(j && span(j,sizeof(*j)))memset(j,0,sizeof(*j));
}
enum pt_event_resource_status pt_event_resource_get(const struct pt_event_resource_job *j,
    uint32_t revision,uint32_t generation,struct pt_event_resource_result *out)
{
    if(!j || !span(j,sizeof(*j)) || !out || overlap(j,sizeof(*j),out,sizeof(*out)))return PT_EVENT_RESOURCE_INVALID;
    if(!current(j,revision,generation))return PT_EVENT_RESOURCE_STALE;
    if(!j->ready || !geometry(j->project) || !disjoint(j->project,out,sizeof(*out)))return PT_EVENT_RESOURCE_INVALID;
    *out=j->result;return PT_EVENT_RESOURCE_OK;
}
