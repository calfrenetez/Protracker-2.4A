#ifndef PT_PAULA_SONG_H
#define PT_PAULA_SONG_H
#include "paula_dispatch.h"
struct pt_paula_song;
enum pt_paula_song_result {PT_PAULA_SONG_OK,PT_PAULA_SONG_PREPARING,PT_PAULA_SONG_DONE,
    PT_PAULA_SONG_INVALID,PT_PAULA_SONG_CAPABILITY,PT_PAULA_SONG_MEMORY,
    PT_PAULA_SONG_STALE,PT_PAULA_SONG_RENDER,PT_PAULA_SONG_DEVICE};
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
/* One attempt: block advancement, stop held readers, confirm adapter quiescence,
 * close cache, then release sequence/jobs/master pins and session. Returns0 while
 * unresolved; retain *song and all contexts. No forced free or polling. Success
 * clears *song and leaves voice owner/bridge closed (must rebind to restart). */
int pt_paula_song_close(struct pt_paula_song **);
#endif
