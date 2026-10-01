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
#endif
