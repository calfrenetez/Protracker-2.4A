#include "wavetable_song.h"
#include "sampler_internal.h"
#include <string.h>
struct pt_wavetable_song {
    struct pt_allocator allocator;struct pt_wavetable_voices *voices;
    struct pt_wavetable_preflight *preflight;struct pt_wavetable_preflight_report report;
    struct pt_render_sequence *sequence;struct pt_render_plan plan;struct pt_render_snapshot resume;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_sampler_pin_job promotion;
    struct pt_project snapshot;struct pt_sampler *sampler;struct pt_project *project;
    struct pt_sampler_wavetable *bridge;struct pt_amigus_wavetable_cache *backend;
    struct pt_amigus_reservation *reservation;
    struct pt_playback_format format;struct pt_render_interval interval;
    uint64_t version;unsigned generation,rate,pending,closing,done,range,restored,ready,pin_slot;
    uint32_t remaining;enum pt_wavetable_song_result failure;uint8_t staging[256];
};
static void release_sources(struct pt_wavetable_song *s)
{
    unsigned i;
    pt_sampler_pin_job_cancel(&s->promotion);
    pt_wavetable_preflight_close(&s->preflight);
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(s->pin[i]);s->pin[i]=NULL;}
}
static int stop(struct pt_wavetable_song *s)
{
    s->closing=1;s->pending=0;s->remaining=0;
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    if(s->done)return 1;
    if(!s->ready) {
        release_sources(s);
        if(s->voices->song_owner==s)s->voices->song_owner=NULL;
        s->done=1;return 1;
    }
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
    /* Channel selection is a UI cursor, not a playback setting. */
    s->snapshot.channels.selected=s->project->channels.selected;
    if(s->sampler->generation!=s->generation || memcmp(s->project,&s->snapshot,sizeof(s->snapshot)) ||
       s->voices->song_owner!=s || s->voices->bridge!=s->bridge ||
       s->bridge->sampler!=s->sampler || s->bridge->project!=s->project ||
       s->bridge->table!=s->project->samples || s->bridge->count!=s->project->sample_count ||
       s->bridge->generation!=s->generation || s->bridge->version!=s->version ||
       s->bridge->backend!=s->backend || s->backend->reservation!=s->reservation ||
       !pt_amigus_wavetable_cache_current(s->backend))
        return fail(s,PT_WAVETABLE_SONG_STALE);
    return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_begin(struct pt_wavetable_voices *v,
    const struct pt_render_options *o,const struct pt_playback_format *f,const struct pt_allocator *a,
    struct pt_wavetable_preflight_report *report,struct pt_wavetable_song **out)
{
    struct pt_wavetable_song *s;struct pt_wavetable_preflight *work=NULL;unsigned i;
    enum pt_wavetable_capability capability;
    if(!v || !v->bridge || v->closing || v->song_owner || !o || !f || !a || !a->allocate || !a->release || !report || !out)
        return PT_WAVETABLE_SONG_INVALID;
    if(o->row_range && !v->api.restore)return PT_WAVETABLE_SONG_RANGE;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(v->voice[i].held)return PT_WAVETABLE_SONG_INVALID;
    if(!pt_sampler_wavetable_sync(v->bridge))return PT_WAVETABLE_SONG_STALE;
    capability=pt_wavetable_preflight_begin(v->bridge->project,o,f,v->api.control!=NULL,1,v->api.restore!=NULL,a,report,&work);
    if(capability!=PT_WAVETABLE_PENDING)return capability==PT_WAVETABLE_MEMORY?PT_WAVETABLE_SONG_MEMORY:PT_WAVETABLE_SONG_CAPABILITY;
    s=a->allocate(a->context,sizeof(*s));
    if(!s){pt_wavetable_preflight_close(&work);return PT_WAVETABLE_SONG_MEMORY;}
    memset(s,0,sizeof(*s));s->allocator=*a;s->voices=v;s->format=*f;s->rate=o->rate;s->range=o->row_range;
    s->preflight=work;s->report=*report;
    s->sampler=v->bridge->sampler;s->project=v->bridge->project;s->generation=s->sampler->generation;
    s->bridge=v->bridge;s->backend=s->bridge->backend;s->reservation=s->backend->reservation;
    s->version=v->bridge->version;memcpy(&s->snapshot,s->project,sizeof(s->snapshot));
    v->song_owner=s;*out=s;return PT_WAVETABLE_SONG_PREPARING;
}
enum pt_wavetable_song_result pt_wavetable_song_prepare(struct pt_wavetable_song *s,
    struct pt_wavetable_preflight_report *report)
{
    enum pt_wavetable_song_result result;enum pt_wavetable_capability capability;
    if(!s || !report)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    *report=s->report;if(s->ready)return PT_WAVETABLE_SONG_OK;
    if(s->report.result==PT_WAVETABLE_PENDING) {
        capability=pt_wavetable_preflight_step(s->preflight,&s->report);*report=s->report;
        if(capability!=PT_WAVETABLE_PENDING && capability!=PT_WAVETABLE_COMPATIBLE)
            return fail(s,capability==PT_WAVETABLE_MEMORY?PT_WAVETABLE_SONG_MEMORY:PT_WAVETABLE_SONG_CAPABILITY);
        return PT_WAVETABLE_SONG_PREPARING;
    }
    while(s->pin_slot<s->project->sample_count) {
        unsigned slot=s->pin_slot;
        if(s->report.samples[slot]) {
            struct pt_pcm pcm;unsigned ready=0;enum pt_edit_result edit;
            if(!s->promotion.value)
                edit=pt_sampler_pin_job_begin(&s->promotion,s->sampler,s->project,slot,s->generation);
            else edit=pt_sampler_pin_job_step(&s->promotion,PT_SAMPLER_PIN_CHUNK,&pcm,&s->pin[slot],&ready);
            if(edit!=PT_EDIT_OK)return fail(s,edit==PT_EDIT_CAPACITY?PT_WAVETABLE_SONG_MEMORY:PT_WAVETABLE_SONG_STALE);
            if(ready)++s->pin_slot;
            return PT_WAVETABLE_SONG_PREPARING; /* At most one allocation or 4 KiB copy. */
        }
        ++s->pin_slot;
    }
    if(!pt_wavetable_preflight_take(s->preflight,&s->sequence))return fail(s,PT_WAVETABLE_SONG_RENDER);
    pt_wavetable_preflight_close(&s->preflight);s->ready=1;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_open(struct pt_wavetable_voices *v,
    const struct pt_render_options *o,const struct pt_playback_format *f,const struct pt_allocator *a,
    struct pt_wavetable_preflight_report *report,struct pt_wavetable_song **out)
{
    struct pt_wavetable_song *s=NULL;enum pt_wavetable_song_result result;
    if(!out)return PT_WAVETABLE_SONG_INVALID;
    result=pt_wavetable_song_begin(v,o,f,a,report,&s);
    while(result==PT_WAVETABLE_SONG_PREPARING)result=pt_wavetable_song_prepare(s,report);
    if(result!=PT_WAVETABLE_SONG_OK){pt_wavetable_song_close(&s);return result;}
    *out=s;return result;
}
enum pt_wavetable_song_result pt_wavetable_song_next(struct pt_wavetable_song *s,struct pt_render_interval *out)
{
    enum pt_wavetable_song_result result;
    if(!s || !out)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(s->pending)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_next(s->sequence,&s->interval)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    if(s->range && s->interval.emit && !s->restored) {
        if(pt_render_sequence_snapshot(s->sequence,&s->resume)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
        if(pt_wavetable_restore_dispatch(s->voices,s->version,s->rate,&s->resume,&s->format,s->staging,sizeof(s->staging))!=1)
            return fail(s,PT_WAVETABLE_SONG_DEVICE);
        s->restored=1;
    }
    s->remaining=s->interval.frames;s->pending=1;*out=s->interval;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_consume(struct pt_wavetable_song *s,uint32_t frames)
{
    enum pt_wavetable_song_result result;
    if(!s || !frames || frames>256)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(!s->pending || frames>s->remaining)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_consume(s->sequence,frames)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    s->remaining-=frames;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_complete(struct pt_wavetable_song *s)
{
    enum pt_wavetable_song_result result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(!s->pending || s->remaining)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_complete(s->sequence,&s->plan)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    if(s->interval.end)return stop(s)?PT_WAVETABLE_SONG_DONE:PT_WAVETABLE_SONG_STOPPING;
    if((!s->range || s->restored) && pt_wavetable_dispatch(s->voices,s->version,s->rate,&s->plan,&s->format,s->staging,sizeof(s->staging))!=1)
        return fail(s,PT_WAVETABLE_SONG_DEVICE);
    s->pending=0;return PT_WAVETABLE_SONG_OK;
}
