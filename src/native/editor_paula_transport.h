#ifndef PT_NATIVE_EDITOR_PAULA_TRANSPORT_H
#define PT_NATIVE_EDITOR_PAULA_TRANSPORT_H
#include "editor_paula.h"
#include "eclock_alarm.h"
#include "../core/elapsed_clock.h"
/* Zero-init, noncopyable, serialized enclosing owner. Prepare with native.begin/
 * advance, then start once ready. Service does one bounded scheduler call and
 * polls private alarm IO without waiting. Wakeups never supply logical time.
 * Caller drives preparation/service and Wait using signal(); no UI installed.
 * Refusal/error is terminal until explicit cleanup/restart; never catch up or
 * rebase. All borrowed editor/master/context storage outlives successful detach. */
enum pt_native_paula_phase {
    PT_NATIVE_PAULA_NONE,PT_NATIVE_PAULA_OPEN,PT_NATIVE_PAULA_BEGIN,
    PT_NATIVE_PAULA_SERVICE_CLOCK,PT_NATIVE_PAULA_ENTRY,PT_NATIVE_PAULA_POLL,
    PT_NATIVE_PAULA_CORE,PT_NATIVE_PAULA_BOUNDARY_DEADLINE,PT_NATIVE_PAULA_BOUNDARY_ARM,
    PT_NATIVE_PAULA_POST,PT_NATIVE_PAULA_PERIODIC_DEADLINE,PT_NATIVE_PAULA_PERIODIC_ARM
};
struct pt_native_paula_transport {
    struct pt_native_editor_paula native;
    struct pt_native_eclock clock;struct pt_native_alarm alarm;
    struct pt_native_alarm service_alarm;struct pt_elapsed_clock service_clock;
    uint64_t service_last_frames,service_deadline;unsigned service_clock_ready;
    uint64_t alarm_deadline;unsigned started,done,failed;
    /* Capture once before stop can clear live state. Retained through cleanup;
     * next successful begin resets diagnostics. No additional clock I/O. */
    enum pt_native_paula_phase phase,failure_phase;
    uint64_t failure_frames,failure_last_frames;

};
static inline int pt_native_paula_transport_release(void *context)
{
    struct pt_native_paula_transport *t=context;
    /* Called only after song and device readers/cache have closed. Retain the
     * clock, enclosing owner and editor barrier if timer abort is still pending. */
    if(!pt_native_alarm_close(&t->service_alarm) || !pt_native_alarm_close(&t->alarm))return 0;
    pt_native_eclock_close(&t->clock);
    t->started=t->done=t->service_clock_ready=0;
    /* Retain last observations even if core failure initiated this cleanup
     * before the enclosing service call can snapshot its refusal. */
    return 1;
}
static inline int pt_native_paula_transport_attach(struct pt_native_paula_transport *t,struct pt_editor *e)
{
    if(!t || t->native.binding.editor || t->native.active || t->clock.port || t->alarm.port || t->service_alarm.port ||
       t->native.release_tail)return 0;
    t->native.release_tail=pt_native_paula_transport_release;t->native.release_tail_context=t;
    if(pt_native_editor_paula_attach(&t->native,e))return 1;
    t->native.release_tail=NULL;t->native.release_tail_context=NULL;return 0;
}
static inline enum pt_paula_song_result pt_native_paula_transport_fail(
    struct pt_native_paula_transport *t,enum pt_paula_song_result r)
{
    if(!t->failed) {
        t->failure_phase=t->phase;t->failure_frames=t->service_clock.frames;
        t->failure_last_frames=t->service_last_frames;
    }
    t->failed=1;t->native.failed=1;
    /* One bounded stop attempt. Pending reader/device/timer cleanup stays under
     * the existing editor veto; caller must continue explicit stop/detach. */
    pt_editor_paula_stop(&t->native.binding);return r;
}
static inline enum pt_paula_song_result pt_native_paula_transport_begin(
    struct pt_native_paula_transport *t,const struct pt_render_options *o,size_t budget)
{
    enum pt_paula_song_result r;
    if(!t || t->native.release_tail!=pt_native_paula_transport_release ||
       t->native.release_tail_context!=t)return PT_PAULA_SONG_INVALID;
    r=pt_native_editor_paula_begin(&t->native,o,budget);
    if(r==PT_PAULA_SONG_PREPARING) {
        t->failed=0;t->phase=t->failure_phase=PT_NATIVE_PAULA_NONE;
        t->failure_frames=t->failure_last_frames=0;
    }
    return r;
}
/* Only an immediately returned validated core deadline or startup query may be
 * passed here. Pending absolute identity and actual late-arm check remain exact. */
static inline enum pt_paula_song_result pt_native_paula_transport_arm_deadline(
    struct pt_native_paula_transport *t,uint64_t deadline)
{
    enum pt_alarm_result alarm;t->phase=PT_NATIVE_PAULA_BOUNDARY_ARM;
    if(t->alarm.pending) {
        if(t->alarm_deadline==deadline)return PT_PAULA_SONG_WAITING;
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    }
    alarm=pt_native_alarm_arm(&t->alarm,deadline);
    if(alarm!=PT_ALARM_WAITING)return pt_native_paula_transport_fail(t,
        alarm==PT_ALARM_LATE?PT_PAULA_SONG_DEADLINE:PT_PAULA_SONG_CLOCK);
    t->alarm_deadline=deadline;return PT_PAULA_SONG_WAITING;
}
static inline enum pt_paula_song_result pt_native_paula_transport_arm(struct pt_native_paula_transport *t)
{
    uint64_t deadline;enum pt_paula_song_result r;t->phase=PT_NATIVE_PAULA_BOUNDARY_DEADLINE;
    r=pt_editor_paula_clocked_deadline(&t->native.binding,&deadline);
    if(r!=PT_PAULA_SONG_OK)return pt_native_paula_transport_fail(t,r);
    return pt_native_paula_transport_arm_deadline(t,deadline);
}
/* Service wakeups use their own immutable epoch and fractional carry. Their
 * 128-frame grid never rebases song time or replaces its exact boundary alarm.
 * More than 256 observed frames between bounded calls refuses before dispatch;
 * missed notifications never cause a loop of catch-up calls. */
static inline enum pt_paula_song_result pt_native_paula_transport_observe(
    struct pt_native_paula_transport *t,uint64_t *frames)
{
    uint64_t ticks;uint32_t frequency;
    if(!t->service_clock_ready || !pt_native_eclock_read(&t->clock,&ticks,&frequency) ||
       pt_elapsed_clock_advance(&t->service_clock,frequency,ticks,frames)!=PT_ELAPSED_OK)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    if(*frames-t->service_last_frames>256)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_DEADLINE);
    return PT_PAULA_SONG_OK;
}
static inline enum pt_paula_song_result pt_native_paula_transport_service_arm(struct pt_native_paula_transport *t)
{
    uint64_t frames,next,deadline;enum pt_alarm_result alarm;enum pt_paula_song_result r;
    t->phase=PT_NATIVE_PAULA_POST;
    r=pt_native_paula_transport_observe(t,&frames);if(r!=PT_PAULA_SONG_OK)return r;
    if(t->service_alarm.pending)return PT_PAULA_SONG_WAITING;
    t->phase=PT_NATIVE_PAULA_PERIODIC_DEADLINE;
    if(frames>UINT64_MAX-128)return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    next=(frames/128+1)*128;
    if(pt_elapsed_clock_deadline(&t->service_clock,next,&deadline)!=PT_ELAPSED_OK)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    /* Earlier song boundaries retain their separate alarm and strict gate. */
    t->phase=PT_NATIVE_PAULA_PERIODIC_ARM;
    alarm=pt_native_alarm_arm(&t->service_alarm,deadline);
    if(alarm!=PT_ALARM_WAITING)return pt_native_paula_transport_fail(t,
        alarm==PT_ALARM_LATE?PT_PAULA_SONG_DEADLINE:PT_PAULA_SONG_CLOCK);
    t->service_deadline=deadline;return PT_PAULA_SONG_WAITING;
}
static inline enum pt_paula_song_result pt_native_paula_transport_start(
    struct pt_native_paula_transport *t,uint64_t delay_frames)
{
    enum pt_paula_song_result r;uint64_t ticks;uint32_t frequency;
    if(!t || t->failed || !t->native.active || t->native.failed || !t->native.binding.song ||
       t->started || t->clock.port || t->alarm.port || t->service_alarm.port)return PT_PAULA_SONG_INVALID;
    /* Confirm master preparation before opening/binding any clock. */
    r=pt_native_editor_paula_advance(&t->native,NULL);
    if(r!=PT_PAULA_SONG_OK)return r;
    t->phase=PT_NATIVE_PAULA_OPEN;
    if(!pt_native_eclock_open(&t->clock) || !pt_native_alarm_open(&t->alarm) || !pt_native_alarm_open(&t->service_alarm))
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    t->phase=PT_NATIVE_PAULA_BEGIN;
    r=pt_editor_paula_clocked_begin(&t->native.binding,delay_frames,pt_native_eclock_read,&t->clock);
    if(r!=PT_PAULA_SONG_OK)return pt_native_paula_transport_fail(t,r);
    t->phase=PT_NATIVE_PAULA_SERVICE_CLOCK;
    if(!pt_native_eclock_read(&t->clock,&ticks,&frequency) ||
       pt_elapsed_clock_init(&t->service_clock,frequency,t->native.options.rate,ticks,0)!=PT_ELAPSED_OK)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    t->service_clock_ready=1;t->service_last_frames=0;t->started=1;
    r=pt_native_paula_transport_arm(t);if(r!=PT_PAULA_SONG_WAITING)return r;
    return pt_native_paula_transport_service_arm(t);
}
static inline enum pt_paula_song_result pt_native_paula_transport_service(struct pt_native_paula_transport *t)
{
    uint64_t deadline,frames;enum pt_paula_song_result r,armed;enum pt_alarm_result alarm;
    if(!t || !t->native.active || !t->started || t->failed)return PT_PAULA_SONG_INVALID;
    if(t->done)return PT_PAULA_SONG_DONE;
    t->phase=PT_NATIVE_PAULA_ENTRY;
    r=pt_native_paula_transport_observe(t,&frames);if(r!=PT_PAULA_SONG_OK)return r;
    t->service_last_frames=frames;
    t->phase=PT_NATIVE_PAULA_POLL;
    alarm=pt_native_alarm_poll(&t->service_alarm);
    if(alarm!=PT_ALARM_WAITING && alarm!=PT_ALARM_READY && alarm!=PT_ALARM_IDLE)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    alarm=pt_native_alarm_poll(&t->alarm);
    if(alarm!=PT_ALARM_WAITING && alarm!=PT_ALARM_READY && alarm!=PT_ALARM_IDLE)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    t->phase=PT_NATIVE_PAULA_CORE;
    r=pt_editor_paula_clocked_service_counter(&t->native.binding,&deadline);
    if(r==PT_PAULA_SONG_DONE){t->done=1;return r;}
    if(r!=PT_PAULA_SONG_WAITING && r!=PT_PAULA_SONG_OK)return pt_native_paula_transport_fail(t,r);
    armed=pt_native_paula_transport_arm_deadline(t,deadline);
    if(armed!=PT_PAULA_SONG_WAITING)return armed;
    armed=pt_native_paula_transport_service_arm(t);
    if(armed!=PT_PAULA_SONG_WAITING)return armed;
    return r;
}
/* Readiness/debt must still be serviced before waiting. This signal is only a
 * notification mask for BOTH alarms, not evidence of readiness/deadline success. */
static inline ULONG pt_native_paula_transport_signal(const struct pt_native_paula_transport *t)
{return t && t->started && !t->failed && !t->done?
    pt_native_alarm_signal(&t->alarm)|pt_native_alarm_signal(&t->service_alarm):0;}
#endif
