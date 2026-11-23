#ifndef PT_TEST_NATIVE_CIA_LOCAL_ALARM_H
#define PT_TEST_NATIVE_CIA_LOCAL_ALARM_H

#include "../src/native/eclock_alarm.h"

struct local_alarm_data {
    volatile UBYTE *high, *low;
    struct local_entry *entry;
    struct Task *task;
    ULONG signal;
    volatile ULONG armed, calls, result, stray;
};

#define LOCAL_IRQ_OFFSET(field, offset) \
    typedef char local_irq_##field[(offsetof(struct local_alarm_data, field) == (offset)) ? 1 : -1]
LOCAL_IRQ_OFFSET(high, 0);
LOCAL_IRQ_OFFSET(low, 4);
LOCAL_IRQ_OFFSET(entry, 8);
LOCAL_IRQ_OFFSET(task, 12);
LOCAL_IRQ_OFFSET(signal, 16);
LOCAL_IRQ_OFFSET(armed, 20);
LOCAL_IRQ_OFFSET(calls, 24);
LOCAL_IRQ_OFFSET(result, 28);
LOCAL_IRQ_OFFSET(stray, 32);
#undef LOCAL_IRQ_OFFSET

struct local_alarm {
    struct pt_diagnostic_cia timer;
    struct pt_native_alarm timeout;
    struct local_alarm_data data;
    BYTE signal;
    ULONG deadline_signal;
    unsigned delivered;
};

enum local_alarm_phase {
    LOCAL_ALARM_OWNER, LOCAL_ALARM_TORN, LOCAL_ALARM_TOO_CLOSE,
    LOCAL_ALARM_ARM, LOCAL_ALARM_WAKE, LOCAL_ALARM_RECEIPT
};

struct local_alarm_example {
    unsigned probe, threshold, delta, wake, phase, calls;
    uint8_t raw[3];
};

extern void pt_cia_local_alarm_irq(void);

static int open_alarm(struct local_alarm *alarm, struct pt_diagnostic_cia *clock, uint64_t deadline)
{
    struct Task *task = FindTask(NULL);
    if (!task || !pt_native_alarm_open(&alarm->timeout) ||
        (alarm->signal = AllocSignal(-1)) < 0 || alarm->signal >= 32)
        return 0;

    alarm->data.high = clock->high;
    alarm->data.low = clock->low;
    alarm->data.task = task;
    alarm->data.signal = 1UL << alarm->signal;
    if (pt_native_alarm_arm(&alarm->timeout, deadline) != PT_ALARM_WAITING)
        return 0;
    alarm->deadline_signal = pt_native_alarm_signal(&alarm->timeout);
    return alarm->deadline_signal && !(alarm->data.signal & (alarm->deadline_signal | SIGBREAKF_CTRL_C)) &&
        !(alarm->deadline_signal & SIGBREAKF_CTRL_C) &&
        pt_diagnostic_cia_acquire(&alarm->timer, pt_cia_local_alarm_irq, &alarm->data) &&
        alarm->timer.control != clock->control;
}

static int close_alarm(struct local_alarm *alarm)
{
    if (!pt_diagnostic_cia_close(&alarm->timer))
        return 0;
    for (unsigned i = 0; i < 50; ++i) {
        if (pt_native_alarm_close(&alarm->timeout)) {
            if (alarm->signal >= 0) {
                SetSignal(0, alarm->data.signal);
                FreeSignal(alarm->signal);
                alarm->signal = -1;
            }
            return 1;
        }
        Delay(1);
    }
    return 0;
}

static int alarm_issue(struct local_alarm *alarm, const struct pt_diagnostic_cia *clock,
    struct local_result *row, unsigned ordinal, struct local_alarm_example *example)
{
    struct pt_cia_clock_pair pair = {0};
    struct local_alarm_data *irq = &alarm->data;
    unsigned threshold;
    int armed;

    if (!pt_diagnostic_cia_current(clock) || !pt_diagnostic_cia_current(&alarm->timer) ||
        irq->armed || irq->entry || irq->calls != alarm->delivered || irq->stray)
        return 0;

    Disable();
    pt_cia_direct_clock_pair(clock->high, clock->low, &pair);
    memcpy(example->raw, pair.after, sizeof(example->raw));
    example->probe = pt_cia_stable_count(example->raw);
    example->calls = irq->calls;
    /* Set an early wake only. The original musical frame and assembler's
     * first/excluded count stay unchanged. The last two cases exercise an
     * exhausted early interrupt and an already expired interrupt. */
    if (ordinal < POSITIVE)
        threshold = (unsigned)row->window.first_count + 128;
    else if (ordinal == POSITIVE)
        threshold = example->probe > 1024 ? example->probe - 1024 : 0;
    else
        threshold = example->probe > 128 ? example->probe - 128 : 0;
    example->threshold = threshold;
    if (!threshold || example->probe <= threshold) {
        example->phase = example->probe ? LOCAL_ALARM_TOO_CLOSE : LOCAL_ALARM_TORN;
        memcpy(row->raw.before, example->raw, sizeof(row->raw.before));
        Enable();
        return 2;
    }
    example->phase = LOCAL_ALARM_ARM;
    example->delta = example->probe - threshold;
    SetSignal(0, irq->signal);
    irq->entry = &row->raw;
    irq->result = UINT32_MAX;
    irq->armed = 1;
    armed = pt_diagnostic_cia_arm(&alarm->timer, example->delta);
    if (!armed)
        irq->armed = 0;
    Enable();
    if (!armed)
        return 0;

    example->phase = LOCAL_ALARM_WAKE;
    ULONG wake = Wait(irq->signal | alarm->deadline_signal | SIGBREAKF_CTRL_C);
    example->wake = wake == irq->signal;
    Disable();
    example->calls = irq->calls;
    if (!example->wake || irq->armed || irq->calls != alarm->delivered + 1 || irq->stray || irq->result > 2) {
        Enable();
        return 0;
    }
    ++alarm->delivered;
    example->phase = LOCAL_ALARM_RECEIPT;
    row->status = irq->result;
    irq->entry = NULL;
    Enable();
    return 1;
}
#endif
