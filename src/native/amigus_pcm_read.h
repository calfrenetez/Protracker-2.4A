#ifndef PT_NATIVE_AMIGUS_PCM_READ_H
#define PT_NATIVE_AMIGUS_PCM_READ_H
#include "../core/amigus_reservation.h"
#include <stdint.h>
/* Read-only qualification binding for the observed Mini firmware. Requires a
 * stable exclusive PCM owner and access lease, with no installed interrupt.
 * Only documented non-destructive status registers are accepted. No write,
 * reset, capacity assumption, start or interrupt operation is exposed. */
int pt_native_amigus_pcm_read16(const struct pt_amigus_reservation *, unsigned,
                               uint16_t *);
/* Separate silent qualification: writes only rate-disable, playback IRQ clears
 * and FIFO reset. No capacity/data/format/enable operation. Begin is issued once;
 * poll reads only. Any unconfirmed result retains the caller's access/owner. */
struct pt_native_amigus_quiesce {
    const struct pt_amigus_reservation *owner;
    unsigned requested, confirmed;
};
int pt_native_amigus_quiesce_begin(struct pt_native_amigus_quiesce *,
                                  const struct pt_amigus_reservation *);
int pt_native_amigus_quiesce_poll(struct pt_native_amigus_quiesce *);
/* Qualification-only FIFO long store: same owner/firmware guard, playback and
 * playback IRQs disabled, even pending count <=4. Caller must bound the test to
 * three stores/six words independent of readback. No capacity claim. Must reset
 * and confirm before releasing. */
int pt_native_amigus_pcm_fifo_probe32(const struct pt_amigus_reservation *, uint32_t);
#endif
