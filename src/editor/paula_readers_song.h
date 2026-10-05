#ifndef PT_PAULA_READERS_SONG_H
#define PT_PAULA_READERS_SONG_H
#include "paula_preflight.h"
#include "sampler_paula_readers.h"
#include "../core/render_lookahead.h"

#define PT_PAULA_READERS_SONG_COMMANDS 2U
#define PT_PAULA_READERS_SONG_READERS 8U
struct pt_paula_readers_song;
enum pt_paula_readers_song_result {
    PT_PAULA_READERS_SONG_OK, PT_PAULA_READERS_SONG_PENDING, PT_PAULA_READERS_SONG_DONE,
    PT_PAULA_READERS_SONG_WAIT_ACTIVE, PT_PAULA_READERS_SONG_WAIT_PRESSURE,
    PT_PAULA_READERS_SONG_INVALID, PT_PAULA_READERS_SONG_STALE, PT_PAULA_READERS_SONG_CAPACITY,
    PT_PAULA_READERS_SONG_BUSY, PT_PAULA_READERS_SONG_FAILED
};
enum pt_paula_readers_song_phase {
    PT_PAULA_READERS_SONG_INITIAL, PT_PAULA_READERS_SONG_RETAIN, PT_PAULA_READERS_SONG_AUDIT,
    PT_PAULA_READERS_SONG_POOL_VALIDATE, PT_PAULA_READERS_SONG_OUTPUT_SETUP,
    PT_PAULA_READERS_SONG_NEXT, PT_PAULA_READERS_SONG_FORECAST, PT_PAULA_READERS_SONG_KEYS,
    PT_PAULA_READERS_SONG_LOWER, PT_PAULA_READERS_SONG_PREPARE, PT_PAULA_READERS_SONG_ENQUEUE,
    PT_PAULA_READERS_SONG_PUBLISH, PT_PAULA_READERS_SONG_END, PT_PAULA_READERS_SONG_DRAIN,
    PT_PAULA_READERS_SONG_ERROR
};
struct pt_paula_readers_song_config {
    struct pt_render_options render;
    struct pt_paula_render_caps caps;
    struct pt_paula_readers_config readers;
    struct pt_scheduled_grid grid;
    struct pt_readers_backend backend;
    uint64_t session,absolute_start;
    /* Total ordinary control allocation budget, including this owner and its
     * genuine children. Dedicated representation budget remains readers.chip_budget.
     * The allocator does not imply Fast/Chip placement qualification. */
    size_t control_budget;
};
struct pt_paula_readers_song_status {
    enum pt_paula_readers_song_phase phase;
    enum pt_paula_readers_song_result result;
    uint64_t boundary_frame,terminal_frame,intervals;
    unsigned command_mask,reader_mask,published_mask;
    unsigned retained_masters,done,terminal_stop_requested,cancelled;
    enum pt_render_result render_result;
    enum pt_scheduled_result scheduled_result;
};
/* published_mask includes accepted/uncertain submissions. It never denotes
 * positive adoption, activation, command detachment or reader retirement. */

/* Optional software schedule producer, never legacy PLAY or a device backend.
 * Owns genuine opaque startup/audit/the SAME sequence, lookahead, queue and pool.
 * No certificate, owner cast/copy or unchecked validation bridge is accepted.
 * Uses a fresh deterministic Paula map (previous=NULL), command capacity2 and
 * reader capacity8. Backend declarations are promises, not hardware proof.
 *
 * Before begin, every nonempty master must already have a genuine matching
 * sampler.current version. Missing/mismatched versions refuse; this producer
 * NEVER promotes document PCM while a sequence borrows it. After initial READY,
 * one existing master reference is retained per step. Empty slots are skipped.
 * Project/table/PCM/marker/extension storage and sampler remain alive/immutable
 * through close; every edit between calls changes revision or sampler generation.
 * A valid selection cursor may change. Source/control changes during callbacks
 * are forbidden. Calls are serialized; begin allocation callback reentry is a
 * caller obligation before the owner exists. Later reentry latches failure.
 *
 * Copies config/allocator arguments during begin; originals need live only
 * through that call. Copied callbacks/contexts remain alive through close.
 * Allocator/Chip opaque context extents remain caller-disjoint obligations.
 * Explicit backend context_bytes is guarded. All known full source capacities,
 * live controls and tracked allocation extents are protected before writes.
 * An overlapping returned arena is not fresh ownership and is never released.
 * Begin snapshots bounded metadata spans (<=6144), no PCM/order/event/slice
 * values. It owns one fixed ordinary control plus the actual startup controls;
 * fixed allocator/metadata work is not a wall-clock guarantee.
 * The first constructor also uses a bounded local metadata snapshot; native
 * stack adequacy (including a 65536-byte stack) is NOT qualified by this API.
 */
size_t pt_paula_readers_song_control_size(void);
enum pt_paula_readers_song_result pt_paula_readers_song_begin(const struct pt_allocator *,
    struct pt_sampler *,struct pt_project *,const struct pt_paula_readers_song_config *,
    uint32_t revision,struct pt_paula_readers_song **);
/* One genuine phase operation per call: validation work1..4096; audit <=256
 * ticks/frames; forecast/live consume <=256 frames; one existing reader prep
 * transition or <=4096 master bytes/256 representation bytes. Metadata guards
 * are separately finite. No implicit backend publication or receipt polling.
 * The original grid/start/frame is frozen; pressure/WAIT_ACTIVE never rebases.
 * READY ownership/cache preparation is never positive ACTIVE evidence.
 * Optional status is published only after its complete output guards.
 */
enum pt_paula_readers_song_result pt_paula_readers_song_step(struct pt_paula_readers_song *,
    uint32_t revision,unsigned work,struct pt_paula_readers_song_status *);
enum pt_paula_readers_song_result pt_paula_readers_song_get(struct pt_paula_readers_song *,
    uint32_t revision,struct pt_paula_readers_song_status *);
/* Exactly one oldest unpublished submit attempt. PENDING means confirmed no
 * acceptance and permits an EXPLICIT later call in the same original window.
 * Uncertainty/late/stale latches failure; no retry/catch-up/next publication.
 */
enum pt_scheduled_result pt_paula_readers_song_publish_next(struct pt_paula_readers_song *,
    uint32_t revision);
/* Record index is bounded by 2/8; producer retains the actual ticket/action.
 * Each call performs <=one backend callback. Exact independently valid domain
 * proofs may drain after sticky failure; caller never supplies a proof/key.
 * A detached command may close independently; reader storage remains until
 * exact retirement AND zero referencing commands. Outputs optional.
 * A command cancellation/positive non-issued failure classification stops new
 * production. Cancelling an old replaced reader does not close its successor.
 * Sticky failure or stale source suppresses external receipt publication while
 * exact independent proofs may still release their own domains. For stale
 * source, use NULL output to perform explicit drain without former-table reads.
 */
enum pt_scheduled_result pt_paula_readers_song_service_command(struct pt_paula_readers_song *,
    unsigned index,unsigned cancel,struct pt_readers_command_receipt *);
enum pt_scheduled_result pt_paula_readers_song_service_reader(struct pt_paula_readers_song *,
    unsigned index,unsigned cancel,struct pt_readers_reader_receipt *);
/* DONE is empty renderer end, never STOP or retirement. Explicit STOP uses the
 * unchanged terminal frame and only exact positive prospective original keys.
 * A frame collision/late deadline refuses; no stop-now/one-frame shift.
 */
enum pt_paula_readers_song_result pt_paula_readers_song_terminal_stop(struct pt_paula_readers_song *,
    uint32_t revision);
/* Stops new production and cancels local forecast/preparation/unpublished
 * queue domains once. It sends NO musical STOP and performs no backend poll.
 * Submitted/uncertain domains remain for explicit service. Close refuses until
 * all registered domains quiesce; no automatic retry, destructor poll or force
 * unpin. Cancellation/close do not traverse former source tables.
 * A refused close normally retains its owner. New callback reentry or fixed
 * control changes during child/final release may return0 after consuming it:
 * an unchanged caller slot is NULL, never a freed owner. A preexisting sticky
 * failure does not prevent otherwise quiet, independently proven cleanup.
 */
enum pt_paula_readers_song_result pt_paula_readers_song_cancel(struct pt_paula_readers_song *);
int pt_paula_readers_song_close(struct pt_paula_readers_song **);
#endif
