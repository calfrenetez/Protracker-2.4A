#ifndef PT_NATIVE_EDITOR_MIXED_H
#define PT_NATIVE_EDITOR_MIXED_H
#include "mixed_transport.h"
#include "../editor/editor_mixed.h"
/* Serialize on the editor thread. Native storage is zero-init/noncopyable and
 * disjoint from binding/editor/project storage. Opens two PRIVATE OS2+ timer
 * requests, then delegates adoption to the editor binding. No alarm submission
 * until editor_mixed_service; no global TimerBase, output, IRQ or UI installed.
 * Once pump.active, even on start failure, binding stop/change/dispose owns all
 * cleanup; native storage and all contexts survive until stop returns1.
 * Before adoption failure closes only these idle private requests, preserving
 * the sample owner for the binding's explicit stop. Never use direct pump/owner
 * operations while adopted. Editor frontend event-loop wiring remains separate. */
static inline enum pt_mixed_owner_result pt_native_editor_mixed_start(
    struct pt_native_mixed_transport *n,struct pt_editor_mixed *o,uint64_t delay,uint32_t quantum)
{
    struct pt_mixed_timer_api api;enum pt_mixed_owner_result r;
    if(!n || n->pump.active || n->clock.port || n->clock.request || n->alarm.port || n->alarm.request ||
       !o || !o->editor || !o->owner || o->transport || !quantum || quantum>256)
        return PT_MIXED_OWNER_INVALID;
    if(!pt_native_eclock_open(&n->clock))return PT_MIXED_OWNER_DEVICE;
    if(!pt_native_alarm_open(&n->alarm)){pt_native_eclock_close(&n->clock);return PT_MIXED_OWNER_DEVICE;}
    api=(struct pt_mixed_timer_api){n,pt_native_mixed_read,pt_native_mixed_poll,pt_native_mixed_arm,
        pt_native_mixed_alarm_close,pt_native_mixed_counter_close,pt_native_mixed_signal};
    r=pt_editor_mixed_start(o,&n->pump,delay,quantum,&api);
    if(!n->pump.active) {
        /* No service/arm has run, so both requests are idle and close immediately. */
        pt_native_alarm_close(&n->alarm);pt_native_eclock_close(&n->clock);
    }
    return r;
}
#endif
