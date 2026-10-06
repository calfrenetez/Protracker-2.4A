#ifndef PT_SAMPLER_MIXED_READERS_H
#define PT_SAMPLER_MIXED_READERS_H
#include "sampler.h"
#include "../core/mixed_scheduled_readers.h"
#include "../core/amigus_wavetable_cache.h"
#define PT_SAMPLER_MIXED_COMMANDS 2U
#define PT_SAMPLER_MIXED_READERS 32U
#define PT_SAMPLER_MIXED_ACTIONS 16U
#define PT_SAMPLER_MIXED_CONTEXTS 16U
struct pt_sampler_mixed_pool;
struct pt_sampler_mixed_command;
struct pt_sampler_mixed_reader;
/* A genuine registration is checked by address AND token before dereference.
 * A stale/closed handle cannot authorize a reused address. Never edit a handle.
 * Handle storage is caller-owned, disjoint from every borrowed/owned extent. */
struct pt_sampler_mixed_command_handle {struct pt_sampler_mixed_command *address;uint64_t token;};
struct pt_sampler_mixed_reader_handle {struct pt_sampler_mixed_reader *address;uint64_t token;};
enum pt_sampler_mixed_result {PT_SAMPLER_MIXED_OK,PT_SAMPLER_MIXED_PENDING,
    PT_SAMPLER_MIXED_INVALID,PT_SAMPLER_MIXED_CAPACITY,PT_SAMPLER_MIXED_STALE,PT_SAMPLER_MIXED_BUSY};
struct pt_sampler_mixed_config {
    struct pt_allocator allocator;
    struct pt_sampler *sampler;struct pt_project *project;
    struct pt_mixed_readers_output *queue; /* borrowed genuine queue, NEVER closed here */
    struct pt_amigus_wavetable_cache *backend; /* dedicated, attached and initially empty */
    void *chip_context;void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
    size_t control_budget,chip_budget;uint64_t generation;
    unsigned maximum_commands,maximum_readers,context_count;
    /* Complete original allocator/sampler/Chip/bus/reservation/queue callback
     * contexts, including unknown opaque extents. Every non-NULL callback
     * context must lie in a declared extent. Overlapping borrowed contexts may
     * share one complete extent; mutable factory controls must be disjoint.
     * Sources/context storage is genuine and remains stable through close. */
    struct pt_mixed_readers_span contexts[PT_SAMPLER_MIXED_CONTEXTS];
};
struct pt_sampler_mixed_request {
    enum pt_mixed_readers_kind kind;unsigned track,sample,channel;
    /* TRIGGER: genuine already-held current master, held by caller through READY
     * or cancellation. Each advance retains an independent persistent pin.
     * CONTROL/STOP: NULL expected and original positive getter key only. */
    struct pt_sample_version *expected;struct pt_mixed_readers_key key;
    union {
        struct {uint16_t period;uint8_t volume;} paula;
        struct {
            unsigned bits,little_endian;
            struct pt_amigus_voice_request trigger;
            uint32_t rate;uint16_t left,right; /* CONTROL only */
        } amigus;
    } geometry;
};
/* Opt-in owner-thread software factory. No casts/copies of legacy owners, no
 * public validation/READY certificate, promotion, timer, voice/DMA/MMIO/PLAY,
 * song/editor integration, production backend or hardware acceptance.
 * One pool/support allocation + <=2 command/<=32 reader allocations, counted
 * together against control_budget. Chip leases have a separate byte budget and
 * exact pointer/size ledger. Cache CAPACITY may evict unpinned derived entries;
 * live pins and caller outputs remain protected, not a byte-identical cache.
 * Require already-established genuine sampler masters. Open verifies zero held
 * command/reader counts on the borrowed queue. Its owner requires factory-only
 * enqueue/services while bound, and keeps that queue alive until factory close.
 * Fresh session provenance and future exclusivity remain caller obligations.
 * The queue owner may explicitly stop unpublished queue domains locally; this
 * releases genuine transferred holders without musical STOP or forced live quiet. Open performs finite
 * metadata admission and begins its OWN cancellable incremental validator.
 * Complete workspace CAPACITY/config/output/source/context/queue/backend spans
 * and budget are checked BEFORE writes, writable reentry faults or callbacks.
 * Private scratch contents are unspecified after an admitted attempt. Fresh
 * zeroed workspace has genuine queried size/alignment and expires after open.
 * Config expires after open; copied callbacks/contexts and sampler/project fixed
 * controls outlive close. Source/header/value edits are forbidden within calls;
 * between calls every edit changes revision or sampler generation, except valid
 * selection cursor changes. Fixed identities precede every former-table walk.
 * Recognized allocation aliases are never initialized/released as fresh memory.
 * Fresh returns after detectable callback fault/staleness are released once.
 */
size_t pt_sampler_mixed_workspace_size(void);
size_t pt_sampler_mixed_workspace_alignment(void);
size_t pt_sampler_mixed_pool_size(void);
size_t pt_sampler_mixed_command_size(void);
size_t pt_sampler_mixed_reader_size(void);
enum pt_sampler_mixed_result pt_sampler_mixed_open(const struct pt_sampler_mixed_config *,
    uint32_t revision,void *workspace,size_t capacity,struct pt_sampler_mixed_pool **);
/* <=work actual validator items (1..PT_PROJECT_VALIDATION_WORK_MAX), or one
 * post-validation private metadata bind with separate actual owned()==1 gate.
 * No cache acquisition, master pin, clock/submit/service or voice start here.
 * Completion is internal to this pool, never a caller transferable certificate.
 */
enum pt_sampler_mixed_result pt_sampler_mixed_advance_validation(struct pt_sampler_mixed_pool *,uint32_t,unsigned);
/* Full immutable requests are guarded/copied before allocation callbacks.
 * Private controls transfer only at typed enqueue. Genuine master expected pins
 * remain caller-held through preparation; duplicate TRIGGERs own independent
 * pins/leases. CONTROL/STOP retain only numeric original keys, no predecessor
 * pointer/owner borrow. Paula supports whole signed8 ONCE/even padded geometry;
 * AmiGUS supports selected-channel8/16 with explicit endian and voice-plan bounds.
 */
enum pt_sampler_mixed_result pt_sampler_mixed_begin(struct pt_sampler_mixed_pool *,uint32_t,
    uint64_t frame,const struct pt_sampler_mixed_request *,unsigned,struct pt_sampler_mixed_command_handle *);
/* One transition/action OR <=256 derived cache bytes, never bulk promotion.
 * Each persistent master pin is distinct from prepared-job TEMP pins. HIT/LOAD,
 * completion/cancel of the TEMP job cannot retire the persistent pin.
 * Partial/cache failures cancel ONLY untransferred resources and latch the
 * command failure. No automatic retry/rebuild. Full card capacity is obtained
 * by matching the genuine resource pointer to arena.block, NEVER lease.slot.
 */
enum pt_sampler_mixed_result pt_sampler_mixed_advance(struct pt_sampler_mixed_pool *,uint32_t,
    struct pt_sampler_mixed_command_handle,unsigned *completed_actions);
enum pt_mixed_readers_result pt_sampler_mixed_enqueue(struct pt_sampler_mixed_pool *,uint32_t,
    struct pt_sampler_mixed_command_handle,uint64_t *ticket);
/* Ownership handle only, not ACTIVE. Retain before detached command close.
 * Contexts remain registered after independent terminal proof until explicit
 * handle close. LIVE/unadopted/uncertain resources cannot be force-cancelled.
 */
enum pt_sampler_mixed_result pt_sampler_mixed_reader(struct pt_sampler_mixed_pool *,
    struct pt_sampler_mixed_command_handle,unsigned,struct pt_sampler_mixed_reader_handle *);
enum pt_mixed_readers_result pt_sampler_mixed_reader_key(struct pt_sampler_mixed_pool *,
    struct pt_sampler_mixed_reader_handle,struct pt_mixed_readers_key *);
/* Serialized borrowed-queue services. Complete outputs protect full pool,
 * every registered LIVE/RETIRED context, captured source capacities, contexts
 * and Chip arenas before writes/callbacks. <=one corresponding underlying call.
 * Effects/proofs retain their actual classification. Factory failure suppresses
 * external receipts; NULL out explicitly drains after stale source without any
 * former-table walk. Source/queue/backend owners stay alive through draining.
 */
enum pt_mixed_readers_result pt_sampler_mixed_publish(struct pt_sampler_mixed_pool *,uint64_t);
enum pt_mixed_readers_result pt_sampler_mixed_service_command(struct pt_sampler_mixed_pool *,
    uint64_t,unsigned cancel,struct pt_mixed_readers_command_receipt *);
enum pt_mixed_readers_result pt_sampler_mixed_service_reader(struct pt_sampler_mixed_pool *,
    uint64_t,unsigned action,unsigned cancel,struct pt_mixed_readers_reader_receipt *);
/* Cancel/local stop touches only untransferred factory resources: no clock,
 * publish, domain polling, musical STOP or queue-close. Retained LIVE domains
 * require exact command detach and exact reader retirement plus zero refs.
 * Close uses only captured own jobs/pins/leases and numeric guards, never old
 * project/source tables. Registration/ledger/handle retires BEFORE each release
 * callback. A final callback fault may report0 after consuming a slot toNULL;
 * never retry freed storage. Repeated NULL-handle/NULL-pool close reads no expired
 * controls. After all handles close, factory close retires/clears its dedicated derived
 * backend cache identity; it never detaches/closes queue/backend/reservation.
 */
enum pt_sampler_mixed_result pt_sampler_mixed_cancel(struct pt_sampler_mixed_pool *,struct pt_sampler_mixed_command_handle);
enum pt_sampler_mixed_result pt_sampler_mixed_stop(struct pt_sampler_mixed_pool *);
int pt_sampler_mixed_command_close(struct pt_sampler_mixed_pool *,struct pt_sampler_mixed_command_handle *);
int pt_sampler_mixed_reader_close(struct pt_sampler_mixed_pool *,struct pt_sampler_mixed_reader_handle *);
int pt_sampler_mixed_close(struct pt_sampler_mixed_pool **);
#endif
