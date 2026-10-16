#ifndef PT_PREPARED_PAULA_MUTATION_BARRIER_H
#define PT_PREPARED_PAULA_MUTATION_BARRIER_H
#if defined(PT_PREPARED_PAULA_MUTATION_NATIVE_TASK_PROBE_ONLY)
#if PT_PREPARED_PAULA_MUTATION_NATIVE_TASK_PROBE_ONLY != 1
#error "The native TASK mutation probe requires the explicit value 1"
#endif
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER) || defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
#error "The native TASK mutation probe forbids HOST mutation and lifetime options"
#endif
#if !defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY) || PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY != 1
#error "The native TASK mutation probe requires the original value-1 native TASK lifetime option"
#endif
#if !defined(__m68k__) || !defined(__amigaos__)
#error "The native TASK mutation probe requires a genuine m68k AmigaOS compiler target"
#endif
#else
#if !defined(PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER)
#error "The prepared mutation barrier requires its explicit HOST opt-in"
#elif PT_PREPARED_PAULA_ENABLE_HOST_MUTATION_BARRIER != 1
#error "The prepared mutation barrier HOST option requires value 1"
#endif
#if !defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME) || PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME != 1
#error "The prepared mutation barrier requires the original HOST lifetime"
#endif
#if defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY) || defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The prepared mutation barrier has no native eligibility"
#endif
#endif
#include "prepared_paula_lifetime.h"

struct pt_editor;
struct pt_sampler_paula;
struct pt_prepared_paula_mutation_barrier;
enum pt_prepared_paula_mutation_result {
    PT_PREPARED_PAULA_MUTATION_OK=0,
    PT_PREPARED_PAULA_MUTATION_PENDING,
    PT_PREPARED_PAULA_MUTATION_READY,
    PT_PREPARED_PAULA_MUTATION_QUEUED,
    PT_PREPARED_PAULA_MUTATION_CLEANED,
    PT_PREPARED_PAULA_MUTATION_CLOSED,
    PT_PREPARED_PAULA_MUTATION_INVALID,
    PT_PREPARED_PAULA_MUTATION_ALIAS,
    PT_PREPARED_PAULA_MUTATION_BUSY,
    PT_PREPARED_PAULA_MUTATION_HELD
};
enum pt_prepared_paula_mutation_phase {
    PT_PREPARED_PAULA_MUTATION_ATTACHED=1,
    PT_PREPARED_PAULA_MUTATION_PREPARING,
    PT_PREPARED_PAULA_MUTATION_EMPTY,
    PT_PREPARED_PAULA_MUTATION_RETAINED,
    PT_PREPARED_PAULA_MUTATION_STOPPED,
    PT_PREPARED_PAULA_MUTATION_DISPOSING,
    PT_PREPARED_PAULA_MUTATION_DETACHABLE,
    PT_PREPARED_PAULA_MUTATION_FINISHED,
    PT_PREPARED_PAULA_MUTATION_UNCERTAIN
};
struct pt_prepared_paula_mutation_config {
    struct pt_editor *editor;
    struct pt_sampler_paula *bridge;
    struct pt_prepared_paula_sources *sources;size_t source_capacity;
    struct pt_prepared_paula_lifetime *lifetime;size_t lifetime_capacity;
    unsigned track,sample[PT_PREPARED_PAULA_SOURCES_COUNT];
    struct pt_prepared_paula_lifetime_config queue;
    struct pt_readers_backend backend;
};

/* A single-use, exclusive compositor of ordinary serialized TASK work. The
 * original explicit HOST mode remains a HOST model. The separate native TASK
 * mutation probe-only mode admits these same unpublished RAM operations in a
 * genuine original normal TASK, outside Disable/Enable, IRQ/exception, signal,
 * supervisor or delivery entry. It admits no frontend wiring, gate, backend
 * publication/activation, timer, DMA/audio, output, quiet or timing authority.
 * Existing source/lifetime/native21 bodies are unchanged; their native TASK
 * option alone does not admit this compositor. Neither HOST option is allowed
 * in native mutation probe mode. The included native lifetime partition still
 * excludes publication APIs, PUBLISH phase3 and cancel callback authority.
 *
 * Caller has fully validated the immutable project/PCM/markers and established
 * exactly three distinct nonempty mono8/16/24 masters. The original dedicated
 * EMPTY bridge is genuinely bound to &editor->sampler and editor->project, with
 * original budgeted Chip callbacks. No other reader, bridge user, playback,
 * sampler job, preview/workflow/capture operation or mutation may coexist. No
 * registered recording_busy function OR context is admitted or called here.
 * Scalar idle facts are not proof that unrelated external readers ended.
 *
 * All complete editor/project/document/table/PCM/marker/bridge/sampler/allocator
 * and backend/context extents and all THREE workspace capacities remain live
 * until CLOSED, or indefinitely on HELD. Project/history/sampler/source/allocator
 * state is immutable; existing editor status/UI/cursor bookkeeping on refused
 * cooperating actions is allowed. No reset/copy/reinit,
 * direct unhook, source free, allocator replacement, in-place write or callback
 * mutation/reentry. The config object may expire after successful begin copies
 * it; its named pointees cannot. Callbacks returning fresh storage must satisfy
 * their original disjoint/live allocation contract; arbitrary callback writes/
 * frees cannot be rolled back. Unknown context lengths remain caller duties.
 *
 * Uses only the existing veto-capable editor hook, never chains/replaces another
 * hook. While attached it ALWAYS refuses editor changes, latches cancellation
 * and closes enqueue admission. Hook execution never performs child/resource
 * work. A hook arriving during a service/child stack latches HELD; actual child
 * transfers remain recorded in original owners before the outer return.
 * Direct editor init, preview/source load, workflow preparation, direct sampler
 * or document APIs and current native New/Load/exit are not universally guarded
 * by that hook; they are excluded, not newly integrated by this interface.
 */
size_t pt_prepared_paula_mutation_barrier_size(void);
size_t pt_prepared_paula_mutation_barrier_alignment(void);

/* Full aligned capacities>=their respective sizes; only owned prefixes ZERO.
 * Tails are guarded complete intervals, never read/written. Numeric full-span
 * admission precedes corresponding header reads and any write. Full NEW control
 * capacities are disjoint from borrowed controls/data/context. Existing genuine
 * borrowed context arenas may overlap as their actual allocator contract permits.
 * Newly derived Chip allocations must remain disjoint from every FULL declared
 * callback/control context extent, including tails; borrowed-input overlap does
 * not permit overwriting callback metadata with a new derived cache copy.
 * Two queue context slots are reserved for full
 * barrier capacity and whole editor; supplied context_count must be<=14 and no
 * context is silently discarded. No source/queue/allocator/backend call here;
 * install/authenticate the existing hook before later child callbacks. Failed
 * admission preserves workspaces/inputs/previous editor hook. No active reuse.
 */
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_begin(
    struct pt_prepared_paula_mutation_barrier *,size_t capacity,
    const struct pt_prepared_paula_mutation_config *);

/* <=ONE original child lifecycle operation per call, no loop: source init;
 * source step (<=256 copy bytes); queue init; pre-exposure source cancellation;
 * EMPTY queue close; once actual unpublished local stop; one deferred captured
 * drain step; final queue close; dedicated bridge close. Initial source init
 * refusal is recoverable only under its genuine no-callback/no-resource
 * constructor guarantee. Queue init/enqueue/stop/close ambiguity is HELD, never
 * probed or retried. Source CAPACITY/STALE may use original later captured cancel;
 * any refused child disposal/structural uncertainty retains all anchors.
 * Bridge close is one existing fixed32-slot operation and can synchronously
 * release several allocations, not a one-allocator/WCET guarantee. It is recorded
 * before phase-aware postchecks stop following its cleared bridge pointers.
 * Source/cache guards are fresh before/after callback work; new cache intervals
 * are checked against all new controls before any later copy/free. No cached
 * proof across calls, copied source ledger or opaque child layout inspection.
 * READY/QUEUED wait for explicit cancellation; CLEANED remains attached/vetoing.
 */
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_step(
    struct pt_prepared_paula_mutation_barrier *);

/* Exactly one actual unpublished enqueue from READY. Only actual positive child
 * result records retention; a non-OK/uncertain result never means no transfer.
 * Admission closes permanently on cancel or editor-hook request. No backend
 * publish/cancel/poll, caller receipt, pin/span or reader authority is exposed.
 */
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_enqueue_once(
    struct pt_prepared_paula_mutation_barrier *);
/* Latch only. Later top-level step performs cleanup; never on this call stack. */
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_cancel(
    struct pt_prepared_paula_mutation_barrier *);

/* Numeric own-storage observations only; never follow children or borrowed
 * contexts. UNKNOWN/invalid storage returns phase0 or INVALID failure. Sticky
 * first classified source failure remains after genuine cleanup/detach; HELD
 * records FAULT if no earlier failure. Neither scalar grants release authority.
 */
enum pt_prepared_paula_mutation_phase pt_prepared_paula_mutation_barrier_state(
    const struct pt_prepared_paula_mutation_barrier *);
enum pt_prepared_paula_sources_result pt_prepared_paula_mutation_barrier_failure(
    const struct pt_prepared_paula_mutation_barrier *);

/* Separate callback-free detach only after every original child, queue and
 * dedicated bridge positively ended. Authenticate exact original editor/hook/
 * context and absence of void guard immediately before existing NULL setter;
 * foreign/missing hook is HELD and never removed/repaired. Clear borrowed anchors
 * only after actual detach. CLOSED is terminal, observational/idempotent and may
 * be queried without any editor/child/config/context storage still accessible.
 */
enum pt_prepared_paula_mutation_result pt_prepared_paula_mutation_barrier_detach(
    struct pt_prepared_paula_mutation_barrier *);
#endif
