#include "../src/native/eclock.h"
#include "native_cia_owner.h"
#include <proto/dos.h>
#include <stdio.h>

enum { PAIRS = 48, COUNTDOWN = 65535, MARK = 0x2452 };

struct clock_pair {
    uint8_t before[3], after[3];
    uint16_t marker;
};

typedef char clock_pair_size[(sizeof(struct clock_pair) == 8) ? 1 : -1];
typedef char marker_offset[(offsetof(struct clock_pair, marker) == 6) ? 1 : -1];

extern void pt_cia_direct_clock_pair(volatile UBYTE *high, volatile UBYTE *low, struct clock_pair *);
extern void pt_cia_direct_clock_stray(void);
static volatile ULONG unexpected_irqs;

static unsigned stable_count(const uint8_t triplet[3])
{
    return triplet[0] == triplet[2] ? ((unsigned)triplet[0] << 8) | triplet[1] : 0;
}

static int start_masked_countdown(struct pt_diagnostic_cia *owner)
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

static int retain_owner(void) __attribute__((noreturn));
static int retain_owner(void)
{
    puts("CIA direct clock HOLD: original resource did not close");
    fflush(stdout);
    for (;;)
        Delay(50);
}

int main(void)
{
    struct pt_native_eclock clock = {0};
    struct pt_diagnostic_cia owner = {0};
    struct clock_pair pairs[PAIRS] = {{{0}, {0}, 0}};
    uint64_t first = 0, last = 0;
    uint32_t hz = 0, end_hz = 0;
    unsigned captured = 0, invalid = 0, verified = 0, latest = COUNTDOWN, bound_ok = 0;
    unsigned chip = 0, bit = 0;
    int rc = 20;

    if (!pt_native_eclock_open(&clock) ||
        !pt_diagnostic_cia_acquire(&owner, pt_cia_direct_clock_stray, (void *)&unexpected_irqs))
        goto finish;
    chip = owner.chip;
    bit = owner.bit;
    if (!pt_native_eclock_read(&clock, &first, &hz) || (hz != 709379 && hz != 715909) ||
        !start_masked_countdown(&owner))
        goto finish;

    for (; captured < PAIRS; ++captured) {
        Disable();
        pt_cia_direct_clock_pair(owner.high, owner.low, pairs + captured);
        Enable();
    }
    if (!pt_native_eclock_read(&clock, &last, &end_hz) || end_hz != hz || last <= first ||
        last - first >= COUNTDOWN || !(*owner.control & 1U))
        goto finish;

    bound_ok = 1;
    for (unsigned i = 0; i < captured; ++i) {
        const struct clock_pair *pair = pairs + i;
        const unsigned before = stable_count(pair->before);
        const unsigned after = stable_count(pair->after);
        if (pair->marker != MARK)
            goto finish;
        if (!before || !after) {
            ++invalid;
            continue;
        }
        if (after > before || before > latest)
            goto finish;
        latest = after;
        ++verified;
    }
    if (verified >= PAIRS / 2 && COUNTDOWN - latest <= last - first)
        rc = 0;

finish:
    if (!pt_diagnostic_cia_close(&owner))
        return retain_owner();
    pt_native_eclock_close(&clock);
    if (unexpected_irqs || owner.held || owner.resource || clock.opened)
        rc = 20;
    printf("CIA direct h=%lu end_h=%lu first=%08lx:%08lx last=%08lx:%08lx chip=%u timer=%u captured=%u bound=%u\n",
        (unsigned long)hz, (unsigned long)end_hz,
        (unsigned long)(first >> 32), (unsigned long)first,
        (unsigned long)(last >> 32), (unsigned long)last, chip, bit, captured, bound_ok);
    for (unsigned i = 0; i < captured; ++i) {
        const struct clock_pair *p = pairs + i;
        printf("CIA pair %02u %02x%02x%02x %02x%02x%02x %04x\n", i,
            (unsigned)p->before[0], (unsigned)p->before[1], (unsigned)p->before[2],
            (unsigned)p->after[0], (unsigned)p->after[1], (unsigned)p->after[2], (unsigned)p->marker);
    }
    printf("CIA direct observed verified=%u invalid=%u stray=%lu closed=1 rc=%d\n",
        verified, invalid, (unsigned long)unexpected_irqs, rc);
    return rc;
}
