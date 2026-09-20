#include <string.h>
#include <stdio.h>
#include "sampler.h"
#include "wav.h"
#include "svx.h"
struct pt_sample_version {
    struct pt_sample sample;
    struct pt_sampler *owner;
    struct pt_sample_version *backing; /* Flat owner of shared PCM/markers, or NULL. */
    size_t bytes;
    unsigned references;
};
struct sample_change {struct pt_sampler *owner;unsigned slot,resampled;struct pt_sample_version *before,*after;};
void pt_sampler_init(struct pt_sampler *s,const struct pt_allocator *a,size_t budget)
{memset(s,0,sizeof(*s));s->allocator=*a;s->budget=budget;}
static void retain(struct pt_sample_version *v) {++v->references;}
static void release_version(struct pt_sample_version *v)
{
    if(v && !--v->references) {
        struct pt_sampler *s=v->owner;struct pt_sample_version *backing=v->backing;
        s->bytes-=v->bytes;s->allocator.release(s->allocator.context,v);release_version(backing);
    }
}
void pt_sampler_release(struct pt_sampler *s)
{
    unsigned i;for(i=0;i<PT_PROJECT_SAMPLES;++i) {release_version(s->current[i]);s->current[i]=NULL;}
    if(s->table) {s->bytes-=s->table_bytes;s->allocator.release(s->allocator.context,s->table);s->table=NULL;s->table_original=NULL;s->table_bytes=0;}
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
        (!a->pcm.frames || a->pcm.data==b->pcm.data || !memcmp(a->pcm.data,b->pcm.data,(size_t)a->pcm.frames*a->pcm.channels*sizeof(int32_t))) &&
        (!a->slice_count || a->slices==b->slices || !memcmp(a->slices,b->slices,(size_t)a->slice_count*sizeof(uint32_t)));
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
enum pt_edit_result pt_sampler_append_generated(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_pcm *format,const char *name,pt_sample_fill fill,void *context)
{
    struct appended_sample *c;struct pt_sample sample;struct pt_sample *table;
    struct pt_pcm target;struct pt_edit_resource resource;enum pt_edit_result result;
    size_t table_bytes,needed,length=0;
    if(!s || !s->allocator.allocate || !s->allocator.release || !h || !format || !name || !fill ||
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
    sample.volume=64;memcpy(sample.name,name,length);c->value=version(s,&sample);
    result=PT_EDIT_CAPACITY;if(!c->value)goto fail;
    c->before=p->samples;c->after=table;c->count=p->sample_count;
    target=c->value->sample.pcm;
    result=fill(context,&target);if(result!=PT_EDIT_OK)goto fail;
    if(target.data!=c->value->sample.pcm.data || target.capacity!=c->value->sample.pcm.capacity ||
       target.frames!=format->frames || target.rate!=format->rate || target.channels!=format->channels || target.bits!=format->bits ||
       pt_pcm_validate(&c->value->sample.pcm)!=PT_PCM_OK) {result=PT_EDIT_INVALID;goto fail;}
    resource=(struct pt_edit_resource){c,append_apply,append_discard};
    result=pt_pattern_resource_apply(p,h,&resource);if(result==PT_EDIT_OK)return result;
fail:
    append_discard(c);
    if(table_bytes) {s->bytes-=table_bytes;s->allocator.release(s->allocator.context,table);}
    return result;
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
    release_version(c->before);release_version(c->after);s->allocator.release(s->allocator.context,c);
}
static enum pt_edit_result commit_kind(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,struct pt_sample_version *after,unsigned resampled)
{
    struct sample_change *c;struct pt_edit_resource resource;enum pt_edit_result result;
    if(same(&p->samples[slot],&after->sample)) {release_version(after);return PT_EDIT_OK;}
    c=s->allocator.allocate(s->allocator.context,sizeof(*c));
    if(!c) {release_version(after);return PT_EDIT_CAPACITY;}
    c->owner=s;c->slot=slot;c->resampled=resampled;c->after=after;c->before=s->current[slot];
    if(c->before)retain(c->before);
    else if(after->backing && same(&p->samples[slot],&after->backing->sample)) {c->before=after->backing;retain(c->before);}
    else c->before=version(s,&p->samples[slot]);
    if(!c->before) {release_version(after);s->allocator.release(s->allocator.context,c);return PT_EDIT_CAPACITY;}
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

enum pt_svx_result pt_sampler_svx_size(const struct pt_sample *sample,size_t *size)
{
    struct pt_svx_info info={0};
    if(!sample || !sample->pcm.frames)return PT_SVX_INVALID;
    if(sample->loop>PT_LOOP_FORWARD || sample->slice_count || sample->finetune)return PT_SVX_UNSUPPORTED;
    info.loop_start=sample->loop_start;info.loop_end=sample->loop_end;info.volume=(uint32_t)sample->volume*1024;
    return pt_svx_size(&sample->pcm,&info,size);
}
enum pt_svx_result pt_sampler_svx_encode(const struct pt_sample *sample,uint8_t *bytes,size_t capacity,size_t *written)
{
    size_t size;struct pt_svx_info info={0};enum pt_svx_result result=pt_sampler_svx_size(sample,&size);
    if(result!=PT_SVX_OK)return result;
    memcpy(info.name,sample->name,sizeof(info.name));info.loop_start=sample->loop_start;
    info.loop_end=sample->loop_end;info.volume=(uint32_t)sample->volume*1024;
    return pt_svx_encode(&sample->pcm,&info,bytes,capacity,written);
}

enum pt_edit_result pt_sampler_import_raw(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,unsigned slot,const uint8_t *bytes,size_t length,const char *name,const struct pt_raw_format *format)
{
    struct pt_sample sample={0};struct pt_sample_version *v;size_t i,count;uint32_t frames;
    if(!s || !s->allocator.allocate || !s->allocator.release || pt_project_validate(p,NULL)!=PT_PROJECT_OK || slot>=p->sample_count)return PT_EDIT_INVALID;
    if(!bytes || pt_raw_frames(length,format,&frames)!=PT_RAW_OK || !frames)return PT_EDIT_UNSUPPORTED;
    count=(size_t)p->pattern_count*64*p->channels.count;
    for(i=0;i<count;++i)if(p->events[i].instrument==slot+1 && p->events[i].slice)return PT_EDIT_UNSUPPORTED;
    sample.volume=64;snprintf(sample.name,sizeof(sample.name),"%s",name?name:"IMPORTED RAW");
    sample.pcm.frames=frames;sample.pcm.rate=format->rate;sample.pcm.bits=format->bits;sample.pcm.channels=format->channels;
    v=version(s,&sample);if(!v)return PT_EDIT_CAPACITY;
    if(pt_raw_decode(bytes,length,format,&v->sample.pcm)!=PT_RAW_OK) {release_version(v);return PT_EDIT_INVALID;}
    return commit(s,p,h,slot,v);
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
