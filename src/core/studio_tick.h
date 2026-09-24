#ifndef PT_STUDIO_TICK_H
#define PT_STUDIO_TICK_H
#include "studio_mix.h"
struct pt_studio_tick;
/* Owner-thread interval reader, borrowing a mixer which must outlive it.
 * Do not read the mixer directly while an interval is pending. Caller schedules
 * tracker commands at drained boundaries; tempo changes affect the NEXT begin.
 * Uses ideal-BPM Q32 timing, not CIA cycles or a wall-clock/deadline guarantee.
 * One bounded allocation; closing frees only the reader, not mixer/master pins. */
struct pt_studio_tick *pt_studio_tick_open(const struct pt_allocator *,struct pt_studio_mix *,uint64_t frame_limit);
void pt_studio_tick_close(struct pt_studio_tick *);
/* Refuses invalid BPM, undrained interval or exhausted total frame budget.
 * Failure leaves interval/fractional timing unchanged. */
enum pt_pcm_result pt_studio_tick_begin(struct pt_studio_tick *,unsigned bpm);
uint32_t pt_studio_tick_remaining(const struct pt_studio_tick *);
/* output->frames must be1..min(256,remaining). Successful read consumes exactly
 * those frames; failed mixer read consumes none. Output/clipped contracts are
 * studio_read's. No allocation. No automatic command dispatch or device queue. */
enum pt_pcm_result pt_studio_tick_read(struct pt_studio_tick *,struct pt_pcm *,uint64_t *clipped);
#endif
