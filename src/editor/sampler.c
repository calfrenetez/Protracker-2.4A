#include <string.h>
#include <stdio.h>
#include "sampler.h"
#include "wav.h"
struct pt_sample_version {
    struct pt_sample sample;
    struct pt_sampler *owner;
    size_t bytes;
    unsigned references;
};
struct sample_change {struct pt_sampler *owner;unsigned slot;struct pt_sample_version *before,*after;};
void pt_sampler_init(struct pt_sampler *s,const struct pt_allocator *a,size_t budget)
{memset(s,0,sizeof(*s));s->allocator=*a;s->budget=budget;}
static void retain(struct pt_sample_version *v) {++v->references;}
static void release_version(struct pt_sample_version *v)
{
    if(v && !--v->references) {
        struct pt_sampler *s=v->owner;s->bytes-=v->bytes;s->allocator.release(s->allocator.context,v);
    }
}
void pt_sampler_release(struct pt_sampler *s)
{
    unsigned i;for(i=0;i<PT_PROJECT_SAMPLES;++i) {release_version(s->current[i]);s->current[i]=NULL;}
}
static struct pt_sample_version *version(struct pt_sampler *s,const struct pt_sample *sample)
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
    if(values && sample->pcm.data)memcpy(v->sample.pcm.data,sample->pcm.data,values*sizeof(int32_t));
    if(slices)memcpy(v->sample.slices,sample->slices,slices);
    s->bytes+=bytes;return v;
}
static int same(const struct pt_sample *a,const struct pt_sample *b)
{
    return !memcmp(a->name,b->name,sizeof(a->name)) && a->pcm.frames==b->pcm.frames && a->pcm.rate==b->pcm.rate &&
        a->pcm.channels==b->pcm.channels && a->pcm.bits==b->pcm.bits && a->loop_start==b->loop_start &&
        a->loop_end==b->loop_end && a->crossfade==b->crossfade && a->slice_count==b->slice_count &&
        a->loop==b->loop && a->volume==b->volume && a->interpolation==b->interpolation && a->finetune==b->finetune &&
        (!a->pcm.frames || !memcmp(a->pcm.data,b->pcm.data,(size_t)a->pcm.frames*a->pcm.channels*sizeof(int32_t))) &&
        (!a->slice_count || !memcmp(a->slices,b->slices,(size_t)a->slice_count*sizeof(uint32_t)));
}
static int apply(void *context,struct pt_project *p,int direction)
{
    struct sample_change *c=context;struct pt_sampler *s=c->owner;size_t i,count;
    struct pt_sample_version *expected=direction>0?c->before:c->after,*replacement=direction>0?c->after:c->before;
    if(pt_project_validate(p,NULL)!=PT_PROJECT_OK || c->slot>=p->sample_count || !same(&p->samples[c->slot],&expected->sample))return 0;
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==c->slot+1 && p->events[i].slice>replacement->sample.slice_count)return 0;
    retain(replacement);release_version(s->current[c->slot]);s->current[c->slot]=replacement;
    p->samples[c->slot]=replacement->sample;++s->generation;return 1;
}
static void discard(void *context)
{
    struct sample_change *c=context;struct pt_sampler *s=c->owner;
    release_version(c->before);release_version(c->after);s->allocator.release(s->allocator.context,c);
}
static enum pt_edit_result commit(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,struct pt_sample_version *after)
{
    struct sample_change *c;struct pt_edit_resource resource;enum pt_edit_result result;
    if(same(&p->samples[slot],&after->sample)) {release_version(after);return PT_EDIT_OK;}
    c=s->allocator.allocate(s->allocator.context,sizeof(*c));
    if(!c) {release_version(after);return PT_EDIT_CAPACITY;}
    c->owner=s;c->slot=slot;c->after=after;c->before=s->current[slot];
    if(c->before)retain(c->before);else c->before=version(s,&p->samples[slot]);
    if(!c->before) {release_version(after);s->allocator.release(s->allocator.context,c);return PT_EDIT_CAPACITY;}
    resource.context=c;resource.apply=apply;resource.discard=discard;
    result=pt_pattern_resource_apply(p,h,&resource);if(result!=PT_EDIT_OK)discard(c);
    return result;
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
    struct pt_wav_info info;struct pt_sample sample;struct pt_sample_version *v;size_t i,count;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(pt_wav_inspect(bytes,length,&info)!=PT_WAV_OK || !info.frames)return PT_EDIT_UNSUPPORTED;
    /* A replacement cannot invalidate saved slice references in any pattern. */
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice)return PT_EDIT_UNSUPPORTED;
    memset(&sample,0,sizeof(sample));sample.volume=64;
    snprintf(sample.name,sizeof(sample.name),"%s",name?name:"IMPORTED WAV");
    sample.pcm.frames=info.frames;sample.pcm.rate=info.rate;sample.pcm.channels=info.channels;sample.pcm.bits=info.bits;
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    if(pt_wav_decode(bytes,length,&v->sample.pcm)!=PT_WAV_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit(s,p,h,slot,v);
}
