#ifndef PT_NATIVE_AMIGUS_WAVETABLE_READ_H
#define PT_NATIVE_AMIGUS_WAVETABLE_READ_H
#include "../core/amigus_reservation.h"
#include <stdint.h>
/* Qualification only. Stable zero-initialized context, exclusive Mini wavetable
 * lease, observed firmware. Eight global IRQ/mask reads; never bank selection,
 * sample data, reset or voice control. Flags do not prove voices stopped or RAM
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
#endif
