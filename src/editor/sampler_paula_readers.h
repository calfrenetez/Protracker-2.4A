#ifndef PT_SAMPLER_PAULA_READERS_H
#define PT_SAMPLER_PAULA_READERS_H
#include "sampler.h"
#include "../core/scheduled_readers.h"
#define PT_PAULA_READERS_COMMANDS 8
#define PT_PAULA_READERS_PERSISTENT 8
#define PT_PAULA_READERS_ACTIONS 4
struct pt_paula_readers_pool;
struct pt_paula_readers_command;
struct pt_paula_readers_reader;
enum pt_paula_readers_result {PT_PAULA_READERS_OK,PT_PAULA_READERS_PENDING,PT_PAULA_READERS_INVALID,
    PT_PAULA_READERS_CAPACITY,PT_PAULA_READERS_STALE,PT_PAULA_READERS_BUSY};
struct pt_paula_readers_config {
    unsigned maximum_commands,maximum_readers;size_t control_budget,chip_budget;uint64_t generation;
    void *chip_context;void *(*chip_allocate)(void *,size_t);void (*chip_release)(void *,void *,size_t);
};
/* CONTROL/STOP copies only this immutable full original key and numeric route.
 * The key must come from this adapter's positive reader_key getter. No predecessor pointer/master/cache borrow exists in its preparation or READY
 * holder. Every task-side validation resolves a still registered genuine reader.
 * TRIGGER has a zero key. Same cache/slot/version alone is never ACTIVE proof. */
struct pt_paula_readers_request {
    enum pt_scheduled_kind kind;unsigned track,sample,channel;struct pt_readers_key key;
};
struct pt_paula_readers_view {
    const uint8_t *data;size_t bytes;uint32_t frames;unsigned track,sample,channel,slot;uint64_t token;
};
/* New independent bounded pool. Borrowed sampler/project/allocator/queue stay
 * alive through close. No owner casts/copies, in-place source edits or reentry.
 * Caller inputs/outputs stay disjoint from opaque queue/backend/allocator/Chip
 * callback context extents that this adapter cannot discover. Legacy open
 * performs synchronous initial project validation and allocation. No activation/backend
 * implementation or native DMA/IRQ/ref-transfer qualification is implied. */
/* Optional cancellable initial setup, task-side only. Zero-initialize this
 * caller-owned workspace; its fields are private and it owns no allocations.
 * begin OK means initialized, never semantically validated. step returns
 * PENDING until the complete current project passes, then OK without creating a
 * pool. work must be 1..PT_PROJECT_VALIDATION_WORK_MAX; each step charges at most
 * work validator items (no wall-clock promise). No pool, pin, Chip allocation,
 * cache lease, or backend call occurs before transfer.
 * Keep project/tables/full master capacities, sampler, allocator/config objects
 * and callback contexts alive through transfer/cancel. Borrowed source metadata
 * and values remain immutable during every call, including callbacks. Between
 * calls every edit changes revision or sampler generation,
 * except a valid channel-selection cursor. Header/table/count/tag changes refuse
 * STALE before former tables are read; a resumed descriptor is checked as well.
 * Workspace and transfer output must be disjoint from all known source/control
 * spans, including unused master capacity. Opaque callback contexts additionally
 * stay caller-disjoint. Wrapped spans and aliases refuse without publication.
 * transfer requires completed/current validation and does one fixed pool
 * allocation synchronously. Callback mutation/reentry is rechecked before any
 * source traversal/publication. This detects fixed-header/generation changes,
 * not arbitrary in-place edits made in violation of the immutable-call rule.
 * CAPACITY leaves a ready job retryable; ordinary
 * refusals preserve output, and callback reentry latches INVALID until cancel.
 * Successful transfer consumes/zeros the workspace. cancel reads no former
 * source tables and frees nothing; while a callback is active it refuses BUSY
 * and latches failure. Do not copy/modify a live workspace or reuse it before
 * cancel/success. Configuration generation is the uint64 scheduler domain,
 * distinct from caller revision and captured sampler generation.
 * No general validation certificate or unchecked public bridge bind is exposed.
 */
struct pt_paula_readers_preparation {
    struct pt_project_validation validation;
    struct pt_allocator allocator,sampler_allocator;
    struct pt_paula_readers_config config;
    const struct pt_allocator *allocator_source;
    const struct pt_paula_readers_config *config_source;
    struct pt_sampler *sampler;
    struct pt_sample *table,*table_original;
    size_t table_bytes;
    unsigned generation,state,busy,failed;
};
enum pt_paula_readers_result pt_paula_readers_prepare_begin(struct pt_paula_readers_preparation *,
    const struct pt_allocator *,struct pt_sampler *,struct pt_project *,
    const struct pt_paula_readers_config *,uint32_t revision);
enum pt_paula_readers_result pt_paula_readers_prepare_step(struct pt_paula_readers_preparation *,
    uint32_t revision,unsigned work);
enum pt_paula_readers_result pt_paula_readers_prepare_transfer(struct pt_paula_readers_preparation *,
    uint32_t revision,struct pt_paula_readers_pool **);
enum pt_paula_readers_result pt_paula_readers_prepare_cancel(struct pt_paula_readers_preparation *);
enum pt_paula_readers_result pt_paula_readers_open(const struct pt_allocator *,struct pt_sampler *,
    struct pt_project *,const struct pt_paula_readers_config *,struct pt_paula_readers_pool **);
enum pt_paula_readers_result pt_paula_readers_begin(struct pt_paula_readers_pool *,struct pt_readers_output *,
    uint64_t,const struct pt_paula_readers_request *,unsigned,struct pt_paula_readers_command **);
/* One transition/action OR <=4096 master bytes OR <=256 Chip bytes. Trigger
 * resources remain private until ready/transfer. Controls allocate no master or
 * Chip copy/ref. Invalid aliases preserve state/output; valid stale/error calls
 * cancel only untransferred resources. Submitted domains require exact proofs. */
enum pt_paula_readers_result pt_paula_readers_step(struct pt_paula_readers_command *,unsigned *);
enum pt_paula_readers_result pt_paula_readers_view(struct pt_paula_readers_command *,unsigned,
    struct pt_paula_readers_view *);
enum pt_scheduled_result pt_paula_readers_enqueue(struct pt_paula_readers_command *,
    const struct pt_scheduled_batch *,uint64_t *);
/* Independent reader handle exists after successful trigger transfer. Retain it
 * before closing the detached command. A stale command cannot resolve a reused
 * reader address/token. This getter is ownership/geometry, not ACTIVE proof. */
enum pt_paula_readers_result pt_paula_readers_reader(struct pt_paula_readers_command *,unsigned,
    struct pt_paula_readers_reader **);
enum pt_paula_readers_result pt_paula_readers_reader_view(struct pt_paula_readers_reader *,struct pt_paula_readers_view *);
enum pt_scheduled_result pt_paula_readers_reader_key(struct pt_paula_readers_reader *,struct pt_readers_key *);
/* Command DETACHED releases only its small holder. Reader retirement separately
 * releases the genuine master pin/cache lease after all referencing commands
 * detach. Uncertain/unadopted submitted readers retain storage. LIVE close/cancel
 * refuses; no force unpin/reset/retry. Explicit handle close removes registration.
 * Retired handles remain safe to close after generation/source invalidation. */
enum pt_paula_readers_result pt_paula_readers_cancel(struct pt_paula_readers_command *);
int pt_paula_readers_command_close(struct pt_paula_readers_command *);
int pt_paula_readers_reader_close(struct pt_paula_readers_reader *);
int pt_paula_readers_close(struct pt_paula_readers_pool *);
#endif
