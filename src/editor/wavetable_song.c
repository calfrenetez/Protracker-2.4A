#include "wavetable_song.h"
#include "sampler_internal.h"
#include "wavetable_internal.h"
#include "sampler_wavetable_internal.h"
#include "../core/render_lookahead.h"
#include <string.h>
struct pt_wavetable_song {
    struct pt_allocator allocator;struct pt_wavetable_voices *voices;
    struct pt_wavetable_prepared sources;
    struct pt_sampler_upload_job upload;struct pt_render_lookahead ahead;unsigned forecast;
    struct pt_cache_lease lease[PT_RENDER_ACTIONS];
    unsigned slot[PT_RENDER_ACTIONS],held[PT_RENDER_ACTIONS],batch_count,batch_at,uploading;
    struct pt_wavetable_preflight *preflight;struct pt_wavetable_preflight_report report;
    struct pt_render_sequence *sequence;struct pt_render_plan plan;struct pt_render_snapshot resume;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_sampler_pin_job promotion;
    struct pt_project snapshot;struct pt_sampler *sampler;struct pt_project *project;
    struct pt_sampler_wavetable *bridge;struct pt_amigus_wavetable_cache *backend;
    struct pt_amigus_reservation *reservation;
    struct pt_playback_format format;struct pt_render_interval interval;
    uint64_t version;unsigned generation,rate,pending,closing,done,range,restored,ready,pin_slot;
    uint64_t clock_start,clock_last,clock_deadline;unsigned clock_armed;
    uint32_t remaining;enum pt_wavetable_song_result failure;uint8_t staging[256];
};
static void cancel_batch(struct pt_wavetable_song *s)
{
    unsigned i;pt_sampler_upload_cancel(&s->upload);pt_render_lookahead_cancel(&s->ahead);s->forecast=0;
    for(i=0;i<s->batch_count;++i)if(s->held[i]) {
        pt_cache_unpin(&s->backend->cache,s->lease[i]);s->held[i]=0;
    }
    s->batch_count=s->batch_at=s->uploading=0;
}
static void release_sources(struct pt_wavetable_song *s)
{
    unsigned i;
    cancel_batch(s);pt_sampler_pin_job_cancel(&s->promotion);
    pt_wavetable_preflight_close(&s->preflight);
    pt_render_sequence_close(s->sequence);s->sequence=NULL;
    for(i=0;i<PT_PROJECT_SAMPLES;++i){pt_sampler_unpin(s->pin[i]);s->pin[i]=NULL;}
}
static int stop(struct pt_wavetable_song *s)
{
    s->closing=1;s->clock_armed=0;s->pending=0;s->remaining=0;cancel_batch(s);
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
static int source_current(void *context)
{
    struct pt_wavetable_song *s=context;
    if(!s || s->failure || s->closing)return 0;
    /* Channel selection is a UI cursor, not a playback setting. */
    s->snapshot.channels.selected=s->project->channels.selected;
    if(s->sampler->generation!=s->generation || memcmp(s->project,&s->snapshot,sizeof(s->snapshot)) ||
       s->voices->song_owner!=s || s->voices->bridge!=s->bridge ||
       s->bridge->sampler!=s->sampler || s->bridge->project!=s->project ||
       s->bridge->table!=s->project->samples || s->bridge->count!=s->project->sample_count ||
       s->bridge->generation!=s->generation || s->bridge->version!=s->version ||
       s->bridge->backend!=s->backend || s->backend->reservation!=s->reservation ||
       !pt_amigus_wavetable_cache_current(s->backend))
        return 0;
    return 1;
}
static enum pt_wavetable_song_result current(struct pt_wavetable_song *s)
{
    if(!s)return PT_WAVETABLE_SONG_INVALID;
    if(s->failure)return s->failure;
    if(s->closing)return s->done?PT_WAVETABLE_SONG_DONE:PT_WAVETABLE_SONG_STOPPING;
    return source_current(s)?PT_WAVETABLE_SONG_OK:fail(s,PT_WAVETABLE_SONG_STALE);
}
static enum pt_cache_result source_acquire(void *context,unsigned slot,const struct pt_playback_format *f,
    uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{
    struct pt_wavetable_song *s=context;struct pt_pcm pcm;struct pt_sample_version *pin;unsigned i;
    (void)f;(void)staging;(void)capacity;
    /* Commit may only transfer a lease already acquired by this exact batch.
       No new cache acquisition or writes are allowed inside device dispatch. */
    if(!out || !source_current(s) || !s->ready || !s->uploading || s->batch_at!=s->batch_count ||
       slot>=s->project->sample_count ||
       pt_sampler_pin_current(s->sampler,s->project,slot,s->generation,s->pin[slot],&pcm,&pin)!=PT_EDIT_OK)return PT_CACHE_INVALID;
    pt_sampler_unpin(pin);
    for(i=0;i<s->batch_count;++i)if(s->held[i] && s->slot[i]==slot) {
        *out=s->lease[i];s->held[i]=0;return PT_CACHE_HIT;
    }
    return PT_CACHE_INVALID;
}
static int source_location(void *context,struct pt_cache_lease lease,uint32_t *address,uint32_t *bytes)
{
    struct pt_wavetable_song *s=context;struct pt_sample_cache *cache;
    if(!source_current(s) || !s->ready)return 0;
    cache=&s->backend->cache;
    if(!pt_cache_data(cache,lease) || cache->entry[lease.slot].valid!=1 ||
       cache->entry[lease.slot].version!=s->version)return 0;
    return pt_amigus_wavetable_cache_location(s->backend,lease,address,bytes);
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
    s->sources=(struct pt_wavetable_prepared){s,source_current,source_acquire,source_location};
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
/* Only the immutable audited sequence supplies these descriptors. Capture slots
   for this batch, never all project samples. Duplicate triggers own separate
   cache pins, so consuming one cannot evict another pending voice's resource. */
static int batch_begin(struct pt_wavetable_song *s,unsigned restore)
{
    unsigned i,slot,n=restore?s->resume.channels:s->plan.count;
    if(n>PT_RENDER_ACTIONS || (restore && n>16))return 0;
    s->uploading=restore?1:2;
    for(i=0;i<n;++i) {
        const struct pt_pcm *pcm;
        if(restore){if(!s->resume.voice[i].active)continue;pcm=s->resume.voice[i].pcm;}
        else {if(s->plan.action[i].kind!=PT_RENDER_TRIGGER)continue;pcm=s->plan.action[i].voice.pcm;}
        for(slot=0;slot<s->project->sample_count;++slot)if(pcm==&s->project->samples[slot].pcm)break;
        if(slot==s->project->sample_count)return 0;
        s->slot[s->batch_count++]=slot;
    }
    return 1;
}
/* One allocation/cache hit OR <=256-byte upload per call. READY is returned on
   a later call after the last acquisition: no device callback shares an upload
   step. Recheck EVERY prepared descriptor/address before the first callback. */
static int batch_step(struct pt_wavetable_song *s)
{
    unsigned i=s->batch_at;enum pt_cache_result r;
    if(i<s->batch_count) {
        if(s->upload.bridge)r=pt_sampler_upload_step(&s->upload,s->staging,sizeof(s->staging),&s->lease[i]);
        else r=pt_sampler_upload_begin_prepared(&s->upload,s->bridge,s->slot[i],s->generation,s->version,
            s->pin[s->slot[i]],&s->format,&s->lease[i]);
        if(r==PT_CACHE_LOAD || r==PT_CACHE_HIT){s->held[i]=1;++s->batch_at;}
        else if(r!=PT_CACHE_PENDING)return -1;
        return 0;
    }
    for(i=0;i<s->batch_count;++i) {
        struct pt_pcm pcm;struct pt_sample_version *pin;uint32_t address,bytes;
        if(pt_sampler_pin_current(s->sampler,s->project,s->slot[i],s->generation,s->pin[s->slot[i]],&pcm,&pin)!=PT_EDIT_OK)return -1;
        pt_sampler_unpin(pin);
        if(!s->held[i] || !source_location(s,s->lease[i],&address,&bytes))return -1;
    }
    return 1;
}
enum pt_wavetable_song_result pt_wavetable_song_prefetch(struct pt_wavetable_song *s)
{
    unsigned ready=0;int batch;enum pt_wavetable_song_result result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(!s->pending || (s->range && !s->restored) || (s->uploading && !s->forecast))return PT_WAVETABLE_SONG_INVALID;
    if(!s->forecast) {
        if(pt_render_lookahead_begin(&s->ahead,s->sequence)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
        s->forecast=1;return PT_WAVETABLE_SONG_UPLOADING;
    }
    if(s->forecast==1) {
        if(pt_render_lookahead_step(&s->ahead,256,&s->plan,&ready)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
        if(ready){if(!batch_begin(s,0))return fail(s,PT_WAVETABLE_SONG_DEVICE);s->forecast=2;}
        return PT_WAVETABLE_SONG_UPLOADING;
    }
    batch=batch_step(s);
    if(batch<0)return fail(s,PT_WAVETABLE_SONG_DEVICE);
    if(!batch)return PT_WAVETABLE_SONG_UPLOADING;
    s->forecast=3;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_next_step(struct pt_wavetable_song *s,struct pt_render_interval *out)
{
    enum pt_wavetable_song_result result;int batch;
    if(!s || !out || s->clock_armed)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(s->uploading==2)return PT_WAVETABLE_SONG_UPLOADING;
    if(s->uploading==1) {
        batch=batch_step(s);
        if(batch<0)return fail(s,PT_WAVETABLE_SONG_DEVICE);
        if(!batch)return PT_WAVETABLE_SONG_UPLOADING;
        if(pt_wavetable_restore_prepared(s->voices,s->version,s->rate,&s->resume,&s->format,s->staging,sizeof(s->staging),&s->sources)!=1)
            return fail(s,PT_WAVETABLE_SONG_DEVICE);
        cancel_batch(s);s->restored=1;
    }else {
        if(s->pending)return PT_WAVETABLE_SONG_INVALID;
        if(pt_render_sequence_next(s->sequence,&s->interval)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
        if(s->range && s->interval.emit && !s->restored) {
            if(pt_render_sequence_snapshot(s->sequence,&s->resume)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
            if(!batch_begin(s,1))return fail(s,PT_WAVETABLE_SONG_DEVICE);
            return PT_WAVETABLE_SONG_UPLOADING;
        }
    }
    s->remaining=s->interval.frames;s->pending=1;*out=s->interval;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_next(struct pt_wavetable_song *s,struct pt_render_interval *out)
{
    enum pt_wavetable_song_result r;
    do{r=pt_wavetable_song_next_step(s,out);}while(r==PT_WAVETABLE_SONG_UPLOADING && s->uploading==1);
    return r;
}
static enum pt_wavetable_song_result consume(struct pt_wavetable_song *s,uint32_t frames)
{
    enum pt_wavetable_song_result result;
    if(!s || !frames || frames>256)return PT_WAVETABLE_SONG_INVALID;
    result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(s->uploading && !s->forecast)return PT_WAVETABLE_SONG_UPLOADING;
    if(!s->pending || frames>s->remaining)return PT_WAVETABLE_SONG_INVALID;
    if(pt_render_sequence_consume(s->sequence,frames)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
    s->remaining-=frames;return PT_WAVETABLE_SONG_OK;
}
static enum pt_wavetable_song_result complete_step(struct pt_wavetable_song *s)
{
    int batch;enum pt_wavetable_song_result result=current(s);if(result)return result;
    if(!s->ready)return PT_WAVETABLE_SONG_PREPARING;
    if(s->uploading==1)return PT_WAVETABLE_SONG_UPLOADING;
    if(s->forecast) {
        if(s->remaining)return PT_WAVETABLE_SONG_INVALID;
        result=pt_wavetable_song_prefetch(s);if(result)return result;
        if(pt_render_lookahead_commit(&s->ahead)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
        s->forecast=0;
        if(s->interval.end)return stop(s)?PT_WAVETABLE_SONG_DONE:PT_WAVETABLE_SONG_STOPPING;
        if(pt_wavetable_dispatch_prepared(s->voices,s->version,s->rate,&s->plan,&s->format,s->staging,sizeof(s->staging),&s->sources)!=1)
            return fail(s,PT_WAVETABLE_SONG_DEVICE);
        cancel_batch(s);s->pending=0;return PT_WAVETABLE_SONG_OK;
    }
    if(s->uploading==2) {
        batch=batch_step(s);
        if(batch<0)return fail(s,PT_WAVETABLE_SONG_DEVICE);
        if(!batch)return PT_WAVETABLE_SONG_UPLOADING;
        if(pt_wavetable_dispatch_prepared(s->voices,s->version,s->rate,&s->plan,&s->format,s->staging,sizeof(s->staging),&s->sources)!=1)
            return fail(s,PT_WAVETABLE_SONG_DEVICE);
        cancel_batch(s);
    }else {
        if(!s->pending || s->remaining)return PT_WAVETABLE_SONG_INVALID;
        if(pt_render_sequence_complete(s->sequence,&s->plan)!=PT_RENDER_OK)return fail(s,PT_WAVETABLE_SONG_RENDER);
        if(s->interval.end)return stop(s)?PT_WAVETABLE_SONG_DONE:PT_WAVETABLE_SONG_STOPPING;
        if(!s->range || s->restored) {
            if(!batch_begin(s,0))return fail(s,PT_WAVETABLE_SONG_DEVICE);
            return PT_WAVETABLE_SONG_UPLOADING;
        }
    }
    s->pending=0;return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_consume(struct pt_wavetable_song *s,uint32_t frames)
{return s && !s->clock_armed?consume(s,frames):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_wavetable_song_complete_step(struct pt_wavetable_song *s)
{return s && !s->clock_armed?complete_step(s):PT_WAVETABLE_SONG_INVALID;}
enum pt_wavetable_song_result pt_wavetable_song_clock_arm(struct pt_wavetable_song *s,uint64_t start)
{
    enum pt_wavetable_song_result r=current(s);if(r)return r;
    if(s->clock_armed || !s->ready || !s->pending || !s->interval.emit || !s->interval.frames ||
       s->remaining!=s->interval.frames || (s->range && !s->restored) || (s->uploading && !s->forecast))
        return PT_WAVETABLE_SONG_INVALID;
    if(start>UINT64_MAX-s->remaining)return fail(s,PT_WAVETABLE_SONG_CLOCK);
    s->clock_start=s->clock_last=start;s->clock_deadline=start+s->remaining;s->clock_armed=1;
    return PT_WAVETABLE_SONG_OK;
}
enum pt_wavetable_song_result pt_wavetable_song_clock_service(struct pt_wavetable_song *s,uint64_t now)
{
    uint64_t debt;uint32_t frames;enum pt_wavetable_song_result r=current(s);if(r)return r;
    if(!s->clock_armed)return PT_WAVETABLE_SONG_INVALID;
    if(now<s->clock_last)return fail(s,PT_WAVETABLE_SONG_CLOCK);
    if(now>s->clock_deadline)return fail(s,PT_WAVETABLE_SONG_DEADLINE);
    s->clock_last=now;
    debt=now-s->clock_start-(s->interval.frames-s->remaining);
    if(now==s->clock_deadline && (s->forecast!=3 || debt>256))return fail(s,PT_WAVETABLE_SONG_DEADLINE);
    frames=debt>256?256:(uint32_t)debt;
    if(frames){r=consume(s,frames);if(r)return r;}
    if(now==s->clock_deadline) {
        r=complete_step(s);s->clock_armed=0;return r;
    }
    r=pt_wavetable_song_prefetch(s);
    return r==PT_WAVETABLE_SONG_OK || r==PT_WAVETABLE_SONG_UPLOADING?PT_WAVETABLE_SONG_WAITING:r;
}
enum pt_wavetable_song_result pt_wavetable_song_complete(struct pt_wavetable_song *s)
{
    enum pt_wavetable_song_result r;
    do{r=pt_wavetable_song_complete_step(s);}while(r==PT_WAVETABLE_SONG_UPLOADING && (s->uploading==2 || s->forecast));
    return r;
}
