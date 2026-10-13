#ifndef PT_SAMPLER_PAULA_READERS_INVENTORY_INTERNAL_H
#define PT_SAMPLER_PAULA_READERS_INVENTORY_INTERNAL_H
#include "sampler_paula_readers.h"
/* Pure owner-thread metadata observation of a genuine pool's registered
 * preparing existing-master pin jobs. Caller owns the original pool, sampler,
 * project and captured current pins for the entire call. Every original live and pending
 * suffix owner remains living and immutable; bounded numeric/link consistency
 * does not freeze its payload or detect every valid in-place edit.
 * Only pending_head/count may differ from the captured sampler bytes. No new
 * current master, live allocation, charge, allocator/fault change, callback,
 * mutation, pin/ref transfer, READY/quiet/backend/native authority is granted.
 * Call only through the constructor-issued pool, never a caller certificate.
 * Failed/cancelled action state is not retirement evidence; genuine job removal
 * and the ordinary command/reader cancellation/close contracts remain required. */
int pt_paula_readers_inventory_current(const struct pt_paula_readers_pool *,
    const struct pt_sampler *,const struct pt_project *,const struct pt_sampler *);
#endif
