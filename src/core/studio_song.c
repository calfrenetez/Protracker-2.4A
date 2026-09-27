#include "studio_internal.h"
#include <string.h>
struct pt_studio_song {
    struct pt_allocator allocator;struct pt_render_sequence *sequence;
    struct pt_studio_mix *mix;struct pt_studio_binding bindings[255];
    struct pt_render_plan plan;struct pt_render_interval interval;
    struct pt_pcm block;int32_t samples[512];
    unsigned voices,count,pending,done,measured,ready;uint32_t remaining;
    uint8_t required[255];
    enum pt_render_result failure;
};
void pt_studio_song_stop(struct pt_studio_song *s)
{
    if(!s)return;
    pt_studio_close(s->mix);s->mix=NULL;
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    s->done=1;s->pending=0;s->remaining=0;s->ready=0;
}
void pt_studio_song_close(struct pt_studio_song *s)
{if(s) {struct pt_allocator a=s->allocator;pt_studio_song_stop(s);a.release(a.context,s);}}
enum pt_render_result pt_studio_song_begin(const struct pt_project *p,const struct pt_render_options *o,
    const struct pt_allocator *a,const struct pt_studio_source *source,const struct pt_studio_binding *bindings,unsigned count,
    struct pt_studio_song **out)
{
    struct pt_studio_song *s;unsigned i;enum pt_render_result result;
    if(!out || !p || !o || o->rate!=48000 || o->bits!=24 || !a || !a->allocate || !a->release ||
       !source || !source->acquire || !source->release || count>255 || count!=p->sample_count ||
       (count && (!bindings || !p->samples)))return PT_RENDER_INVALID;
    for(i=0;i<count;++i)if(bindings[i].pcm!=&p->samples[i].pcm)return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->count=count;s->voices=p->channels.count;
    if(count)memcpy(s->bindings,bindings,count*sizeof(*bindings));
    result=pt_render_sequence_begin(p,o,a,&s->sequence);
    if(result!=PT_RENDER_OK) {pt_studio_song_close(s);return result;}
    s->mix=pt_studio_open(a,source,s->voices);
    if(!s->mix) {pt_studio_song_close(s);return PT_RENDER_MEMORY;}
    s->block=(struct pt_pcm){s->samples,512,0,48000,2,24};*out=s;return PT_RENDER_OK;
}
enum pt_render_result pt_studio_song_prepare(struct pt_studio_song *s,unsigned *ready)
{
    enum pt_render_result result;unsigned i,j;
    if(!s || !ready)return PT_RENDER_INVALID;
    *ready=0;
    if(s->failure)return s->failure;
    if(s->done)return PT_RENDER_INVALID;
    if(s->ready) {*ready=1;return PT_RENDER_OK;}
    if(!s->measured) {
        result=pt_render_sequence_prepare(s->sequence,256,&s->measured);
        if(result!=PT_RENDER_OK)goto fail;
        return PT_RENDER_OK;
    }
    if(!s->pending) {
        result=pt_render_sequence_next(s->sequence,&s->interval);if(result!=PT_RENDER_OK)goto fail;
        s->pending=1;s->remaining=s->interval.frames;return PT_RENDER_OK;
    }
    if(s->remaining) {
        unsigned frames=s->remaining<256?s->remaining:256;
        result=pt_render_sequence_consume(s->sequence,frames);if(result!=PT_RENDER_OK)goto fail;
        s->remaining-=frames;return PT_RENDER_OK;
    }
    result=pt_render_sequence_complete(s->sequence,&s->plan);if(result!=PT_RENDER_OK)goto fail;
    if(s->interval.end) {
        result=pt_render_sequence_rewind(s->sequence);if(result!=PT_RENDER_OK)goto fail;
        s->pending=0;s->ready=1;*ready=1;return PT_RENDER_OK;
    }
    for(i=0;i<s->plan.count;++i) {
        const struct pt_render_action *a=s->plan.action+i;const struct pt_pcm *pcm;
        if(a->kind!=PT_RENDER_TRIGGER && a->kind!=PT_RENDER_SEGMENT && a->kind!=PT_RENDER_REPEAT)continue;
        pcm=a->kind==PT_RENDER_REPEAT?a->voice.repeat_pcm:a->voice.pcm;
        for(j=0;j<s->count;++j)if(s->bindings[j].pcm==pcm)break;
        if(j==s->count) {result=PT_RENDER_SAMPLE;goto fail;}
        s->required[j]=1;
    }
    s->pending=0;return PT_RENDER_OK;
fail:
    s->failure=result;pt_studio_song_stop(s);return result;
}
int pt_studio_song_required(const struct pt_studio_song *s,uint8_t samples[255])
{
    if(!s || !samples || !s->ready || s->done || s->failure)return 0;
    memcpy(samples,s->required,sizeof(s->required));return 1;
}
enum pt_render_result pt_studio_song_open(const struct pt_project *p,const struct pt_render_options *o,
    const struct pt_allocator *a,const struct pt_studio_source *source,const struct pt_studio_binding *bindings,unsigned count,
    struct pt_studio_song **out)
{
    struct pt_studio_song *s=NULL;unsigned ready=0;enum pt_render_result result;
    if(!out)return PT_RENDER_INVALID;
    result=pt_studio_song_begin(p,o,a,source,bindings,count,&s);
    while(result==PT_RENDER_OK && !ready)result=pt_studio_song_prepare(s,&ready);
    if(result!=PT_RENDER_OK) {pt_studio_song_close(s);return result;}
    *out=s;return PT_RENDER_OK;
}
static enum pt_render_result pull(struct pt_studio_song *s,unsigned max_frames,
    const struct pt_pcm **pcm,unsigned *done,unsigned prepared)
{
    enum pt_render_result result;uint64_t clipped;
    if(!s || !pcm || !done || !max_frames || max_frames>256)return PT_RENDER_INVALID;
    *pcm=NULL;*done=s->done;
    if(s->failure)return s->failure;
    if(s->done)return PT_RENDER_OK;
    if(!s->ready)return PT_RENDER_INVALID;
    if(!s->pending) {
        result=pt_render_sequence_next(s->sequence,&s->interval);if(result!=PT_RENDER_OK)goto fail;
        s->pending=1;s->remaining=s->interval.frames;return PT_RENDER_OK;
    }
    if(s->remaining) {
        s->block.frames=s->remaining<max_frames?s->remaining:max_frames;
        if(pt_studio_read(s->mix,&s->block,&clipped)!=PT_PCM_OK) {result=PT_RENDER_SAMPLE;goto fail;}
        result=pt_render_sequence_consume(s->sequence,s->block.frames);if(result!=PT_RENDER_OK)goto fail;
        s->remaining-=s->block.frames;if(s->interval.emit)*pcm=&s->block;return PT_RENDER_OK;
    }
    result=pt_render_sequence_complete(s->sequence,&s->plan);if(result!=PT_RENDER_OK)goto fail;
    if(s->interval.end) {pt_studio_song_stop(s);*done=1;return PT_RENDER_OK;}
    if((prepared?pt_studio_dispatch_prepared(s->mix,s->voices,&s->plan,s->bindings,s->count):
        pt_studio_dispatch(s->mix,s->voices,&s->plan,s->bindings,s->count))!=PT_PCM_OK) {result=PT_RENDER_SAMPLE;goto fail;}
    s->pending=0;return PT_RENDER_OK;
fail:
    s->failure=result;pt_studio_song_stop(s);*done=1;return result;
}
enum pt_render_result pt_studio_song_pull(struct pt_studio_song *s,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{return pull(s,frames,pcm,done,0);}
enum pt_render_result pt_studio_song_pull_prepared(struct pt_studio_song *s,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{return pull(s,frames,pcm,done,1);}
