#ifndef PT_TEST_NATIVE_CIA_ALARM_LIFECYCLE_H
#define PT_TEST_NATIVE_CIA_ALARM_LIFECYCLE_H

#include "../src/native/eclock_alarm.h"
#include "native_cia_countdown.h"
#include <proto/dos.h>

struct IOAudio;
struct local_alarm_data {
    volatile UBYTE *high, *low;
    void *entry;
    struct Task *task;
    ULONG signal;
    volatile ULONG armed, calls, result, stray;
    const struct IOAudio *audio_lock;
    ULONG command;
    UWORD allocation_key, dma_before;
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
LOCAL_IRQ_OFFSET(audio_lock, 36);
LOCAL_IRQ_OFFSET(command, 40);
LOCAL_IRQ_OFFSET(allocation_key, 44);
LOCAL_IRQ_OFFSET(dma_before, 46);
#undef LOCAL_IRQ_OFFSET

struct local_alarm {
    struct pt_diagnostic_cia timer;
    struct pt_native_alarm timeout;
    struct local_alarm_data data;
    BYTE signal;
    ULONG deadline_signal;
    unsigned delivered;
};

static int open_alarm(struct local_alarm *alarm, struct pt_diagnostic_cia *clock,
    uint64_t deadline, void (*handler)(void))
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
        pt_diagnostic_cia_acquire(&alarm->timer, handler, &alarm->data) &&
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
#endif
