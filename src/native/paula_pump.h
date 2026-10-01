#ifndef PT_NATIVE_PAULA_PUMP_H
#define PT_NATIVE_PAULA_PUMP_H
#include "editor_paula_transport.h"
enum pt_native_pump_result {PT_NATIVE_PUMP_FAILED=-2,PT_NATIVE_PUMP_INVALID=-1,
    PT_NATIVE_PUMP_WORK,PT_NATIVE_PUMP_WAKE,PT_NATIVE_PUMP_STOPPED,
    PT_NATIVE_PUMP_DONE,PT_NATIVE_PUMP_HOLD};
/* Zero-init, noncopyable, single serialized calling-task owner of an already
 * started transport. No task priority change. One step performs ONE actual
 * bounded service, then optionally Exec Wait on its private alarms plus a
 * mandatory caller-owned termination mask. WORK needs another bounded step;
 * WAKE needs a fresh actual service, never synthetic time or signal readiness.
 * The caller bounds work iterations and owns abort delivery. Pending timers
 * provide notifications, not a hard scheduling/hang guarantee. No UI installed.
 * Abort/DONE/error close once; HOLD requires explicit close while retaining ALL
 * transport/editor/master/storage and callback contexts. No automatic retry,
 * detach, disposal, forced release, catchup or epoch/deadline changes. */
struct pt_native_paula_pump {struct pt_native_paula_transport *transport;
    struct Task *task;unsigned active,stopping;enum pt_paula_song_result last;
    /* Diagnostic only: no time/readiness decisions; retained after close. */
    unsigned wait_calls;ULONG last_wake;};
static inline int pt_native_paula_pump_bind(struct pt_native_paula_pump *p,
    struct pt_native_paula_transport *t)
{
    struct Task *task;
    if(!p || p->active || p->transport || !t || t->pump || !t->started || !t->native.active ||
       !t->native.binding.song || t->failed || t->done || !pt_native_paula_transport_signal(t))return 0;
    task=FindTask(NULL);
    if(!task || !t->clock.port || !t->alarm.port || !t->service_alarm.port ||
       t->clock.port->mp_SigTask!=task || t->alarm.port->mp_SigTask!=task ||
       t->service_alarm.port->mp_SigTask!=task)return 0;
    p->transport=t;p->task=task;p->active=1;p->stopping=0;p->last=PT_PAULA_SONG_OK;p->wait_calls=0;p->last_wake=0;t->pump=p;return 1;
}
static inline int pt_native_paula_pump_close(struct pt_native_paula_pump *p)
{
    if(!p)return 0;
    if(!p->active)return !p->transport;
    if(FindTask(NULL)!=p->task || p->transport->pump!=p)return 0;
    p->stopping=1;p->transport->can_wait=0;
    if(!pt_editor_paula_stop(&p->transport->native.binding))return 0;
    p->transport->pump=NULL;p->transport=NULL;p->task=NULL;p->active=p->stopping=0;return 1;
}
static inline enum pt_native_pump_result pt_native_paula_pump_stop(
    struct pt_native_paula_pump *p,enum pt_native_pump_result result)
{return pt_native_paula_pump_close(p)?result:PT_NATIVE_PUMP_HOLD;}
static inline enum pt_native_pump_result pt_native_paula_pump_step(
    struct pt_native_paula_pump *p,ULONG abort_mask)
{
    struct pt_native_paula_transport *t;ULONG mask,wake;enum pt_paula_song_result r;
    if(!p || !p->active || !p->transport || p->transport->pump!=p || FindTask(NULL)!=p->task)return PT_NATIVE_PUMP_INVALID;
    if(p->stopping)return PT_NATIVE_PUMP_HOLD;
    t=p->transport;
    mask=pt_native_alarm_signal(&t->alarm)|pt_native_alarm_signal(&t->service_alarm);
    if(!abort_mask || (abort_mask&mask))return PT_NATIVE_PUMP_INVALID;
    if(SetSignal(0,0)&abort_mask)return pt_native_paula_pump_stop(p,PT_NATIVE_PUMP_STOPPED);
    r=pt_native_paula_transport_service(t);p->last=r;
    if(r==PT_PAULA_SONG_DONE)return pt_native_paula_pump_stop(p,PT_NATIVE_PUMP_DONE);
    if(r!=PT_PAULA_SONG_OK && r!=PT_PAULA_SONG_WAITING)
        return pt_native_paula_pump_stop(p,PT_NATIVE_PUMP_FAILED);
    mask=pt_native_paula_transport_wait_mask(t);
    if(!mask)return PT_NATIVE_PUMP_WORK;
    ++p->wait_calls;wake=Wait(mask|abort_mask);p->last_wake=wake;
    if(FindTask(NULL)!=p->task){p->stopping=1;t->can_wait=0;return PT_NATIVE_PUMP_HOLD;}
    if(wake&abort_mask)return pt_native_paula_pump_stop(p,PT_NATIVE_PUMP_STOPPED);
    return PT_NATIVE_PUMP_WAKE;
}
#endif
