#ifndef PT_SCHEDULED_OUTPUT_H
#define PT_SCHEDULED_OUTPUT_H
#include <stddef.h>
#include "elapsed_clock.h"
struct pt_allocator;
#define PT_SCHEDULED_BATCHES 8
#define PT_SCHEDULED_ACTIONS 64
#define PT_SCHEDULED_SPANS 128
enum pt_scheduled_result {
    PT_SCHEDULED_OK,PT_SCHEDULED_PENDING,PT_SCHEDULED_UNSUPPORTED,
    PT_SCHEDULED_INVALID,PT_SCHEDULED_CAPACITY,PT_SCHEDULED_STALE,
    PT_SCHEDULED_CLOCK,PT_SCHEDULED_LATE,PT_SCHEDULED_BACKEND
};
enum pt_scheduled_kind {PT_SCHEDULED_TRIGGER,PT_SCHEDULED_CONTROL,PT_SCHEDULED_STOP};
/* Already resolved signed8 mono Paula geometry, not a master/conversion input.
 * Ordered actions in one batch share a single original musical boundary.
 * STOP has zero data/words/period/volume; CONTROL has zero data/words.
 * TRIGGER has an aligned retained cache address, words1..65535, period1..65535,
 * volume0..64. Backend still validates actual memory/device/pitch capabilities. */
struct pt_scheduled_action {
    enum pt_scheduled_kind kind;unsigned slot;
    const uint8_t *data;uint16_t words,period;uint8_t volume;
};
struct pt_scheduled_batch {
    uint64_t generation,frame;unsigned count;
    struct pt_scheduled_action action[PT_SCHEDULED_ACTIONS];
};
struct pt_scheduled_span {const void *data;size_t bytes;};
/* Task-prepared, uniquely held owner. It retains every required master version,
 * cache lease AND continuing reader needed by this event. spans includes full
 * master/cache capacities and mutable owner metadata, not just audible ranges.
 * current==1 confirms those original identities remain held and immutable.
 * release is called exactly once after successful transfer and confirmed reader
 * retirement, or for a never-submitted event during stop. No allocator, conversion,
 * cache acquisition or eviction is performed by this queue. Context outlives close.
 * Caller must not release/copy/reuse the token after enqueue succeeds. On refusal
 * caller retains it unchanged. Distinct live events require distinct owner tokens. */
struct pt_scheduled_owner {
    void *context;uint64_t token;
    int (*current)(void *,uint64_t,uint64_t);
    void (*release)(void *,uint64_t);
    const struct pt_scheduled_span *spans;unsigned count;
};
/* The explicit backend contract is required before allocating a queue. It is
 * never hardware qualification. Existing immediate audio.device/voice callbacks
 * cannot be substituted or wrapped by passing predicted timestamps as observations. */
#define PT_SCHEDULED_TIMESTAMPED 1U
#define PT_SCHEDULED_ATOMIC_PUBLICATION 2U
#define PT_SCHEDULED_READER_RETIREMENT 4U
#define PT_SCHEDULED_REQUIRED 7U
struct pt_scheduled_caps {unsigned flags,maximum_batches,maximum_actions;};
struct pt_scheduled_grid {uint64_t epoch,generation;uint32_t frequency,rate;};
/* Immutable until retirement. Admission is [first,last), computed on the original
 * epoch/frame grid with elapsed_clock. It is not one exact EClock tick. */
struct pt_scheduled_event {
    uint64_t ticket,first,last;struct pt_scheduled_batch batch;
};
enum pt_scheduled_completion {PT_SCHEDULED_EXECUTED,PT_SCHEDULED_CANCELLED,PT_SCHEDULED_FAILED};
struct pt_scheduled_receipt {
    enum pt_scheduled_completion completion;uint64_t observed,issued;
};
/* Task-side bounded/nonblocking publication/control, never an IRQ entry point.
 * read_clock returns actual counter/frequency. submit must publish the complete
 * immutable event atomically, validate the actual clock/generation at publication,
 * and arrange activation within its frame admission window independently of task
 * wakeup. 1 accepted,0 REFUSED WITH NO REFERENCES,-1 uncertain (may retain).
 * No automatic retry after uncertain submission. Backend may never edit event.
 * poll/cancel return1 ONLY when no future activation, DMA reader or callback can
 * reference any event/owner storage;0 pending,-1 uncertain failure. poll1 provides
 * actual observation/issue ticks for EXECUTED; invalid/late reports fail but reader
 * retirement still permits release. Cancellation1 proves retirement without timing.
 * All callbacks are serialized, do not reenter or edit borrowed inputs. Backend
 * owns task/interrupt exclusion and actual activation; neither is implemented here. */
struct pt_scheduled_backend {
    void *context;size_t context_bytes;struct pt_scheduled_caps caps;
    int (*read_clock)(void *,uint64_t *,uint32_t *);
    int (*submit)(void *,const struct pt_scheduled_event *);
    int (*poll)(void *,uint64_t,struct pt_scheduled_receipt *);
    int (*cancel)(void *,uint64_t);
};
struct pt_scheduled_output;
/* context_bytes names the backend's complete mutable control storage; outputs
 * may not alias it. Allocator-owned contexts obey pt_allocator's existing
 * disjoint-output/storage contract and must not alias the returned handle. */
enum pt_scheduled_result pt_scheduled_output_open(const struct pt_allocator *,
    const struct pt_scheduled_grid *,const struct pt_scheduled_backend *,unsigned,
    struct pt_scheduled_output **);
/* No backend submission. Copies complete descriptors/spans and transfers the
 * held owner once. Exact strictly increasing frame order; bounded full queue
 * refuses, never overwrites a submitted event. *ticket and all inputs/state are
 * unchanged on refusal, including aliases into full protected storage. */
enum pt_scheduled_result pt_scheduled_output_enqueue(struct pt_scheduled_output *,
    const struct pt_scheduled_batch *,const struct pt_scheduled_owner *,uint64_t *ticket);
/* Explicit task-side submission for one queued ticket. Must still be strictly
 * before its first admissible tick. No allocation/conversion/editor/voice calls
 * and no immediate-start fallback. Confirmed backend refusal preserves event;
 * uncertain submit latches failure and retains event until poll/cancel1. */
enum pt_scheduled_result pt_scheduled_output_publish(struct pt_scheduled_output *,uint64_t);
enum pt_scheduled_result pt_scheduled_output_poll(struct pt_scheduled_output *,uint64_t);
/* Closes publication permanently. At most one cancel per submitted ticket/call;
 * unsubmitted events release immediately. Pending/errors retain all uncertain
 * owners. Call stop again or poll explicitly; never discard a failed live queue. */
enum pt_scheduled_result pt_scheduled_output_stop(struct pt_scheduled_output *);
unsigned pt_scheduled_output_held(const struct pt_scheduled_output *);
/* No implicit cancellation. Returns0 with state/storage unchanged while held. */
int pt_scheduled_output_close(struct pt_scheduled_output *);
#endif
