#include <string.h>
#include "paula_song.h"
#include "paula_internal.h"
#include "sampler_internal.h"
#include "project_snapshot.h"
#include "../core/render_lookahead.h"
struct pt_paula_song {
    struct pt_allocator allocator;struct pt_paula_voices *voices;
    struct pt_sampler_paula *bridge;struct pt_sampler *sampler;struct pt_project *project,snapshot;
    int (*quiesce)(void *);void *quiesce_context;
    struct pt_paula_voice_api api;struct pt_paula_render_caps caps;struct pt_render_options options;
    struct pt_render_lookahead ahead;unsigned forecast;
    struct pt_render_sequence *sequence;struct pt_render_plan plan;struct pt_paula_prepared batch;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_sampler_pin_job job;
    struct pt_paula_preflight_report report;struct pt_render_interval interval;
    int8_t map[PT_CHANNEL_LIMIT];uint64_t version;unsigned generation,slot,analyzed,ready,pending,done,closing;
    uint32_t remaining;enum pt_paula_song_result failure;
};
static enum pt_paula_song_result fail(struct pt_paula_song *s,enum pt_paula_song_result result)
{
    unsigned i;pt_render_lookahead_cancel(&s->ahead);pt_paula_cancel(&s->batch);s->failure=result;s->closing=1;s->voices->closing=1;
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    for(i=0;i<PT_PAULA_VOICES;++i)if(s->voices->voice[i].held)
        pt_paula_stop_owned(s->voices,(unsigned)s->voices->voice[i].track,s);
    return result;
}
static enum pt_paula_song_result current(struct pt_paula_song *s)
{
    unsigned i;struct pt_pcm pcm;struct pt_sample_version *pin;
    if(!s)return PT_PAULA_SONG_INVALID;
    if(s->failure)return s->failure;
    if(s->closing)return PT_PAULA_SONG_INVALID;
    s->snapshot.channels.selected=s->project->channels.selected;
    if(s->voices->song_owner!=s || s->voices->bridge!=s->bridge || s->voices->closing ||
       s->bridge->sampler!=s->sampler || s->bridge->project!=s->project || s->bridge->closing ||
       s->sampler->generation!=s->generation || s->bridge->generation!=s->generation ||
       s->bridge->version!=s->version || s->bridge->table!=s->project->samples ||
       s->bridge->count!=s->project->sample_count ||
       !pt_project_snapshot_equal(s->project,&s->snapshot) || memcmp(s->map,s->voices->map,sizeof(s->map)) ||
       s->api.context!=s->voices->api.context || s->api.start!=s->voices->api.start ||
       s->api.stop!=s->voices->api.stop || s->api.control!=s->voices->api.control ||
       s->quiesce!=s->voices->quiesce || s->quiesce_context!=s->voices->quiesce_context)
        return fail(s,PT_PAULA_SONG_STALE);
    for(i=0;i<s->project->sample_count;++i)if(s->pin[i]) {
        if(pt_sampler_pin_current(s->sampler,s->project,i,s->generation,s->pin[i],&pcm,&pin)!=PT_EDIT_OK)
            return fail(s,PT_PAULA_SONG_STALE);
        pt_sampler_unpin(pin);
    }
    return PT_PAULA_SONG_OK;
}
enum pt_paula_song_result pt_paula_song_begin(struct pt_paula_voices *v,
    const struct pt_render_options *o,const struct pt_paula_render_caps *caps,
    const struct pt_allocator *a,struct pt_paula_song **out)
{
    struct pt_paula_song *s;unsigned i;
    if(!v || !v->bridge || !o || !a || !a->allocate || !a->release || !out ||
       !pt_paula_render_caps_valid(caps) || v->song_owner || v->closing)return PT_PAULA_SONG_INVALID;
    for(i=0;i<PT_PAULA_VOICES;++i)if(v->voice[i].held)return PT_PAULA_SONG_INVALID;
    if(pt_paula_voices_sync(v)!=1)return PT_PAULA_SONG_INVALID;
    if(o->row_range)return PT_PAULA_SONG_CAPABILITY;
    for(i=0;i<v->bridge->project->channels.count;++i)if((o->tracks&(1U<<i)) && v->map[i]<0)
        return PT_PAULA_SONG_CAPABILITY;
    s=a->allocate(a->context,sizeof(*s));if(!s)return PT_PAULA_SONG_MEMORY;
    memset(s,0,sizeof(*s));s->allocator=*a;s->voices=v;s->bridge=v->bridge;
    s->sampler=s->bridge->sampler;s->project=s->bridge->project;s->snapshot=*s->project;
    s->quiesce=v->quiesce;s->quiesce_context=v->quiesce_context;
    s->options=*o;s->caps=*caps;s->api=v->api;s->generation=s->sampler->generation;
    s->version=s->bridge->version;memcpy(s->map,v->map,sizeof(s->map));v->song_owner=s;*out=s;
    return PT_PAULA_SONG_PREPARING;
}
enum pt_paula_song_result pt_paula_song_prepare(struct pt_paula_song *s,struct pt_paula_preflight_report *out)
{
    enum pt_paula_song_result state=current(s);enum pt_edit_result edit;unsigned ready;
    struct pt_pcm pcm;struct pt_sample_version *pin;
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->analyzed) {
        enum pt_paula_capability result=pt_paula_preflight_take(s->project,&s->options,s->map,&s->caps,
            s->api.control!=NULL,&s->allocator,&s->report,&s->sequence);
        if(out)*out=s->report;
        if(result!=PT_PAULA_COMPATIBLE)return fail(s,result==PT_PAULA_MEMORY?PT_PAULA_SONG_MEMORY:PT_PAULA_SONG_CAPABILITY);
        s->analyzed=1;return PT_PAULA_SONG_PREPARING;
    }
    if(out)*out=s->report;
    if(s->ready)return PT_PAULA_SONG_OK;
    if(s->job.owner) {
        edit=pt_sampler_pin_job_step(&s->job,PT_SAMPLER_PIN_CHUNK,&pcm,&pin,&ready);
        if(edit!=PT_EDIT_OK)return fail(s,edit==PT_EDIT_CAPACITY?PT_PAULA_SONG_MEMORY:PT_PAULA_SONG_STALE);
        if(ready){s->pin[s->slot]=pin;++s->slot;}
        return PT_PAULA_SONG_PREPARING;
    }
    while(s->slot<s->project->sample_count && !s->report.samples[s->slot])++s->slot;
    if(s->slot==s->project->sample_count){s->ready=1;return PT_PAULA_SONG_OK;}
    edit=pt_sampler_pin_job_begin(&s->job,s->sampler,s->project,s->slot,s->generation);
    if(edit!=PT_EDIT_OK)return fail(s,edit==PT_EDIT_CAPACITY?PT_PAULA_SONG_MEMORY:PT_PAULA_SONG_STALE);
    return PT_PAULA_SONG_PREPARING;
}
enum pt_paula_song_result pt_paula_song_next(struct pt_paula_song *s,struct pt_render_interval *out)
{
    enum pt_paula_song_result state=current(s);
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(!out || s->pending)return PT_PAULA_SONG_INVALID;
    if(s->done)return PT_PAULA_SONG_DONE;
    if(pt_render_sequence_next(s->sequence,&s->interval)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
    s->pending=1;s->remaining=s->interval.frames;*out=s->interval;return PT_PAULA_SONG_OK;
}
enum pt_paula_song_result pt_paula_song_consume(struct pt_paula_song *s,uint32_t frames)
{
    enum pt_paula_song_result state=current(s);
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(!s->pending || !frames || frames>256 || frames>s->remaining)return PT_PAULA_SONG_INVALID;
    if(pt_render_sequence_consume(s->sequence,frames)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
    s->remaining-=frames;return PT_PAULA_SONG_OK;
}
enum pt_paula_song_result pt_paula_song_prefetch(struct pt_paula_song *s)
{
    unsigned ready=0;enum pt_paula_song_result state=current(s);
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(!s->pending || (!s->forecast && (s->batch.preparing || s->batch.ready)))return PT_PAULA_SONG_INVALID;
    if(!s->forecast) {
        if(pt_render_lookahead_begin(&s->ahead,s->sequence)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
        s->forecast=1;return PT_PAULA_SONG_PREPARING;
    }
    if(s->forecast==1) {
        if(pt_render_lookahead_step(&s->ahead,256,&s->plan,&ready)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
        if(ready) {
            if(!pt_paula_prepare_begin_owned(&s->batch,s->voices,s->version,s->options.rate,&s->plan,&s->caps,s->pin,s))
                return fail(s,PT_PAULA_SONG_DEVICE);
            s->forecast=2;
        }
        return PT_PAULA_SONG_PREPARING;
    }
    if(s->forecast==3)return PT_PAULA_SONG_OK;
    switch(pt_paula_prepare_step_owned(&s->batch)) {
    case PT_CACHE_LOAD:s->forecast=3;return PT_PAULA_SONG_OK;
    case PT_CACHE_PENDING:return PT_PAULA_SONG_PREPARING;
    default:return fail(s,PT_PAULA_SONG_DEVICE);
    }
}
enum pt_paula_song_result pt_paula_song_stage(struct pt_paula_song *s)
{
    enum pt_paula_song_result state=current(s);
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(!s->pending || s->remaining)return PT_PAULA_SONG_INVALID;
    if(s->forecast)return pt_paula_song_prefetch(s);
    if(s->batch.ready)return PT_PAULA_SONG_OK;
    if(!s->batch.preparing) {
        if(pt_render_sequence_complete(s->sequence,&s->plan)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
        if(!pt_paula_prepare_begin_owned(&s->batch,s->voices,s->version,s->options.rate,&s->plan,&s->caps,s->pin,s))
            return fail(s,PT_PAULA_SONG_DEVICE);
        return PT_PAULA_SONG_PREPARING;
    }
    switch(pt_paula_prepare_step_owned(&s->batch)) {
    case PT_CACHE_LOAD:return PT_PAULA_SONG_OK;
    case PT_CACHE_PENDING:return PT_PAULA_SONG_PREPARING;
    default:return fail(s,PT_PAULA_SONG_DEVICE);
    }
}
enum pt_paula_song_result pt_paula_song_complete(struct pt_paula_song *s)
{
    enum pt_paula_song_result state=current(s);int result;
    if(state!=PT_PAULA_SONG_OK)return state;
    if(s->forecast) {
        if(s->remaining)return PT_PAULA_SONG_INVALID;
        if(s->forecast!=3)return PT_PAULA_SONG_PREPARING;
        if(pt_render_lookahead_commit(&s->ahead)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
        s->forecast=0;
    }
    if(s->batch.preparing)return PT_PAULA_SONG_PREPARING;
    /* Compatibility callers that never stage retain synchronous preparation.
     * Explicit incremental callers must finish stage before complete emits. */
    do{state=pt_paula_song_stage(s);}while(state==PT_PAULA_SONG_PREPARING && s->ready && s->pending && !s->remaining);
    if(state!=PT_PAULA_SONG_OK)return state;
    result=pt_paula_apply(&s->batch);
    if(result!=1) {
        /* Dispatch runtime failure already attempted each held stop once. */
        if(result<0){s->failure=PT_PAULA_SONG_DEVICE;s->closing=1;pt_render_sequence_close(s->sequence);s->sequence=NULL;return s->failure;}
        return fail(s,PT_PAULA_SONG_DEVICE);
    }
    s->pending=0;s->done=s->interval.end;return s->done?PT_PAULA_SONG_DONE:PT_PAULA_SONG_OK;
}
int pt_paula_song_close(struct pt_paula_song **out)
{
    struct pt_paula_song *s;struct pt_allocator a;unsigned i;
    if(!out)return 0;
    s=*out;if(!s)return 1;
    s->closing=1;pt_render_lookahead_cancel(&s->ahead);pt_paula_cancel(&s->batch);
    if(!pt_paula_close_owned(s->voices,s))return 0;
    pt_render_sequence_close(s->sequence);pt_sampler_pin_job_cancel(&s->job);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)pt_sampler_unpin(s->pin[i]);
    a=s->allocator;a.release(a.context,s);*out=NULL;return 1;
}
