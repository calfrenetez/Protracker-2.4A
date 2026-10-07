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
#endif
