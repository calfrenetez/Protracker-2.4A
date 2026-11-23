#ifndef PT_NATIVE_AMIGUS_WAVETABLE_READ_H
#define PT_NATIVE_AMIGUS_WAVETABLE_READ_H
#include "../core/amigus_reservation.h"
#include <stdint.h>
/* Qualification only. Stable zero-initialized context, exclusive Mini wavetable
 * lease, observed firmware. Never writes registers or changes the bank.
 * The global IRQ/mask reads do not prove voices stopped or RAM
 * capacity. Ownership/descriptor loss latches refusal until context discarded
 * after the caller ends its lease. No hardware state was created by this API. */
struct pt_native_amigus_wavetable_read {
    const struct pt_amigus_reservation *owner;
    void *card;
    uintptr_t registers;
    unsigned faulted;
};
int pt_native_amigus_wavetable_read_bind(struct pt_native_amigus_wavetable_read *,
                                       const struct pt_amigus_reservation *);
int pt_native_amigus_wavetable_read16(struct pt_native_amigus_wavetable_read *,
                                    unsigned, uint16_t *);
/* Read 0x20 from the bank already selected by the device owner. The
 * write-only selector is untouched. A control word alone proves neither
 * the bank identity nor that access to its sample RAM has quiesced. */
int pt_native_amigus_wavetable_read_selected_control(struct pt_native_amigus_wavetable_read *, uint16_t *);

enum {
    PT_AMIGUS_VOICE_CONTROL_OFFSET = 0x20,
    PT_AMIGUS_VOICE_RELEASE_OFFSET = 0x3c,
    PT_AMIGUS_VOICE_SNAPSHOT_WORDS = (PT_AMIGUS_VOICE_RELEASE_OFFSET - PT_AMIGUS_VOICE_CONTROL_OFFSET) / 2 + 1
};

struct pt_amigus_voice_snapshot {
    uint16_t words[PT_AMIGUS_VOICE_SNAPSHOT_WORDS];
};

/* Sample the currently selected bank's documented control and setup registers.
 * The caller serializes selector access. A control change or lost lease refuses
 * the entire snapshot and leaves the output untouched. This neither names the
 * selected voice nor observes its current sample position or memory activity. */
int pt_native_amigus_wavetable_read_selected_voice(struct pt_native_amigus_wavetable_read *,
                                                  struct pt_amigus_voice_snapshot *);
#endif
