#include <string.h>
#include "sampler.h"
#include "mod_project.h"
struct slot_change {
    struct pt_sampler *owner;
    struct pt_sample *before,*after;
    unsigned count;
};
static void empty(struct pt_sample *v)
{memset(v,0,sizeof(*v));v->pcm.bits=8;v->pcm.channels=1;v->pcm.rate=PT_CLASSIC_RATE;}
static int is_empty(const struct pt_sample *v)
{
    char name[PT_PROJECT_NAME]={0};
    return !memcmp(v->name,name,sizeof(name)) && !v->pcm.frames && v->pcm.bits==8 && v->pcm.channels==1 && v->pcm.rate==PT_CLASSIC_RATE &&
        !v->loop && !v->loop_start && !v->loop_end && !v->crossfade && !v->slice_count && !v->volume && !v->finetune && !v->interpolation;
}
static int apply(void *context,struct pt_project *p,int direction)
{
    struct slot_change *c=context;struct pt_sampler *s=c->owner;struct pt_project probe;
    struct pt_sample *from=direction>0?c->before:c->after,*to=direction>0?c->after:c->before;
    unsigned count=c->count+(direction<0?1U:0U);
    if(p->samples!=from || p->sample_count!=count || pt_project_validate(p,NULL)!=PT_PROJECT_OK ||
       (s->table && s->table!=c->after))return 0;
    if(direction<0) {
        if(!is_empty(&p->samples[c->count]))return 0;
        probe=*p;probe.sample_count=(uint16_t)c->count;
        if(pt_project_validate(&probe,NULL)!=PT_PROJECT_OK)return 0;
    }
    if(to!=from && c->count)memcpy(to,from,c->count*sizeof(*to));
    if(direction>0)empty(&to[c->count]);
    if(!s->table) {s->table=c->after;s->table_original=c->before;s->table_bytes=PT_PROJECT_SAMPLES*sizeof(struct pt_sample);}
    p->samples=to;p->sample_count=(uint16_t)(c->count+(direction>0?1U:0U));++s->generation;return 1;
}
static void discard(void *context)
{struct slot_change *c=context;struct pt_sampler *s=c->owner;s->bytes-=sizeof(*c);s->allocator.release(s->allocator.context,c);}
enum pt_edit_result pt_sampler_add_slot(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h)
{
    struct slot_change *c;struct pt_sample *table;struct pt_edit_resource resource;enum pt_edit_result result;
    size_t table_bytes,needed;
    if(!s || !s->allocator.allocate || !s->allocator.release || !h || pt_project_validate(p,NULL)!=PT_PROJECT_OK)return PT_EDIT_INVALID;
    if(p->sample_count==PT_PROJECT_SAMPLES)return PT_EDIT_CAPACITY;
    if(s->table && p->samples!=s->table && p->samples!=s->table_original)return PT_EDIT_CONFLICT;
    table_bytes=s->table?0:PT_PROJECT_SAMPLES*sizeof(struct pt_sample);needed=sizeof(*c)+table_bytes;
    if(s->bytes>s->budget || needed>s->budget-s->bytes)return PT_EDIT_CAPACITY;
    c=s->allocator.allocate(s->allocator.context,sizeof(*c));if(!c)return PT_EDIT_CAPACITY;s->bytes+=sizeof(*c);
    table=s->table;if(!table) {
        table=s->allocator.allocate(s->allocator.context,table_bytes);
        if(!table) {c->owner=s;discard(c);return PT_EDIT_CAPACITY;}
        s->bytes+=table_bytes;
    }
    c->owner=s;c->before=p->samples;c->after=table;c->count=p->sample_count;
    resource=(struct pt_edit_resource){c,apply,discard};result=pt_pattern_resource_apply(p,h,&resource);
    if(result!=PT_EDIT_OK) {
        discard(c);if(table_bytes) {s->bytes-=table_bytes;s->allocator.release(s->allocator.context,table);}
    }
    return result;
}
