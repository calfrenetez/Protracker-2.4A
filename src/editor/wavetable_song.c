#include "wavetable_song.h"
#include <string.h>
struct pt_wavetable_song {
    struct pt_allocator allocator;struct pt_wavetable_voices *voices;
    struct pt_render_sequence *sequence;struct pt_render_plan plan;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];
    struct pt_project snapshot;struct pt_sampler *sampler;struct pt_project *project;
    struct pt_playback_format format;struct pt_render_interval interval;
    uint64_t version;unsigned generation,rate,pending,closing,done;
    uint32_t remaining;enum pt_wavetable_song_result failure;uint8_t staging[256];
};
static void release_sources(struct pt_wavetable_song *s)
{
    unsigned i;
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(s->pin[i]);s->pin[i]=NULL;}
}
static int stop(struct pt_wavetable_song *s)
{
    s->closing=1;s->pending=0;s->remaining=0;
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    if(s->done)return 1;
    if(!pt_wavetable_voices_close(s->voices))return 0;
    release_sources(s);s->done=1;return 1;
}
int pt_wavetable_song_close(struct pt_wavetable_song **song)
{
    struct pt_wavetable_song *s;struct pt_allocator a;
    if(!song)return 0;
    s=*song;if(!s)return 1;
    if(!stop(s))return 0;
    a=s->allocator;a.release(a.context,s);*song=NULL;return 1;
}
static enum pt_wavetable_song_result fail(struct pt_wavetable_song *s,enum pt_wavetable_song_result result)
{s->failure=result;stop(s);return result;}
static enum pt_wavetable_song_result current(struct pt_wavetable_song *s)
{
    if(!s)return PT_WAVETABLE_SONG_INVALID;
    if(s->failure)return s->failure;
    if(s->closing)return s->done?PT_WAVETABLE_SONG_DONE:PT_WAVETABLE_SONG_STOPPING;
    if(s->sampler->generation!=s->generation || memcmp(s->project,&s->snapshot,sizeof(s->snapshot)) ||
       s->voices->song_owner!=s || !s->voices->bridge || !pt_sampler_wavetable_sync(s->voices->bridge) || s->voices->bridge->version!=s->version)
        return fail(s,PT_WAVETABLE_SONG_STALE);
    return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_open(struct pt_wavetable_voices *v,
    const struct pt_render_options *o,const struct pt_playback_format *f,const struct pt_allocator *a,
    struct pt_wavetable_preflight_report *report,struct pt_wavetable_song **out)
{
    struct pt_wavetable_song *s;unsigned i;enum pt_render_result rendered;
    enum pt_wavetable_song_result result=PT_WAVETABLE_SONG_MEMORY;
    if(!v || !v->bridge || v->closing || v->song_owner || !o || !f || !a || !a->allocate || !a->release || !report || !out)
        return PT_WAVETABLE_SONG_INVALID;
    if(o->row_range)return PT_WAVETABLE_SONG_RANGE;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(v->voice[i].held)return PT_WAVETABLE_SONG_INVALID;
    if(!pt_sampler_wavetable_sync(v->bridge))return PT_WAVETABLE_SONG_STALE;
    if(pt_wavetable_preflight(v->bridge->project,o,f,v->api.control!=NULL,a,report)!=PT_WAVETABLE_COMPATIBLE)
        return report->result==PT_WAVETABLE_MEMORY?PT_WAVETABLE_SONG_MEMORY:PT_WAVETABLE_SONG_CAPABILITY;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_WAVETABLE_SONG_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->voices=v;s->format=*f;s->rate=o->rate;
    s->sampler=v->bridge->sampler;s->project=v->bridge->project;s->generation=s->sampler->generation;
    for(i=0;i<s->project->sample_count;++i)if(report->samples[i]) {
        struct pt_pcm pcm;enum pt_edit_result edit=pt_sampler_pin(s->sampler,s->project,i,s->generation,&pcm,&s->pin[i]);
        if(edit!=PT_EDIT_OK){result=edit==PT_EDIT_CAPACITY?PT_WAVETABLE_SONG_MEMORY:PT_WAVETABLE_SONG_STALE;goto failed;}
    }
    rendered=pt_render_sequence_open(s->project,o,a,&s->sequence);
    if(rendered!=PT_RENDER_OK){result=rendered==PT_RENDER_MEMORY?PT_WAVETABLE_SONG_MEMORY:PT_WAVETABLE_SONG_RENDER;goto failed;}
    if(!pt_sampler_wavetable_sync(v->bridge)){result=PT_WAVETABLE_SONG_STALE;goto failed;}
    s->version=v->bridge->version;memcpy(&s->snapshot,s->project,sizeof(s->snapshot));v->song_owner=s;*out=s;return PT_WAVETABLE_SONG_OK;
failed:
    release_sources(s);a->release(a->context,s);return result;
}
enum pt_wavetable_song_result pt_wavetable_song_next(struct pt_wavetable_song *s,struct pt_render_interval *out)
{
    enum pt_wavetable_song_result result;
    if(!s || !out)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    if(s->pending)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_next(s->sequence,&s->interval)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    s->remaining=s->interval.frames;s->pending=1;*out=s->interval;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_consume(struct pt_wavetable_song *s,uint32_t frames)
{
    enum pt_wavetable_song_result result;
    if(!s || !frames || frames>256)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    if(!s->pending || frames>s->remaining)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_consume(s->sequence,frames)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    s->remaining-=frames;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_complete(struct pt_wavetable_song *s)
{
    enum pt_wavetable_song_result result=current(s);if(result)return result;
    if(!s->pending || s->remaining)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_complete(s->sequence,&s->plan)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    if(s->interval.end)return stop(s)?PT_WAVETABLE_SONG_DONE:PT_WAVETABLE_SONG_STOPPING;
    if(pt_wavetable_dispatch(s->voices,s->version,s->rate,&s->plan,&s->format,s->staging,sizeof(s->staging))!=1)
        return fail(s,PT_WAVETABLE_SONG_DEVICE);
    s->pending=0;return PT_WAVETABLE_SONG_OK;
}
