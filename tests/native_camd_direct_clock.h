#ifndef PT_TEST_NATIVE_CAMD_DIRECT_CLOCK_H
#define PT_TEST_NATIVE_CAMD_DIRECT_CLOCK_H

#include "native_cia_countdown.h"

enum { CAMD_DIRECT_GROUPS = 8, CAMD_GROUP_ROWS = 4, MINIMUM_COHERENT_ROWS = 20, MAX_DIRECT_TICKS = 60000 };
_Static_assert(PT_CAMD_PUT == -138, "Timed CAMD PutMidi vector");

extern void pt_camd_direct_clock_baseline(struct Library *, void *, ULONG,
    volatile UBYTE *, volatile UBYTE *, struct pt_cia_clock_pair *);
extern void pt_camd_direct_clock_put(struct Library *, void *, ULONG,
    volatile UBYTE *, volatile UBYTE *, struct pt_cia_clock_pair *);

struct camd_direct_row {
    struct pt_cia_clock_pair baseline, put;
    ULONG bytes, stamp;
    unsigned received;
};

struct camd_direct_group {
    uint64_t first_tick, last_tick;
    uint32_t hz;
    unsigned chip, timer, attempted, coherent;
    struct camd_direct_row rows[CAMD_GROUP_ROWS];
};

static void report_camd_clock(const struct pt_cia_clock_pair *pair)
{
    printf("%02x%02x%02x/%02x%02x%02x", (unsigned)pair->before[0], (unsigned)pair->before[1],
        (unsigned)pair->before[2], (unsigned)pair->after[0], (unsigned)pair->after[1], (unsigned)pair->after[2]);
}

static int capture_camd_group(struct Library *library, void *sender, void *receiver, ULONG *stamp,
    struct pt_native_eclock *timebase, unsigned group_index, struct camd_direct_group *group)
{
    struct pt_diagnostic_cia clock = {0};
    volatile ULONG stray = 0;
    uint32_t last_hz = 0;
    int complete = 0;

    if (!pt_native_eclock_read(timebase, &group->first_tick, &group->hz) ||
        (group->hz != 709379 && group->hz != 715909) ||
        !pt_diagnostic_cia_acquire(&clock, pt_cia_direct_clock_stray, (void *)&stray) ||
        !pt_cia_start_masked_countdown(&clock))
        goto finish;

    for (unsigned i = 0; i < CAMD_GROUP_ROWS; ++i) {
        const unsigned index = group_index * CAMD_GROUP_ROWS + i;
        const uint32_t packed = index & 1 ? UINT32_C(0x863c0000) : UINT32_C(0x963c6400);
        const ULONG marker = UINT32_C(0xcad10000) | index;
        uint32_t message[2] = {0};
        if (!pt_diagnostic_cia_current(&clock) || !(*clock.control & 1U))
            goto finish;
        struct camd_direct_row *row = group->rows + group->attempted++;
        *stamp = marker;
        pt_camd_direct_clock_baseline(library, sender, packed, clock.high, clock.low, &row->baseline);
        pt_camd_direct_clock_put(library, sender, packed, clock.high, clock.low, &row->put);
        *stamp = ~marker;
        if (!(BOOL)pt_native_camd_vector(library, PT_CAMD_GET, 0, receiver, message))
            goto finish;
        row->bytes = message[0];
        row->stamp = message[1];
        row->received = 1;
        if (row->bytes >> 8 != packed >> 8 || row->stamp != marker ||
            (UBYTE)pt_native_camd_vector(library, PT_CAMD_GET_ERROR, 0, receiver, NULL) ||
            row->baseline.marker != 0x2452 || row->put.marker != 0x2452)
            goto finish;
        const unsigned before = pt_cia_stable_count(row->baseline.before);
        const unsigned baseline = pt_cia_stable_count(row->baseline.after);
        const unsigned start = pt_cia_stable_count(row->put.before);
        const unsigned stop = pt_cia_stable_count(row->put.after);
        if (before && baseline && start && stop) {
            if (before <= baseline || baseline <= start || start <= stop)
                goto finish;
            ++group->coherent;
        }
    }
    complete = (*clock.control & 1U) && pt_native_eclock_read(timebase, &group->last_tick, &last_hz) &&
        group->hz == last_hz && group->last_tick > group->first_tick &&
        group->last_tick - group->first_tick < MAX_DIRECT_TICKS;

finish:
    group->chip = clock.chip;
    group->timer = clock.bit;
    if (!pt_diagnostic_cia_close(&clock)) {
        puts("CAMD direct HOLD: original timer and MIDI resources retained");
        fflush(stdout);
        for (;;)
            Delay(50);
    }
    return complete && !stray;
}

static int measure_direct(struct Library *library, void *sender, void *receiver, ULONG *stamp)
{
    struct pt_native_eclock timebase = {0};
    struct camd_direct_group groups[CAMD_DIRECT_GROUPS] = {0};
    unsigned attempted = 0, completed = 0, coherent = 0;
    if (pt_native_eclock_open(&timebase)) {
        for (unsigned i = 0; i < CAMD_DIRECT_GROUPS; ++i) {
            attempted = i + 1;
            if (!capture_camd_group(library, sender, receiver, stamp, &timebase, i, groups + i))
                break;
            coherent += groups[i].coherent;
            ++completed;
        }
    }
    pt_native_eclock_close(&timebase);
    for (unsigned i = 0; i < attempted; ++i) {
        const struct camd_direct_group *group = groups + i;
        printf("CAMD direct group %u hz=%lu timer=%u/%u start=%08lx:%08lx end=%08lx:%08lx attempted=%u coherent=%u\n",
            i, (unsigned long)group->hz, group->chip, group->timer,
            (unsigned long)(group->first_tick >> 32), (unsigned long)group->first_tick,
            (unsigned long)(group->last_tick >> 32), (unsigned long)group->last_tick, group->attempted, group->coherent);
        for (unsigned j = 0; j < group->attempted; ++j) {
            const struct camd_direct_row *row = group->rows + j;
            printf("CAMD direct row %u baseline=", i * CAMD_GROUP_ROWS + j);
            report_camd_clock(&row->baseline);
            printf(" put=");
            report_camd_clock(&row->put);
            printf(" message=%06lx stamp=%08lx received=%u\n",
                (unsigned long)(row->bytes >> 8), (unsigned long)row->stamp, row->received);
        }
    }
    return completed == CAMD_DIRECT_GROUPS && coherent >= MINIMUM_COHERENT_ROWS && !timebase.opened ? 0 : 20;
}
#endif
