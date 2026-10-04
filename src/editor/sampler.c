#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "sampler.h"
#include "sampler_internal.h"
#include "../core/pcm_internal.h"
#include "wav.h"
#include "svx.h"
struct pt_sample_version {
    struct pt_sample sample;
    struct pt_sampler *owner;
    struct pt_sample_version *backing; /* Flat owner of shared PCM/markers, or NULL. */
    size_t bytes;
    int32_t *owned_pcm; /* Separate allocation adopted only after publication. */
    unsigned references;
};
struct sample_change {struct pt_sampler *owner;unsigned slot,resampled;struct pt_sample_version *before,*after;};

static int output_overlap(const void *out,size_t bytes,const void *owner,size_t size)
{
    uintptr_t x=(uintptr_t)out,y=(uintptr_t)owner;
    if(!bytes || !size)return 0;
    if(bytes>UINTPTR_MAX-x || size>UINTPTR_MAX-y)return 1;
    return x<y+size && y<x+bytes;
}
static int sample_output_disjoint(const struct pt_sample *sample,const void *out,size_t bytes)
{
    if(sample->pcm.capacity>SIZE_MAX/sizeof(int32_t) ||
       (sample->pcm.capacity && !sample->pcm.data) ||
       (sample->slice_count && !sample->slices))return 0;
    return !output_overlap(out,bytes,sample->pcm.data,sample->pcm.capacity*sizeof(int32_t)) &&
        !output_overlap(out,bytes,sample->slices,(size_t)sample->slice_count*sizeof(*sample->slices));
}
int pt_sampler_version_output_disjoint(const struct pt_sample_version *v,const void *out,size_t bytes)
{
    const struct pt_sample_version *backing;
    if(!out || !bytes || bytes>UINTPTR_MAX-(uintptr_t)out)return 0;
    if(!v)return 1;
    if(output_overlap(out,bytes,v,sizeof(*v)) || !sample_output_disjoint(&v->sample,out,bytes))return 0;
    /* bytes is budget accounting, not the extent of every version header:
     * adopted PCM is separate, and metadata revisions retain a flat backing. */
    backing=v->backing;
    return !backing || (!backing->backing &&
        !output_overlap(out,bytes,backing,sizeof(*backing)) && sample_output_disjoint(&backing->sample,out,bytes));
}
static int version_span_add(struct pt_sampler_storage_span *a,unsigned *n,const void *p,size_t bytes)
{
    if(!bytes)return 1;
    if(!p || bytes>UINTPTR_MAX-(uintptr_t)p || *n==PT_SAMPLER_VERSION_SPANS)return 0;
    a[*n].data=p;a[(*n)++].bytes=bytes;return 1;
}
int pt_sampler_version_spans(const struct pt_sample_version *v,
    struct pt_sampler_storage_span *out,unsigned capacity,unsigned *count)
{
    struct pt_sampler_storage_span a[PT_SAMPLER_VERSION_SPANS];unsigned n=0,i;
    const struct pt_sample_version *p=v;
    if(!v || !out || !count)return 0;
    for(i=0;i<2 && p;++i) {
        if(p->sample.pcm.capacity>SIZE_MAX/sizeof(int32_t) ||
           !version_span_add(a,&n,p,sizeof(*p)) ||
           !version_span_add(a,&n,p->sample.pcm.data,p->sample.pcm.capacity*sizeof(int32_t)) ||
           !version_span_add(a,&n,p->sample.slices,(size_t)p->sample.slice_count*sizeof(uint32_t)))return 0;
        if(i && p->backing)return 0;
        p=p->backing;
    }
    if(n>capacity || !pt_sampler_output_disjoint(v->owner,out,n*sizeof(*out)) ||
       !pt_sampler_output_disjoint(v->owner,count,sizeof(*count)) ||
       !pt_sampler_version_output_disjoint(v,out,n*sizeof(*out)) ||
       !pt_sampler_version_output_disjoint(v,count,sizeof(*count)) ||
       output_overlap(out,n*sizeof(*out),count,sizeof(*count)))return 0;
    memcpy(out,a,n*sizeof(*out));*count=n;return 1;
}
int pt_sampler_output_disjoint(const struct pt_sampler *s,const void *out,size_t bytes)
{
    unsigned i;
    if(!s || !out || !bytes || bytes>UINTPTR_MAX-(uintptr_t)out ||
       output_overlap(out,bytes,s,sizeof(*s)) || (s->table_bytes && !s->table) ||
       output_overlap(out,bytes,s->table,s->table_bytes))return 0;
    for(i=0;i<PT_PROJECT_SAMPLES;++i)
        if(!pt_sampler_version_output_disjoint(s->current[i],out,bytes))return 0;
    return 1;
}
int pt_sampler_pin_job_output_disjoint(const struct pt_sampler_pin_job *j,const void *out,size_t bytes)
{
    if(!j || !out || !bytes || bytes>UINTPTR_MAX-(uintptr_t)out ||
       output_overlap(out,bytes,j,sizeof(*j)))return 0;
    if(!j->value)return 1;
    return sample_output_disjoint(&j->source,out,bytes) &&
        pt_sampler_version_output_disjoint(j->value,out,bytes) &&
        pt_sampler_version_output_disjoint(j->previous,out,bytes);
}

void pt_sampler_init(struct pt_sampler *s,const struct pt_allocator *a,size_t budget)
{memset(s,0,sizeof(*s));s->allocator=*a;s->budget=budget;}
static void retain(struct pt_sample_version *v) {++v->references;}
static void release_version(struct pt_sample_version *v)
{
    if(v && !--v->references) {
        struct pt_sampler *s=v->owner;struct pt_sample_version *backing=v->backing;
        s->bytes-=v->bytes;
        if(v->owned_pcm)s->allocator.release(s->allocator.context,v->owned_pcm);
        s->allocator.release(s->allocator.context,v);release_version(backing);
    }
}
void pt_sampler_release(struct pt_sampler *s)
{
    unsigned i;for(i=0;i<PT_PROJECT_SAMPLES;++i) {release_version(s->current[i]);s->current[i]=NULL;}
    if(s->table) {s->bytes-=s->table_bytes;s->allocator.release(s->allocator.context,s->table);s->table=NULL;s->table_original=NULL;s->table_bytes=0;}
}
static struct pt_sample_version *version_allocate(struct pt_sampler *s,const struct pt_sample *sample)
{
    size_t values,bytes,slices;struct pt_sample_version *v;
    if(sample->pcm.frames>SIZE_MAX/sizeof(int32_t)/sample->pcm.channels)return NULL;
    values=(size_t)sample->pcm.frames*sample->pcm.channels;slices=(size_t)sample->slice_count*sizeof(uint32_t);
    if(values>(SIZE_MAX-sizeof(*v)-slices)/sizeof(int32_t))return NULL;
    bytes=sizeof(*v)+values*sizeof(int32_t)+slices;
    if(s->bytes>s->budget || bytes>s->budget-s->bytes)return NULL;
    v=s->allocator.allocate(s->allocator.context,bytes);if(!v)return NULL;
    memset(v,0,sizeof(*v));v->owner=s;v->bytes=bytes;v->references=1;v->sample=*sample;
    v->sample.pcm.data=(int32_t *)(v+1);v->sample.pcm.capacity=values;
    v->sample.slices=(uint32_t *)(v->sample.pcm.data+values);
    s->bytes+=bytes;return v;
}
/* Charge the full allocation, including unused recording capacity. Until the
 * append commits, the caller retains ownership and rollback frees only header. */
static struct pt_sample_version *version_borrow_owned(struct pt_sampler *s,const struct pt_sample *sample)
{
    struct pt_sample_version *v;size_t bytes;
    if(sample->pcm.capacity>(SIZE_MAX-sizeof(*v))/sizeof(int32_t))return NULL;
    bytes=sizeof(*v)+sample->pcm.capacity*sizeof(int32_t);
    if(s->bytes>s->budget || bytes>s->budget-s->bytes)return NULL;
    v=s->allocator.allocate(s->allocator.context,sizeof(*v));if(!v)return NULL;
    memset(v,0,sizeof(*v));v->owner=s;v->bytes=bytes;v->references=1;v->sample=*sample;
    s->bytes+=bytes;return v;
}
static struct pt_sample_version *version(struct pt_sampler *s,const struct pt_sample *sample)
{
    struct pt_sample_version *v=version_allocate(s,sample);size_t values;
    if(!v)return NULL;
    values=v->sample.pcm.capacity;
    if(values && sample->pcm.data)memcpy(v->sample.pcm.data,sample->pcm.data,values*sizeof(int32_t));
    if(sample->slice_count)memcpy(v->sample.slices,sample->slices,(size_t)sample->slice_count*sizeof(uint32_t));
    return v;
}
static int same(const struct pt_sample *a,const struct pt_sample *b)
{
    return !memcmp(a->name,b->name,sizeof(a->name)) && a->pcm.frames==b->pcm.frames && a->pcm.rate==b->pcm.rate &&
        a->pcm.channels==b->pcm.channels && a->pcm.bits==b->pcm.bits && a->loop_start==b->loop_start &&
        a->loop_end==b->loop_end && a->crossfade==b->crossfade && a->slice_count==b->slice_count &&
        a->loop==b->loop && a->volume==b->volume && a->interpolation==b->interpolation && a->finetune==b->finetune &&
        (!a->pcm.frames || a->pcm.data==b->pcm.data || !memcmp(a->pcm.data,b->pcm.data,(size_t)a->pcm.frames*a->pcm.channels*sizeof(int32_t))) &&
        (!a->slice_count || a->slices==b->slices || !memcmp(a->slices,b->slices,(size_t)a->slice_count*sizeof(uint32_t)));
}
enum pt_edit_result pt_sampler_pin(struct pt_sampler *s,struct pt_project *p,unsigned slot,
    unsigned generation,struct pt_pcm *pcm,struct pt_sample_version **token)
{
    struct pt_sample_version *v;
    if(!s || !pcm || !token || !s->allocator.allocate || !s->allocator.release ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(generation!=s->generation)return PT_EDIT_CONFLICT;
    v=s->current[slot];
    if(v && (!same(&v->sample,&p->samples[slot]) || v->references==UINT_MAX))return PT_EDIT_CONFLICT;
    if(!v) {
        v=version(s,&p->samples[slot]);if(!v)return PT_EDIT_CAPACITY;
        s->current[slot]=v;p->samples[slot]=v->sample;
    }
    retain(v);*pcm=v->sample.pcm;*token=v;return PT_EDIT_OK;
}
void pt_sampler_unpin(struct pt_sample_version *v) {release_version(v);}
static int same_storage(const struct pt_sample *a,const struct pt_sample *b)
{
    /* Pointer equality first keeps same() from scanning PCM or marker arrays. */
    return a->pcm.data==b->pcm.data && a->pcm.capacity==b->pcm.capacity &&
        a->slices==b->slices && same(a,b);
}
enum pt_edit_result pt_sampler_pin_current(struct pt_sampler *s,struct pt_project *p,unsigned slot,
    unsigned generation,struct pt_sample_version *expected,struct pt_pcm *pcm,struct pt_sample_version **token)
{
    if(!s || !p || !p->samples || !pcm || !token || !expected ||
       p->sample_count>PT_PROJECT_SAMPLES || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(generation!=s->generation || s->current[slot]!=expected || expected->owner!=s ||
       expected->references==UINT_MAX || !same_storage(&expected->sample,p->samples+slot))return PT_EDIT_CONFLICT;
    retain(expected);*pcm=expected->sample.pcm;*token=expected;return PT_EDIT_OK;
}
void pt_sampler_pin_job_cancel(struct pt_sampler_pin_job *j)
{
    if(!j)return;
    release_version(j->value);memset(j,0,sizeof(*j));
}
enum pt_edit_result pt_sampler_pin_job_begin(struct pt_sampler_pin_job *j,struct pt_sampler *s,
    struct pt_project *p,unsigned slot,unsigned generation)
{
    struct pt_sample_version *v;const struct pt_sample *source;
    if(!j || j->value || !s || !s->allocator.allocate || !s->allocator.release ||
       !p || !p->samples || p->sample_count>PT_PROJECT_SAMPLES || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(generation!=s->generation)return PT_EDIT_CONFLICT;
    source=p->samples+slot;
    if(pt_pcm_shape(&source->pcm)!=PT_PCM_OK || source->slice_count>PT_PROJECT_SLICES ||
       (source->slice_count && !source->slices))return PT_EDIT_INVALID;
    v=s->current[slot];
    if(v && (!same_storage(&v->sample,source) || v->references==UINT_MAX))return PT_EDIT_CONFLICT;
    if(v)retain(v);
    else {v=version_allocate(s,source);if(!v)return PT_EDIT_CAPACITY;}
    memset(j,0,sizeof(*j));j->owner=s;j->project=p;j->table=p->samples;j->count=p->sample_count;
    j->slot=slot;j->generation=generation;j->source=*source;j->previous=s->current[slot];j->value=v;
    j->values=(size_t)source->pcm.frames*source->pcm.channels*sizeof(int32_t);
    return PT_EDIT_OK;
}
enum pt_edit_result pt_sampler_pin_job_step(struct pt_sampler_pin_job *j,size_t bytes,
    struct pt_pcm *pcm,struct pt_sample_version **token,unsigned *ready)
{
    struct pt_sample_version *v;size_t n,slices;
    if(!j || !j->value || !bytes || bytes>PT_SAMPLER_PIN_CHUNK || !pcm || !token || !ready)return PT_EDIT_INVALID;
    if(j->failure)return j->failure;
    if(j->owner->generation!=j->generation || j->project->samples!=j->table ||
       j->project->sample_count!=j->count || j->owner->current[j->slot]!=j->previous ||
       !same_storage(j->table+j->slot,&j->source))return j->failure=PT_EDIT_CONFLICT;
    v=j->value;slices=(size_t)j->source.slice_count*sizeof(uint32_t);
    if(!j->previous) {
        n=j->values-j->copied_values;if(n>bytes)n=bytes;
        if(n)memcpy((uint8_t *)v->sample.pcm.data+j->copied_values,(const uint8_t *)j->source.pcm.data+j->copied_values,n);
        j->copied_values+=n;bytes-=n;
        n=slices-j->copied_slices;if(n>bytes)n=bytes;
        if(n)memcpy((uint8_t *)v->sample.slices+j->copied_slices,(const uint8_t *)j->source.slices+j->copied_slices,n);
        j->copied_slices+=n;
        if(j->copied_values<j->values || j->copied_slices<slices){*ready=0;return PT_EDIT_OK;}
        /* Publish only a complete immutable version. The job reference becomes
         * the caller pin; one new reference belongs to sampler.current. */
        retain(v);j->owner->current[j->slot]=v;j->table[j->slot]=v->sample;
    }
    *pcm=v->sample.pcm;*token=v;*ready=1;memset(j,0,sizeof(*j));return PT_EDIT_OK;
}

struct appended_sample {
    struct pt_sampler *owner;
    struct pt_sample *before,*after;
    struct pt_sample_version *value;
    unsigned count;
};
static int append_apply(void *context,struct pt_project *p,int direction)
{
    struct appended_sample *c=context;struct pt_sampler *s=c->owner;struct pt_project probe;
    struct pt_sample *from=direction>0?c->before:c->after,*to=direction>0?c->after:c->before;
    if(p->samples!=from || p->sample_count!=c->count+(direction<0?1U:0U) ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || (s->table && s->table!=c->after))return 0;
    if(direction<0) {
        if(!same(&p->samples[c->count],&c->value->sample))return 0;
        probe=*p;probe.sample_count=(uint16_t)c->count;
        if(pt_project_validate(&probe,NULL)!=PT_PROJECT_OK)return 0;
    }
    if(to!=from && c->count)memcpy(to,from,c->count*sizeof(*to));
    if(direction>0) {retain(c->value);to[c->count]=c->value->sample;}
    release_version(s->current[c->count]);s->current[c->count]=direction>0?c->value:NULL;
    if(!s->table) {s->table=c->after;s->table_original=c->before;s->table_bytes=PT_PROJECT_SAMPLES*sizeof(struct pt_sample);}
    p->samples=to;p->sample_count=(uint16_t)(c->count+(direction>0?1U:0U));++s->generation;return 1;
}
static void append_discard(void *context)
{
    struct appended_sample *c=context;struct pt_sampler *s=c->owner;
    release_version(c->value);s->bytes-=sizeof(*c);s->allocator.release(s->allocator.context,c);
}
static enum pt_edit_result append_sample(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_pcm *format,const char *name,pt_sample_fill fill,void *context,struct pt_pcm *owned)
{
    struct appended_sample *c;struct pt_sample sample;struct pt_sample *table;
    struct pt_pcm target;struct pt_edit_resource resource;enum pt_edit_result result;
    size_t table_bytes,needed,length=0;
    if(!s || !s->allocator.allocate || !s->allocator.release || !h || !format || !name || (!fill && !owned) ||
       pt_project_validate(p,NULL)!=PT_PROJECT_OK || !format->frames || !format->rate || format->rate>192000 ||
       (format->channels!=1 && format->channels!=2) || (format->bits!=8 && format->bits!=16 && format->bits!=24))return PT_EDIT_INVALID;
    while(length<PT_PROJECT_NAME && name[length])++length;
    if(length==PT_PROJECT_NAME)return PT_EDIT_INVALID;
    if(p->sample_count==PT_PROJECT_SAMPLES)return PT_EDIT_CAPACITY;
    if(s->table && p->samples!=s->table && p->samples!=s->table_original)return PT_EDIT_CONFLICT;
    table_bytes=s->table?0:PT_PROJECT_SAMPLES*sizeof(struct pt_sample);needed=sizeof(*c)+table_bytes;
    if(s->bytes>s->budget || needed>s->budget-s->bytes)return PT_EDIT_CAPACITY;
    c=s->allocator.allocate(s->allocator.context,sizeof(*c));if(!c)return PT_EDIT_CAPACITY;
    memset(c,0,sizeof(*c));c->owner=s;s->bytes+=sizeof(*c);
    table=s->table;
    if(!table) {
        table=s->allocator.allocate(s->allocator.context,table_bytes);
        if(!table) {append_discard(c);return PT_EDIT_CAPACITY;}
        s->bytes+=table_bytes;
    }
    memset(&sample,0,sizeof(sample));sample.pcm=*format;sample.pcm.data=NULL;sample.pcm.capacity=0;
    sample.volume=64;memcpy(sample.name,name,length);
    if(owned)sample.pcm=*owned;
    c->value=owned?version_borrow_owned(s,&sample):version(s,&sample);
    result=PT_EDIT_CAPACITY;if(!c->value)goto fail;
    c->before=p->samples;c->after=table;c->count=p->sample_count;
    target=c->value->sample.pcm;
    if(!owned) {result=fill(context,&target);if(result!=PT_EDIT_OK)goto fail;}
    if(target.data!=c->value->sample.pcm.data || target.capacity!=c->value->sample.pcm.capacity ||
       target.frames!=format->frames || target.rate!=format->rate || target.channels!=format->channels || target.bits!=format->bits ||
       pt_pcm_validate(&c->value->sample.pcm)!=PT_PCM_OK) {result=PT_EDIT_INVALID;goto fail;}
    resource=(struct pt_edit_resource){c,append_apply,append_discard};
    result=pt_pattern_resource_apply(p,h,&resource);
    if(result==PT_EDIT_OK) {
        if(owned) {c->value->owned_pcm=owned->data;memset(owned,0,sizeof(*owned));}
        return result;
    }
fail:
    append_discard(c);
    if(table_bytes) {s->bytes-=table_bytes;s->allocator.release(s->allocator.context,table);}
    return result;
}
enum pt_edit_result pt_sampler_append_generated(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_pcm *format,const char *name,pt_sample_fill fill,void *context)
{return append_sample(s,p,h,format,name,fill,context,NULL);}
enum pt_edit_result pt_sampler_append_owned(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,struct pt_pcm *pcm,const struct pt_allocator *allocator,const char *name)
{
    if(!s || !allocator || allocator->context!=s->allocator.context ||
       allocator->allocate!=s->allocator.allocate || allocator->release!=s->allocator.release ||
       !pcm || pt_pcm_validate(pcm)!=PT_PCM_OK)return PT_EDIT_INVALID;
    return append_sample(s,p,h,pcm,name,NULL,NULL,pcm);
}
static int apply(void *context,struct pt_project *p,int direction)
{
    struct sample_change *c=context;struct pt_sampler *s=c->owner;size_t i,count;
    struct pt_sample_version *expected=direction>0?c->before:c->after,*replacement=direction>0?c->after:c->before;
    if(pt_project_validate(p,NULL)!=PT_PROJECT_OK || c->slot>=p->sample_count || !same(&p->samples[c->slot],&expected->sample))return 0;
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==c->slot+1 && p->events[i].slice) {
        unsigned slice=p->events[i].slice;
        if(slice>replacement->sample.slice_count || (!c->resampled && expected->sample.slices[slice-1]!=replacement->sample.slices[slice-1]))return 0;
    }
    retain(replacement);release_version(s->current[c->slot]);s->current[c->slot]=replacement;
    p->samples[c->slot]=replacement->sample;++s->generation;return 1;
}
static void discard(void *context)
{
    struct sample_change *c=context;struct pt_sampler *s=c->owner;
    release_version(c->before);release_version(c->after);s->bytes-=sizeof(*c);s->allocator.release(s->allocator.context,c);
}
static enum pt_edit_result commit_kind(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,struct pt_sample_version *after,unsigned resampled)
{
    struct sample_change *c;struct pt_edit_resource resource;enum pt_edit_result result;
    if(same(&p->samples[slot],&after->sample)) {release_version(after);return PT_EDIT_OK;}
    if(s->bytes>s->budget || sizeof(*c)>s->budget-s->bytes) {release_version(after);return PT_EDIT_CAPACITY;}
    c=s->allocator.allocate(s->allocator.context,sizeof(*c));
    if(!c) {release_version(after);return PT_EDIT_CAPACITY;}
    s->bytes+=sizeof(*c);
    c->owner=s;c->slot=slot;c->resampled=resampled;c->after=after;c->before=s->current[slot];
    if(c->before)retain(c->before);
    else if(after->backing && same(&p->samples[slot],&after->backing->sample)) {c->before=after->backing;retain(c->before);}
    else c->before=version(s,&p->samples[slot]);
    if(!c->before) {discard(c);return PT_EDIT_CAPACITY;}
    resource.context=c;resource.apply=apply;resource.discard=discard;
    result=pt_pattern_resource_apply(p,h,&resource);if(result!=PT_EDIT_OK)discard(c);
    return result;
}
static enum pt_edit_result commit(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,struct pt_sample_version *after)
{return commit_kind(s,p,h,slot,after,0);}
enum pt_edit_result pt_sampler_attributes(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const char *name,unsigned volume,int finetune)
{
    struct pt_sample_version *base,*v;size_t length=0;int owned;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count || !name || volume>64 || finetune< -8 || finetune>7)return PT_EDIT_INVALID;
    while(length<PT_PROJECT_NAME && name[length])++length;
    if(length==PT_PROJECT_NAME)return PT_EDIT_INVALID;
    if(!strcmp(p->samples[slot].name,name) && p->samples[slot].volume==volume && p->samples[slot].finetune==finetune)return PT_EDIT_OK;
    base=s->current[slot];owned=base==NULL;
    if(base && !same(&base->sample,&p->samples[slot]))return PT_EDIT_CONFLICT;
    if(!base)base=version(s,&p->samples[slot]);
    if(!base)return PT_EDIT_CAPACITY;
    if(s->bytes>s->budget || sizeof(*v)>s->budget-s->bytes) {if(owned)release_version(base);return PT_EDIT_CAPACITY;}
    v=s->allocator.allocate(s->allocator.context,sizeof(*v));
    if(!v) {if(owned)release_version(base);return PT_EDIT_CAPACITY;}
    memset(v,0,sizeof(*v));v->sample=base->sample;v->owner=s;v->bytes=sizeof(*v);v->references=1;
    v->backing=base->backing?base->backing:base;retain(v->backing);s->bytes+=v->bytes;
    /* Copy possibly aliased input before releasing the temporary source owner. */
    memset(v->sample.name,0,sizeof(v->sample.name));memcpy(v->sample.name,name,length);
    v->sample.volume=(uint8_t)volume;v->sample.finetune=(int8_t)finetune;
    if(owned)release_version(base);
    return commit(s,p,h,slot,v);
}
enum pt_edit_result pt_sampler_edit(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,enum pt_pcm_edit op,uint32_t start,uint32_t end,unsigned gain)
{
    struct pt_sample_version *v;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(start>=end || end>p->samples[slot].pcm.frames)return PT_EDIT_INVALID;
    v=version(s,&p->samples[slot]);if(!v)return PT_EDIT_CAPACITY;
    if(pt_pcm_edit(&v->sample.pcm,op,start,end,gain)!=PT_PCM_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit(s,p,h,slot,v);
}
enum pt_edit_result pt_sampler_import(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const uint8_t *bytes,size_t length,const char *name)
{
    struct pt_wav_info info;struct pt_svx_info svx;int iff;struct pt_sample sample;struct pt_sample_version *v;size_t i,count;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    iff=bytes && length>=12 && !memcmp(bytes,"FORM",4) && !memcmp(bytes+8,"8SVX",4);
    if(iff) {
        if(pt_svx_inspect(bytes,length,&svx)!=PT_SVX_OK || !svx.frames)return PT_EDIT_UNSUPPORTED;
        info.frames=svx.frames;info.rate=svx.rate;info.channels=1;info.bits=8;
    } else if(pt_wav_inspect(bytes,length,&info)!=PT_WAV_OK || !info.frames)return PT_EDIT_UNSUPPORTED;
    /* A replacement cannot invalidate saved slice references in any pattern. */
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice)return PT_EDIT_UNSUPPORTED;
    memset(&sample,0,sizeof(sample));sample.volume=64;
    snprintf(sample.name,sizeof(sample.name),"%s",iff && svx.name[0]?svx.name:name?name:"IMPORTED SAMPLE");
    if(iff) {
        sample.volume=(uint8_t)((svx.volume*64+32768)/65536);
        sample.loop=svx.loop_end?PT_LOOP_FORWARD:PT_LOOP_NONE;
        sample.loop_start=svx.loop_start;sample.loop_end=svx.loop_end;
    }
    sample.pcm.frames=info.frames;sample.pcm.rate=info.rate;sample.pcm.channels=info.channels;sample.pcm.bits=info.bits;
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    if(iff?pt_svx_decode(bytes,length,&v->sample.pcm)!=PT_SVX_OK:pt_wav_decode(bytes,length,&v->sample.pcm)!=PT_WAV_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit(s,p,h,slot,v);
}
enum pt_edit_result pt_sampler_loop(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,enum pt_loop_kind kind,uint32_t start,uint32_t end,uint32_t fade)
{
    struct pt_sample_version *v;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(kind<PT_LOOP_NONE || kind>PT_LOOP_CROSSFADE)return PT_EDIT_INVALID;
    if(kind==PT_LOOP_NONE) {if(start || end || fade)return PT_EDIT_INVALID;}
    else if(start>=end || end>p->samples[slot].pcm.frames ||
        (kind==PT_LOOP_CROSSFADE?(!fade || fade>(end-start)/2):fade!=0))return PT_EDIT_INVALID;
    v=version(s,&p->samples[slot]);if(!v)return PT_EDIT_CAPACITY;
    if(kind==PT_LOOP_CROSSFADE) {
        if(pt_pcm_crossfade_loop(&v->sample.pcm,start,end,fade,&start)!=PT_PCM_OK) {release_version(v);return PT_EDIT_INVALID;}
        kind=PT_LOOP_FORWARD;
    }
    v->sample.loop=(uint8_t)kind;v->sample.loop_start=start;v->sample.loop_end=end;v->sample.crossfade=0;
    return commit(s,p,h,slot,v);
}
enum pt_edit_result pt_sampler_slices(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const uint32_t *markers,size_t count)
{
    struct pt_sample sample;struct pt_sample_version *v;size_t i,events;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    sample=p->samples[slot];
    if(!pt_slices_valid(sample.pcm.frames,markers,count))return PT_EDIT_INVALID;
    events=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<events;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice) {
        unsigned slice=p->events[i].slice;
        if(slice>count || sample.slices[slice-1]!=markers[slice-1])return PT_EDIT_UNSUPPORTED;
    }
    sample.slices=(uint32_t *)markers;sample.slice_count=(uint16_t)count;
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    return commit(s,p,h,slot,v);
}

static uint32_t scale_frame(uint32_t frame,uint32_t rate,uint32_t old_rate,int ceil)
{return (uint32_t)(((uint64_t)frame*rate+(ceil?old_rate-1:0))/old_rate);}
enum pt_edit_result pt_sampler_convert_quality(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,unsigned bits,uint32_t rate,unsigned filtered)
{
    struct pt_sample sample;const struct pt_sample *source;struct pt_sample_version *v;struct pt_pcm from;uint32_t frames;unsigned i,resampled;enum pt_pcm_result result;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count ||
        (bits!=8 && bits!=16 && bits!=24) || !rate || rate>192000 || filtered>1)return PT_EDIT_INVALID;
    source=&p->samples[slot];sample=*source;
    if(!sample.pcm.frames)return PT_EDIT_INVALID;
    if(bits==sample.pcm.bits && rate==sample.pcm.rate)return PT_EDIT_OK;
    if(filtered && (uint64_t)rate*128<sample.pcm.rate)return PT_EDIT_UNSUPPORTED;
    if(pt_pcm_resampled_frames(&sample.pcm,rate,&frames)!=PT_PCM_OK)return PT_EDIT_CAPACITY;
    resampled=rate!=sample.pcm.rate;
    if(resampled && sample.loop) {
        sample.loop_start=scale_frame(sample.loop_start,rate,sample.pcm.rate,0);
        sample.loop_end=scale_frame(sample.loop_end,rate,sample.pcm.rate,1);
        sample.crossfade=scale_frame(sample.crossfade,rate,sample.pcm.rate,0);
        if(sample.loop_start>=sample.loop_end || sample.loop_end>frames ||
            (sample.loop==PT_LOOP_CROSSFADE && (!sample.crossfade || sample.crossfade>(sample.loop_end-sample.loop_start)/2)))return PT_EDIT_UNSUPPORTED;
    }
    sample.pcm.data=NULL;sample.pcm.frames=frames;sample.pcm.rate=rate;
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    if(resampled)for(i=0;i<sample.slice_count;++i) {
        v->sample.slices[i]=scale_frame(source->slices[i],rate,source->pcm.rate,0);
        if(v->sample.slices[i]>=frames || (i && v->sample.slices[i]<=v->sample.slices[i-1])) {release_version(v);return PT_EDIT_UNSUPPORTED;}
    }
    result=resampled?(filtered?pt_pcm_resample_filtered_progress(&source->pcm,&v->sample.pcm,s->progress,s->progress_context):pt_pcm_resample(&source->pcm,&v->sample.pcm)):pt_pcm_convert(&source->pcm,&v->sample.pcm);
    if(result!=PT_PCM_OK) {release_version(v);return result==PT_PCM_CANCELLED?PT_EDIT_CANCELLED:PT_EDIT_INVALID;}
    from=v->sample.pcm;v->sample.pcm.bits=(uint8_t)bits;
    if(pt_pcm_convert(&from,&v->sample.pcm)!=PT_PCM_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit_kind(s,p,h,slot,v,resampled);
}

enum pt_edit_result pt_sampler_convert(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,unsigned bits,uint32_t rate)
{return pt_sampler_convert_quality(s,p,h,slot,bits,rate,0);}

static int svx_output_disjoint(const struct pt_sample *sample,const void *out,size_t bytes)
{
    return out && bytes && bytes<=UINTPTR_MAX-(uintptr_t)out &&
        !output_overlap(out,bytes,sample,sizeof(*sample)) && sample_output_disjoint(sample,out,bytes);
}
enum pt_svx_result pt_sampler_svx_size(const struct pt_sample *sample,size_t *size)
{
    struct pt_svx_info info={0};size_t n;enum pt_svx_result result;
    if(!sample || !sample->pcm.frames)return PT_SVX_INVALID;
    if(sample->loop>PT_LOOP_FORWARD || sample->slice_count || sample->finetune)return PT_SVX_UNSUPPORTED;
    if(!size)return PT_SVX_INVALID;
    info.loop_start=sample->loop_start;info.loop_end=sample->loop_end;info.volume=(uint32_t)sample->volume*1024;
    result=pt_svx_size(&sample->pcm,&info,&n);if(result!=PT_SVX_OK)return result;
    if(!svx_output_disjoint(sample,size,sizeof(*size)))return PT_SVX_ALIAS;
    *size=n;return PT_SVX_OK;
}
enum pt_svx_result pt_sampler_svx_encode(const struct pt_sample *sample,uint8_t *bytes,size_t capacity,size_t *written)
{
    size_t size;struct pt_svx_info info={0};enum pt_svx_result result=pt_sampler_svx_size(sample,&size);
    if(result!=PT_SVX_OK)return result;
    if(!bytes || !written)return PT_SVX_INVALID;
    if(capacity<size)return PT_SVX_CAPACITY;
    if(!svx_output_disjoint(sample,bytes,size) || !svx_output_disjoint(sample,written,sizeof(*written)))return PT_SVX_ALIAS;
    memcpy(info.name,sample->name,sizeof(info.name));info.loop_start=sample->loop_start;
    info.loop_end=sample->loop_end;info.volume=(uint32_t)sample->volume*1024;
    return pt_svx_encode(&sample->pcm,&info,bytes,capacity,written);
}

enum pt_edit_result pt_sampler_import_raw_fill(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,size_t length,const char *name,const struct pt_raw_format *format,pt_sampler_raw_fill fill,void *context)
{
    struct pt_sample sample={0};struct pt_sample_version *v;size_t i,count;uint32_t frames;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(!fill || pt_raw_frames(length,format,&frames)!=PT_RAW_OK || !frames)return PT_EDIT_UNSUPPORTED;
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice)return PT_EDIT_UNSUPPORTED;
    sample.volume=64;snprintf(sample.name,sizeof(sample.name),"%s",name?name:"IMPORTED RAW");
    sample.pcm.frames=frames;sample.pcm.rate=format->rate;sample.pcm.bits=format->bits;sample.pcm.channels=format->channels;
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    if(fill(context,v->sample.pcm.data,frames,format)!=1 || pt_pcm_validate(&v->sample.pcm)!=PT_PCM_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit(s,p,h,slot,v);
}

enum pt_edit_result pt_sampler_import_svx_fill(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const struct pt_svx_info *info,const char *name,pt_sampler_raw_fill fill,void *context)
{
    struct pt_sample sample={0};struct pt_sample_version *v;struct pt_raw_format format;size_t i,count;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(!info || !fill || !info->frames || !info->rate || info->rate>65535 || info->volume>65536 ||
       !memchr(info->name,0,sizeof(info->name)) || (info->loop_end?(info->loop_start>=info->loop_end || info->loop_end>info->frames):info->loop_start!=0))return PT_EDIT_UNSUPPORTED;
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice)return PT_EDIT_UNSUPPORTED;
    snprintf(sample.name,sizeof(sample.name),"%s",info->name[0]?info->name:name?name:"IMPORTED SAMPLE");
    sample.volume=(uint8_t)((info->volume*64+32768)/65536);
    sample.loop=info->loop_end?PT_LOOP_FORWARD:PT_LOOP_NONE;sample.loop_start=info->loop_start;sample.loop_end=info->loop_end;
    sample.pcm.frames=info->frames;sample.pcm.rate=info->rate;sample.pcm.bits=8;sample.pcm.channels=1;
    format=(struct pt_raw_format){info->rate,8,1,0,0};
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    if(fill(context,v->sample.pcm.data,info->frames,&format)!=1 || pt_pcm_validate(&v->sample.pcm)!=PT_PCM_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit(s,p,h,slot,v);
}

struct raw_memory {const uint8_t *bytes;size_t length;};
static int raw_memory_fill(void *context,int32_t *data,uint32_t frames,const struct pt_raw_format *format)
{
    struct raw_memory *r=context;struct pt_pcm pcm={data,(size_t)frames*format->channels,frames,format->rate,format->channels,format->bits};
    return pt_raw_decode(r->bytes,r->length,format,&pcm)==PT_RAW_OK;
}
enum pt_edit_result pt_sampler_import_raw(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const uint8_t *bytes,size_t length,const char *name,const struct pt_raw_format *format)
{
    struct raw_memory r={bytes,length};if(!bytes)return PT_EDIT_UNSUPPORTED;
    return pt_sampler_import_raw_fill(s,p,h,slot,length,name,format,raw_memory_fill,&r);
}

enum pt_edit_result pt_sampler_import_slot(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const struct pt_project *source,unsigned selected)
{
    struct pt_sample_version *v;size_t i,count;const struct pt_sample *sample;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || pt_project_validate(source,NULL)!=PT_PROJECT_OK || slot>=p->sample_count || selected>=source->sample_count)return PT_EDIT_INVALID;
    sample=&source->samples[selected];if(!sample->pcm.frames)return PT_EDIT_INVALID;
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice) {
        unsigned slice=p->events[i].slice;
        if(slice>sample->slice_count || sample->slices[slice-1]!=p->samples[slot].slices[slice-1])return PT_EDIT_UNSUPPORTED;
    }
    v=version(s,sample);if(!v)return PT_EDIT_CAPACITY;
    return commit(s,p,h,slot,v);
}
#include "sampler_workflow.h"
#include "mod_project.h"
struct workflow_entry {
    unsigned slot;struct pt_sample source;
    struct pt_sample_version *previous,*before,*after;
    size_t copied_values,copied_slices;
    unsigned retained;
};
struct workflow_span {const void *data;size_t bytes;};
struct pt_sampler_workflow {
    struct pt_sampler *owner;struct pt_project *project;struct pt_pattern_history *history;
    struct pt_project snapshot;struct pt_pattern_history history_snapshot;
    struct pt_sample copy_source;unsigned source_slot;
    unsigned generation,count,kind,ready,initialized,first_apply;
    unsigned append,copy_started,copy_done,table_owned;
    uint32_t start,end;size_t marker_first,copy_values,copy_slices;
    struct pt_sample *table_after;
    size_t bytes,reserved_bytes,span_count;struct workflow_span *spans;struct pt_sampler_workflow_stats stats;
    struct workflow_entry entries[];
};
static int workflow_project_apart(const struct pt_project *p,const void *out,size_t bytes)
{
    unsigned i;size_t n;
    if(!p || !out || !bytes || bytes>UINTPTR_MAX-(uintptr_t)out ||
       !p->channels.count || p->channels.count>PT_CHANNEL_LIMIT ||
       p->sample_count>PT_PROJECT_SAMPLES || p->pattern_count>PT_PROJECT_PATTERNS ||
       p->order_count>PT_PROJECT_ORDERS || p->extension_count>4090)return 0;
    n=(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count;
    if(output_overlap(out,bytes,p,sizeof(*p)) ||
       output_overlap(out,bytes,p->events,n*sizeof(*p->events)) ||
       output_overlap(out,bytes,p->orders,p->order_count*sizeof(*p->orders)) ||
       output_overlap(out,bytes,p->samples,p->sample_count*sizeof(*p->samples)) ||
       output_overlap(out,bytes,p->extensions,p->extension_count*sizeof(*p->extensions)))return 0;
    if((n && !p->events) || (p->order_count && !p->orders) ||
       (p->sample_count && !p->samples) || (p->extension_count && !p->extensions))return 0;
    for(i=0;i<p->sample_count;++i)if(!sample_output_disjoint(p->samples+i,out,bytes))return 0;
    for(i=0;i<p->extension_count;++i) {
        if((p->extensions[i].length && !p->extensions[i].data) ||
           output_overlap(out,bytes,p->extensions[i].data,p->extensions[i].length))return 0;
    }
    return 1;
}
static int workflow_history_apart(const struct pt_pattern_history *h,const void *out,size_t bytes)
{
    return h && h->command_capacity<=SIZE_MAX/sizeof(*h->commands) &&
        h->change_capacity<=SIZE_MAX/sizeof(*h->changes) &&
        !output_overlap(out,bytes,h,sizeof(*h)) &&
        !output_overlap(out,bytes,h->commands,h->command_capacity*sizeof(*h->commands)) &&
        !output_overlap(out,bytes,h->changes,h->change_capacity*sizeof(*h->changes));
}
static int workflow_external_apart(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const void *out,size_t bytes)
{return pt_sampler_output_disjoint(s,out,bytes) && workflow_project_apart(p,out,bytes) && workflow_history_apart(h,out,bytes);}
static int workflow_apart(const struct pt_sampler_workflow *c,const void *out,size_t bytes)
{
    unsigned i;
    if(!c || !out || !bytes || output_overlap(out,bytes,c,c->bytes) ||
       !pt_sampler_output_disjoint(c->owner,out,bytes) ||
       output_overlap(out,bytes,c->project,sizeof(*c->project)) ||
       output_overlap(out,bytes,c->history,sizeof(*c->history)) ||
       (c->table_owned && output_overlap(out,bytes,c->table_after,PT_PROJECT_SAMPLES*sizeof(*c->table_after))))return 0;
    /* Captured extents protect source outputs without walking a changed or
     * released table. Cancellation needs only living controls/private owners. */
    for(i=0;i<c->span_count;++i)
        if(output_overlap(out,bytes,c->spans[i].data,c->spans[i].bytes))return 0;
    for(i=0;i<c->count;++i)
        if(!pt_sampler_version_output_disjoint(c->entries[i].before,out,bytes) ||
           !pt_sampler_version_output_disjoint(c->entries[i].after,out,bytes))return 0;
    return 1;
}
static int workflow_same_project(const struct pt_project *a,const struct pt_project *b)
{
    size_t offset=offsetof(struct pt_project,channels)+offsetof(struct pt_channels,selected);
    return !memcmp(a,b,offset) && !memcmp((const uint8_t *)a+offset+1,(const uint8_t *)b+offset+1,sizeof(*a)-offset-1);
}
static int workflow_preview_current(const struct pt_sample_usage_preview *v,const struct pt_project *p,
    const struct pt_pattern_history *h,const struct pt_sampler *s)
{
    unsigned i;
    if(!v || v->project!=p || v->count!=p->sample_count || v->revision!=h->revision ||
       v->generation!=s->generation || !workflow_same_project(&v->snapshot,p))return 0;
    for(i=0;i<v->count;++i)if(memcmp(&v->rows[i].identity,p->samples+i,sizeof(*p->samples)))return 0;
    return 1;
}
static int workflow_history_room(const struct pt_project *p,const struct pt_pattern_history *h)
{
    return h && h->bound_events==p->events && h->bound_patterns==p->pattern_count &&
        h->bound_channels==p->channels.count && h->commands && h->changes &&
        h->command_capacity && h->change_capacity && h->count<h->command_capacity &&
        h->cursor<=h->count && h->used<=h->change_capacity && h->next_revision &&
        h->next_revision<UINT32_MAX;
}
static int workflow_current(const struct pt_sampler_workflow *c)
{
    unsigned i;
    if(c->owner->generation!=c->generation || !workflow_same_project(c->project,&c->snapshot) ||
       memcmp(c->history,&c->history_snapshot,sizeof(c->history_snapshot)))return 0;
    for(i=0;i<c->count;++i) {
        const struct workflow_entry *e=c->entries+i;
        if(e->slot<c->snapshot.sample_count &&
           (memcmp(c->project->samples+e->slot,&e->source,sizeof(e->source)) || c->owner->current[e->slot]!=e->previous))return 0;
    }
    if(c->kind==2 && memcmp(c->project->samples+c->source_slot,&c->copy_source,sizeof(c->copy_source)))return 0;
    return 1;
}
static struct pt_sample_version *workflow_version_allocate(struct pt_sampler *s,const struct pt_sample *sample,
    struct pt_sampler_workflow *c,struct pt_sampler_workflow **out,enum pt_edit_result *result)
{
    struct pt_sample_version *v;size_t capacity=sample->pcm.capacity,slices,bytes;
    slices=(size_t)sample->slice_count*sizeof(uint32_t);
    if(capacity>(SIZE_MAX-sizeof(*v)-slices)/sizeof(int32_t))return NULL;
    bytes=sizeof(*v)+capacity*sizeof(int32_t)+slices;
    if(s->bytes>s->budget || bytes>s->budget-s->bytes)return NULL;
    v=s->allocator.allocate(s->allocator.context,bytes);if(!v)return NULL;
    if(!workflow_current(c)) {s->allocator.release(s->allocator.context,v);*result=PT_EDIT_CONFLICT;return NULL;}
    if(!workflow_apart(c,v,bytes) || output_overlap(v,bytes,out,sizeof(*out))) {
        s->allocator.release(s->allocator.context,v);*result=PT_EDIT_ALIAS;return NULL;
    }
    memset(v,0,sizeof(*v));v->owner=s;v->bytes=bytes;v->references=1;v->sample=*sample;
    v->sample.pcm.data=capacity?(int32_t *)(v+1):NULL;
    v->sample.slices=sample->slice_count?(uint32_t *)((uint8_t *)(v+1)+capacity*sizeof(int32_t)):NULL;
    s->bytes+=bytes;return v;
}
static void workflow_discard(void *context)
{
    struct pt_sampler_workflow *c=context;struct pt_sampler *s=c->owner;unsigned i;
    for(i=0;i<c->count;++i) {release_version(c->entries[i].before);release_version(c->entries[i].after);}
    if(c->table_owned) {
        size_t n=PT_PROJECT_SAMPLES*sizeof(*c->table_after);
        s->bytes-=n;s->allocator.release(s->allocator.context,c->table_after);
    }
    s->bytes-=c->bytes;s->allocator.release(s->allocator.context,c);
}
void pt_sampler_workflow_cancel(struct pt_sampler_workflow **job)
{
    struct pt_sampler_workflow *c;
    if(!job || !(c=*job) || c->initialized || !workflow_apart(c,job,sizeof(*job)))return;
    *job=NULL;workflow_discard(c);
}
static void workflow_span_add(struct pt_sampler_workflow *c,const void *data,size_t bytes)
{if(bytes)c->spans[c->span_count++]=(struct workflow_span){data,bytes};}
static struct pt_sampler_workflow *workflow_allocate(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_sample_usage_preview *v,unsigned count,unsigned kind,
    struct pt_sampler_workflow **out,enum pt_edit_result *result)
{
    struct pt_sampler_workflow *c;unsigned i,generation=s->generation;
    struct pt_project snapshot=*p;struct pt_pattern_history history_snapshot=*h;struct pt_allocator allocator=s->allocator;
    size_t maximum=6+(size_t)p->sample_count*2+p->extension_count;
    size_t bytes=sizeof(*c)+count*sizeof(struct workflow_entry);
    if(maximum>(SIZE_MAX-bytes)/sizeof(struct workflow_span))return NULL;
    bytes+=maximum*sizeof(struct workflow_span);
    if(s->bytes>s->budget || bytes>s->budget-s->bytes)return NULL;
    c=allocator.allocate(allocator.context,bytes);if(!c)return NULL;
    if(s->generation!=generation || !workflow_same_project(p,&snapshot) || memcmp(h,&history_snapshot,sizeof(*h)) ||
       !workflow_preview_current(v,p,h,s)) {allocator.release(allocator.context,c);*result=PT_EDIT_CONFLICT;return NULL;}
    if(s->bytes>s->budget || bytes>s->budget-s->bytes) {allocator.release(allocator.context,c);return NULL;}
    if(!workflow_external_apart(s,p,h,c,bytes) || output_overlap(c,bytes,v,sizeof(*v)) ||
       output_overlap(c,bytes,out,sizeof(*out))) {allocator.release(allocator.context,c);*result=PT_EDIT_ALIAS;return NULL;}
    memset(c,0,bytes);c->owner=s;c->project=p;c->history=h;c->snapshot=snapshot;
    c->history_snapshot=history_snapshot;c->generation=generation;c->count=count;c->kind=kind;c->bytes=bytes;
    c->spans=(struct workflow_span *)(c->entries+count);
    workflow_span_add(c,p->events,(size_t)p->pattern_count*PT_PROJECT_ROWS*p->channels.count*sizeof(*p->events));
    workflow_span_add(c,p->orders,p->order_count*sizeof(*p->orders));
    workflow_span_add(c,p->samples,p->sample_count*sizeof(*p->samples));
    workflow_span_add(c,p->extensions,p->extension_count*sizeof(*p->extensions));
    workflow_span_add(c,h->commands,h->command_capacity*sizeof(*h->commands));
    workflow_span_add(c,h->changes,h->change_capacity*sizeof(*h->changes));
    for(i=0;i<p->sample_count;++i) {
        workflow_span_add(c,p->samples[i].pcm.data,p->samples[i].pcm.capacity*sizeof(int32_t));
        workflow_span_add(c,p->samples[i].slices,p->samples[i].slice_count*sizeof(uint32_t));
    }
    for(i=0;i<p->extension_count;++i)workflow_span_add(c,p->extensions[i].data,p->extensions[i].length);
    c->reserved_bytes=s->bytes;s->bytes+=bytes;return c;
}
static int workflow_before(struct pt_sampler_workflow *c,unsigned i,unsigned slot,
    struct pt_sampler_workflow **out,enum pt_edit_result *result)
{
    struct workflow_entry *e=c->entries+i;struct pt_sampler *s=c->owner;
    if(e->slot!=slot)return 0;
    if(slot<c->snapshot.sample_count) {
        if(e->previous) {
            if(e->previous->owner!=s || e->previous->references==UINT_MAX ||
               !same_storage(&e->previous->sample,&e->source))return 0;
            retain(e->previous);e->before=e->previous;e->retained=1;
        } else e->before=workflow_version_allocate(s,&e->source,c,out,result);
        if(!e->before)return 0;
        c->stats.retained_master_bytes+=(uint64_t)e->source.pcm.capacity*sizeof(int32_t);
    }
    return 1;
}
static enum pt_edit_result workflow_begin_valid(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_sample_usage_preview *v,unsigned stopped,
    struct pt_sampler_workflow **out)
{
    if(!s || !p || !h || !v || !out || !s->allocator.allocate || !s->allocator.release)return PT_EDIT_INVALID;
    if(!pt_sampler_output_disjoint(s,out,sizeof(*out)) || output_overlap(out,sizeof(*out),p,sizeof(*p)) ||
       output_overlap(out,sizeof(*out),h,sizeof(*h)) || output_overlap(out,sizeof(*out),v,sizeof(*v)))return PT_EDIT_ALIAS;
    if(!stopped)return PT_EDIT_CONFLICT;
    if(!workflow_preview_current(v,p,h,s))return PT_EDIT_CONFLICT;
    if(!workflow_external_apart(s,p,h,out,sizeof(*out)))return PT_EDIT_ALIAS;
    if(*out)return PT_EDIT_INVALID;
    if(!workflow_history_room(p,h))return PT_EDIT_CAPACITY;
    /* Initial full validation is synchronous, as in existing sampler mutations. */
    if(pt_project_validate(p,NULL)!=PT_PROJECT_OK)return PT_EDIT_INVALID;
    if(s->table && p->samples!=s->table && p->samples!=s->table_original)return PT_EDIT_CONFLICT;
    return PT_EDIT_OK;
}
enum pt_edit_result pt_sampler_cleanup_begin(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_sample_usage_preview *v,
    const uint8_t selected[PT_PROJECT_SAMPLES],unsigned stopped,struct pt_sampler_workflow **out)
{
    struct pt_sampler_workflow *c;unsigned i,n=0;uint8_t choice[PT_PROJECT_SAMPLES];enum pt_edit_result result;
    result=workflow_begin_valid(s,p,h,v,stopped,out);if(result!=PT_EDIT_OK)return result;
    if(!selected)return PT_EDIT_INVALID;
    if(output_overlap(out,sizeof(*out),selected,PT_PROJECT_SAMPLES))return PT_EDIT_ALIAS;
    memcpy(choice,selected,sizeof(choice));
    for(i=0;i<PT_PROJECT_SAMPLES;++i)if(choice[i]) {
        if(i>=v->count || !(v->rows[i].flags&PT_USAGE_ELIGIBLE) ||
           (v->rows[i].flags&(PT_USAGE_REFERENCED|PT_USAGE_RESERVED|PT_USAGE_PROTECTED|PT_USAGE_UNKNOWN|PT_USAGE_FREE)))return PT_EDIT_UNSUPPORTED;
        ++n;
    }
    if(!n)return PT_EDIT_INVALID;
    result=PT_EDIT_CAPACITY;c=workflow_allocate(s,p,h,v,n,1,out,&result);if(!c)return result;
    n=0;
    for(i=0;i<v->count;++i)if(choice[i]) {
        struct workflow_entry *e=c->entries+n++;
        e->slot=i;e->source=p->samples[i];e->previous=s->current[i];
    }
    n=0;
    for(i=0;i<v->count;++i)if(choice[i]) {
        if(!workflow_before(c,n++,i,out,&result)) {workflow_discard(c);return result;}
        if(!workflow_current(c)) {workflow_discard(c);return PT_EDIT_CONFLICT;}
    }
    c->stats.affected_slots=n;c->stats.slots_freed=n;c->stats.staged_bytes=s->bytes-c->reserved_bytes;
    if(!workflow_apart(c,out,sizeof(*out))) {workflow_discard(c);return PT_EDIT_ALIAS;}
    *out=c;return PT_EDIT_OK;
}
enum pt_edit_result pt_sampler_copy_begin(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_sample_usage_preview *v,unsigned source_slot,
    uint32_t start,uint32_t end,unsigned stopped,struct pt_sampler_workflow **out)
{
    struct pt_sampler_workflow *c;struct pt_sample sample;unsigned slot,i,n=0;enum pt_edit_result result;
    result=workflow_begin_valid(s,p,h,v,stopped,out);if(result!=PT_EDIT_OK)return result;
    if(source_slot>=p->sample_count || start>=end || end>p->samples[source_slot].pcm.frames)return PT_EDIT_INVALID;
    for(slot=0;slot<p->sample_count;++slot) {
        const struct pt_sample_usage_row *r=v->rows+slot;struct pt_sample empty;
        memset(&empty,0,sizeof(empty));empty.pcm.bits=8;empty.pcm.channels=1;empty.pcm.rate=PT_CLASSIC_RATE;
        if((r->flags&PT_USAGE_FREE) && !(r->flags&(PT_USAGE_REFERENCED|PT_USAGE_RESERVED|PT_USAGE_PROTECTED|PT_USAGE_UNKNOWN|PT_USAGE_CONFIGURED|PT_USAGE_ELIGIBLE)) &&
           !memcmp(p->samples+slot,&empty,sizeof(empty)))break;
    }
    if(slot==p->sample_count && slot==PT_PROJECT_SAMPLES)return PT_EDIT_CAPACITY;
    if(slot==p->sample_count) {
        if(v->rows[slot].flags&(PT_USAGE_RESERVED|PT_USAGE_PROTECTED|PT_USAGE_UNKNOWN))return PT_EDIT_UNSUPPORTED;
        for(i=0;i<p->extension_count;++i) {
            const struct pt_extension *e=p->extensions+i;
            if(e->id!=PT_CLASSIC_HEADER_TAG || e->version!=1 || e->length!=1084)return PT_EDIT_UNSUPPORTED;
        }
    }
    result=PT_EDIT_CAPACITY;c=workflow_allocate(s,p,h,v,1,2,out,&result);if(!c)return result;
    c->source_slot=source_slot;c->copy_source=p->samples[source_slot];c->start=start;c->end=end;
    c->append=slot==p->sample_count;c->entries[0].slot=slot;
    if(!c->append) {c->entries[0].source=p->samples[slot];c->entries[0].previous=s->current[slot];}
    if(!workflow_before(c,0,slot,out,&result)) {workflow_discard(c);return result;}
    if(!workflow_current(c)) {workflow_discard(c);return PT_EDIT_CONFLICT;}
    if(c->append) {
        c->table_after=s->table;
        if(!c->table_after) {
            size_t bytes=PT_PROJECT_SAMPLES*sizeof(*c->table_after);
            if(s->bytes>s->budget || bytes>s->budget-s->bytes) {workflow_discard(c);return PT_EDIT_CAPACITY;}
            c->table_after=s->allocator.allocate(s->allocator.context,bytes);
            if(!c->table_after) {workflow_discard(c);return PT_EDIT_CAPACITY;}
            if(!workflow_current(c)) {s->allocator.release(s->allocator.context,c->table_after);c->table_after=NULL;workflow_discard(c);return PT_EDIT_CONFLICT;}
            if(!workflow_apart(c,c->table_after,bytes) || output_overlap(c->table_after,bytes,out,sizeof(*out))) {
                s->allocator.release(s->allocator.context,c->table_after);c->table_after=NULL;workflow_discard(c);return PT_EDIT_ALIAS;
            }
            s->bytes+=bytes;c->table_owned=1;
        }
    }
    sample=c->copy_source;sample.pcm.frames=end-start;sample.pcm.capacity=(size_t)(end-start)*sample.pcm.channels;
    if(sample.loop && sample.loop_start>=start && sample.loop_end<=end) {
        sample.loop_start-=start;sample.loop_end-=start;
    } else {sample.loop=PT_LOOP_NONE;sample.loop_start=sample.loop_end=sample.crossfade=0;}
    for(i=0;i<sample.slice_count && sample.slices[i]<start;++i) {}
    c->marker_first=i;
    while(i+n<sample.slice_count && sample.slices[i+n]<end)++n;
    sample.slice_count=(uint16_t)n;sample.slices=n?sample.slices+i:NULL;
    c->entries[0].after=workflow_version_allocate(s,&sample,c,out,&result);
    if(!c->entries[0].after) {workflow_discard(c);return result;}
    if(!workflow_current(c)) {workflow_discard(c);return PT_EDIT_CONFLICT;}
    c->stats.affected_slots=1;c->stats.destination_slot=slot;c->stats.appended=c->append;c->stats.staged_bytes=s->bytes-c->reserved_bytes;
    if(!workflow_apart(c,out,sizeof(*out))) {workflow_discard(c);return PT_EDIT_ALIAS;}
    *out=c;return PT_EDIT_OK;
}
/* Copy a private retention version, zeroing unused capacity without reading it. */
static int workflow_copy_before(struct workflow_entry *e)
{
    struct pt_pcm *to=&e->before->sample.pcm;size_t active=(size_t)e->source.pcm.frames*e->source.pcm.channels,n,offset=e->copied_values;
    if(e->retained)return 1;
    if(offset<to->capacity) {
        n=to->capacity-offset;if(n>PT_SAMPLER_PIN_CHUNK/sizeof(int32_t))n=PT_SAMPLER_PIN_CHUNK/sizeof(int32_t);
        if(offset<active) {
            size_t copied=active-offset;if(copied>n)copied=n;
            memcpy(to->data+offset,e->source.pcm.data+offset,copied*sizeof(int32_t));
            if(copied<n)memset(to->data+offset+copied,0,(n-copied)*sizeof(int32_t));
        } else memset(to->data+offset,0,n*sizeof(int32_t));
        e->copied_values+=n;return 0;
    }
    offset=e->copied_slices;
    if(offset<e->source.slice_count) {
        n=e->source.slice_count-offset;if(n>PT_SAMPLER_PIN_CHUNK/sizeof(uint32_t))n=PT_SAMPLER_PIN_CHUNK/sizeof(uint32_t);
        memcpy(e->before->sample.slices+offset,e->source.slices+offset,n*sizeof(uint32_t));e->copied_slices+=n;return 0;
    }
    return 1;
}
enum pt_edit_result pt_sampler_workflow_step(struct pt_sampler_workflow *c,unsigned *ready)
{
    unsigned i;size_t n;
    if(!c || !ready || c->initialized)return PT_EDIT_INVALID;
    if(!workflow_apart(c,ready,sizeof(*ready)))return PT_EDIT_ALIAS;
    if(!workflow_current(c))return PT_EDIT_CONFLICT;
    if(c->ready) {*ready=1;return PT_EDIT_OK;}
    for(i=0;i<c->count;++i)if(c->entries[i].before && !workflow_copy_before(c->entries+i)) {*ready=0;return PT_EDIT_OK;}
    if(c->kind==2) {
        struct pt_sample *sample=&c->entries[0].after->sample;
        size_t values=(size_t)sample->pcm.frames*sample->pcm.channels;
        if(c->copy_values<values) {
            n=values-c->copy_values;if(n>PT_SAMPLER_PIN_CHUNK/sizeof(int32_t))n=PT_SAMPLER_PIN_CHUNK/sizeof(int32_t);
            memcpy(sample->pcm.data+c->copy_values,c->copy_source.pcm.data+(size_t)c->start*sample->pcm.channels+c->copy_values,n*sizeof(int32_t));
            c->copy_values+=n;*ready=0;return PT_EDIT_OK;
        }
        if(c->copy_slices<sample->slice_count) {
            n=sample->slice_count-c->copy_slices;if(n>PT_SAMPLER_PIN_CHUNK/sizeof(uint32_t))n=PT_SAMPLER_PIN_CHUNK/sizeof(uint32_t);
            for(i=0;i<n;++i)sample->slices[c->copy_slices+i]=c->copy_source.slices[c->marker_first+c->copy_slices+i]-c->start;
            c->copy_slices+=n;*ready=0;return PT_EDIT_OK;
        }
    }
    c->ready=1;*ready=1;return PT_EDIT_OK;
}
static void workflow_empty(struct pt_sample *sample)
{memset(sample,0,sizeof(*sample));sample->pcm.bits=8;sample->pcm.channels=1;sample->pcm.rate=PT_CLASSIC_RATE;}
static int workflow_referenced(const struct pt_sampler_workflow *c,unsigned slot)
{
    size_t i,n=(size_t)c->project->pattern_count*PT_PROJECT_ROWS*c->project->channels.count;
    for(i=0;i<n;++i)if(c->project->events[i].instrument==slot+1)return 1;
    return 0;
}
static int workflow_apply(void *context,struct pt_project *p,int direction)
{
    struct pt_sampler_workflow *c=context;struct pt_sampler *s=c->owner;struct pt_sample empty;
    unsigned i;struct pt_sample *table;unsigned expected_count=c->snapshot.sample_count;
    if(p!=c->project || (direction!=-1 && direction!=1) || !c->ready)return 0;
    if(c->append && direction<0)++expected_count;
    if(p->sample_count!=expected_count || p->events!=c->snapshot.events || p->pattern_count!=c->snapshot.pattern_count ||
       p->channels.count!=c->snapshot.channels.count || s->generation==UINT_MAX)return 0;
    if(!c->initialized) {if(direction<0 || !workflow_current(c))return 0;}
    workflow_empty(&empty);
    table=c->append?(direction>0?c->snapshot.samples:c->table_after):c->snapshot.samples;
    if(p->samples!=table)return 0;
    for(i=0;i<c->count;++i) {
        struct workflow_entry *e=c->entries+i;const struct pt_sample *expected;
        struct pt_sample_version *expected_version;
        if(c->append && direction>0) {
            if(s->current[e->slot])return 0;
        } else {
            if(direction>0) {
                expected=c->initialized?&e->before->sample:&e->source;
                expected_version=c->initialized?e->before:e->previous;
            } else {expected=e->after?&e->after->sample:&empty;expected_version=e->after;}
            if(memcmp(p->samples+e->slot,expected,sizeof(*expected)) || s->current[e->slot]!=expected_version)return 0;
        }
        if((direction>0 && (!c->append || c->kind==1)) || (c->append && direction<0))
            if(workflow_referenced(c,e->slot))return 0;
        if((direction>0?e->after:e->before) && (direction>0?e->after:e->before)->references==UINT_MAX)return 0;
    }
    if(c->append) {
        struct pt_sample *to=direction>0?c->table_after:c->snapshot.samples;
        if(to!=table && c->snapshot.sample_count)memcpy(to,table,c->snapshot.sample_count*sizeof(*to));
        if(!s->table) {s->table=c->table_after;s->table_original=c->snapshot.samples;s->table_bytes=PT_PROJECT_SAMPLES*sizeof(*to);c->table_owned=0;}
        p->samples=to;
    }
    for(i=0;i<c->count;++i) {
        struct workflow_entry *e=c->entries+i;
        struct pt_sample_version *next=direction>0?e->after:e->before;
        if(next)retain(next);
        release_version(s->current[e->slot]);s->current[e->slot]=next;
        if(next)p->samples[e->slot]=next->sample;
        else if(e->slot<c->snapshot.sample_count)workflow_empty(p->samples+e->slot);
    }
    p->sample_count=(uint16_t)(c->snapshot.sample_count+(c->append && direction>0));
    ++s->generation;c->initialized=1;return 1;
}
enum pt_edit_result pt_sampler_workflow_commit(struct pt_sampler_workflow **job,unsigned stopped,
    struct pt_sampler_workflow_stats *stats)
{
    struct pt_sampler_workflow *c;struct pt_edit_resource resource;enum pt_edit_result result;
    if(!job || !(c=*job) || !stats || c->initialized || !c->ready)return PT_EDIT_INVALID;
    if(!workflow_apart(c,job,sizeof(*job)) || !workflow_apart(c,stats,sizeof(*stats)) ||
       output_overlap(job,sizeof(*job),stats,sizeof(*stats)))return PT_EDIT_ALIAS;
    if(!stopped || !workflow_current(c))return PT_EDIT_CONFLICT;
    if(!workflow_history_room(c->project,c->history) || c->owner->generation==UINT_MAX)return PT_EDIT_CAPACITY;
    resource=(struct pt_edit_resource){c,workflow_apply,workflow_discard};
    result=pt_pattern_resource_apply(c->project,c->history,&resource);
    if(result!=PT_EDIT_OK)return result;
    *stats=c->stats;*job=NULL;return PT_EDIT_OK;
}
