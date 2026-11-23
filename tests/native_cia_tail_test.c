#include "../src/core/countdown_window.h"
#include "../src/core/elapsed_clock.h"
#include "native_cia_alarm_lifecycle.h"
#include <stdio.h>

enum { FRAME_RATE = 48000, FIRST_COUNT = 13, REFUSED_COUNT = 5, MIN_SPAN = 6, EARLY_ALARM = 128 };

struct clock_owner {
    struct Task *task;
    ULONG signal;
    volatile ULONG reloads;
};

struct tail_entry {
    uint16_t first, excluded, attempts, marker;
    uint8_t before[3], after[3];
};

_Static_assert(offsetof(struct clock_owner, signal) == 4 &&
    offsetof(struct clock_owner, reloads) == 8, "Countdown interrupt layout");
_Static_assert(offsetof(struct tail_entry, marker) == 6 && offsetof(struct tail_entry, before) == 8 &&
    offsetof(struct tail_entry, after) == 11, "Countdown tail observation layout");

extern void pt_cia_reload_irq(void);
extern void pt_cia_local_tail_irq(void);

static int sample(const struct pt_diagnostic_cia *clock, const struct clock_owner *owner,
    unsigned wanted, struct pt_cia_clock_pair *point)
{
    if (!pt_diagnostic_cia_current(clock) || owner->reloads != wanted)
        return 0;
    pt_cia_direct_clock_pair(clock->high, clock->low, point);
    return owner->reloads == wanted && point->marker == 0x2452 && pt_cia_stable_count(point->after);
}

static int bounds(uint32_t hz, uint16_t count, unsigned first_count, uint64_t *first, uint64_t *end)
{
    struct pt_elapsed_clock grid;
    /* Pick an epoch for this test before arming its actual issue alarm.
     * Its first logical sample includes the original counter reload. */
    return count > first_count &&
        pt_elapsed_clock_init(&grid, hz, FRAME_RATE, count - first_count, 0) == PT_ELAPSED_OK &&
        pt_elapsed_clock_deadline(&grid, 0, first) == PT_ELAPSED_OK &&
        pt_elapsed_clock_deadline(&grid, 1, end) == PT_ELAPSED_OK && *end > (uint64_t)count + 1;
}

static int hold(void) __attribute__((noreturn));
static int hold(void)
{
    puts("CIA tail HOLD: original timer resources retained");
    fflush(stdout);
    for (;;)
        Delay(50);
}

int main(void)
{
    struct pt_native_eclock timebase = {0};
    struct pt_diagnostic_cia clock = {0};
    struct local_alarm alarm = {.signal = -1};
    struct local_alarm_data *irq = &alarm.data;
    struct clock_owner source = {.task = FindTask(NULL)};
    struct pt_countdown_observation anchor = {0}, later = {0};
    struct pt_countdown_window aperture = {0}, refused = {0};
    struct pt_countdown_bracket bracket = {0};
    struct pt_cia_clock_pair original = {0}, arm = {0}, after_reload = {0};
    struct tail_entry raw = {0};
    uint64_t now, first = 0, end = 0, refused_first = 0, refused_end = 0;
    uint32_t hz = 0;
    int rc = 20, planner_refused = 0;
    BYTE signal = -1;
    unsigned lead = 0, delay = 0;
    ULONG issued_reload = UINT32_MAX, status = UINT32_MAX;

    if (!source.task || !pt_native_eclock_open(&timebase) || !pt_native_eclock_read(&timebase, &now, &hz) ||
        (hz != 709379 && hz != 715909) || now > UINT64_MAX - hz ||
        (signal = AllocSignal(-1)) < 0 || signal >= 32)
        goto finish;
    source.signal = 1UL << signal;
    if ((source.signal & SIGBREAKF_CTRL_C) ||
        !pt_diagnostic_cia_acquire(&clock, pt_cia_reload_irq, &source) ||
        !open_alarm(&alarm, &clock, now + hz, pt_cia_local_tail_irq) ||
        (source.signal & (irq->signal | alarm.deadline_signal)) ||
        !pt_diagnostic_cia_arm_mode(&clock, UINT16_MAX, 1) || !sample(&clock, &source, 0, &original))
        goto finish;
    anchor.count = pt_cia_stable_count(original.after);
    if (!bounds(hz, anchor.count, REFUSED_COUNT, &refused_first, &refused_end) ||
        pt_countdown_window_intersect(&anchor, refused_first, refused_end, MIN_SPAN, &refused) ||
        refused.first || refused.end || !bounds(hz, anchor.count, FIRST_COUNT, &first, &end) ||
        pt_countdown_window_prepare(anchor.tick, anchor.count, first, end, &refused) ||
        !pt_countdown_window_intersect(&anchor, first, end, MIN_SPAN, &aperture))
        goto finish;
    planner_refused = 1;
    raw.first = aperture.first_count;
    raw.excluded = aperture.end_count;
    lead = raw.first + EARLY_ALARM;

    Disable();
    if (!sample(&clock, &source, 0, &arm) || pt_cia_stable_count(arm.after) <= lead) {
        Enable();
        goto finish;
    }
    delay = pt_cia_stable_count(arm.after) - lead;
    irq->entry = &raw;
    irq->reload = &source.reloads;
    irq->expected_reload = 0;
    irq->observed_reload = UINT32_MAX;
    irq->result = UINT32_MAX;
    irq->armed = 1;
    int armed = pt_diagnostic_cia_arm(&alarm.timer, delay);
    if (!armed)
        irq->armed = 0;
    Enable();
    if (!armed)
        goto finish;

    const ULONG abort = alarm.deadline_signal | SIGBREAKF_CTRL_C;
    const ULONG wake = Wait(irq->signal | source.signal | abort);
    Disable();
    if ((wake & abort) || !(wake & irq->signal) || irq->armed || irq->calls != 1 || irq->stray) {
        irq->armed = 0;
        Enable();
        goto finish;
    }
    issued_reload = irq->observed_reload;
    status = irq->result;
    irq->entry = NULL;
    Enable();
    if (!(wake & source.signal) && Wait(source.signal | abort) != source.signal)
        goto finish;
    if (!sample(&clock, &source, 1, &after_reload))
        goto finish;
    later = anchor;
    if (!pt_countdown_extend(&later, 1, pt_cia_stable_count(after_reload.after)) ||
        later.tick <= anchor.count || issued_reload != 0 || status != 1 || raw.marker != 0x4752 ||
        !pt_countdown_window_bracket(&aperture, pt_cia_stable_count(raw.before), pt_cia_stable_count(raw.after),
            &bracket))
        goto finish;
    rc = 0;

finish:
    if (!close_alarm(&alarm) || !pt_diagnostic_cia_close(&clock))
        return hold();
    if (signal >= 0 && signal < 32) {
        SetSignal(0, source.signal);
        FreeSignal(signal);
    }
    pt_native_eclock_close(&timebase);
    if (source.reloads != 1 || irq->calls != 1 || irq->stray)
        rc = 20;
    printf("CIA tail hz=%lu clock=%u/%u alarm=%u/%u original=%02x%02x%02x after=%02x%02x%02x "
        "after_tick=%lu reloads=%lu\n", (unsigned long)hz, clock.chip, clock.bit, alarm.timer.chip, alarm.timer.bit,
        (unsigned)original.after[0], (unsigned)original.after[1], (unsigned)original.after[2],
        (unsigned)after_reload.after[0], (unsigned)after_reload.after[1], (unsigned)after_reload.after[2],
        (unsigned long)later.tick, (unsigned long)source.reloads);
    printf("CIA tail plan frame=%lu/%lu aperture=%lu/%lu count=%u/%u too_short=%lu/%lu refused=%d\n",
        (unsigned long)first, (unsigned long)end, (unsigned long)aperture.first, (unsigned long)aperture.end,
        (unsigned)raw.first, (unsigned)raw.excluded, (unsigned long)refused_first, (unsigned long)refused_end,
        planner_refused);
    printf("CIA tail issue generation=%lu status=%lu arm=%02x%02x%02x lead=%u delay=%u attempts=%u "
        "pre=%02x%02x%02x post=%02x%02x%02x mark=%04x ticks=%lu/%lu\n",
        (unsigned long)issued_reload, (unsigned long)status,
        (unsigned)arm.after[0], (unsigned)arm.after[1], (unsigned)arm.after[2], lead, delay, (unsigned)raw.attempts,
        (unsigned)raw.before[0], (unsigned)raw.before[1], (unsigned)raw.before[2],
        (unsigned)raw.after[0], (unsigned)raw.after[1], (unsigned)raw.after[2], (unsigned)raw.marker,
        (unsigned long)bracket.first, (unsigned long)bracket.last);
    printf("CIA tail finished callbacks=%lu stray=%lu closed=1 rc=%d\n",
        (unsigned long)irq->calls, (unsigned long)irq->stray, rc);
    return rc;
}
