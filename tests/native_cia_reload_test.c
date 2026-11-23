#include "../src/core/countdown_window.h"
#include "../src/native/eclock_alarm.h"
#include "native_cia_countdown.h"
#include <proto/dos.h>
#include <stdio.h>

enum { MIN_RELOADS = 1024, MAX_RELOADS = 1200, OBSERVATION_SECONDS = 110 };

struct reload_irq {
    struct Task *task;
    ULONG signal;
    volatile ULONG calls;
};

_Static_assert(offsetof(struct reload_irq, task) == 0 && offsetof(struct reload_irq, signal) == 4 &&
    offsetof(struct reload_irq, calls) == 8, "CIA reload interrupt layout");
extern void pt_cia_reload_irq(void);

struct reload_probe {
    uint64_t before, after;
    struct pt_cia_clock_pair pair;
    unsigned first_reload, last_reload;
};

static int observe(struct pt_native_eclock *timebase, const struct pt_diagnostic_cia *counter,
    const struct reload_irq *irq, unsigned desired, uint32_t hz, struct reload_probe *row)
{
    uint32_t before_hz, after_hz;
    if (!pt_diagnostic_cia_current(counter) || !pt_native_eclock_read(timebase, &row->before, &before_hz))
        return 0;
    row->first_reload = irq->calls;
    pt_cia_direct_clock_pair(counter->high, counter->low, &row->pair);
    row->last_reload = irq->calls;
    return pt_native_eclock_read(timebase, &row->after, &after_hz) && hz == before_hz && hz == after_hz &&
        row->after > row->before && row->first_reload == desired && row->last_reload == desired &&
        row->pair.marker == 0x2452 && pt_cia_stable_count(row->pair.after) > 4096;
}

static void report_probe(unsigned index, const struct reload_probe *row)
{
    printf("CIA reload probe %u before=%08lx:%08lx after=%08lx:%08lx calls=%u/%u count=%02x%02x%02x\n",
        index, (unsigned long)(row->before >> 32), (unsigned long)row->before,
        (unsigned long)(row->after >> 32), (unsigned long)row->after,
        row->first_reload, row->last_reload, (unsigned)row->pair.after[0],
        (unsigned)row->pair.after[1], (unsigned)row->pair.after[2]);
}

static int hold(void) __attribute__((noreturn));
static int hold(void)
{
    puts("CIA reload HOLD: original timer resources retained");
    fflush(stdout);
    for (;;)
        Delay(50);
}

int main(void)
{
    struct pt_diagnostic_cia counter = {0};
    struct pt_native_eclock timebase = {0};
    struct pt_native_alarm timeout = {0};
    struct reload_irq irq = {.task = FindTask(NULL)};
    struct reload_probe initial = {0}, final = {0};
    struct pt_countdown_observation timeline = {0};
    uint64_t now, earliest = 0, latest = 0;
    uint32_t hz = 0;
    int rc = 20;
    BYTE signal = -1;
    unsigned wakeups = 0, reloads = 0;

    if (!irq.task || !pt_native_eclock_open(&timebase) || !pt_native_eclock_read(&timebase, &now, &hz) ||
        (hz != 709379 && hz != 715909) || now > UINT64_MAX - (uint64_t)OBSERVATION_SECONDS * hz ||
        !pt_native_alarm_open(&timeout) || (signal = AllocSignal(-1)) < 0 || signal >= 32 ||
        pt_native_alarm_arm(&timeout, now + (uint64_t)OBSERVATION_SECONDS * hz) != PT_ALARM_WAITING)
        goto finish;
    irq.signal = 1UL << signal;
    const ULONG timeout_signal = pt_native_alarm_signal(&timeout);
    if (!timeout_signal || (timeout_signal & (SIGBREAKF_CTRL_C | irq.signal)) || (irq.signal & SIGBREAKF_CTRL_C) ||
        !pt_diagnostic_cia_acquire(&counter, pt_cia_reload_irq, &irq) ||
        !pt_diagnostic_cia_arm_mode(&counter, UINT16_MAX, 1) ||
        !observe(&timebase, &counter, &irq, 0, hz, &initial))
        goto finish;
    /* Exec may coalesce repeated signals while the task is not scheduled.
     * Count underflows in the callback and bracket the observed generation. */
    while (reloads < MIN_RELOADS) {
        if (Wait(irq.signal | timeout_signal | SIGBREAKF_CTRL_C) != irq.signal)
            goto finish;
        const ULONG observed = irq.calls;
        if (observed < reloads || observed > MAX_RELOADS)
            goto finish;
        if (observed != reloads)
            ++wakeups;
        reloads = observed;
    }
    if (!observe(&timebase, &counter, &irq, reloads, hz, &final) || final.before <= initial.after)
        goto finish;
    earliest = final.before - initial.after;
    latest = final.after - initial.before;
    timeline.count = pt_cia_stable_count(initial.pair.after);
    if (!pt_countdown_extend(&timeline, reloads, pt_cia_stable_count(final.pair.after)) ||
        timeline.tick < earliest || timeline.tick > latest ||
        timeline.tick - earliest >= reloads || latest - timeline.tick >= reloads)
        goto finish;
    rc = 0;

finish:
    if (!pt_diagnostic_cia_close(&counter))
        return hold();
    for (unsigned i = 0; !pt_native_alarm_close(&timeout); ++i) {
        if (i == 49)
            return hold();
        Delay(1);
    }
    if (signal >= 0 && signal < 32) {
        SetSignal(0, 1UL << signal);
        FreeSignal(signal);
    }
    pt_native_eclock_close(&timebase);
    if (irq.calls != reloads || counter.held || timebase.opened || timeout.opened)
        rc = 20;
    report_probe(0, &initial);
    report_probe(1, &final);
    printf("CIA reload finished hz=%lu chip=%u timer=%u wakes=%u calls=%lu bounds=%lu/%lu ticks=%lu closed=1 rc=%d\n",
        (unsigned long)hz, counter.chip, counter.bit, wakeups, (unsigned long)irq.calls, (unsigned long)earliest,
        (unsigned long)latest, (unsigned long)timeline.tick, rc);
    return rc;
}
