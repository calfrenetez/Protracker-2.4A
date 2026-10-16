#ifndef PT_EDITOR_PREPARED_PAULA_SOURCES_H
#define PT_EDITOR_PREPARED_PAULA_SOURCES_H
#include <stddef.h>

#define PT_PREPARED_PAULA_SOURCES_COUNT 3U
struct pt_sampler_paula;
struct pt_prepared_paula_sources;

enum pt_prepared_paula_sources_result {
    PT_PREPARED_PAULA_SOURCES_OK=0,
    PT_PREPARED_PAULA_SOURCES_PENDING,
    PT_PREPARED_PAULA_SOURCES_READY,
    PT_PREPARED_PAULA_SOURCES_CLOSED,
    PT_PREPARED_PAULA_SOURCES_INVALID,
    PT_PREPARED_PAULA_SOURCES_ALIAS,
    PT_PREPARED_PAULA_SOURCES_STALE,
    PT_PREPARED_PAULA_SOURCES_CAPACITY,
    PT_PREPARED_PAULA_SOURCES_BUSY,
    PT_PREPARED_PAULA_SOURCES_FAULT
};
enum pt_prepared_paula_sources_phase {
    PT_PREPARED_PAULA_SOURCES_PREPARING=1,
    PT_PREPARED_PAULA_SOURCES_PREPARED,
    PT_PREPARED_PAULA_SOURCES_CANCELLING,
    PT_PREPARED_PAULA_SOURCES_FINISHED,
    PT_PREPARED_PAULA_SOURCES_HELD
};
/* Numeric accounting observation only. No field is a pin, lease, source
 * pointer, currentness seal, activation/borrow grant or release certificate. */
struct pt_prepared_paula_sources_status {
    enum pt_prepared_paula_sources_phase phase;
    enum pt_prepared_paula_sources_result failure;
    unsigned completed,master_pins,chip_leases,master_job,chip_job;
};

/* Original normal-TASK, serialized preparation only; never IRQ/supervisor,
 * signal, delivery callback or inside a Disable/Enable metadata gate. All
 * borrowed inputs are ordinary complete accessible objects. Task context is a
 * static caller precondition, not something these portable functions detect.
 * No copying/editing/reinitializing an active owner or its jobs, no callbacks
 * that mutate/reenter/destroy this owner or its borrowed sources/contexts.
 *
 * Caller has ALREADY fully validated the immutable project/PCM/markers and
 * established all three current immutable masters through existing sampler
 * ownership. bridge is genuinely bound to that sampler/project, with original
 * budgeted CPU-writable Chip allocation/release callbacks and a Paula route.
 * Exactly three distinct nonempty mono sample slots are admitted;8/16/24-bit
 * masters remain authoritative. This API does not promote document-backed
 * samples, validate arbitrary alleged pins, bind a new cache or swap allocators.
 * Only the channel-selection cursor may move between serialized calls.
 *
 * Caller retains original bridge/sampler/project/table/source/allocator storage
 * through CLOSED or any retained failure. In-place sample/marker writes are
 * forbidden, not detected by metadata checks. Known complete owner capacity,
 * project/table/source/version/cache spans protect outputs. Unknown opaque
 * context extents additionally remain caller-disjoint. Constructor arguments
 * may expire after successful init; their named pointees/contexts may not.
 *
 * The default interface has no source/PCM/cache-pointer export, backend/reader
 * binding, publication/arm/apply, timer, output or external borrow interface.
 * Its cancellation is ONLY disposal of never-exposed software resources.
 * The separate explicit HOST-only lifetime opt-in may retain this exact ledger
 * for authentic core queue callbacks; it supplies no caller source pointer or
 * quiet certificate and has no native eligibility. Public cancellation refuses
 * a retained custodian. No DMA/source quiet or real interrupt lifetime follows.
 */
size_t pt_prepared_paula_sources_size(void);
size_t pt_prepared_paula_sources_alignment(void);

/* Complete aligned capacity>=size(); only the owned size() prefix must be
 * zero and is checked. The capacity tail is never read/written, but its full
 * extent protects subsequent outputs. Refusal preserves owner/inputs and calls
 * no child operation or allocator. Success returns OK and prepares no source. */
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_init(
    struct pt_prepared_paula_sources *,size_t capacity,struct pt_sampler_paula *,
    unsigned track,const unsigned sample[PT_PREPARED_PAULA_SOURCES_COUNT]);

/* At most ONE child transition per call: real master pin-job begin, real
 * master pin-job completion, Chip job begin, or <=256 packed signed8 Chip bytes.
 * Existing current masters take the no-copy pin path, but their reference
 * transfers only after the real completion step. HIT/final LOAD each transfer
 * one lease; PENDING transfers none. The Chip job's temporary master reference
 * is independent of the persistent pin retained here.
 *
 * Fresh original identity/source checks surround child work and callback-bearing
 * calls; changed generation/table/route/descriptor/version refuses further
 * preparation. No caller-visible PCM/token/lease output exists. READY means
 * three complete locally owned pins+leases, never permission for native output.
 * Failure is sticky; use cancel_step, never reset/retry a failed child. Budgeted
 * allocation/eviction/metadata work can remain synchronous; no WCET/latency or
 * change to any existing rolling work/deadline/exact schedule is claimed. */
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_step(
    struct pt_prepared_paula_sources *);

/* Own-metadata observation only; no retained currentness proof and no child
 * job/retain/release/allocator action. Pure held-span/current/disjointness checks
 * still guard publication. Refusal preserves out. No resources or source
 * identities are exported. */
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_get(
    struct pt_prepared_paula_sources *,struct pt_prepared_paula_sources_status *out);

#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
/* The private HOST opt-in makes cancellation return BUSY while a genuine
 * custody reservation/transfer is retained; no writes or child work then. */
#endif
/* Permanently stops preparation. At most ONE original pending-job cancellation
 * or ONE completed lease/pin release per call, using captured own resources,
 * not stale former source tables. Returns PENDING until local ownership ends,
 * then CLOSED; repeated CLOSED is read-only/idempotent. Original failure remains
 * observable after successful cleanup. Closed storage is not reusable/reset.
 *
 * A refused lease release or reentry/uncertain accounting retains the surviving
 * owner/context anchors (HELD/FAULT); no corrective release loop, forced close,
 * counter rollback or resurrection of an already-consumed reference. Every
 * actual release is consumed once before any later callback can repeat it. */
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_cancel_step(
    struct pt_prepared_paula_sources *);
#endif
