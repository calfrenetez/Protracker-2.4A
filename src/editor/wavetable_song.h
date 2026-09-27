#ifndef PT_WAVETABLE_SONG_H
#define PT_WAVETABLE_SONG_H
#include "wavetable_dispatch.h"
struct pt_wavetable_song;
enum pt_wavetable_song_result {
    PT_WAVETABLE_SONG_OK, PT_WAVETABLE_SONG_DONE, PT_WAVETABLE_SONG_STOPPING,
    PT_WAVETABLE_SONG_INVALID, PT_WAVETABLE_SONG_RANGE, PT_WAVETABLE_SONG_CAPABILITY,
    PT_WAVETABLE_SONG_MEMORY, PT_WAVETABLE_SONG_STALE, PT_WAVETABLE_SONG_RENDER,
    PT_WAVETABLE_SONG_DEVICE, PT_WAVETABLE_SONG_PREPARING, PT_WAVETABLE_SONG_UPLOADING,
    PT_WAVETABLE_SONG_WAITING, PT_WAVETABLE_SONG_CLOCK, PT_WAVETABLE_SONG_DEADLINE
};
/* Serial owner-thread session. A successful open takes exclusive use of an
 * already-bound, idle voice owner/bridge until close (outer reservation remains
 * caller-owned). Do not use its direct trigger/dispatch/close APIs meanwhile.
 * Complete capability preflight occurs BEFORE source pins, uploads or starts.
 * Only slots restored at range start or triggered during output are master-pinned
 * (whole-song sessions pin all triggered slots). Promotion
 * may retain owned Fast copies in sampler.current even if a later open step
 * fails; sample content/precision/history remain unchanged. On failure *out and
 * voice ownership are unchanged; report receives capability analysis if run.
 * Project, sampler, voices, allocator and callback contexts must outlive close.
 * Project patterns/orders/metadata are BORROWED and MUST remain immutable;
 * stop/close before edits, replacement or sampler reinitialization. Sampler
 * generation, table/header/bridge/backend/reservation identities and current
 * resource ownership are checked on every step without rescanning master PCM;
 * these guards do not detect in-place writes to pattern/order/sample arrays.
 * Ready dispatch/restore uses private current-version source callbacks without
 * project-wide or selected-value rescans. Exact prepared pins cover uploads;
 * bounded shape checks remain. Public bridge/dispatch validation stays unchanged.
 * Options/format are copied. Row-range playback requires an explicit exact-restore
 * callback; missing support returns RANGE. No native device/scheduler. */
enum pt_wavetable_song_result pt_wavetable_song_open(struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_playback_format *,const struct pt_allocator *,
    struct pt_wavetable_preflight_report *,struct pt_wavetable_song **out);
/* Begin claims the idle owner and returns PREPARING with a published handle.
 * Next/consume/complete return PREPARING without output until prepare returns OK.
 * Each prepare validates current generation/header/bridge, then performs one
 * analysis step, reserves one selected master or copies at most 4096 PCM/marker
 * bytes AFTER full capability success, or transfers the audited sequence without
 * remeasurement. Partial masters remain unpublished and cancel releases their
 * reservation; complete versions publish atomically. Static input validation/sync
 * at begin, allocator calls and metadata resets remain synchronous: no hard
 * latency guarantee. The ownership predicate is called; no uploads/voice callbacks during preparation.
 * Close cancels pending/failed preparation and leaves the idle voice owner/bridge
 * bound for caller reuse/close (outer reservation retained). Failure poisons the
 * handle; close still required. Already-promoted unchanged sampler copies may
 * remain. Caller must cancel before any borrowed source/project edit; the editor
 * binding supplies that barrier. Successful synchronous open is a wrapper. */
enum pt_wavetable_song_result pt_wavetable_song_begin(struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_playback_format *,const struct pt_allocator *,
    struct pt_wavetable_preflight_report *,struct pt_wavetable_song **out);
enum pt_wavetable_song_result pt_wavetable_song_prepare(struct pt_wavetable_song *,struct pt_wavetable_preflight_report *);
/* next -> consume elapsed frames (1..256 each, exactly interval.frames) ->
 * complete. A zero-frame/end interval still requires complete. Caller schedules
 * time for emit=1; emit=0 pre-roll advances silently without waiting or any voice
 * dispatch. consume only advances software phase, never waits or generates audio.
 * For ranges, next restores the exact snapshot ONCE at the first emit=1 interval,
 * before returning its frames. Thus next may upload/invoke restore callbacks and
 * fail with DEVICE. Even a final emitting interval must restore before consuming.
 * Synchronous complete applies one plan with internal256-byte staging. Natural end requests
 * confirmed stop and returns DONE or STOPPING. No later interval can restart it.
 * Invalid protocol is refused without advancement. Stale/internal/device errors
 * poison playback and attempt bounded stop; close must still complete. A failed
 * dispatch can make its own cleanup stop attempt before the session stop pass. */
enum pt_wavetable_song_result pt_wavetable_song_next(struct pt_wavetable_song *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_wavetable_song_consume(struct pt_wavetable_song *,uint32_t frames);
enum pt_wavetable_song_result pt_wavetable_song_complete(struct pt_wavetable_song *);
/* Yielding alternatives to next/complete. Repeat the SAME operation while it
 * returns UPLOADING. Each call begins one selected cache acquisition OR uploads
 * <=256bytes OR commits an entirely acquired batch. No voice callbacks before all
 * required leases/descriptors/addresses pass. No following interval/consume can
 * advance while uploading; next output stays unchanged. Calling the other step
 * while pending returns UPLOADING without work. Cancel via close/Stop/edit barrier.
 * Initial allocation and final bounded action batch/callbacks remain synchronous;
 * this is not real-time scheduling. Existing next/complete drive matching steps
 * synchronously. Values/project remain immutable and owner-thread serialized. */
enum pt_wavetable_song_result pt_wavetable_song_next_step(struct pt_wavetable_song *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_wavetable_song_complete_step(struct pt_wavetable_song *);
/* Split next-step: prepare selects the interval and, for range startup, acquires
 * its restore leases in bounded steps WITHOUT voice callbacks. UPLOADING leaves
 * out unchanged; OK publishes the staged interval, repeatable without advancing
 * time or doing more upload work. No consume/complete/clock-arm before commit.
 * Commit requires readiness, revalidates source/cache identities, invokes any
 * exact restore once WITHOUT upload/allocation, and makes the interval pending.
 * Early/double commit refuses. Cancel via close/editor barrier. Legacy next_step
 * wraps prepare+commit; callers choosing the split protocol must not call that
 * wrapper until ready to start. No clock/deadline is implied by commit itself.
 * Whole-song zero-frame commands then use prefetch+complete separately; merely
 * preparing or committing a zero interval does not dispatch its commands. */
enum pt_wavetable_song_result pt_wavetable_song_next_prepare(struct pt_wavetable_song *,struct pt_render_interval *);
enum pt_wavetable_song_result pt_wavetable_song_next_commit(struct pt_wavetable_song *);
/* Optional lookahead after next succeeds: repeatedly prefetch while the current
 * interval elapses. Computes upcoming commands on private state in <=256-frame
 * steps, then prepares only that batch's caches in <=256-byte upload steps.
 * UPLOADING means more work; OK means ready, with NO voice callbacks/early starts.
 * Unlike after-boundary uploading, consume may advance the CURRENT interval
 * while prefetch is pending. Complete refuses until all interval frames elapsed;
 * complete_step then checks/commits ready state without conversion or allocation.
 * If not ready at completion, complete_step continues bounded preparation and
 * returns UPLOADING; a real scheduler must separately enforce its late policy.
 * No prefetch during silent range pre-roll or pending range restoration. Stop/
 * edit cancels both forecast and uploads. Real-time deadlines remain caller-owned. */
enum pt_wavetable_song_result pt_wavetable_song_prefetch(struct pt_wavetable_song *);
/* Optional strict clock gate for ONE positive emitting interval already returned
 * by next, with no frames consumed. start_frame is an injected absolute frame
 * timestamp in options.rate units. Sources/clock/callbacks are owner-thread,
 * non-reentrant. Arm starts no voice and refuses zero/silent/restoration intervals.
 * An unrepresentable deadline poisons playback with CLOCK and requests stop.
 * While armed, direct next/consume/complete refuse; prefetch remains permitted.
 * service timestamps must be monotonic. Before deadline each call consumes at
 * most256 elapsed frames and performs one bounded prefetch step, returning WAITING.
 * Repeated timestamps allow phase debt/preparation to catch up without inventing
 * elapsed time. At EXACT deadline, prefetch must ALREADY be ready and phase debt
 * <=256; then it commits once without upload/allocation and disarms. Late service
 * or unfinished work returns DEADLINE and requests stop, never a late trigger.
 * Regression returns CLOCK and requests stop. Failed/unconfirmed stops retain
 * existing voice/master ownership until close succeeds. Caller owns subsequent
 * intervals, initial zero-frame commands and range pre-roll/restoration; this is
 * a strict interval gate, not a native event loop or real-time timing guarantee. */
enum pt_wavetable_song_result pt_wavetable_song_clock_arm(struct pt_wavetable_song *,uint64_t start_frame);
enum pt_wavetable_song_result pt_wavetable_song_clock_service(struct pt_wavetable_song *,uint64_t now_frame);
/* Whole-song owner-thread scheduler with an injected absolute frame clock.
 * Begin only immediately after prepare/open, before any next; start must leave
 * room for the preflight's total frame bound (including silent pre-roll).
 * Step BEFORE start performs one bounded startup/pre-roll/cache operation and
 * returns WAITING, or OK when primed. It writes the next absolute deadline on
 * success. At start it requires ALREADY primed; no callbacks before that time.
 * During playback WAITING includes prefetch/elapsed-phase work. Exact boundaries
 * dispatch only ready plans and arm the following positive interval in the SAME
 * call, preserving absolute phase. DONE/STOPPING/error uses existing close rules.
 * Repeated timestamps may service work; decreasing time/overflow fails CLOCK,
 * late/unready deadlines fail DEADLINE and stop. No sleeping/clock read or catch-up
 * trigger. Other advancement APIs refuse while scheduled; close/editor barriers
 * remain available. Start sufficiently in the future for bounded pre-roll/jobs.
 * Caller must keep polling before and AT the returned frame deadlines. Timestamp
 * sampling and callbacks are non-reentrant/synchronous: no hardware latency or
 * real-time guarantee, and no native PLAY/card binding is enabled here. */
enum pt_wavetable_song_result pt_wavetable_song_schedule_begin(struct pt_wavetable_song *,uint64_t start_frame);
enum pt_wavetable_song_result pt_wavetable_song_schedule_step(struct pt_wavetable_song *,uint64_t now_frame,uint64_t *deadline);
/* Optional sampled-clock service adapter. read returns1 and a monotonic64-bit
 * tick counter plus its stable ticks/second frequency. Callback/context must
 * outlive close, never block/reenter/edit, and perform no device voice action.
 * Begin reads once to establish frame zero and schedules start_delay frames in
 * the future. Service reads exactly once, converts with retained fractional
 * carry, then performs ONE schedule_step. Counter wrap/regression, frequency
 * change, conversion overflow or failed read returns CLOCK and requests stop;
 * uncertain leases remain retained. No rebasing/retry of failed clocks.
 * Raw schedule_step refuses while bound. Returned deadline stays in frame units;
 * caller wakeups/timer ownership, clock accuracy and callback duration remain
 * separate. This adapter does not sleep or enable a native device/PLAY action. */
typedef int (*pt_wavetable_clock_read)(void *,uint64_t *ticks,uint32_t *frequency);
enum pt_wavetable_song_result pt_wavetable_song_clocked_begin(struct pt_wavetable_song *,uint64_t start_delay,pt_wavetable_clock_read,void *);
enum pt_wavetable_song_result pt_wavetable_song_clocked_service(struct pt_wavetable_song *,uint64_t *deadline);
/* Translate the current scheduled deadline to the first counter tick that
 * reaches it, without reading the clock or advancing playback. Unrepresentable
 * deadline fails CLOCK and stops. The caller must still service before/at that
 * boundary; this is not a wakeup guarantee or late-service tolerance. */
enum pt_wavetable_song_result pt_wavetable_song_clocked_deadline(struct pt_wavetable_song *,uint64_t *ticks);
/* One stop attempt per held voice; no polling. Retains ALL master pins/controller
 * and unconfirmed device leases until all stops and bridge detach succeed.
 * Returns0 while unresolved; caller must retain/retry *song. On success frees
 * controller, nulls *song and returns1. NULL *song is already closed. */
int pt_wavetable_song_close(struct pt_wavetable_song **song);
#endif
