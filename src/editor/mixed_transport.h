#ifndef PT_MIXED_TRANSPORT_H
#define PT_MIXED_TRANSPORT_H
#include "mixed_owner.h"
enum pt_mixed_timer_result {PT_MIXED_TIMER_ERROR=-1,PT_MIXED_TIMER_WAITING=0,
    PT_MIXED_TIMER_READY=1,PT_MIXED_TIMER_LATE=2};
/* Already-open PRIVATE timer/counter context. Each callback is bounded,
 * synchronous, non-reentrant and cannot mutate project/voice/transport state.
 * arm returns WAITING only after submission; other results submit nothing.
 * poll READY confirms completion, ERROR may retain pending I/O. alarm_close
 * cancels/polls once: return0 retains ALL request resources, return1 confirms
 * no pending I/O and releases them. counter_close runs only after both alarm
 * and sample owner are closed, return1 confirms release. Never wait on unfinished
 * I/O in a callback. signal returns only this private alarm's pending mask. */
struct pt_mixed_timer_api {
    void *context;pt_mixed_clock_read read;
    enum pt_mixed_timer_result (*poll)(void *),(*arm)(void *,uint64_t);
    int (*alarm_close)(void *),(*counter_close)(void *);
    uint32_t (*signal)(void *);
};
/* Zero-init once, noncopyable owner-thread pump. Valid begin adopts the already
 * open timer context AND exclusive control of *owner, even if clocked begin
 * subsequently fails. Invalid arguments transfer nothing. Caller owns storage
 * for handle/context/project/bridges until close returns1; all are disjoint from pump/API storage; no external owner
 * calls, edits or disposal while active. No native output/device binding here. */
struct pt_mixed_transport {
    struct pt_mixed_owner **owner,*captured;struct pt_mixed_timer_api timer;
    uint32_t quantum;uint64_t armed;
    unsigned active,pending,closing,alarm_closed,counter_closed,done;
    enum pt_mixed_owner_result failure;
};
enum pt_mixed_owner_result pt_mixed_transport_begin(struct pt_mixed_transport *,
    struct pt_mixed_owner **,uint64_t delay,uint32_t quantum,const struct pt_mixed_timer_api *);
/* At most ONE poll and ONE bounded owner service, then one wake/arm. PREPARING means
 * call again without sleeping; WAITING exposes the private signal for an event
 * loop. Exact wakeup is NOT guaranteed: service resamples honest elapsed time,
 * strict late/unprimed/skipped boundaries fail with no catch-up/rebase/retry.
 * Failure latches and attempts retained voice stops once through owner fault;
 * subsequent service never retries callbacks. DONE retains everything for close. */
enum pt_mixed_owner_result pt_mixed_transport_service(struct pt_mixed_transport *);
uint32_t pt_mixed_transport_signal(const struct pt_mixed_transport *);
/* One alarm-close attempt and one combined reader/barrier close attempt per
 * call. Preserve counter until BOTH confirm. Return0 requires retaining every
 * remaining context/storage and later explicit close; never force release. */
int pt_mixed_transport_close(struct pt_mixed_transport *);
#endif
