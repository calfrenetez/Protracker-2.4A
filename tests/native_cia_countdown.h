#ifndef PT_TEST_NATIVE_CIA_COUNTDOWN_H
#define PT_TEST_NATIVE_CIA_COUNTDOWN_H

#include "native_cia_owner.h"
#include <stddef.h>

enum { PT_CIA_COUNTDOWN_MAX = 65535 };

struct pt_cia_clock_pair {
    uint8_t before[3], after[3];
    uint16_t marker;
};

typedef char pt_cia_clock_pair_size[(sizeof(struct pt_cia_clock_pair) == 8) ? 1 : -1];
typedef char pt_cia_clock_marker_offset[(offsetof(struct pt_cia_clock_pair, marker) == 6) ? 1 : -1];

void pt_cia_direct_clock_pair(volatile UBYTE *high, volatile UBYTE *low, struct pt_cia_clock_pair *);
void pt_cia_direct_clock_stray(void);

static inline unsigned pt_cia_stable_count(const uint8_t triplet[3])
{
    return triplet[0] == triplet[2] ? ((unsigned)triplet[0] << 8) | triplet[1] : 0;
}

static inline int pt_cia_start_masked_countdown(struct pt_diagnostic_cia *owner)
{
    if (!pt_diagnostic_cia_current(owner))
        return 0;
    /* Only this stopped, acquired interval timer was masked by acquisition.
     * Force-load one shot from E-clock without changing peripheral output,
     * serial mode or TOD configuration. The local observation must end early. */
    Disable();
    if (*owner->control != owner->saved_control) {
        Enable();
        return 0;
    }
    *owner->low = 0xff;
    *owner->high = 0xff;
    PT_CIA_CONTROL_WRITE(owner->control, (owner->saved_control & (owner->bit ? 0x80U : 0xc0U)) | 0x19U);
    Enable();
    return 1;
}
#endif
