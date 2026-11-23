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
#endif
