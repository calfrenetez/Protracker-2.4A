#ifndef PT_PAULA_SONG_H
#define PT_PAULA_SONG_H
#include "paula_dispatch.h"
struct pt_paula_song;
enum pt_paula_song_result {PT_PAULA_SONG_OK,PT_PAULA_SONG_PREPARING,PT_PAULA_SONG_DONE,
    PT_PAULA_SONG_INVALID,PT_PAULA_SONG_CAPABILITY,PT_PAULA_SONG_MEMORY,
    PT_PAULA_SONG_STALE,PT_PAULA_SONG_RENDER,PT_PAULA_SONG_DEVICE,
    PT_PAULA_SONG_WAITING,PT_PAULA_SONG_CLOCK,PT_PAULA_SONG_DEADLINE};
/* Claim an idle bound voice owner; publish a cancellable handle, no pins/output.
 * Options/caps copied. Selected audio tracks must all route to Paula; other
 * tracks remain in the full16-track shared global flow. Mixed selected outputs,
 * range restore, repeat/segment and native scheduling are not supported here.
 * Inputs/contexts outlive successful close. Stop/close BEFORE any source edit,
 * undo, replacement or sampler disposal. Patterns/orders/sample arrays remain
 * borrowed immutable; header/generation guards do not detect in-place writes.
 * Allocator/device callbacks serialized, non-reentrant and must not edit.
 * Public direct voice/dispatch/close operations refuse while session owns it. */
enum pt_paula_song_result pt_paula_song_begin(struct pt_paula_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,
    const struct pt_allocator *,struct pt_paula_song **);
/* First prepare performs full silent capability traversal bounded by options'
 * tick/frame limits, then retains the SAME rewound sequence. Subsequent calls
 * reserve one selected master or copy <=4096 PCM/marker bytes. All used masters
 * stay pinned before ready/output; unused slots are not promoted. Setup,
 * traversal and allocations are synchronous, not a hard latency guarantee.
 * Failures poison handle, request bounded stops, and still require close.
 * Already-promoted unchanged masters may remain sampler-owned after cancellation. */
enum pt_paula_song_result pt_paula_song_prepare(struct pt_paula_song *,struct pt_paula_preflight_report *);
/* next -> consume elapsed frames1..256 until exact interval length -> complete.
 * Zero/end intervals still require complete. Complete applies prepared Paula
 * batch only after preceding interval; no sleep, PCM mixing or clock read.
 * Bad protocol refuses unchanged. Runtime errors discard sequence, block retry
 * and retain master/cache ownership until confirmed close. Caller owns timing.
 * DONE also requires close; it is not confirmation of hardware quiescence. */
enum pt_paula_song_result pt_paula_song_next(struct pt_paula_song *,struct pt_render_interval *);
enum pt_paula_song_result pt_paula_song_consume(struct pt_paula_song *,uint32_t);
/* Optional prepare-ahead on the SAME pending shared interval, including while
 * live frames remain. Each call snapshots, advances a private command copy by
 * <=256 frames, resolves its plan, or advances one bounded cache preparation
 * step. No live consume or device callbacks. Consume may proceed serially.
 * Ready is idempotent; complete refuses early/incomplete preparation and commits
 * lookahead only after exact live consumption, then applies without conversion.
 * Cannot mix a started ordinary stage with prefetch. Close/error cancels both.
 * Caller still owns monotonic timing/deadlines; no clock scheduler is provided. */
enum pt_paula_song_result pt_paula_song_prefetch(struct pt_paula_song *);
/* Optional stage after logical consume finishes, before complete emits output.
 * Advances/checks the plan, then each call reserves one candidate OR copies
 * <=256 output bytes OR advances one action without callbacks. PREPARING requires
 * another stage; complete refuses output while preparation is pending. Stage
 * once ready is idempotent. Allocation/eviction remain synchronous. Complete
 * applies a staged batch without cache acquisition/conversion. Calling complete
 * without stage preserves the synchronous compatibility path. Caller owns real
 * time/deadlines; this does not provide a prepare-ahead clock scheduler. Close
 * or failure cancels unstarted candidates before ordinary reader cleanup. */
enum pt_paula_song_result pt_paula_song_stage(struct pt_paula_song *);
enum pt_paula_song_result pt_paula_song_complete(struct pt_paula_song *);
/* Strict single-interval gate using caller-supplied absolute elapsed frame time.
 * Arm a positive emitting pending interval before live consumption; optional
 * prefetch may already be underway. Check start+frames overflow. While armed,
 * direct next/prepare/consume/prefetch/stage/complete refuse; close remains valid.
 * Service advances <=256 elapsed frames and one preparation step before deadline.
 * Same timestamp may be serviced repeatedly. At EXACT start+interval.frames,
 * forecast must ALREADY be ready and remaining frame debt<=256: commit/apply only,
 * no cache preparation/allocation. Returns OK/DONE at boundary, WAITING earlier.
 * Regression/overflow -> CLOCK; late/unready/excess debt -> DEADLINE. Poison once,
 * cancel unstarted candidates and request stops, retaining uncertain ownership
 * until close confirms cleanup. No catch-up output or retry. No clock reads,
 * sleeps, native timers or whole-song scheduler; caller supplies honest time. */
enum pt_paula_song_result pt_paula_song_clock_arm(struct pt_paula_song *,uint64_t);
enum pt_paula_song_result pt_paula_song_clock_service(struct pt_paula_song *,uint64_t);
/* Optional whole-song schedule after prepare, BEFORE any next. Absolute frame
 * start and start+measured total checked before output. Each pre-start step does
 * one next/lookahead/cache operation; OK means startup ready, WAITING more work.
 * At exact start only an already-ready initial batch may apply. Each exact
 * boundary applies its ready plan and arms the following positive interval in
 * the same call, preserving absolute phase. step returns WAITING with next
 * deadline; DONE still needs close. NULL deadline refuses unchanged; errors/DONE
 * preserve deadline output. Regression/late/unready poison without catch-up.
 * Manual interval APIs and prepare refuse while scheduled; close remains valid.
 * Repeated timestamps permitted for bounded preparation. No clock read/sleep or
 * native timing guarantee; borrowed arrays/callbacks remain immutable as above. */
enum pt_paula_song_result pt_paula_song_schedule_begin(struct pt_paula_song *,uint64_t);
enum pt_paula_song_result pt_paula_song_schedule_step(struct pt_paula_song *,uint64_t,uint64_t *);
/* One attempt: block advancement, stop held readers, confirm adapter quiescence,
 * close cache, then release sequence/jobs/master pins and session. Returns0 while
 * unresolved; retain *song and all contexts. No forced free or polling. Success
 * clears *song and leaves voice owner/bridge closed (must rebind to restart). */
int pt_paula_song_close(struct pt_paula_song **);
#endif
