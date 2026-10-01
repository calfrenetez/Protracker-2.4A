#ifndef PT_NATIVE_EDITOR_PAULA_TRANSPORT_H
#define PT_NATIVE_EDITOR_PAULA_TRANSPORT_H
#include "editor_paula.h"
#include "eclock_alarm.h"
/* Zero-init, noncopyable, serialized enclosing owner. Prepare with native.begin/
 * advance, then start once ready. Service does one bounded scheduler call and
 * polls private alarm IO without waiting. Wakeups never supply logical time.
 * Caller drives preparation/service and Wait using signal(); no UI installed.
 * Refusal/error is terminal until explicit cleanup/restart; never catch up or
 * rebase. All borrowed editor/master/context storage outlives successful detach. */
struct pt_native_paula_transport {
    struct pt_native_editor_paula native;
    struct pt_native_eclock clock;struct pt_native_alarm alarm;
    uint64_t alarm_deadline;unsigned started,done,failed;
};
static inline int pt_native_paula_transport_release(void *context)
{
    struct pt_native_paula_transport *t=context;
    /* Called only after song and device readers/cache have closed. Retain the
     * clock, enclosing owner and editor barrier if timer abort is still pending. */
    if(!pt_native_alarm_close(&t->alarm))return 0;
    pt_native_eclock_close(&t->clock);
    t->started=t->done=0;t->alarm_deadline=0;return 1;
}
static inline int pt_native_paula_transport_attach(struct pt_native_paula_transport *t,struct pt_editor *e)
{
    if(!t || t->native.binding.editor || t->native.active || t->clock.port || t->alarm.port ||
       t->native.release_tail)return 0;
    t->native.release_tail=pt_native_paula_transport_release;t->native.release_tail_context=t;
    if(pt_native_editor_paula_attach(&t->native,e))return 1;
    t->native.release_tail=NULL;t->native.release_tail_context=NULL;return 0;
}
static inline enum pt_paula_song_result pt_native_paula_transport_fail(
    struct pt_native_paula_transport *t,enum pt_paula_song_result r)
{
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
    if(r==PT_PAULA_SONG_PREPARING)t->failed=0;
    return r;
}
static inline enum pt_paula_song_result pt_native_paula_transport_arm(struct pt_native_paula_transport *t)
{
    uint64_t deadline;enum pt_paula_song_result r;enum pt_alarm_result alarm;
    r=pt_editor_paula_clocked_deadline(&t->native.binding,&deadline);
    if(r!=PT_PAULA_SONG_OK)return pt_native_paula_transport_fail(t,r);
    if(t->alarm.pending) {
        if(t->alarm_deadline==deadline)return PT_PAULA_SONG_WAITING;
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    }
    alarm=pt_native_alarm_arm(&t->alarm,deadline);
    if(alarm!=PT_ALARM_WAITING)return pt_native_paula_transport_fail(t,
        alarm==PT_ALARM_LATE?PT_PAULA_SONG_DEADLINE:PT_PAULA_SONG_CLOCK);
    t->alarm_deadline=deadline;return PT_PAULA_SONG_WAITING;
}
static inline enum pt_paula_song_result pt_native_paula_transport_start(
    struct pt_native_paula_transport *t,uint64_t delay_frames)
{
    enum pt_paula_song_result r;
    if(!t || t->failed || !t->native.active || t->native.failed || !t->native.binding.song ||
       t->started || t->clock.port || t->alarm.port)return PT_PAULA_SONG_INVALID;
    /* Confirm master preparation before opening/binding any clock. */
    r=pt_native_editor_paula_advance(&t->native,NULL);
    if(r!=PT_PAULA_SONG_OK)return r;
    if(!pt_native_eclock_open(&t->clock) || !pt_native_alarm_open(&t->alarm))
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    r=pt_editor_paula_clocked_begin(&t->native.binding,delay_frames,pt_native_eclock_read,&t->clock);
    if(r!=PT_PAULA_SONG_OK)return pt_native_paula_transport_fail(t,r);
    t->started=1;return pt_native_paula_transport_arm(t);
}
static inline enum pt_paula_song_result pt_native_paula_transport_service(struct pt_native_paula_transport *t)
{
    uint64_t ignored;enum pt_paula_song_result r,armed;enum pt_alarm_result alarm;
    if(!t || !t->native.active || !t->started || t->failed)return PT_PAULA_SONG_INVALID;
    if(t->done)return PT_PAULA_SONG_DONE;
    alarm=pt_native_alarm_poll(&t->alarm);
    if(alarm!=PT_ALARM_WAITING && alarm!=PT_ALARM_READY && alarm!=PT_ALARM_IDLE)
        return pt_native_paula_transport_fail(t,PT_PAULA_SONG_CLOCK);
    r=pt_editor_paula_clocked_service(&t->native.binding,&ignored);
    if(r==PT_PAULA_SONG_DONE){t->done=1;return r;}
    if(r!=PT_PAULA_SONG_WAITING && r!=PT_PAULA_SONG_OK)return pt_native_paula_transport_fail(t,r);
    armed=pt_native_paula_transport_arm(t);
    if(armed!=PT_PAULA_SONG_WAITING)return armed;
    return r;
}
/* Readiness/debt must still be serviced before waiting. This signal is only a
 * notification mask, not evidence that a batch is prepared or deadline met. */
static inline ULONG pt_native_paula_transport_signal(const struct pt_native_paula_transport *t)
{return t && t->started && !t->failed && !t->done?pt_native_alarm_signal(&t->alarm):0;}
#endif
