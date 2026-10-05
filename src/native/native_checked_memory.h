/* Private task-side checked-memory ownership and guarded-extent contract. */
#ifndef PT_PRIVATE_NATIVE_CHECKED_MEMORY_H
#define PT_PRIVATE_NATIVE_CHECKED_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include "master_memory.h"

#define PT_PRIVATE_MEMORY_ALLOCATIONS 32U
#define PT_PRIVATE_MEMORY_GUARDS 16U

enum pt_private_memory_result {
    PT_PRIVATE_MEMORY_OK = 0,
    PT_PRIVATE_MEMORY_INVALID,
    PT_PRIVATE_MEMORY_BUDGET,
    PT_PRIVATE_MEMORY_CAPACITY,
    PT_PRIVATE_MEMORY_FAST_UNAVAILABLE,
    PT_PRIVATE_MEMORY_ALIAS,
    PT_PRIVATE_MEMORY_TYPE,
    PT_PRIVATE_MEMORY_ACCOUNTING,
    PT_PRIVATE_MEMORY_BUSY,
    PT_PRIVATE_MEMORY_SEALED,
    PT_PRIVATE_MEMORY_RETAINED,
    PT_PRIVATE_MEMORY_OWNED,
    PT_PRIVATE_MEMORY_FAULT
};
enum pt_private_memory_phase {
    PT_PRIVATE_MEMORY_UNINITIALIZED = 0,
    PT_PRIVATE_MEMORY_PREPARE,
    PT_PRIVATE_MEMORY_SEALED_ALLOCATIONS,
    PT_PRIVATE_MEMORY_RETAIN_ALL,
    PT_PRIVATE_MEMORY_FINISHED
};
struct pt_private_memory_span { const void *data; size_t bytes; };
struct pt_private_memory_allocation { void *pointer; size_t bytes; };
struct pt_private_native_memory {
    struct pt_master_memory pool;
    struct pt_private_memory_allocation allocation[PT_PRIVATE_MEMORY_ALLOCATIONS];
    struct pt_private_memory_span guard[PT_PRIVATE_MEMORY_GUARDS];
    /* A refused non-NULL AllocMem result is never published or automatically
     * freed. This is an ambiguous returned interval, not a new owner receipt. */
    struct pt_private_memory_allocation ambiguous;
    uint32_t initialized;
    unsigned guard_count, live, peak_live, allocations, releases;
    unsigned phase, busy, failed, last_result;
};
struct pt_private_memory_stats {
    size_t limit, used, reserve;
    unsigned live, peak_live, allocations, releases, phase, failed, last_result;
};

/* Caller provides a genuine zero-initialized, writable control object. Init
 * queries memory availability only; it performs no allocation. The <=16 guard
 * descriptors are copied; the original vector need live only for this call.
 * Their nonempty pointee extents remain alive/stable through finish or hold.
 * Copied descriptor identities/extents are immutable; controlled bytes may
 * mutate under their own serialized owner API. Sample bytes obey the separate
 * immutable-source contract. Full source/control/backend/allocator/output
 * extents are guarded; unlisted opaque extents remain caller responsibility.
 *
 * Serialized original normal-task entry is required, including init's OS
 * queries. Do not mutate this object or borrowed input during a call. Init
 * does not detect recursive access before initialization. No IRQ use.
 *
 * Fast-only is strict: the shared policy chooses a class but this helper
 * refuses unless it is MEMF_FAST. The optional caller ceiling clamps the real
 * reserve-aware ceiling. Zero ceiling is valid and refuses all allocations.
 * On refusal the original control/guard bytes remain unchanged. */
int pt_private_native_memory_init(struct pt_private_native_memory *, size_t,
    const struct pt_private_memory_span *, unsigned);

/* pt_allocator-compatible callbacks. Only PREPARE may allocate. All native
 * entry allocations must complete before vector acquisition; seal beforehand.
 * Exact requested payload bytes are charged, with no legacy prepended header.
 * Zero/overflow/budget/capacity refusals call no AllocMem. Return values are
 * ordinary task control/data, intended Fast; native placement is NOT proven
 * by this unexecuted source. TypeOfMem checks base and final byte plus the
 * real AllocMem MEMF_FAST|MEMF_PUBLIC contract, not a per-byte scan/certificate.
 *
 * A non-NULL returned interval must be representable, disjoint from this whole
 * control, guards and every live allocation. Otherwise retain ambiguity:
 * never FreeMem an interval that might name existing owned/borrowed storage.
 * Reentry or uncertain classification permanently disables allocation/free.
 */
void *pt_private_native_allocate(void *, size_t);
/* Release accepts NULL as a no-op or an exact live base pointer. It never
 * reads an allocation header or probes/frees a foreign/interior/duplicate
 * pointer. Integer API exposes refusal; void callback latches the same fault.
 * Only a matching registry size is passed to FreeMem after checked accounting
 * and placement. A valid completed FreeMem is recorded as consumed even if
 * reentry subsequently latches retention; no freed pointer is then retained.
 *
 * Sealing alone is NOT permission to free. After a source is exposed, the
 * caller independently proves exact source shutdown and command/reader quiet
 * before any release. No allocator result creates a quiet/retirement token.
 */
int pt_private_native_release_checked(struct pt_private_native_memory *, void *);
void pt_private_native_release(void *, void *);

/* Seal is irreversible. retain is also irreversible and disables both
 * callbacks; no retry/recovery/unseal or bulk-free API exists. On uncertainty
 * the entry must keep the entire HUNK/process/OS/context/source lifetime and
 * use its separately reviewed passive hold outside matched task exclusion.
 * This module does not abort, Exit, print, wait, manipulate a vector or invoke
 * hold automatically. All allocator calls are outside activation/IRQ paths.
 */
int pt_private_native_memory_seal(struct pt_private_native_memory *);
int pt_private_native_memory_retain(struct pt_private_native_memory *);
/* Finish never releases anything. OK requires exact zero ownership/accounting
 * and no fault. It is not OS-restoration/source-quiet or native-run acceptance.
 * OWNED is an incomplete-ownership report: phase is unchanged, so independently
 * permitted exact releases remain possible. RETAIN_ALL is the irreversible
 * uncertainty state. Any refusal forbids destroying a still-live context.
 */
int pt_private_native_memory_finish(struct pt_private_native_memory *);
/* Full fixed control/guards/live capacities protect output. Stale/ambiguous
 * representability or aliases refuse without publishing stats. */
int pt_private_native_memory_stats(const struct pt_private_native_memory *,
    struct pt_private_memory_stats *);
#endif
