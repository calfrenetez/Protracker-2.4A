#ifndef PT_MIXED_OWNER_H
#define PT_MIXED_OWNER_H
#include "mixed_preflight.h"
#include "paula_voices.h"
struct pt_mixed_owner;
enum pt_mixed_owner_result {PT_MIXED_OWNER_OK,PT_MIXED_OWNER_PREPARING,
    PT_MIXED_OWNER_INVALID,PT_MIXED_OWNER_MEMORY,PT_MIXED_OWNER_CAPABILITY,PT_MIXED_OWNER_STALE,PT_MIXED_OWNER_DEVICE,PT_MIXED_OWNER_RENDER,PT_MIXED_OWNER_DONE,PT_MIXED_OWNER_CLOCK,PT_MIXED_OWNER_DEADLINE,PT_MIXED_OWNER_WAITING};
/* Combined master owner and numerical schedule; native output remains unfinished. Claim both
 * idle bound engines of the SAME sampler/project. Copies options/capabilities;
 * no source pins, cache allocation or output at begin. Ownership predicates
 * may run during guards. Public direct operations
 * refuse while owned. Arrays/PCM and engine/API/context identities immutable;
 * only channels.selected may change. Serialize, no callback edits/reentry.
 * Project, sampler, bridges, engines, reservation and allocators outlive close.
 * Stop/close before edit/undo/import/disposal; in-place writes are forbidden,
 * not detected. Failure preserves *out. No auto device/library reservation. */
enum pt_mixed_owner_result pt_mixed_owner_begin(struct pt_paula_voices *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    const struct pt_allocator *,struct pt_mixed_owner **);
/* Initial calls incrementally gate and retain ONE rewound sequence. One call
 * sets up analysis, then each advances <=256 measurement ticks, one next,
 * <=256 consumed frames, or one complete/backend-plan check. Cancel/close ends
 * unfinished analysis safely. No master pin/cache/output before full success.
 * Subsequent calls
 * reserve one master or copy <=4096 bytes. Pins union of both used-source masks
 * exactly once per slot; unused slots remain unpromoted. No derived copies or
 * output callbacks; live backend ownership predicates remain checked. Initial
 * validation/static scans/allocation remain synchronous, outside playback deadline;
 * bounded progress does not promise wall-clock latency.
 * Failure poisons handle; close still required. Completed unchanged promotions
 * may remain sampler-owned after cancel. Ready is NOT capacity/output evidence. */
enum pt_mixed_owner_result pt_mixed_owner_prepare(struct pt_mixed_owner *,struct pt_mixed_report *);
/* ONE retained shared sequence: next publishes a pending interval, consume
 * advances 1..256 declared elapsed frames, prefetch snapshots that same interval
 * and advances only its copied phase <=256 frames or one cache/conversion step.
 * Complete requires all live frames consumed AND both routes ready; it never
 * prepares synchronously, allocates, uploads or reconverts commands. Early
 * completion refuses, unready completion returns PREPARING without output.
 * Output interval is preserved on refusal/DONE. Prefetch emits no voice starts;
 * live consume may interleave serialized calls. DONE retains pins/readers until
 * close. Stale/render/cache/output failures poison session and attempt each held
 * stop once when API identities are safe; no retry/catch-up. Context/PCM immutability
 * and genuine elapsed-time reporting remain caller obligations. These manual
 * interval APIs provide no clock, native devices, repeats/segments or range restore. */
enum pt_mixed_owner_result pt_mixed_owner_next(struct pt_mixed_owner *,struct pt_render_interval *);
enum pt_mixed_owner_result pt_mixed_owner_consume(struct pt_mixed_owner *,uint32_t);
enum pt_mixed_owner_result pt_mixed_owner_prefetch(struct pt_mixed_owner *);
enum pt_mixed_owner_result pt_mixed_owner_complete(struct pt_mixed_owner *);
/* Numerical single-interval deadline gate, NOT actual clock/timing acceptance.
 * Arm a fresh full positive emitting pending interval at absolute FRAME start.
 * Service monotonic supplied frames, advance live debt <=256 and one forecast/
 * cache/conversion step before the boundary. At exact deadline BOTH routes must
 * already be ready and final debt <=256; commit does no preparation. No early
 * output. Regression/overflow -> CLOCK; late/unready/excess debt -> DEADLINE,
 * poison/cancel and bounded retained-reader stops. No catch-up/retry. Manual
 * next/consume/prefetch/complete/prepare refuse while armed. Completion disarms;
 * caller arms each next interval separately. Whole-song numerical schedule below;
 * sampled-reader binding below; native timer/device output and sound/performance remain unfinished. */
enum pt_mixed_owner_result pt_mixed_owner_clock_arm(struct pt_mixed_owner *,uint64_t start);
enum pt_mixed_owner_result pt_mixed_owner_clock_service(struct pt_mixed_owner *,uint64_t now);
/* Whole-song numerical schedule: begin requires prepared, unvisited owner.
 * Validates start+full measured frames before output. Pre-start steps prime the
 * startup batch without starts; OK means primed. At exact start apply ONLY primed
 * startup and arm next positive interval; each exact later boundary commits ready
 * batch and arms the next from that absolute boundary. WAITING writes next absolute
 * FRAME deadline; errors/DONE preserve it. Manual interval/preparation APIs refuse
 * while scheduled. NULL deadline refuses without advancing. Regression/overflow
 * -> CLOCK; late/unprimed start/live deadline -> DEADLINE; cancel/stop/retain as
 * above, no catch-up/retry. Caller supplies honest numerical time. No actual clock,
 * native event-loop/timer/DMA/device/audio or physical timing acceptance here. */
enum pt_mixed_owner_result pt_mixed_owner_schedule_begin(struct pt_mixed_owner *,uint64_t start);
enum pt_mixed_owner_result pt_mixed_owner_schedule_step(struct pt_mixed_owner *,uint64_t now,uint64_t *deadline);
/* Bind a serialized immutable monotonic counter reader to this same schedule.
 * Delay is sample FRAMES from the first successful observation (new epoch).
 * Reader returns1 with ticks and stable nonzero ticks/second frequency. No edits,
 * reentry or clock rebasing. Fractional carry avoids per-poll rounding drift.
 * service reports next FRAME deadline; deadline converts it to the first counter
 * tick at/after it. Low frequencies may skip exact frames: strict schedule then
 * refuses DEADLINE, never catches up. Read/frequency/regression/overflow faults
 * poison CLOCK and cancel/stop retained readers once. Outputs unchanged on errors
 * or DONE; DONE never reads the counter again. NULL outputs refuse without reads.
 * Numerical/manual APIs refuse once bound. This adapter performs no timer I/O,
 * waits or actual device output; injected counters do not prove native timing. */
typedef int (*pt_mixed_clock_read)(void *,uint64_t *ticks,uint32_t *frequency);
enum pt_mixed_owner_result pt_mixed_owner_clocked_begin(struct pt_mixed_owner *,uint64_t delay,pt_mixed_clock_read,void *);
enum pt_mixed_owner_result pt_mixed_owner_clocked_service(struct pt_mixed_owner *,uint64_t *frame_deadline);
enum pt_mixed_owner_result pt_mixed_owner_clocked_deadline(struct pt_mixed_owner *,uint64_t *tick_deadline);
/* Detect generation/header/API/map/backend changes without bulk PCM scans. */
enum pt_mixed_owner_result pt_mixed_owner_current(struct pt_mixed_owner *);
/* Cancel partial promotion and sequence, block both engines, attempt each reader
 * stop and barrier once. Retain ALL master pins and both ownership tokens until
 * BOTH drains confirm, including interrupt quiescence. Retry pending/error;
 * never force release. Failure leaves handle. Success frees handle, leaves both
 * engines bound but closing: caller closes/rebinds them separately. API/context
 * identities must remain/restored to captured values for safe cleanup. No wait,
 * cache detach, library close, native DMA or hardware acceptance here. */
int pt_mixed_owner_close(struct pt_mixed_owner **);
#endif
