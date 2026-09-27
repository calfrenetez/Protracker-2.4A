#include "sampler_song.h"
#include "sampler_internal.h"
#include <string.h>
struct pt_sampler_song {
    struct pt_allocator allocator;struct pt_sampler_studio provider;
    struct pt_studio_song *song;struct pt_studio_binding bindings[PT_PROJECT_SAMPLES];
    struct pt_project snapshot;struct pt_sampler_pin_job promotion;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];uint8_t required[PT_PROJECT_SAMPLES];
    unsigned generation,count,analyzed,ready,pin_slot;enum pt_render_result failure;
};
void pt_sampler_song_stop(struct pt_sampler_song *s)
{if(s) {unsigned i;pt_studio_song_close(s->song);s->song=NULL;s->ready=0;
    pt_sampler_pin_job_cancel(&s->promotion);
    for(i=0;i<PT_PROJECT_SAMPLES;++i) {pt_sampler_unpin(s->pin[i]);s->pin[i]=NULL;}
}}
void pt_sampler_song_close(struct pt_sampler_song *s)
{if(s) {struct pt_allocator a=s->allocator;pt_sampler_song_stop(s);a.release(a.context,s);}}
enum pt_render_result pt_sampler_song_begin(struct pt_sampler *sampler,struct pt_project *p,
    const struct pt_render_options *o,const struct pt_allocator *a,struct pt_sampler_song **out)
{
    struct pt_sampler_song *s;struct pt_studio_source source;enum pt_render_result result;unsigned i;
    if(!sampler || !p || !out || !a || !a->allocate || !a->release || p->sample_count>PT_PROJECT_SAMPLES ||
       (p->sample_count && !p->samples))return PT_RENDER_INVALID;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_RENDER_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->provider=(struct pt_sampler_studio){sampler,p};
    s->generation=sampler->generation;s->count=p->sample_count;memcpy(&s->snapshot,p,sizeof(*p));
    for(i=0;i<s->count;++i)s->bindings[i]=(struct pt_studio_binding){&p->samples[i].pcm,i+1,s->generation};
    source=pt_sampler_studio_source(&s->provider);
    result=pt_studio_song_begin(p,o,a,&source,s->bindings,s->count,&s->song);
    if(result!=PT_RENDER_OK) {pt_sampler_song_close(s);return result;}
    *out=s;return PT_RENDER_OK;
}
static enum pt_render_result fail(struct pt_sampler_song *s,enum pt_render_result result)
{s->failure=result;pt_sampler_song_stop(s);return result;}
static int current(struct pt_sampler_song *s)
{
    s->snapshot.channels.selected=s->provider.project->channels.selected;
    return s->generation==s->provider.sampler->generation &&
        !memcmp(s->provider.project,&s->snapshot,sizeof(s->snapshot));
}
enum pt_render_result pt_sampler_song_prepare(struct pt_sampler_song *s,unsigned *ready)
{
    enum pt_render_result result;
    if(!s || !ready)return PT_RENDER_INVALID;
    *ready=0;if(s->failure)return s->failure;
    if(!s->song)return PT_RENDER_INVALID;
    if(!current(s))return fail(s,PT_RENDER_INVALID);
    if(s->ready) {*ready=1;return PT_RENDER_OK;}
    if(!s->analyzed) {
        result=pt_studio_song_prepare(s->song,&s->analyzed);
        if(result!=PT_RENDER_OK)return fail(s,result);
        if(s->analyzed && !pt_studio_song_required(s->song,s->required))return fail(s,PT_RENDER_INVALID);
        return PT_RENDER_OK;
    }
    while(s->pin_slot<s->count) {
        unsigned slot=s->pin_slot;
        if(s->required[slot]) {
            struct pt_pcm pcm;unsigned complete=0;enum pt_edit_result edit;
            if(!s->promotion.value)
                edit=pt_sampler_pin_job_begin(&s->promotion,s->provider.sampler,s->provider.project,slot,s->generation);
            else edit=pt_sampler_pin_job_step(&s->promotion,PT_SAMPLER_PIN_CHUNK,&pcm,&s->pin[slot],&complete);
            if(edit!=PT_EDIT_OK)return fail(s,edit==PT_EDIT_CAPACITY?PT_RENDER_MEMORY:PT_RENDER_INVALID);
            if(complete)++s->pin_slot;
            return PT_RENDER_OK;
        }
        ++s->pin_slot;
    }
    s->ready=1;*ready=1;return PT_RENDER_OK;
}
enum pt_render_result pt_sampler_song_open(struct pt_sampler *sampler,struct pt_project *p,
    const struct pt_render_options *o,const struct pt_allocator *a,struct pt_sampler_song **out)
{
    struct pt_sampler_song *s=NULL;unsigned ready=0;enum pt_render_result result;
    if(!out)return PT_RENDER_INVALID;
    result=pt_sampler_song_begin(sampler,p,o,a,&s);
    while(result==PT_RENDER_OK && !ready)result=pt_sampler_song_prepare(s,&ready);
    if(result!=PT_RENDER_OK) {pt_sampler_song_close(s);return result;}
    *out=s;return PT_RENDER_OK;
}
enum pt_render_result pt_sampler_song_pull(struct pt_sampler_song *s,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    unsigned ready;
    if(!s || !pcm || !done || !frames || frames>256)return PT_RENDER_INVALID;
    *pcm=NULL;*done=1;
    if(s->failure)return s->failure;
    if(!s->song)return PT_RENDER_OK;
    if(!current(s))return fail(s,PT_RENDER_INVALID);
    if(!s->ready) {
        enum pt_render_result result=pt_sampler_song_prepare(s,&ready);
        *done=result!=PT_RENDER_OK;return result;
    }
    s->failure=pt_studio_song_pull(s->song,frames,pcm,done);
    if(s->failure || *done)pt_sampler_song_stop(s);
    return s->failure;
}
