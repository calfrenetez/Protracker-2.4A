#include <string.h>
#include "paula_song.h"
#include "paula_internal.h"
#include "sampler_internal.h"
#include "project_snapshot.h"
#include "../core/render_lookahead.h"
#include "../core/elapsed_clock.h"
struct pt_paula_song {
    struct pt_allocator allocator;struct pt_paula_voices *voices;
    struct pt_sampler_paula *bridge;struct pt_sampler *sampler;struct pt_project *project,snapshot;
    int (*quiesce)(void *);void *quiesce_context;
    struct pt_paula_voice_api api;struct pt_paula_render_caps caps;struct pt_render_options options;
    struct pt_render_lookahead ahead;unsigned forecast;
    struct pt_paula_preflight *analysis;struct pt_render_sequence *sequence;struct pt_render_plan plan;struct pt_paula_prepared batch;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];struct pt_sampler_pin_job job;
    struct pt_paula_preflight_report report;struct pt_render_interval interval;
    int8_t map[PT_CHANNEL_LIMIT];uint64_t version;unsigned generation,slot,analyzed,ready,pending,done,closing;
    uint64_t clock_start,clock_last,clock_deadline;unsigned clock_armed;
    uint64_t schedule_start,schedule_last;unsigned schedule_phase,schedule_seen,visited,priming;
    struct pt_elapsed_clock elapsed;pt_paula_clock_read clock_read;void *clock_context;unsigned clock_bound;
    uint32_t remaining;enum pt_paula_song_result failure;
};
static enum pt_paula_song_result fail(struct pt_paula_song *s,enum pt_paula_song_result result)
{
    unsigned i;pt_render_lookahead_cancel(&s->ahead);pt_paula_cancel(&s->batch);s->clock_bound=0;s->schedule_phase=0;s->clock_armed=0;s->failure=result;s->closing=1;s->voices->closing=1;
    pt_paula_preflight_close(&s->analysis);pt_render_sequence_close(s->sequence);s->sequence=NULL;
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
    if(s->clock_armed || s->schedule_phase)return PT_PAULA_SONG_INVALID;
    if(!s->analyzed) {
        enum pt_paula_capability result=s->analysis?pt_paula_preflight_step(s->analysis,&s->report):
            pt_paula_preflight_begin(s->project,&s->options,s->map,&s->caps,s->api.control!=NULL,
                &s->allocator,&s->report,&s->analysis);
        if(out)*out=s->report;
        if(result==PT_PAULA_PENDING)return PT_PAULA_SONG_PREPARING;
        if(result!=PT_PAULA_COMPATIBLE)return fail(s,result==PT_PAULA_MEMORY?PT_PAULA_SONG_MEMORY:PT_PAULA_SONG_CAPABILITY);
        if(!pt_paula_preflight_transfer(s->analysis,&s->sequence))return fail(s,PT_PAULA_SONG_RENDER);
        pt_paula_preflight_close(&s->analysis);s->analyzed=1;return PT_PAULA_SONG_PREPARING;
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
/* Only within a serialized call which has checked current(), with no
 * intervening external callback. Public entry points retain their guards. */
static enum pt_paula_song_result next_validated(struct pt_paula_song *s,struct pt_render_interval *out)
{
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(s->clock_armed || !out || s->pending)return PT_PAULA_SONG_INVALID;
    if(s->done)return PT_PAULA_SONG_DONE;
    if(pt_render_sequence_next(s->sequence,&s->interval)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
    s->visited=1;s->pending=1;s->remaining=s->interval.frames;*out=s->interval;return PT_PAULA_SONG_OK;
}
static enum pt_paula_song_result next(struct pt_paula_song *s,struct pt_render_interval *out)
{
    enum pt_paula_song_result state=current(s);
    return state==PT_PAULA_SONG_OK?next_validated(s,out):state;
}
static enum pt_paula_song_result consume(struct pt_paula_song *s,uint32_t frames)
{
    enum pt_paula_song_result state=current(s);
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(!s->pending || !frames || frames>256 || frames>s->remaining)return PT_PAULA_SONG_INVALID;
    if(pt_render_sequence_consume(s->sequence,frames)!=PT_RENDER_OK)return fail(s,PT_PAULA_SONG_RENDER);
    s->remaining-=frames;return PT_PAULA_SONG_OK;
}
static enum pt_paula_song_result prefetch_validated(struct pt_paula_song *s)
{
    unsigned ready=0;
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
static enum pt_paula_song_result prefetch(struct pt_paula_song *s)
{
    enum pt_paula_song_result state=current(s);
    return state==PT_PAULA_SONG_OK?prefetch_validated(s):state;
}
static enum pt_paula_song_result stage(struct pt_paula_song *s)
{
    enum pt_paula_song_result state=current(s);
    if(state!=PT_PAULA_SONG_OK)return state;
    if(!s->ready)return PT_PAULA_SONG_PREPARING;
    if(!s->pending || s->remaining)return PT_PAULA_SONG_INVALID;
    if(s->forecast)return prefetch_validated(s);
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
static enum pt_paula_song_result complete(struct pt_paula_song *s)
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
    do{state=stage(s);}while(state==PT_PAULA_SONG_PREPARING && s->ready && s->pending && !s->remaining);
    if(state!=PT_PAULA_SONG_OK)return state;
    result=pt_paula_apply(&s->batch);
    if(result!=1) {
        /* Dispatch runtime failure already attempted each held stop once. */
        if(result<0){s->failure=PT_PAULA_SONG_DEVICE;s->closing=1;pt_render_sequence_close(s->sequence);s->sequence=NULL;return s->failure;}
        return fail(s,PT_PAULA_SONG_DEVICE);
    }
    s->pending=0;s->done=s->interval.end;return s->done?PT_PAULA_SONG_DONE:PT_PAULA_SONG_OK;
}
enum pt_paula_song_result pt_paula_song_consume(struct pt_paula_song *s,uint32_t frames)
{return s && !s->schedule_phase && !s->clock_armed?consume(s,frames):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_paula_song_prefetch(struct pt_paula_song *s)
{return s && !s->schedule_phase && !s->clock_armed?prefetch(s):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_paula_song_stage(struct pt_paula_song *s)
{return s && !s->schedule_phase && !s->clock_armed?stage(s):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_paula_song_complete(struct pt_paula_song *s)
{return s && !s->schedule_phase && !s->clock_armed?complete(s):PT_PAULA_SONG_INVALID;}
static enum pt_paula_song_result clock_arm(struct pt_paula_song *s,uint64_t start)
{
    enum pt_paula_song_result r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(s->clock_armed || !s->ready || !s->pending || !s->interval.emit || !s->interval.frames ||
       s->remaining!=s->interval.frames || (!s->forecast && (s->batch.preparing || s->batch.ready)))
        return PT_PAULA_SONG_INVALID;
    if(start>UINT64_MAX-s->remaining)return fail(s,PT_PAULA_SONG_CLOCK);
    s->clock_start=s->clock_last=start;s->clock_deadline=start+s->remaining;s->clock_armed=1;
    return PT_PAULA_SONG_OK;
}
static enum pt_paula_song_result clock_service(struct pt_paula_song *s,uint64_t now)
{
    uint64_t debt;uint32_t frames;enum pt_paula_song_result r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!s->clock_armed)return PT_PAULA_SONG_INVALID;
    if(now<s->clock_last)return fail(s,PT_PAULA_SONG_CLOCK);
    if(now>s->clock_deadline)return fail(s,PT_PAULA_SONG_DEADLINE);
    s->clock_last=now;
    debt=now-s->clock_start-(s->interval.frames-s->remaining);
    if(now==s->clock_deadline && (s->forecast!=3 || debt>256))return fail(s,PT_PAULA_SONG_DEADLINE);
    frames=debt>256?256:(uint32_t)debt;
    if(frames){r=consume(s,frames);if(r!=PT_PAULA_SONG_OK)return r;}
    if(now==s->clock_deadline){r=complete(s);s->clock_armed=0;return r;}
    r=prefetch(s);
    return r==PT_PAULA_SONG_OK || r==PT_PAULA_SONG_PREPARING?PT_PAULA_SONG_WAITING:r;
}
enum pt_paula_song_result pt_paula_song_next(struct pt_paula_song *s,struct pt_render_interval *out)
{return s && !s->schedule_phase?next(s,out):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_paula_song_clock_arm(struct pt_paula_song *s,uint64_t start)
{return s && !s->schedule_phase?clock_arm(s,start):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_paula_song_clock_service(struct pt_paula_song *s,uint64_t now)
{return s && !s->schedule_phase?clock_service(s,now):PT_PAULA_SONG_INVALID;}
enum {SCHEDULE_NEXT=1,SCHEDULE_ZERO,SCHEDULE_READY_NEXT,SCHEDULE_READY_ZERO,SCHEDULE_RUNNING};
enum pt_paula_song_result pt_paula_song_schedule_begin(struct pt_paula_song *s,uint64_t start)
{
    enum pt_paula_song_result r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!s->ready || s->visited || s->schedule_phase || s->clock_armed)return PT_PAULA_SONG_INVALID;
    if(start>UINT64_MAX-s->report.frames)return fail(s,PT_PAULA_SONG_CLOCK);
    s->schedule_start=start;s->schedule_seen=0;s->schedule_phase=SCHEDULE_NEXT;
    return PT_PAULA_SONG_OK;
}
static enum pt_paula_song_result scheduled_interval(struct pt_paula_song *s,uint64_t start)
{
    struct pt_render_interval span;enum pt_paula_song_result r=next(s,&span);
    if(r!=PT_PAULA_SONG_OK)return r;
    /* Whole-song44.1/48kHz traversal has positive emitting runtime intervals.
     * Refuse unexpected zero/silent work at a live boundary. */
    if(!span.emit || !span.frames)return fail(s,PT_PAULA_SONG_RENDER);
    r=clock_arm(s,start);if(r==PT_PAULA_SONG_OK)s->schedule_phase=SCHEDULE_RUNNING;return r;
}
/* One bounded startup transition, independent of any clock or frame epoch.
 * Caller validated ownership; empty plans may advance without voice output. */
static enum pt_paula_song_result startup_step(struct pt_paula_song *s)
{
    enum pt_paula_song_result r;struct pt_render_interval span;
    switch(s->schedule_phase) {
    case SCHEDULE_NEXT:
        r=next_validated(s,&span);if(r!=PT_PAULA_SONG_OK)return r;
        if(!span.emit)return fail(s,PT_PAULA_SONG_RENDER);
        s->schedule_phase=span.frames?SCHEDULE_READY_NEXT:SCHEDULE_ZERO;break;
    case SCHEDULE_ZERO:
        r=prefetch_validated(s);if(r==PT_PAULA_SONG_PREPARING)break;
        if(r!=PT_PAULA_SONG_OK)return r;
        if(!s->plan.count && !s->interval.end) {
            r=complete(s);if(r!=PT_PAULA_SONG_OK)return r;s->schedule_phase=SCHEDULE_NEXT;
        }else s->schedule_phase=SCHEDULE_READY_ZERO;
        break;
    default:break; /* Ready implies no additional work/output before start. */
    }
    return s->schedule_phase==SCHEDULE_READY_NEXT || s->schedule_phase==SCHEDULE_READY_ZERO?
        PT_PAULA_SONG_OK:PT_PAULA_SONG_WAITING;
}
enum pt_paula_song_result pt_paula_song_prime(struct pt_paula_song *s)
{
    enum pt_paula_song_result r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!s->ready || s->clock_armed || s->clock_bound)return PT_PAULA_SONG_INVALID;
    if(!s->priming) {
        if(s->visited || s->schedule_phase)return PT_PAULA_SONG_INVALID;
        s->priming=1;s->schedule_phase=SCHEDULE_NEXT;
    }
    return startup_step(s);
}
static enum pt_paula_song_result schedule_step(struct pt_paula_song *s,uint64_t now,uint64_t *deadline)
{
    enum pt_paula_song_result r;
    if(!deadline)return PT_PAULA_SONG_INVALID;
    r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!s->schedule_phase)return PT_PAULA_SONG_INVALID;
    if(s->schedule_seen && now<s->schedule_last)return fail(s,PT_PAULA_SONG_CLOCK);
    s->schedule_seen=1;s->schedule_last=now;
    if(s->schedule_phase==SCHEDULE_RUNNING) {
        r=clock_service(s,now);
        if(r==PT_PAULA_SONG_OK){r=scheduled_interval(s,now);if(r!=PT_PAULA_SONG_OK)return r;}
        else if(r!=PT_PAULA_SONG_WAITING)return r;
        *deadline=s->clock_deadline;return PT_PAULA_SONG_WAITING;
    }
    if(now>s->schedule_start)return fail(s,PT_PAULA_SONG_DEADLINE);
    if(now==s->schedule_start) {
        if(s->schedule_phase==SCHEDULE_READY_NEXT) {
            r=clock_arm(s,now);if(r!=PT_PAULA_SONG_OK)return r;s->schedule_phase=SCHEDULE_RUNNING;
        }else if(s->schedule_phase==SCHEDULE_READY_ZERO) {
            r=complete(s);if(r!=PT_PAULA_SONG_OK)return r;
            r=scheduled_interval(s,now);if(r!=PT_PAULA_SONG_OK)return r;
        }else return fail(s,PT_PAULA_SONG_DEADLINE);
        *deadline=s->clock_deadline;return PT_PAULA_SONG_WAITING;
    }
    r=startup_step(s);
    if(r!=PT_PAULA_SONG_OK && r!=PT_PAULA_SONG_WAITING)return r;
    *deadline=s->schedule_start;return r;
}
enum pt_paula_song_result pt_paula_song_schedule_step(struct pt_paula_song *s,uint64_t now,uint64_t *deadline)
{return s && !s->clock_bound && !s->priming?schedule_step(s,now,deadline):PT_PAULA_SONG_INVALID;}
enum pt_paula_song_result pt_paula_song_clocked_begin(struct pt_paula_song *s,uint64_t delay,pt_paula_clock_read read,void *context)
{
    uint64_t ticks;uint32_t frequency;enum pt_paula_song_result r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!read || !s->ready || s->clock_armed || s->clock_bound)return PT_PAULA_SONG_INVALID;
    if(s->priming) {
        if(s->schedule_phase!=SCHEDULE_READY_NEXT && s->schedule_phase!=SCHEDULE_READY_ZERO)return PT_PAULA_SONG_INVALID;
    }else if(s->visited || s->schedule_phase)return PT_PAULA_SONG_INVALID;
    if(delay>UINT64_MAX-s->report.frames)return fail(s,PT_PAULA_SONG_CLOCK);
    if(read(context,&ticks,&frequency)!=1 || pt_elapsed_clock_init(&s->elapsed,frequency,s->options.rate,ticks,0)!=PT_ELAPSED_OK)
        return fail(s,PT_PAULA_SONG_CLOCK);
    /* The foreign reader can invalidate borrowed ownership before binding. */
    r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(s->priming) {
        s->schedule_start=delay;s->schedule_seen=0;s->priming=0;
    }else {r=pt_paula_song_schedule_begin(s,delay);if(r!=PT_PAULA_SONG_OK)return r;}
    s->clock_read=read;s->clock_context=context;s->clock_bound=1;return PT_PAULA_SONG_OK;
}
enum pt_paula_song_result pt_paula_song_clocked_service(struct pt_paula_song *s,uint64_t *deadline)
{
    uint64_t ticks,frames;uint32_t frequency;enum pt_paula_song_result r;
    if(!deadline)return PT_PAULA_SONG_INVALID;
    r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!s->clock_bound)return PT_PAULA_SONG_INVALID;
    if(s->clock_read(s->clock_context,&ticks,&frequency)!=1 ||
       pt_elapsed_clock_advance(&s->elapsed,frequency,ticks,&frames)!=PT_ELAPSED_OK)
        return fail(s,PT_PAULA_SONG_CLOCK);
    return schedule_step(s,frames,deadline);
}
enum pt_paula_song_result pt_paula_song_clocked_service_state(struct pt_paula_song *s,uint64_t *ticks,unsigned *can_wait)
{
    uint64_t frame,counter;unsigned ready;enum pt_paula_song_result r;
    if(!ticks || !can_wait)return PT_PAULA_SONG_INVALID;
    r=pt_paula_song_clocked_service(s,&frame);
    if(r!=PT_PAULA_SONG_OK && r!=PT_PAULA_SONG_WAITING)return r;
    /* Same serialized step has already performed both current guards. This
       read-only conversion needs no second public ownership/deadline query. */
    if(pt_elapsed_clock_deadline(&s->elapsed,frame,&counter)!=PT_ELAPSED_OK)
        return fail(s,PT_PAULA_SONG_CLOCK);
    /* Only this completed serialized step can publish readiness. A running
     * interval needs its forecast AND all observed frame debt consumed. Signals
     * and WAITING alone cannot distinguish pending bounded preparation/debt. */
    ready=s->schedule_phase==SCHEDULE_READY_NEXT || s->schedule_phase==SCHEDULE_READY_ZERO;
    if(s->schedule_phase==SCHEDULE_RUNNING)
        ready=s->clock_armed && s->forecast==3 && s->clock_last>=s->clock_start &&
            s->clock_last-s->clock_start==s->interval.frames-s->remaining;
    *ticks=counter;*can_wait=ready;return r;
}
enum pt_paula_song_result pt_paula_song_clocked_service_counter(struct pt_paula_song *s,uint64_t *ticks)
{
    unsigned can_wait;return pt_paula_song_clocked_service_state(s,ticks,&can_wait);
}
enum pt_paula_song_result pt_paula_song_clocked_deadline(struct pt_paula_song *s,uint64_t *ticks)
{
    enum pt_paula_song_result r;uint64_t frame;
    if(!ticks)return PT_PAULA_SONG_INVALID;
    r=current(s);if(r!=PT_PAULA_SONG_OK)return r;
    if(!s->clock_bound)return PT_PAULA_SONG_INVALID;
    frame=s->schedule_phase==SCHEDULE_RUNNING?s->clock_deadline:s->schedule_start;
    if(pt_elapsed_clock_deadline(&s->elapsed,frame,ticks)!=PT_ELAPSED_OK)return fail(s,PT_PAULA_SONG_CLOCK);
    return PT_PAULA_SONG_OK;
}
int pt_paula_song_close(struct pt_paula_song **out)
{
    struct pt_paula_song *s;struct pt_allocator a;unsigned i;
    if(!out)return 0;
    s=*out;if(!s)return 1;
    s->closing=1;pt_render_lookahead_cancel(&s->ahead);pt_paula_cancel(&s->batch);pt_paula_preflight_close(&s->analysis);
    if(!pt_paula_close_owned(s->voices,s))return 0;
    pt_render_sequence_close(s->sequence);pt_sampler_pin_job_cancel(&s->job);
    for(i=0;i<PT_PROJECT_SAMPLES;++i)pt_sampler_unpin(s->pin[i]);
    a=s->allocator;a.release(a.context,s);*out=NULL;return 1;
}
