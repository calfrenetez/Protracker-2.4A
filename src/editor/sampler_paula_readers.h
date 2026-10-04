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
 * callback context extents that this adapter cannot discover. Initial static
 * project validation and allocation remain synchronous. No activation/backend
 * implementation or native DMA/IRQ/ref-transfer qualification is implied. */
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
