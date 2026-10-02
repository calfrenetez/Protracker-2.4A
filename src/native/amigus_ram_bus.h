#ifndef PT_NATIVE_AMIGUS_RAM_BUS_H
#define PT_NATIVE_AMIGUS_RAM_BUS_H
#include <stdint.h>
#include "../core/amigus_reservation.h"
/* Stable zero-init context, serial owner thread only. Borrows the reservation;
 * bind before cache attach, clear only after cache detach ends its access lease.
 * No card capacity/default region is inferred. Caller must independently verify
 * the supplied RAM region, bus completion and sample ordering before deployment.
 * The reservation/card descriptor outlive this context; never close/reopen or
 * change them behind an attached cache. This adapter installs/starts no voices. */
struct pt_native_amigus_ram_bus {
    struct pt_amigus_reservation *owner;
    void *card;
    uintptr_t registers;
    uint32_t first,bytes;
    unsigned addressed,faulted;
};
/* No I/O. Only the separately identified Mini hardware0/firmware7ea663e7.
 * Requires an open exclusive WAVETABLE reservation, no access/interrupt owner. */
int pt_native_amigus_ram_bus_bind(struct pt_native_amigus_ram_bus *,
    struct pt_amigus_reservation *,uint32_t first,uint32_t bytes);
/* Reservation-only predicate for cache attach; no I/O. Ownership loss latches. */
int pt_native_amigus_ram_bus_owned(void *);
/* Cache callback: requires exactly one live access lease and no interrupt.
 * Only paired address0x14/data0x10 numeric longword stores; full four-byte address
 * must fit the supplied region. Any refusal latches, preventing later writes.
 * Success means synchronous CPU store completion, not verified device contents. */
int pt_native_amigus_ram_bus_write32(void *,unsigned,uint32_t);
/* No I/O. Refuses while access/interrupt ownership may still use this context. */
int pt_native_amigus_ram_bus_clear(struct pt_native_amigus_ram_bus *);
#endif
