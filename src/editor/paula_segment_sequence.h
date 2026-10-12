#ifndef PT_PAULA_SEGMENT_SEQUENCE_H
#define PT_PAULA_SEGMENT_SEQUENCE_H
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The fixed segment sequence producer is a HOST model only"
#endif
#include "sampler_paula_segment_readers.h"
#include "../core/paula_segment_boundary.h"
#include "../core/render_lookahead.h"
struct pt_paula_segment_sequence;
struct pt_paula_segment_sequence_config {
    struct pt_render_options render;
    struct pt_paula_segment_readers_config readers;
    uint64_t origin;
    size_t control_budget;
};
enum pt_paula_segment_sequence_result {
    PT_PAULA_SEQUENCE_PENDING,PT_PAULA_SEQUENCE_READY,PT_PAULA_SEQUENCE_WAIT_ACTIVE,
    PT_PAULA_SEQUENCE_WAIT_PRESSURE,PT_PAULA_SEQUENCE_DONE,PT_PAULA_SEQUENCE_INVALID,
    PT_PAULA_SEQUENCE_STALE,PT_PAULA_SEQUENCE_CAPACITY,PT_PAULA_SEQUENCE_FAILED
};
enum pt_paula_segment_sequence_phase {
    PT_PAULA_SEQUENCE_SETUP,PT_PAULA_SEQUENCE_TAKE,PT_PAULA_SEQUENCE_MEASURE,
    PT_PAULA_SEQUENCE_NEXT,PT_PAULA_SEQUENCE_FORECAST,PT_PAULA_SEQUENCE_CONSUME,
    PT_PAULA_SEQUENCE_AUDIT,PT_PAULA_SEQUENCE_REWIND,PT_PAULA_SEQUENCE_POOL_OPEN,
    PT_PAULA_SEQUENCE_POOL_PREPARE,PT_PAULA_SEQUENCE_KEY,PT_PAULA_SEQUENCE_BEGIN,
    PT_PAULA_SEQUENCE_ENQUEUE,PT_PAULA_SEQUENCE_PUBLISH,PT_PAULA_SEQUENCE_COMMIT,
    PT_PAULA_SEQUENCE_END,PT_PAULA_SEQUENCE_CANCELLED,PT_PAULA_SEQUENCE_ERROR
};
struct pt_paula_segment_sequence_status {
    enum pt_paula_segment_sequence_phase phase;
    enum pt_paula_segment_sequence_result result;
    enum pt_render_result render_result;
    enum pt_scheduled_result scheduled_result;
    uint64_t frame,boundary,intervals,ticket[4];
    /* accepted or uncertain submit mask; not ACTIVE/quiet/retirement proof */
    unsigned audit_events,live_events,published_mask,detached_mask,reader_retired;
    unsigned cancelled,failed,callback_faults;size_t control_bytes,peak_control_bytes;
};
/* HOST-only, serialized, noncopyable fixed four-event producer. Input project
 * is already immutable/validated; bridge and its three genuine CURRENT caller
 * pins are admitted by the existing sampler API and remain held until close.
 * All source storage and callback contexts outlive close. Any edit changes the
 * revision/generation. No unannounced in-place edit/concurrent callback safety.
 * Normal one-order/one-pattern traversal only; no row-range or general song.
 *
 * Caller supplies a separately owned ordinary control workspace (full capacity
 * guarded). It is retained until successful close; size is included in the
 * total control budget along with actual child allocation sizes. Original
 * allocator/config argument structs are copied; callback contexts remain live.
 * The five original constructor argument/output spans are protected during
 * begin callbacks only, then erased; copied argument lifetime is not extended.
 * At most128 captured Source spans and16 live child allocations; overflow
 * refuses. Known source/full capacities and allocated controls are disjoint; unenumerated
 * opaque callback extents additionally remain caller-disjoint obligations.
 * Constructor metadata/setup begin is finite synchronous work, not an IRQ or
 * latency promise. Subsequent step performs ONE phase operation: <=work setup
 * items (1..256), <=work timeline ticks/frames, or one existing finite-pool step
 * (<=256 representation bytes). These units have different costs.
 *
 * Audit traverses and rewinds the SAME genuinely measured renderer sequence.
 * It checks exactly four nonempty ordered plans/dependency membership at
 * origin+960/1920/2880/3840. It supplies no fake key/certificate: continuation
 * geometry/currentness is admitted only LIVE by boundary+finite APIs after a
 * genuine positive key getter. Plans apply AFTER their preceding interval, at
 * the beginning of the next interval; a silent genuine row precedes SEGMENT.
 * Source/queue/ownership history is never rewound. Each holder index0..3 is
 * used once. Actual queue command slots retire through independent proofs;
 * no fifth command or rolling holder reuse is authorized.
 *
 * step never publishes. publish_next makes one explicit original-deadline
 * attempt. Clean PENDING freezes plan/frame/ticket for an explicit later call;
 * no clock rebase, catch-up, automatic retry or musical stop-now. Uncertainty,
 * stale source or any actual callback reentry latches failure. Service APIs
 * still permit genuine independent cleanup using retained original identities.
 * Command detach and reader retirement are separate; either proof order is
 * permitted. Cancellation stops local work only; no forced quiet or unpin.
 * A new release callback fault makes close return0 with its original owner
 * retained. Already consumed child slots/bytes stay consumed; a later explicit
 * close may consume only the remaining outer control, never double-dispose.
 * Native output/fetch boundaries/IRQ/MMIO/frontend/timing remain inhibited.
 */
size_t pt_paula_segment_sequence_control_size(void);
size_t pt_paula_segment_sequence_control_alignment(void);
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_begin(
    const struct pt_allocator *,struct pt_sampler_paula *,
    const struct pt_paula_segment_sequence_config *,const struct pt_paula_segment_backend *,
    uint32_t revision,void *workspace,size_t capacity,struct pt_paula_segment_sequence **);
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_step(
    struct pt_paula_segment_sequence *,uint32_t revision,unsigned work);
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_get(
    struct pt_paula_segment_sequence *,struct pt_paula_segment_sequence_status *);
enum pt_scheduled_result pt_paula_segment_sequence_publish_next(
    struct pt_paula_segment_sequence *,uint32_t revision);
enum pt_scheduled_result pt_paula_segment_sequence_service_command(
    struct pt_paula_segment_sequence *,unsigned index,unsigned cancel);
enum pt_scheduled_result pt_paula_segment_sequence_service_reader(
    struct pt_paula_segment_sequence *,unsigned cancel);
enum pt_paula_segment_sequence_result pt_paula_segment_sequence_cancel(struct pt_paula_segment_sequence *);
int pt_paula_segment_sequence_close(struct pt_paula_segment_sequence **);
#endif
