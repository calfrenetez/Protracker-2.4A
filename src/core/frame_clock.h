#ifndef PT_FRAME_CLOCK_H
#define PT_FRAME_CLOCK_H
#include <stdint.h>
/* Explicit ideal-BPM reference clock: 2.5/BPM seconds per tick. This is not a
 * CIA latch/cycle model. Carries Q32 fractional frames across tempo changes.
 * Truncation is less than 2^-32 frame per tick; no float or allocation. */
struct pt_frame_clock {
    uint64_t frames,limit;
    uint32_t rate,fraction;
};
enum pt_clock_result { PT_CLOCK_OK, PT_CLOCK_INVALID, PT_CLOCK_LIMIT };
/* A nonzero rate <=192000 and explicit nonzero total-frame budget are required.
 * Failures leave clock/output unchanged. Integer totals are floor(accumulated
 * Q32 frames), never independently rounded tick lengths. */
enum pt_clock_result pt_frame_clock_init(struct pt_frame_clock *,uint32_t rate,uint64_t frame_limit);
enum pt_clock_result pt_frame_clock_tick(struct pt_frame_clock *,unsigned bpm,uint32_t *frames);
#endif
