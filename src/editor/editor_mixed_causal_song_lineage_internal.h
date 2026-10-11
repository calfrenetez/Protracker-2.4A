#ifndef PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INTERNAL_H
#include "editor_mixed_causal_lineage_source_internal.h"
#include "editor_mixed_causal_lineage_control16_prepare_internal.h"
#include "../core/mixed_readers_causal_factory_internal.h"
#include "sampler_establish.h"
#include "mixed_quantized_audit.h"
#include "../core/render_lookahead.h"

#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS 3U
#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PARENTS 11U
#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_CONTEXTS 7U
#define PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CONTEXTS 10U
struct pt_editor_mixed_causal_song_lineage;

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
struct pt_editor_mixed_causal_song_lineage_configuration {
    struct pt_editor_mixed_causal_prepare_inputs preparation;
    struct pt_mixed_causal_control_stop_port lineage;
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
struct pt_editor_mixed_causal_song_lineage_outputs {
    struct pt_mixed_quantized_audit *audit;
    struct pt_render_sequence *sequence;
    struct pt_mixed_plan_normalizer *normalizer;
    struct pt_render_interval interval;
    struct pt_render_plan plan;
    struct pt_mixed_plan_quantized_batch normalized,candidate[3];
    struct pt_editor_mixed_readers_quantized_batch batch;
    struct pt_editor_mixed_causal_lineage_control16_batch control;
    struct pt_editor_mixed_causal_lineage_stop_batch stop;
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

enum pt_editor_mixed_causal_song_lineage_result {
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_INVALID=0,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PENDING,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_ACTIVE,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_WAIT_CAPACITY,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISH,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLISHED,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CANCELLED,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_STALE,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFUSED,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CAPACITY,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FAULT,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CLOSED
};
enum pt_editor_mixed_causal_song_lineage_phase {
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_EMPTY=0,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_CURRENT,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PIN_FINISH,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ESTABLISH_CLOSE,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ACTIVATE,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_GET,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TAKE,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_TRANSFER_CHECK,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONTROLLER,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOAD,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_NEXT,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FORECAST_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FORECAST_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_BEGIN,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_STEP,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_GET,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CLOSE,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_LOWER,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_BEGIN,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_BATCH_STEP,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REFERENCES,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_ENQUEUE,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PUBLICATION,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_CONSUME,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_COMMIT,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_END,
    PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_DRAIN,PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_FINISHED
};
struct pt_editor_mixed_causal_song_lineage_allocation {
    /* Unresolved is numeric custody only. It cannot authorize initialization,
     * release, retry or recovery. Its full request remains budgeted and blocks
     * final SOURCE/pin closure for this producer lifetime. */
    uintptr_t address;size_t bytes;unsigned live,unresolved;
};
struct pt_editor_mixed_causal_song_lineage_command {
    struct pt_editor_mixed_command_ref reference;
    uint64_t ticket;unsigned published,uncertain;
};
/* Three numeric original identities survive real C1 disposal and slot reuse.
 * No retired handle address is stored or dereferenced. Service selects history
 * index0..2, not the reused two-slot live C index. */
struct pt_editor_mixed_causal_song_lineage_history {
    struct pt_editor_mixed_command_ref reference;
    uint64_t ticket;unsigned published,uncertain,detached,service_seen;
};
struct pt_editor_mixed_causal_song_lineage_reader {
    struct pt_editor_mixed_reader_ref reference;
    unsigned retirement_consumed,uncertain;
};
struct pt_editor_mixed_causal_song_lineage_status {
    enum pt_editor_mixed_causal_song_lineage_result result,first_error;
    enum pt_editor_mixed_causal_song_lineage_phase phase;
    enum pt_mixed_readers_result scheduled_result;
    uint64_t consumed_frames,intervals,target;
    unsigned pin_count,command_count,reader_count,allocation_requests;
};

/* Zero-init caller-owned state, serialized and noncopyable. The controller and
 * producer are distinct subobjects of the same full parent aggregate (the parent
 * may have callback contexts and unused tail beyond either sizeof). Before begin,
 * set exact whole joint, controller=&joint.original.original and configuration only. SI/SB are original separate controls.
 * SI uses E/F/B/A/AQ/Q in immutable[0..5], AR/U in mutable[0..1], with exact full
 * capacities. SI.preparation names this whole composite preparation; SI.original.preparation names its original member. All other
 * active state and U owner/result slots initially zero.
 * Persistent pins are genuine ownership, not a readiness certificate. The
 * C2/R32 maps contain only actual refs returned into disjoint admitted locals; descriptive U is never a controller output target.
 */
struct pt_editor_mixed_causal_song_rolling;
struct pt_editor_mixed_causal_song_lineage {
    struct pt_editor_mixed_causal_prepare *controller;
    struct pt_editor_mixed_causal_lineage_source_prepare *joint;
    struct pt_editor_mixed_causal_song_lineage_configuration configuration,saved;
    struct pt_editor_mixed_causal_song_lineage *original;
    struct pt_editor_mixed_causal_lineage_inputs preparation;
    const struct pt_editor_mixed_causal_lineage_source_inputs *source;
    struct pt_editor_mixed_causal_source_borrow *borrow;
    struct pt_editor_mixed_causal_song_lineage_outputs *outputs;
    struct pt_sampler_storage_span parent;
    struct pt_sampler_storage_span extent[8];
    struct pt_allocator allocator;
    struct pt_sampler_storage_span establish_parents[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_PARENTS];
    struct pt_mixed_plan_span audit_contexts[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_AUDIT_CONTEXTS];
    struct pt_mixed_plan_span normalizer_contexts[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_Q_CONTEXTS];
    struct pt_mixed_quantized_audit_inputs audit_inputs;
    struct pt_mixed_plan_inputs normalizer_inputs;
    struct pt_sampler_establish establish;
    struct pt_sampler_pin_job pin_job;
    struct pt_sample_version *pin[PT_PROJECT_SAMPLES];
    struct pt_render_lookahead lookahead;
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS];
    struct pt_mixed_plan_quantized_batch frozen[3];
    struct pt_editor_mixed_causal_song_lineage_command command[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_causal_song_lineage_history history[3];
    struct pt_editor_mixed_reader_ref track[PT_MIXED_PLAN_RECORDS];
    struct pt_editor_mixed_causal_song_lineage_reader reader[PT_SAMPLER_MIXED_READERS];
    struct pt_editor_mixed_causal_song_lineage_allocation allocation[PT_EDITOR_MIXED_CAUSAL_SONG_LINEAGE_REQUESTS];
    struct pt_mixed_quantized_audit_report audited;
    uint8_t samples[2][PT_PROJECT_SAMPLES];
    uint64_t frames,intervals,target;
    size_t base_bytes,live_bytes;
    uint32_t revision,generation;
    unsigned adopted,busy,reentries,cancelled,activated,requests,pin_slot,pin_count;
    unsigned action,working_command,remaining,cleanup_phase,cleanup_slot;
    unsigned sequence_closed,borrow_closed,hook_closed,quiet_ambiguous;
    unsigned candidate_count,candidate_pending,lineage_index,attempted_mask;
    enum pt_editor_mixed_causal_song_lineage_phase phase;
    enum pt_editor_mixed_causal_song_lineage_result result,first_error;
    enum pt_mixed_readers_result scheduled_result;
    /* Constructor-issued disjoint rolling wrapper, never a copied capability.
     * Existing constructor keeps this zero and its three-profile stays closed. */
    struct pt_editor_mixed_causal_song_rolling *rolling_owner;
};

/* Exactly three nonempty TRIGGER/CONTROL16/STOP boundaries, full audit and real EOF replay BEFORE
 * binding/factory/cache/publication. No whole-song PLAY, retrigger, fourth boundary or refill.
 * PUBLISHED means three task-side publications only, not activation/end/quiet.
 * Admission and early SOURCE install only: no external callback/allocation.
 * Failure after actual adoption remains owned and must be explicitly drained. */
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_lineage_begin(
    struct pt_editor_mixed_causal_song_lineage *,const struct pt_editor_mixed_causal_lineage_source_inputs *,
    struct pt_editor_mixed_causal_source_borrow *original);
/* One genuine phase operation or <=min(work,256) frames/Q items; establishment
 * and pin bytes accept work1..4096. No publish, fire, clock or implicit domain service. Later admission uses genuine callback-free composite queries.
 * Preflight uses the taken original sequence to actual EOF before any owner
 * opening/binding/factory/cache/publication. WAIT_CAPACITY preserves the three
 * immutable descriptive snapshots; it cannot rebase or silently service domains. */
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_lineage_step(struct pt_editor_mixed_causal_song_lineage *,unsigned);
/* By-value fixed-control diagnostic only; no source walk/callback/output alias. */
struct pt_editor_mixed_causal_song_lineage_status pt_editor_mixed_causal_song_lineage_get(const struct pt_editor_mixed_causal_song_lineage *);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_lineage_publish(struct pt_editor_mixed_causal_song_lineage *);
/* C indices0..2 name original history; R indices name the actual live ledger, not supplied
 * keys/tickets/proofs. Each explicit service performs <=one original callback.
 * C cleanup always uses NULL receipts; C1 PENDING service must genuinely observe its committed registration before CONTROL. R uses the separate
 * genuine retirement pair without an output receipt. Consumed retirement only
 * suppresses another backend R call: a retained original R-first registration
 * can still request actual close after C detach; actual NULL prunes custody. */
enum pt_mixed_readers_result pt_editor_mixed_causal_song_lineage_service_command(struct pt_editor_mixed_causal_song_lineage *,unsigned,unsigned);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_lineage_service_reader(struct pt_editor_mixed_causal_song_lineage *,unsigned,unsigned);
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_lineage_cancel(struct pt_editor_mixed_causal_song_lineage *);
/* One cleanup phase, no implicit domain service. Actual failure-plus-NULL owner
 * slots are terminal. All pins stay until real external/controller/source quiet;
 * then one unpin per phase under SOURCE, borrow close, separate final hook close.
 * Context/buffers remain alive through positive final close. */
int pt_editor_mixed_causal_song_lineage_close(struct pt_editor_mixed_causal_song_lineage *);
#endif
