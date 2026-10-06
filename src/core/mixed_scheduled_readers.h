#ifndef PT_MIXED_SCHEDULED_READERS_H
#define PT_MIXED_SCHEDULED_READERS_H
#include "document.h"
#include "elapsed_clock.h"
#include "amigus_voice_plan.h"
#define PT_MIXED_READERS_COMMANDS 2U
#define PT_MIXED_READERS_ACTIONS 16U
#define PT_MIXED_READERS_PERSISTENT 32U
#define PT_MIXED_READERS_SPANS 128U
#define PT_MIXED_READERS_VERSION 1U
/* Exact flags: timestamped BOTH routes, atomic whole paired publication,
 * independent command/reader retirement and actual original-key activation. */
#define PT_MIXED_READERS_REQUIRED 15U
enum pt_mixed_readers_result {PT_MIXED_READERS_OK,PT_MIXED_READERS_PENDING,
    PT_MIXED_READERS_UNSUPPORTED,PT_MIXED_READERS_INVALID,PT_MIXED_READERS_CAPACITY,
    PT_MIXED_READERS_STALE,PT_MIXED_READERS_CLOCK,PT_MIXED_READERS_LATE,PT_MIXED_READERS_BACKEND};
enum pt_mixed_readers_route {PT_MIXED_READERS_PAULA=1,PT_MIXED_READERS_AMIGUS=2};
enum pt_mixed_readers_kind {PT_MIXED_READERS_TRIGGER,PT_MIXED_READERS_CONTROL,PT_MIXED_READERS_STOP};
struct pt_mixed_readers_output;
struct pt_mixed_readers_span {const void *data;size_t bytes;};
struct pt_mixed_readers_grid {uint64_t epoch,generation;uint32_t frequency,rate;};
struct pt_mixed_readers_key {
    const struct pt_mixed_readers_output *queue;
    uint64_t session,generation,trigger,owner,serial;
    unsigned action,route,slot;
};
/* Numeric card range and opaque identities, NEVER CPU pointer spans. Logical
 * bytes exclude allocation padding. All full_capacity bytes remain protected
 * by the genuine independent cache lease until exact reader retirement. */
struct pt_mixed_readers_card {
    const void *reservation,*cache;uint64_t version,serial;
    unsigned cache_slot,bits,little_endian,source_channel;
    uint32_t address,logical_bytes,full_capacity;
};
struct pt_mixed_readers_action {
    unsigned route,slot;enum pt_mixed_readers_kind kind;
    union {
        struct {const uint8_t *data;uint16_t words,period;uint8_t volume;} paula;
        struct pt_amigus_voice_plan amigus;
    } geometry;
};
struct pt_mixed_readers_batch {
    uint64_t generation,frame;unsigned count;
    struct pt_mixed_readers_action action[PT_MIXED_READERS_ACTIONS];
};
struct pt_mixed_readers_control {
    void *context;size_t context_bytes;uint64_t token;
    int (*current)(void *,uint64_t,uint64_t);
    void (*terminal)(void *,uint64_t,int);
    void (*release)(void *,uint64_t);
};
/* Genuine exclusive small controls, never casts/copies of ABI2 or prepared
 * owners. CPU spans include complete master/cache capacities and supporting
 * controls. Every shared immutable sample/cache has its OWN genuine pin.
 * Source, declaration vectors and callback contexts remain immutable/readable
 * through their borrow; values are copied before callbacks. Core cannot prove
 * provenance or residency from declarations. Mutable holders are disjoint. */
struct pt_mixed_readers_owner {
    struct pt_mixed_readers_control control;
    const struct pt_mixed_readers_span *spans;unsigned count;
    struct pt_mixed_readers_card card;
};
struct pt_mixed_readers_inputs {
    struct pt_mixed_readers_batch batch;
    struct pt_mixed_readers_control command;
    struct pt_mixed_readers_owner reader[PT_MIXED_READERS_ACTIONS];
    struct pt_mixed_readers_key target[PT_MIXED_READERS_ACTIONS];
};
struct pt_mixed_readers_binding {void *context;size_t context_bytes;};
struct pt_mixed_readers_domain {
    struct pt_mixed_readers_key key;struct pt_mixed_readers_binding binding;
    const struct pt_mixed_readers_span *spans;unsigned count;
    struct pt_mixed_readers_card card;
};
struct pt_mixed_readers_event {
    const struct pt_mixed_readers_output *queue;
    uint64_t session,ticket,first,last;struct pt_mixed_readers_binding binding;
    uint64_t command_owner;struct pt_mixed_readers_batch batch;
    const struct pt_mixed_readers_domain *reader[PT_MIXED_READERS_ACTIONS];
};
enum pt_mixed_readers_command {PT_MIXED_COMMAND_WAITING,PT_MIXED_COMMAND_ISSUED,
    PT_MIXED_COMMAND_CANCELLED_BEFORE,PT_MIXED_COMMAND_CANCELLED_AFTER,
    PT_MIXED_COMMAND_FAILED,PT_MIXED_COMMAND_UNKNOWN};
enum pt_mixed_readers_state {PT_MIXED_READER_NONE,PT_MIXED_READER_RESERVED,
    PT_MIXED_READER_ACTIVE,PT_MIXED_READER_STOP_PENDING,PT_MIXED_READER_DRAINING,
    PT_MIXED_READER_RETIRED,PT_MIXED_READER_UNKNOWN};
enum pt_mixed_readers_adoption {PT_MIXED_UNADOPTED,PT_MIXED_ADOPTED};
enum pt_mixed_readers_tag {PT_MIXED_COMMAND_DOMAIN=1,PT_MIXED_READER_DOMAIN=2};
struct pt_mixed_readers_action_receipt {
    struct pt_mixed_readers_key key;enum pt_mixed_readers_command command;
    enum pt_mixed_readers_state reader;enum pt_mixed_readers_adoption adoption;
    uint64_t observed,issued;
};
struct pt_mixed_readers_command_receipt {
    enum pt_mixed_readers_tag domain;const struct pt_mixed_readers_output *queue;
    uint64_t session,generation,ticket,owner;unsigned count;
    const struct pt_mixed_readers_event *event;struct pt_mixed_readers_binding binding;
    struct pt_mixed_readers_action_receipt action[PT_MIXED_READERS_ACTIONS];
};
struct pt_mixed_readers_reader_receipt {
    enum pt_mixed_readers_tag domain;struct pt_mixed_readers_key key;
    const struct pt_mixed_readers_domain *reference;struct pt_mixed_readers_binding binding;
    enum pt_mixed_readers_state state;enum pt_mixed_readers_adoption adoption;
    uint64_t observed,issued; /* actual ORIGINAL trigger timing */
};
enum pt_mixed_readers_reply {PT_MIXED_UNCERTAIN=-1,PT_MIXED_PENDING=0,
    PT_MIXED_OBSERVATION=1,PT_MIXED_COMMAND_DETACHED=2,PT_MIXED_READER_RETIRE_PROOF=3};
/* One separately declared backend for the complete paired batch. No two-route
 * immediate callback wrapper satisfies this contract. Actual publication and
 * activation compare every original key under backend-owned synchronization.
 * Submit1 accepted,0 NO references/effects,-1 uncertain. Actual activation uses
 * original [first,last), no allocation/conversion/owner/editor traversal. Backend
 * cannot dereference opaque holder bindings or invoke holder callbacks.
 * DETACHED proves only command quiet; RETIRED proves exact full reader quiet,
 * including card/CPU sample/cache capacities and every future activation.
 * All operations serialized, bounded task work, no reentry or input edits.
 * Flags declare capabilities, never qualification or native/IRQ/stack/timing.
 */
struct pt_mixed_readers_backend {
    void *context;size_t context_bytes;unsigned version,flags;
    int (*read_clock)(void *,uint64_t *,uint32_t *);
    int (*submit)(void *,const struct pt_mixed_readers_event *);
    enum pt_mixed_readers_reply (*command)(void *,uint64_t,unsigned cancel,struct pt_mixed_readers_command_receipt *);
    enum pt_mixed_readers_reply (*reader)(void *,const struct pt_mixed_readers_domain *,unsigned cancel,struct pt_mixed_readers_reader_receipt *);
};
struct pt_mixed_readers_config {
    struct pt_allocator allocator;struct pt_mixed_readers_span allocator_context;
    struct pt_mixed_readers_grid grid;uint64_t session;size_t control_budget;
    struct pt_mixed_readers_backend backend;
};
/* Fresh zeroed ordinary scratch of genuine queried size/alignment. Complete
 * capacity, config/output and complete allocator/backend context extents are
 * guarded BEFORE writes/callbacks. Scratch expires after open; copied callbacks
 * and contexts outlive close. Original config is immutable through open only.
 * One fixed queue allocation; budget includes all private metadata/scratch.
 * Recognized allocation aliases are never released as fresh ownership.
 * No public readiness certificate. Session is nonzero/fresh across lifetimes.
 */
size_t pt_mixed_readers_workspace_size(void);
size_t pt_mixed_readers_workspace_alignment(void);
size_t pt_mixed_readers_control_size(void);
enum pt_mixed_readers_result pt_mixed_readers_open(const struct pt_mixed_readers_config *,void *,size_t,struct pt_mixed_readers_output **);
/* Read-only task-side guards for wrappers around a genuine live queue. They
 * perform no writes, callbacks, clock reads, transfer or reentry fault. Current
 * held controls and complete retained source capacities are included, even
 * before publication. admission_valid repeats the existing bounded declaration
 * checks only; its positive result is not provenance, readiness, budget/clock
 * acceptance or lasting authorization. Final enqueue repeats all checks and
 * alone transfers ownership. Never use either query in the activation path. */
int pt_mixed_readers_output_disjoint(const struct pt_mixed_readers_output *,const void *,size_t);
int pt_mixed_readers_admission_valid(struct pt_mixed_readers_output *,const struct pt_mixed_readers_inputs *,uint64_t *);
/* Copies full declarations/spans into private admission scratch before callbacks.
 * Its scratch contents are unspecified after an attempt; refusal preserves all
 * external owners, inputs, outputs and live registrations. Unique(route,slot),
 * strict original frame ordering. TRIGGER transfers new reader independently;
 * CONTROL/STOP require a positive actual ADOPTED+ACTIVE original target key.
 * Paula signed8 semantics; AmiGUS software mask0x800F/enable/rate and numeric
 * bounds/format must match. CONTROL has only rate/gains; STOP has zero geometry.
 */
enum pt_mixed_readers_result pt_mixed_readers_enqueue(struct pt_mixed_readers_output *,const struct pt_mixed_readers_inputs *,uint64_t *);
/* One original clock read and <=one whole-batch submit. Confirmed refusal permits
 * an explicit attempt only while still before the original window. Uncertainty,
 * late/stale/reentry latch failure; no automatic retry/rebase/catch-up/STOP.
 */
enum pt_mixed_readers_result pt_mixed_readers_publish(struct pt_mixed_readers_output *,uint64_t);
/* <=one corresponding callback. Exact independent proof can drain after failure
 * despite invalid classification (terminal valid0); output remains unchanged.
 * NULL out explicitly drains without publishing. Actual effect/proof replies
 * are retained, never converted into invented non-issued/retirement evidence.
 */
enum pt_mixed_readers_result pt_mixed_readers_service_command(struct pt_mixed_readers_output *,uint64_t,unsigned,struct pt_mixed_readers_command_receipt *);
enum pt_mixed_readers_result pt_mixed_readers_service_reader(struct pt_mixed_readers_output *,uint64_t,unsigned,unsigned,struct pt_mixed_readers_reader_receipt *);
enum pt_mixed_readers_result pt_mixed_readers_reader_key(struct pt_mixed_readers_output *,uint64_t,unsigned,struct pt_mixed_readers_key *);
enum pt_mixed_readers_result pt_mixed_readers_stop(struct pt_mixed_readers_output *);
unsigned pt_mixed_readers_commands_held(const struct pt_mixed_readers_output *);
unsigned pt_mixed_readers_readers_held(const struct pt_mixed_readers_output *);
/* No callbacks/polls except final ordinary release. Submitted domains retain
 * every pin until distinct exact proofs AND zero command refs. Local stop drops
 * unpublished domains only, no musical STOP. close0 retains owner, except final
 * release reentry may return0 after consuming slot toNULL. Never retry freed
 * storage. Terminal NULL-slot close avoids expired controls. No production
 * editor/song/backend/device wiring, physical capacity/order/completion, native
 * IRQ/residency/stack/timing/audio/listening qualification follows.
 */
int pt_mixed_readers_close(struct pt_mixed_readers_output **);
#endif
