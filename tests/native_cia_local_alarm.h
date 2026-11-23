#ifndef PT_TEST_NATIVE_CIA_LOCAL_ALARM_H
#define PT_TEST_NATIVE_CIA_LOCAL_ALARM_H

#include "native_cia_alarm_lifecycle.h"

enum local_alarm_phase {
    LOCAL_ALARM_OWNER, LOCAL_ALARM_TORN, LOCAL_ALARM_TOO_CLOSE,
    LOCAL_ALARM_ARM, LOCAL_ALARM_WAKE, LOCAL_ALARM_RECEIPT
};

struct local_alarm_example {
    unsigned probe, threshold, delta, wake, phase, calls;
    uint8_t raw[3];
};

extern void pt_cia_local_alarm_irq(void);

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
