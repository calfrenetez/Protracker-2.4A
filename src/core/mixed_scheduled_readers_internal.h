#ifndef PT_MIXED_SCHEDULED_READERS_INTERNAL_H
#define PT_MIXED_SCHEDULED_READERS_INTERNAL_H
#include "mixed_scheduled_readers.h"

/* Private bounded owner-thread task operation, not an inert query. Resolve the
 * exact retained original ticket/action and invoke its real holder-current
 * callback under the existing queue exclusion. OK means the complete existing
 * ADOPTED, timed ACTIVE gate passed now; PENDING means only a clean live queued
 * reader has not reached that gate. Terminal/fault/stale/uncertain state never
 * becomes PENDING. No key, receipt, clock, service, proof, schedule change,
 * ownership transfer or lasting ACTIVE/quiet permission is produced. Actual
 * enqueue/batch admission must still obtain and validate a fresh genuine key. */
enum pt_mixed_readers_result pt_mixed_readers_reader_readiness(
    struct pt_mixed_readers_output *,uint64_t original_ticket,unsigned original_action);
/* Private task result, not a receipt or validity/quiet certificate. The bit
 * records genuine terminal settlement of the exact actual original domain,
 * including local unpublished queue retirement and consumed invalid semantic
 * proofs. It only suppresses another backend R call; normal errors remain errors.
 * The caller retains its original owner until that owner's actual NULL close. */
struct pt_mixed_reader_retirement {
    enum pt_mixed_readers_result result;
    unsigned retirement_consumed;
};
struct pt_mixed_reader_retirement pt_mixed_readers_retire_original(
    struct pt_mixed_readers_output *,uint64_t original_ticket,
    unsigned original_action,unsigned cancel);
/* Narrow captured-only numeric admission of a factory retirement handle slot.
 * Bounds/partial captured controls precede all owner/span walks. No callback,
 * former source walk, generic output exception or release certificate. */
int pt_mixed_readers_retirement_slot_disjoint(
    const struct pt_mixed_readers_output *,const void *slot,size_t full_bytes);
#endif
