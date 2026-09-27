#ifndef PT_ELAPSED_CLOCK_H
#define PT_ELAPSED_CLOCK_H
#include <stdint.h>
enum pt_elapsed_result {PT_ELAPSED_OK,PT_ELAPSED_INVALID,PT_ELAPSED_REGRESSION,PT_ELAPSED_FREQUENCY,PT_ELAPSED_OVERFLOW};
/* Checked monotonic tick -> sample-frame conversion, no allocation or clock I/O.
 * Frequency is stable ticks/second; rate1..192000 is frames/second. Fractional
 * numerator carry prevents per-poll rounding drift. Tick wrap/regression,
 * frequency change and frame overflow latch a failure; never silently rebase.
 * Invalid init leaves state unchanged. Failed advance preserves output and
 * accumulated time (except failure latch). Output must not overlap this state.
 * Init deliberately starts a NEW epoch; never use it to recover live playback. */
struct pt_elapsed_clock {
    uint64_t ticks,frames;uint32_t frequency,rate,fraction;
    enum pt_elapsed_result failure;
};
enum pt_elapsed_result pt_elapsed_clock_init(struct pt_elapsed_clock *,uint32_t frequency,uint32_t rate,uint64_t ticks,uint64_t frames);
enum pt_elapsed_result pt_elapsed_clock_advance(struct pt_elapsed_clock *,uint32_t frequency,uint64_t ticks,uint64_t *frames);
#endif
