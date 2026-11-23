#ifndef PT_COUNTDOWN_WINDOW_H
#define PT_COUNTDOWN_WINDOW_H

#include <stdint.h>

struct pt_countdown_window {
    uint64_t first, end;
    uint16_t anchor_count, first_count, end_count;
};

enum pt_countdown_status {
    PT_COUNTDOWN_INVALID, PT_COUNTDOWN_EARLY, PT_COUNTDOWN_INSIDE, PT_COUNTDOWN_EXPIRED
};

struct pt_countdown_bracket { uint64_t first, last; };

struct pt_countdown_observation {
    uint64_t tick;
    uint32_t reloads;
    uint16_t count;
};

/* Extend an observation of a continuous 16-bit countdown that reloads to
 * UINT16_MAX. The caller proves that the coherent count and independently
 * observed reload number belonged to the same instant and original owner.
 * Equal observations are allowed. Counter regressions, reload number wrap
 * and absolute tick overflow refuse. The original observation may be updated
 * in place and is preserved on failure. */
int pt_countdown_extend(struct pt_countdown_observation *, uint32_t reloads, uint16_t count);

/* Lower an existing half-open tick window for one already observed countdown.
 * anchor_tick and anchor_count are the same actual instant. The countdown
 * must keep its original ownership, direction and frequency and must not
 * wrap or be armed again. This function does not establish those facts.
 * Keep the whole window above zero and preserve output on refusal. */
int pt_countdown_window_prepare(uint64_t anchor_tick, uint16_t anchor_count,
    uint64_t first, uint64_t end, struct pt_countdown_window *out);

/* Pass complete coherent samples from the same original bounded countdown. */
enum pt_countdown_status pt_countdown_window_status(const struct pt_countdown_window *, uint16_t count);

/* Convert two in-window samples around an observed write back to the original
 * clock ticks. Preserve out if either sample is outside or time regresses. */
int pt_countdown_window_bracket(const struct pt_countdown_window *, uint16_t first_count,
    uint16_t last_count, struct pt_countdown_bracket *out);

#endif
