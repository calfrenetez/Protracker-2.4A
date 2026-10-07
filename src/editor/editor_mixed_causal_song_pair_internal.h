#ifndef PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INTERNAL_H
#include "editor_mixed_causal_source_internal.h"
#include "sampler_establish.h"
#include "mixed_quantized_audit.h"
#include "../core/render_lookahead.h"

#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REQUESTS 3U
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PARENTS 11U
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_CONTEXTS 7U
#define PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_CONTEXTS 10U
struct pt_editor_mixed_causal_song_pair;

/* Caller initialises this configuration before begin, then keeps it unchanged.
 * All pointed callback contexts belong to one complete SOURCE contexts parent:
 * it contains this complete producer, the separate genuine controller and every
 * ordinary/master/progress/Chip/port/bus context. The card/cache/reservation
 * complete parent is B. No narrowed opaque context or unknown outside pointee.
 * No allocator swap: sampler.allocator stays exactly bound throughout.
 * ordinary_budget charges full parent+SI+SB+eight full spans+live external
 * controls; sampler/factory/Chip/card budgets remain separate. audit_budget is
 * the genuine strict audit's separately checked full workspace/control budget.
 */
struct pt_editor_mixed_causal_song_pair_configuration {
    struct pt_editor_mixed_causal_prepare_inputs preparation;
    struct pt_render_options options;
    struct pt_paula_render_caps caps;
    struct pt_playback_format format;
    struct pt_allocator external_allocator;
    size_t ordinary_budget,audit_budget;
    uint64_t absolute_start;
};

/* ORIGINAL typed U parent, supplied at full actual capacity including its tail.
 * These separate nonunion slots are legitimate writable child outputs. Keep U
 * outside the complete P and all immutable child context parents. AR is a
 * separate mutable raw audit result buffer, including full supplied tail.
 */
struct pt_editor_mixed_causal_song_pair_outputs {
    struct pt_mixed_quantized_audit *audit;
    struct pt_render_sequence *sequence;
    struct pt_mixed_plan_normalizer *normalizer;
    struct pt_render_interval interval;
    struct pt_render_plan plan;
    struct pt_mixed_plan_quantized_batch normalized,candidate[2];
    struct pt_editor_mixed_readers_quantized_batch batch;
    struct pt_editor_mixed_command_ref command;
    struct pt_editor_mixed_reader_ref reader;
    uint64_t ticket;
    struct pt_mixed_readers_command_receipt command_receipt;
    struct pt_mixed_readers_reader_receipt reader_receipt;
    struct pt_mixed_quantized_audit_report audit_report;
    struct pt_pcm temporary_pcm,persistent_pcm;
    struct pt_sample_version *temporary_pin,*persistent_pin;
    unsigned ready;
};

enum pt_editor_mixed_causal_song_pair_result {
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_INVALID=0,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PENDING,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_WAIT_ACTIVE,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_WAIT_CAPACITY,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLISH,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLISHED,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CANCELLED,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_STALE,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REFUSED,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CAPACITY,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FAULT,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CLOSED
};
enum pt_editor_mixed_causal_song_pair_phase {
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_EMPTY=0,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_CURRENT,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PIN_FINISH,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ESTABLISH_CLOSE,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ACTIVATE,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_GET,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_TAKE,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_TRANSFER_CHECK,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONTROLLER,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_LOAD,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_NEXT,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FORECAST_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FORECAST_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_GET,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_CLOSE,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_LOWER,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_BATCH_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_BATCH_STEP,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REFERENCES,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_ENQUEUE,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PUBLICATION,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_CONSUME,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_COMMIT,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_END,
    PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_DRAIN,PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_FINISHED
};
struct pt_editor_mixed_causal_song_pair_allocation {
    /* Unresolved is numeric custody only. It cannot authorize initialization,
     * release, retry or recovery. Its full request remains budgeted and blocks
     * final SOURCE/pin closure for this producer lifetime. */
    uintptr_t address;size_t bytes;unsigned live,unresolved;
};
struct pt_editor_mixed_causal_song_pair_command {
    struct pt_editor_mixed_command_ref reference;
    uint64_t ticket;unsigned published,uncertain;
};
struct pt_editor_mixed_causal_song_pair_reader {
    struct pt_editor_mixed_reader_ref reference;
    unsigned retirement_consumed,uncertain;
};
struct pt_editor_mixed_causal_song_pair_status {
    enum pt_editor_mixed_causal_song_pair_result result,first_error;
    enum pt_editor_mixed_causal_song_pair_phase phase;
    enum pt_mixed_readers_result scheduled_result;
    uint64_t consumed_frames,intervals,target;
    unsigned pin_count,command_count,reader_count,allocation_requests;
};

/* Zero-init caller-owned state, serialized and noncopyable. The controller and
 * producer are distinct subobjects of the same full parent aggregate (the parent
 * may have callback contexts and unused tail beyond either sizeof). Before begin,
 * set controller and configuration only. SI/SB are original separate controls.
 * SI uses E/F/B/A/AQ/Q in immutable[0..5], AR/U in mutable[0..1], with exact full
 * capacities. SI.preparation names this producer's preparation field. All other
 * active state and U owner/result slots initially zero.
 * Persistent pins are genuine ownership, not a readiness certificate. The
 * C2/R32 maps contain only actual refs published by the controller to U.
 */
struct pt_editor_mixed_causal_song_pair {
    struct pt_editor_mixed_causal_prepare *controller;
    struct pt_editor_mixed_causal_song_pair_configuration configuration,saved;
    struct pt_editor_mixed_causal_song_pair *original;
    struct pt_editor_mixed_causal_prepare_inputs preparation;
    const struct pt_editor_mixed_causal_source_inputs *source;
    struct pt_editor_mixed_causal_source_borrow *borrow;
    struct pt_editor_mixed_causal_song_pair_outputs *outputs;
    struct pt_sampler_storage_span parent;
    struct pt_sampler_storage_span extent[8];
    struct pt_allocator allocator;
    struct pt_sampler_storage_span establish_parents[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_PARENTS];
    struct pt_mixed_plan_span audit_contexts[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_AUDIT_CONTEXTS];
    struct pt_mixed_plan_span normalizer_contexts[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_Q_CONTEXTS];
    struct pt_mixed_quantized_audit_inputs audit_inputs;
    struct pt_mixed_plan_inputs normalizer_inputs;
    struct pt_sampler_establish establish;
    struct pt_sampler_pin_job pin_job;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];
    struct pt_render_lookahead lookahead;
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS];
    struct pt_mixed_plan_quantized_batch frozen[2];
    struct pt_editor_mixed_causal_song_pair_command command[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_causal_song_pair_reader reader[PT_SAMPLER_MIXED_READERS];
    struct pt_editor_mixed_causal_song_pair_allocation allocation[PT_EDITOR_MIXED_CAUSAL_SONG_PAIR_REQUESTS];
    struct pt_mixed_quantized_audit_report audited;
    uint8_t samples[2][PT_PROJECT_SAMPLES];
    uint64_t frames,intervals,target;
    size_t base_bytes,live_bytes;
    uint32_t revision,generation;
    unsigned adopted,busy,reentries,cancelled,activated,requests,pin_slot,pin_count;
    unsigned action,working_command,remaining,cleanup_phase,cleanup_slot;
    unsigned sequence_closed,borrow_closed,hook_closed,quiet_ambiguous;
    unsigned candidate_count,candidate_pending,pair_index;
    enum pt_editor_mixed_causal_song_pair_phase phase;
    enum pt_editor_mixed_causal_song_pair_result result,first_error;
    enum pt_mixed_readers_result scheduled_result;
};

/* Exactly two pure TRIGGER boundaries, full audit and real EOF replay BEFORE
 * binding/factory/cache/publication. No whole-song PLAY, CONTROL/STOP or refill.
 * PUBLISHED means both task-side publications only, not activation/end/quiet.
 * Admission and early SOURCE install only: no external callback/allocation.
 * Failure after actual adoption remains owned and must be explicitly drained. */
enum pt_editor_mixed_causal_song_pair_result pt_editor_mixed_causal_song_pair_begin(
    struct pt_editor_mixed_causal_song_pair *,const struct pt_editor_mixed_causal_source_inputs *,
    struct pt_editor_mixed_causal_source_borrow *original);
/* One genuine phase operation or <=min(work,256) frames/Q items; establishment
 * and pin bytes accept work1..4096. No publish, fire, clock or domain polling.
 * Preflight uses the taken original sequence to actual EOF before any owner
 * opening/binding/factory/cache/publication. WAIT_CAPACITY preserves the two
 * immutable descriptive snapshots; it cannot rebase or silently service domains. */
enum pt_editor_mixed_causal_song_pair_result pt_editor_mixed_causal_song_pair_step(struct pt_editor_mixed_causal_song_pair *,unsigned);
/* By-value fixed-control diagnostic only; no source walk/callback/output alias. */
struct pt_editor_mixed_causal_song_pair_status pt_editor_mixed_causal_song_pair_get(const struct pt_editor_mixed_causal_song_pair *);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_pair_publish(struct pt_editor_mixed_causal_song_pair *);
/* Ref indices refer only to this producer's actual issued ledger, not supplied
 * keys/tickets/proofs. Each explicit service performs <=one original callback.
 * C cleanup uses NULL receipts after failure/staleness. R uses the separate
 * genuine retirement pair without an output receipt. Consumed retirement only
 * suppresses another backend R call: a retained original R-first registration
 * can still request actual close after C detach; actual NULL prunes custody. */
enum pt_mixed_readers_result pt_editor_mixed_causal_song_pair_service_command(struct pt_editor_mixed_causal_song_pair *,unsigned,unsigned);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_pair_service_reader(struct pt_editor_mixed_causal_song_pair *,unsigned,unsigned);
enum pt_editor_mixed_causal_song_pair_result pt_editor_mixed_causal_song_pair_cancel(struct pt_editor_mixed_causal_song_pair *);
/* One cleanup phase, no implicit domain service. Actual failure-plus-NULL owner
 * slots are terminal. All pins stay until real external/controller/source quiet;
 * then one unpin per phase under SOURCE, borrow close, separate final hook close.
 * Context/buffers remain alive through positive final close. */
int pt_editor_mixed_causal_song_pair_close(struct pt_editor_mixed_causal_song_pair *);
#endif
