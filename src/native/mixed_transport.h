#ifndef PT_NATIVE_MIXED_TRANSPORT_H
#define PT_NATIVE_MIXED_TRANSPORT_H
#include "eclock_alarm.h"
#include "../editor/mixed_transport.h"
/* OS2+ private counter and WAITECLOCK request. Zero-init, noncopyable; no wait,
 * global TimerBase, CIA/MMIO/output or editor event-loop installation here.
 * Open transfers resources to pump even when clocked begin fails; close required
 * while pump.active. Context/project/handle stay alive until close returns1. */
struct pt_native_mixed_transport {
    struct pt_mixed_transport pump;struct pt_native_eclock clock;struct pt_native_alarm alarm;
};
static inline int pt_native_mixed_read(void *context,uint64_t *ticks,uint32_t *frequency)
{return pt_native_eclock_read(&((struct pt_native_mixed_transport *)context)->clock,ticks,frequency);}
static inline enum pt_mixed_timer_result pt_native_mixed_poll(void *context)
{
    enum pt_alarm_result r=pt_native_alarm_poll(&((struct pt_native_mixed_transport *)context)->alarm);
    return r==PT_ALARM_WAITING?PT_MIXED_TIMER_WAITING:r==PT_ALARM_READY?PT_MIXED_TIMER_READY:PT_MIXED_TIMER_ERROR;
}
static inline enum pt_mixed_timer_result pt_native_mixed_arm(void *context,uint64_t ticks)
{
    enum pt_alarm_result r=pt_native_alarm_arm(&((struct pt_native_mixed_transport *)context)->alarm,ticks);
    return r==PT_ALARM_WAITING?PT_MIXED_TIMER_WAITING:r==PT_ALARM_LATE?PT_MIXED_TIMER_LATE:PT_MIXED_TIMER_ERROR;
}
static inline int pt_native_mixed_alarm_close(void *context)
{return pt_native_alarm_close(&((struct pt_native_mixed_transport *)context)->alarm);}
static inline int pt_native_mixed_counter_close(void *context)
{pt_native_eclock_close(&((struct pt_native_mixed_transport *)context)->clock);return 1;}
static inline uint32_t pt_native_mixed_signal(void *context)
{return (uint32_t)pt_native_alarm_signal(&((struct pt_native_mixed_transport *)context)->alarm);}
static inline enum pt_mixed_owner_result pt_native_mixed_open(struct pt_native_mixed_transport *n,
    struct pt_mixed_owner **owner,uint64_t delay,uint32_t quantum)
{
    struct pt_mixed_timer_api api;
    if(!n || n->pump.active || n->clock.port || n->clock.request || n->alarm.port || n->alarm.request ||
       !owner || !*owner || !quantum || quantum>256)return PT_MIXED_OWNER_INVALID;
    if(!pt_native_eclock_open(&n->clock))return PT_MIXED_OWNER_DEVICE;
    if(!pt_native_alarm_open(&n->alarm)){pt_native_eclock_close(&n->clock);return PT_MIXED_OWNER_DEVICE;}
    api=(struct pt_mixed_timer_api){n,pt_native_mixed_read,pt_native_mixed_poll,pt_native_mixed_arm,
        pt_native_mixed_alarm_close,pt_native_mixed_counter_close,pt_native_mixed_signal};
    return pt_mixed_transport_begin(&n->pump,owner,delay,quantum,&api);
}
#endif
