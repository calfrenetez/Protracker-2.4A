#include <string.h>
#include "sample_usage.h"
#include "pcm_internal.h"
#include "mod_project.h"
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(!a || !b || an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
static int span_apart(const void *out,size_t bytes,const void *p,size_t count,size_t width)
{
    if(count>SIZE_MAX/width)return 0;
    return !overlap(out,bytes,p,count*width);
}
int pt_sample_usage_output_disjoint(const struct pt_project *p,const void *out,size_t bytes)
{
    unsigned i;size_t events;
    if(!p || !out || !bytes || bytes>UINTPTR_MAX-(uintptr_t)out ||
       !p->channels.count || p->channels.count>PT_CHANNEL_LIMIT ||
       p->pattern_count>PT_PROJECT_PATTERNS || p->order_count>PT_PROJECT_ORDERS ||
       p->sample_count>PT_PROJECT_SAMPLES || p->extension_count>4090)return 0;
    events=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    if(overlap(out,bytes,p,sizeof(*p)) ||
       !span_apart(out,bytes,p->events,events,sizeof(*p->events)) ||
       !span_apart(out,bytes,p->orders,p->order_count,sizeof(*p->orders)) ||
       !span_apart(out,bytes,p->samples,p->sample_count,sizeof(*p->samples)) ||
       !span_apart(out,bytes,p->extensions,p->extension_count,sizeof(*p->extensions)))return 0;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=p->samples+i;
        if(!span_apart(out,bytes,s->pcm.data,s->pcm.capacity,sizeof(*s->pcm.data)) ||
           !span_apart(out,bytes,s->slices,s->slice_count,sizeof(*s->slices)))return 0;
    }
    for(i=0;i<p->extension_count;++i)
        if(!span_apart(out,bytes,p->extensions[i].data,p->extensions[i].length,1))return 0;
    return 1;
}
static int canonical_empty(const struct pt_sample *s)
{
    static const char name[PT_PROJECT_NAME]={0};
    return !memcmp(s->name,name,sizeof(name)) && !s->pcm.data && !s->pcm.capacity &&
        !s->pcm.frames && s->pcm.bits==8 && s->pcm.channels==1 &&
        s->pcm.rate==PT_CLASSIC_RATE && !s->slices && !s->slice_count &&
        !s->loop && !s->loop_start && !s->loop_end && !s->crossfade &&
        !s->volume && !s->finetune && !s->interpolation;
}
static int metadata_valid(const struct pt_project *p)
{
    unsigned i;
    if(!p || pt_channels_validate(&p->channels)!=PT_CHANNEL_OK ||
       !p->pattern_count || p->pattern_count>PT_PROJECT_PATTERNS ||
       !p->order_count || p->order_count>PT_PROJECT_ORDERS ||
       p->sample_count>PT_PROJECT_SAMPLES || !p->events || !p->orders ||
       (p->sample_count && !p->samples) || p->extension_count>4090 ||
       (p->extension_count && !p->extensions))return 0;
    for(i=0;i<p->order_count;++i)if(p->orders[i]>=p->pattern_count)return 0;
    for(i=0;i<p->sample_count;++i) {
        const struct pt_sample *s=p->samples+i;
        if(pt_pcm_shape(&s->pcm)!=PT_PCM_OK || !memchr(s->name,0,sizeof(s->name)) ||
           s->volume>64 || s->finetune<-8 || s->finetune>7 || s->interpolation>1 ||
           s->slice_count>PT_PROJECT_SLICES || (s->slice_count && !s->slices) ||
           s->loop>PT_LOOP_CROSSFADE ||
           (s->loop && (s->loop_start>=s->loop_end || s->loop_end>s->pcm.frames)) ||
           (!s->loop && (s->loop_start || s->loop_end || s->crossfade)) ||
           (s->loop==PT_LOOP_CROSSFADE && (!s->crossfade || s->crossfade>(s->loop_end-s->loop_start)/2)) ||
           (s->loop!=PT_LOOP_CROSSFADE && s->crossfade))return 0;
    }
    /* Validate all address extents without any bulk PCM/marker/payload read. */
    return pt_sample_usage_output_disjoint(p,&i,sizeof(i));
}
static int same_project(const struct pt_project *a,const struct pt_project *b)
{
    size_t offset=offsetof(struct pt_project,channels)+offsetof(struct pt_channels,selected);
    return !memcmp(a,b,offset) && !memcmp((const uint8_t *)a+offset+1,(const uint8_t *)b+offset+1,sizeof(*a)-offset-1);
}
int pt_sample_usage_current(const struct pt_sample_usage_preview *v,const struct pt_project *p,
    uint32_t revision,unsigned generation)
{
    unsigned i;
    if(!v || !p || v->project!=p || v->revision!=revision || v->generation!=generation ||
       v->count!=p->sample_count || !same_project(&v->snapshot,p))return 0;
    for(i=0;i<v->count;++i)
        if(memcmp(&v->rows[i].identity,p->samples+i,sizeof(*p->samples)))return 0;
    return 1;
}
enum pt_sample_usage_result pt_sample_usage_begin(struct pt_sample_usage_scan *scan,
    const struct pt_project *p,const struct pt_sample_usage_options *o)
{
    unsigned i,unknown=0;
    if(!scan || !o || !metadata_valid(p))return PT_USAGE_INVALID;
    if(!pt_sample_usage_output_disjoint(p,scan,sizeof(*scan)) ||
       overlap(scan,sizeof(*scan),o,sizeof(*o)))return PT_USAGE_ALIAS;
    for(i=0;i<p->extension_count;++i) {
        const struct pt_extension *e=p->extensions+i;
        if(e->id!=PT_CLASSIC_HEADER_TAG || e->version!=1 || e->length!=1084)unknown=1;
    }
    memset(scan,0,sizeof(*scan));scan->value.project=p;scan->value.snapshot=*p;
    scan->value.revision=o->revision;scan->value.generation=o->generation;
    scan->value.count=p->sample_count;
    scan->total_events=(uint32_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    for(i=0;i<PT_PROJECT_SAMPLES;++i) {
        struct pt_sample_usage_row *r=scan->value.rows+i;
        if(o->protected_slots[i])r->flags|=PT_USAGE_PROTECTED;
        if(o->reserved_slots[i])r->flags|=PT_USAGE_RESERVED;
        if(unknown)r->flags|=PT_USAGE_UNKNOWN;
        scan->last_pattern[i]=UINT16_MAX;
        if(i>=p->sample_count)continue;
        const struct pt_sample *s=p->samples+i;
        r->identity=*s;r->active_bytes=(uint64_t)s->pcm.frames*s->pcm.channels*sizeof(int32_t);
        r->resident_bytes=(uint64_t)s->pcm.capacity*sizeof(int32_t);
        if(!s->pcm.frames)r->flags|=PT_USAGE_EMPTY;
        if(!canonical_empty(s))r->flags|=PT_USAGE_CONFIGURED;
    }
    return PT_USAGE_OK;
}
static void complete(struct pt_sample_usage_scan *s)
{
    unsigned i;
    for(i=0;i<s->value.count;++i) {
        struct pt_sample_usage_row *r=s->value.rows+i;
        unsigned protected_flags=PT_USAGE_REFERENCED|PT_USAGE_RESERVED|PT_USAGE_PROTECTED|PT_USAGE_UNKNOWN;
        if(r->flags&protected_flags)++s->value.protected_count;
        else if(!(r->flags&PT_USAGE_CONFIGURED)) {r->flags|=PT_USAGE_FREE;++s->value.free_count;}
        else {r->flags|=PT_USAGE_ELIGIBLE;++s->value.eligible_count;}
    }
    s->ready=1;
}
enum pt_sample_usage_result pt_sample_usage_step(struct pt_sample_usage_scan *s,
    const struct pt_project *p,uint32_t revision,unsigned generation,
    struct pt_sample_usage_preview *out,unsigned *ready)
{
    uint32_t end;
    if(!s || !out || !ready || !p || !s->value.project)return PT_USAGE_INVALID;
    if(overlap(out,sizeof(*out),s,sizeof(*s)) || overlap(ready,sizeof(*ready),s,sizeof(*s)) ||
       overlap(out,sizeof(*out),ready,sizeof(*ready)))return PT_USAGE_ALIAS;
    if(s->failure!=PT_USAGE_OK)return s->failure;
    if(!pt_sample_usage_current(&s->value,p,revision,generation)) {
        s->failure=PT_USAGE_STALE;return s->failure;
    }
    if(!pt_sample_usage_output_disjoint(p,out,sizeof(*out)) ||
       !pt_sample_usage_output_disjoint(p,ready,sizeof(*ready)))return PT_USAGE_ALIAS;
    end=s->next_event+PT_SAMPLE_USAGE_CHUNK;if(end>s->total_events)end=s->total_events;
    while(s->next_event<end) {
        const struct pt_event *e=p->events+s->next_event;
        unsigned pattern=s->next_event/(PT_PROJECT_ROWS*p->channels.count);
        if(!pt_project_event_valid(p,e)) {s->failure=PT_USAGE_INVALID;return s->failure;}
        if(e->instrument) {
            unsigned slot=e->instrument-1;struct pt_sample_usage_row *r=s->value.rows+slot;
            ++r->references;r->flags|=PT_USAGE_REFERENCED;
            if(s->last_pattern[slot]!=pattern) {++r->patterns;s->last_pattern[slot]=(uint16_t)pattern;}
        }
        ++s->next_event;
    }
    if(!s->ready && s->next_event==s->total_events)complete(s);
    if(s->ready) {*out=s->value;*ready=1;}else *ready=0;
    return PT_USAGE_OK;
}
int pt_sample_usage_free(const struct pt_sample_usage_preview *v,unsigned slot)
{return v && slot<v->count && (v->rows[slot].flags&PT_USAGE_FREE)!=0;}
