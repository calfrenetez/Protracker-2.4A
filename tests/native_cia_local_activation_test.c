#include "../src/core/countdown_window.h"
#include "../src/core/elapsed_clock.h"
#include "../src/native/eclock.h"
#include "native_cia_countdown.h"
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

enum { POSITIVE = 12, RECORDS = POSITIVE + 2, RATE = 48000, MARK = 0x4752, ANCHOR_MARK = 0x2452 };

struct local_entry {
    uint16_t first_count, end_count, attempts, marker;
    uint8_t before[3], after[3];
};

struct local_result {
    struct pt_countdown_window window;
    struct local_entry raw;
    unsigned frame, status;
};

typedef char local_marker_offset[(offsetof(struct local_entry, marker) == 6) ? 1 : -1];
typedef char local_before_offset[(offsetof(struct local_entry, before) == 8) ? 1 : -1];
typedef char local_after_offset[(offsetof(struct local_entry, after) == 11) ? 1 : -1];

extern int pt_cia_local_activate(volatile UBYTE *high, volatile UBYTE *low, struct local_entry *);
static volatile ULONG unexpected_irqs;

#include "native_cia_local_alarm.h"

static int hold(void) __attribute__((noreturn));
static int hold(void)
{
    puts("CIA local window HOLD: original timer retained");
    fflush(stdout);
    for (;;)
        Delay(50);
}

static int prepare(struct local_result *row, const struct pt_elapsed_clock *grid, uint16_t anchor, unsigned frame)
{
    uint64_t first, end;
    if (pt_elapsed_clock_deadline(grid, frame, &first) != PT_ELAPSED_OK ||
        pt_elapsed_clock_deadline(grid, frame + 1, &end) != PT_ELAPSED_OK ||
        !pt_countdown_window_prepare(grid->ticks, anchor, first, end, &row->window))
        return 0;
    row->frame = frame;
    row->raw.first_count = row->window.first_count;
    row->raw.end_count = row->window.end_count;
    return 1;
}

static unsigned wait_near(const struct pt_diagnostic_cia *timer, uint16_t first)
{
    /* This wait runs with interrupts enabled and only approaches the original
     * bound. None of its observations can publish the marker. */
    for (unsigned i = 0; i < 16384; ++i) {
        uint8_t raw[3];
        raw[0] = *timer->high;
        raw[1] = *timer->low;
        raw[2] = *timer->high;
        unsigned count = pt_cia_stable_count(raw);
        if (count && count <= (unsigned)first + 80)
            return 1;
    }
    return 0;
}

static int issue(const struct pt_diagnostic_cia *timer, struct local_result *row)
{
    if (!pt_diagnostic_cia_current(timer))
        return 0;
    Disable();
    row->status = pt_cia_local_activate(timer->high, timer->low, &row->raw);
    Enable();
    return 1;
}

int main(int argc, char **argv)
{
    const int irq_mode = argc == 2 && !strcmp(argv[1], "irq");
    struct local_alarm alarm = {.signal = -1};
    struct local_alarm_example arm[RECORDS] = {0};
    struct pt_native_eclock clock = {0};
    struct pt_diagnostic_cia timer = {0};
    struct pt_elapsed_clock epoch;
    struct pt_cia_clock_pair anchor = {0};
    struct local_result row[RECORDS] = {0};
    uint64_t before = 0, after = 0;
    uint32_t hz = 0, end_hz = 0;
    unsigned count = 0, chip = 0, bit = 0, captured = 0, on_frame = 0, outside = 0, missed = 0;
    unsigned observation_complete = 0, attempted = 0;
    int rc = 20;

    if (argc != 1 && !irq_mode)
        goto finish;
    if (!pt_native_eclock_open(&clock) ||
        !pt_diagnostic_cia_acquire(&timer, pt_cia_direct_clock_stray, (void *)&unexpected_irqs))
        goto finish;
    chip = timer.chip;
    bit = timer.bit;
    if (!pt_native_eclock_read(&clock, &before, &hz) || (hz != 709379 && hz != 715909) ||
        (irq_mode && (before > UINT64_MAX - hz || !open_alarm(&alarm, &timer, before + hz))) ||
        !pt_native_eclock_read(&clock, &before, &end_hz) || end_hz != hz ||
        !pt_cia_start_masked_countdown(&timer))
        goto finish;
    Disable();
    pt_cia_direct_clock_pair(timer.high, timer.low, &anchor);
    Enable();
    count = pt_cia_stable_count(anchor.after);
    if (anchor.marker != ANCHOR_MARK || !count || pt_elapsed_clock_init(&epoch, hz, RATE, 0, 0) != PT_ELAPSED_OK)
        goto finish;

    for (unsigned i = 0; i < POSITIVE; ++i) {
        if (!prepare(row + i, &epoch, count, 512 + i * 128))
            goto finish;
    }
    if (!prepare(row + POSITIVE, &epoch, count, 3900) ||
        !prepare(row + POSITIVE + 1, &epoch, count, 1))
        goto finish;

    for (; captured < RECORDS; ++captured) {
        struct local_result *r = row + captured;
        attempted = captured + 1;
        if (irq_mode) {
            r->status = 4;
            int observed = alarm_issue(&alarm, &timer, r, captured, arm + captured);
            if (!observed)
                goto finish;
            if (observed == 2)
                r->status = 3;
        } else {
            const int approach = captured >= POSITIVE || wait_near(&timer, r->window.first_count);
            if (!approach || !issue(&timer, r))
                goto finish;
        }
    }
    if (!pt_native_eclock_read(&clock, &after, &end_hz) || end_hz != hz || after <= before ||
        after - before >= PT_CIA_COUNTDOWN_MAX || !(*timer.control & 1U))
        goto finish;
    observation_complete = 1;
    if (row[POSITIVE].status == 0 && row[POSITIVE + 1].status == 2 &&
        !row[POSITIVE].raw.marker && !row[POSITIVE + 1].raw.marker)
        rc = 0;

finish:
    if ((irq_mode && !close_alarm(&alarm)) || !pt_diagnostic_cia_close(&timer))
        return hold();
    pt_native_eclock_close(&clock);
    if (unexpected_irqs || timer.held || timer.resource || clock.opened)
        rc = 20;

    /* Read and classify complete raw rows only after the timed work and
     * original source are closed. Leave partial rows available in the report. */
    for (unsigned i = 0; i < captured && i < POSITIVE; ++i) {
        const struct local_result *r = row + i;
        struct pt_countdown_bracket bracket;
        const unsigned pre = pt_cia_stable_count(r->raw.before), post = pt_cia_stable_count(r->raw.after);
        if (r->raw.marker == MARK && pre && post &&
            pt_countdown_window_bracket(&r->window, pre, post, &bracket))
            ++on_frame;
        else if (r->raw.marker == MARK)
            ++outside;
        else
            ++missed;
    }

    printf("CIA local hz=%lu end_hz=%lu first=%08lx:%08lx last=%08lx:%08lx "
        "chip=%u timer=%u anchor=%02x%02x%02x captured=%u complete=%u\n",
        (unsigned long)hz, (unsigned long)end_hz,
        (unsigned long)(before >> 32), (unsigned long)before,
        (unsigned long)(after >> 32), (unsigned long)after, chip, bit,
        (unsigned)anchor.after[0], (unsigned)anchor.after[1], (unsigned)anchor.after[2],
        captured, observation_complete);
    for (unsigned i = 0; i < attempted; ++i) {
        const struct local_result *r = row + i;
        const struct local_entry *p = &r->raw;
        printf("CIA frame %02u frame=%u bounds=%lu/%lu count=%u/%u attempts=%u status=%u "
            "pre=%02x%02x%02x post=%02x%02x%02x mark=%04x\n", i, r->frame,
            (unsigned long)r->window.first, (unsigned long)r->window.end,
            (unsigned)p->first_count, (unsigned)p->end_count, (unsigned)p->attempts, r->status,
            (unsigned)p->before[0], (unsigned)p->before[1], (unsigned)p->before[2],
            (unsigned)p->after[0], (unsigned)p->after[1], (unsigned)p->after[2], (unsigned)p->marker);
    }
    printf("CIA local finished on=%u outside=%u missed=%u stray=%lu closed=1 rc=%d\n",
        on_frame, outside, missed, (unsigned long)unexpected_irqs, rc);
    if (irq_mode) {
        for (unsigned i = 0; i < attempted; ++i)
            printf("CIA alarm %02u probe=%u raw=%02x%02x%02x wake_at=%u delta=%u wake=%u phase=%u calls=%u\n",
                i, arm[i].probe, (unsigned)arm[i].raw[0], (unsigned)arm[i].raw[1], (unsigned)arm[i].raw[2],
                arm[i].threshold, arm[i].delta, arm[i].wake, arm[i].phase, arm[i].calls);
        printf("CIA alarm closed chip=%u timer=%u calls=%lu stray=%lu attempted=%u\n",
            alarm.timer.chip, alarm.timer.bit, (unsigned long)alarm.data.calls,
            (unsigned long)alarm.data.stray, attempted);
    }
    return rc;
}
