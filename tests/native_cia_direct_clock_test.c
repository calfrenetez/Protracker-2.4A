#include "../src/native/eclock.h"
#include "native_cia_countdown.h"
#include <proto/dos.h>
#include <stdio.h>

enum { PAIRS = 48, MARK = 0x2452 };

static volatile ULONG unexpected_irqs;

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
    struct pt_cia_clock_pair pairs[PAIRS] = {{{0}, {0}, 0}};
    uint64_t first = 0, last = 0;
    uint32_t hz = 0, end_hz = 0;
    unsigned captured = 0, invalid = 0, verified = 0, latest = PT_CIA_COUNTDOWN_MAX, bound_ok = 0;
    unsigned chip = 0, bit = 0;
    int rc = 20;

    if (!pt_native_eclock_open(&clock) ||
        !pt_diagnostic_cia_acquire(&owner, pt_cia_direct_clock_stray, (void *)&unexpected_irqs))
        goto finish;
    chip = owner.chip;
    bit = owner.bit;
    if (!pt_native_eclock_read(&clock, &first, &hz) || (hz != 709379 && hz != 715909) ||
        !pt_cia_start_masked_countdown(&owner))
        goto finish;

    for (; captured < PAIRS; ++captured) {
        Disable();
        pt_cia_direct_clock_pair(owner.high, owner.low, pairs + captured);
        Enable();
    }
    if (!pt_native_eclock_read(&clock, &last, &end_hz) || end_hz != hz || last <= first ||
        last - first >= PT_CIA_COUNTDOWN_MAX || !(*owner.control & 1U))
        goto finish;

    bound_ok = 1;
    for (unsigned i = 0; i < captured; ++i) {
        const struct pt_cia_clock_pair *pair = pairs + i;
        const unsigned before = pt_cia_stable_count(pair->before);
        const unsigned after = pt_cia_stable_count(pair->after);
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
    if (verified >= PAIRS / 2 && PT_CIA_COUNTDOWN_MAX - latest <= last - first)
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
        const struct pt_cia_clock_pair *p = pairs + i;
        printf("CIA pair %02u %02x%02x%02x %02x%02x%02x %04x\n", i,
            (unsigned)p->before[0], (unsigned)p->before[1], (unsigned)p->before[2],
            (unsigned)p->after[0], (unsigned)p->after[1], (unsigned)p->after[2], (unsigned)p->marker);
    }
    printf("CIA direct observed verified=%u invalid=%u stray=%lu closed=1 rc=%d\n",
        verified, invalid, (unsigned long)unexpected_irqs, rc);
    return rc;
}
