#ifndef PT_SCHEDULED_READERS_H
#define PT_SCHEDULED_READERS_H
#include "scheduled_output.h"
/* New opt-in reference-domain contract. flags7 and lineage-v1 are unchanged. */
#define PT_READERS_VERSION 2U
#define PT_READERS_CONDITIONAL 1U
#define PT_READERS_SPLIT_DOMAINS 2U
#define PT_READERS_REQUIRED 3U
#define PT_READERS_COMMANDS 8
#define PT_READERS_PERSISTENT 8
#define PT_READERS_ACTIONS 4
struct pt_readers_output;
struct pt_readers_key {
    const struct pt_readers_output *queue;
    uint64_t session,generation,trigger,owner,serial;
    unsigned action,slot;
};
/* Version2 publishes opaque identity bindings so a separately compiled backend
 * can name the exact full holder extent in a receipt. Copied from the already
 * staged genuine control metadata before publication; never a borrowed control
 * declaration or a validation/activation certificate. Backends may copy these
 * identity values, never dereference, mutate, copy or release owner storage or
 * invoke its callbacks. No current/release/terminal callback is exposed. */
struct pt_readers_binding {void *context;size_t context_bytes;};
/* Immutable registered reader descriptor. Never aliases a command slot. All
 * sample/loop geometry must lie inside its full declared spans. Core storage
 * remains owned until independently exact retirement AND zero command refs.
 * Backend borrowing ends at retirement: before reporting READER_RETIRED it
 * must drop every persistent reference, even if core storage is still retained
 * for commands. Remaining command metadata cannot revive a retired reader. */
struct pt_readers_domain {
    struct pt_readers_key key;const struct pt_scheduled_span *spans;unsigned count;
    struct pt_readers_binding binding;
};
struct pt_readers_event {
    const struct pt_readers_output *queue;uint64_t session,command_owner;
    struct pt_scheduled_event scheduled;
    const struct pt_readers_domain *reader[PT_READERS_ACTIONS];
    /* Immutable command-only binding, valid through exact COMMAND_DETACHED.
     * It never grants access to the independently retained reader holders. */
    struct pt_readers_binding binding;
};
enum pt_readers_command {PT_READERS_WAITING,PT_READERS_ISSUED,
    PT_READERS_CANCELLED_BEFORE,PT_READERS_CANCELLED_AFTER,PT_READERS_FAILED,PT_READERS_UNKNOWN};
enum pt_readers_state {PT_READERS_NONE,PT_READERS_RESERVED,PT_READERS_ACTIVE,
    PT_READERS_STOP_PENDING,PT_READERS_DRAINING,PT_READERS_RETIRED,PT_READERS_STATE_UNKNOWN};
enum pt_readers_adoption {PT_READERS_UNADOPTED,PT_READERS_ADOPTED};
enum pt_readers_origin {PT_READERS_LOCAL_UNSUBMITTED=1,PT_READERS_BACKEND_ACTUAL=2};
enum pt_readers_tag {PT_READERS_COMMAND_DOMAIN=1,PT_READERS_READER_DOMAIN=2};
struct pt_readers_action_receipt {
    enum pt_readers_command command;enum pt_readers_state reader;
    enum pt_readers_adoption adoption;struct pt_readers_key key;
    uint64_t observed,issued;
};
struct pt_readers_command_receipt {
    enum pt_readers_tag domain;enum pt_readers_origin origin;const struct pt_readers_output *queue;
    uint64_t session,generation,ticket,owner;unsigned count;
    const struct pt_readers_event *event;void *context;size_t context_bytes;
    struct pt_readers_action_receipt action[PT_READERS_ACTIONS];
};
struct pt_readers_reader_receipt {
    enum pt_readers_tag domain;struct pt_readers_key key;
    const struct pt_readers_domain *reference;void *context;size_t context_bytes;
    enum pt_readers_state state;enum pt_readers_adoption adoption;
    /* Actual original TRIGGER timing, not the most recent CONTROL's ticks. */
    uint64_t observed,issued;
};
enum pt_readers_reply {PT_READERS_UNCERTAIN=-1,PT_READERS_PENDING=0,
    PT_READERS_OBSERVATION=1,PT_READERS_COMMAND_DETACHED=2,PT_READERS_READER_RETIRED=3};
/* Every operation is bounded serialized task work, never activation/IRQ work.
 * All callbacks forbid reentry and editing borrowed inputs. read_clock and
 * submit retain the original exact grid/whole-batch/actual-clock contracts.
 * Version/flags are exact: capability declarations are not qualification.
 * Backend compares EVERY actual original key/current reference at publication
 * AND original activation under its own synchronization, before ANY effect.
 * No allocation/conversion/editor/owner traversal at activation, no fallback.
 *
 * COMMAND_DETACHED binds the exact event + full small command holder. It proves
 * no future activation/callback/reader references any command-owned byte. It
 * says NOTHING about DMA/sample/cache references in persistent reader domains.
 * ADOPTED is positive actual reference ownership by the named persistent domain,
 * never submit acceptance/issue/cacheHIT or a prediction. An unadopted or unknown
 * reader stays owned even after independently valid command detachment.
 * READER_RETIRED independently proves no future activation/reader/callback can
 * reference the exact persistent domain/holder/full sample/cache capacities.
 * These are distinct proofs, never legacy ALL_RETIRED/noDMA reinterpretations.
 * Reader storage also remains held until all referencing commands detach. */
struct pt_readers_backend {
    void *context;size_t context_bytes;struct pt_scheduled_caps caps;
    unsigned version,reference_flags,maximum_readers;
    int (*read_clock)(void *,uint64_t *,uint32_t *);
    int (*submit)(void *,const struct pt_readers_event *);
    enum pt_readers_reply (*poll_command)(void *,uint64_t,struct pt_readers_command_receipt *);
    enum pt_readers_reply (*cancel_command)(void *,uint64_t,struct pt_readers_command_receipt *);
    enum pt_readers_reply (*poll_reader)(void *,const struct pt_readers_domain *,struct pt_readers_reader_receipt *);
    enum pt_readers_reply (*cancel_reader)(void *,const struct pt_readers_domain *,struct pt_readers_reader_receipt *);
};
/* Exclusive genuine holders, not casts/copies of legacy prepared owners.
 * Commands own ONLY their full mutable small control extent. Reader control
 * extents are distinct; sample/cache spans belong solely to independently held
 * readers. Shared immutable sample/cache spans between readers are legal when
 * each holder has a genuine independent pin. Contexts may not be fabricated.
 * Inputs are copied on transfer; callbacks/contexts live until their release.
 * terminal precedes exactly-once release; valid0 reports bad classification
 * despite an independently exact domain-release proof. No callback reentry. */
struct pt_readers_control {
    void *context;size_t context_bytes;uint64_t token;
    int (*current)(void *,uint64_t,uint64_t);
    void (*release)(void *,uint64_t);
    void (*terminal)(void *,uint64_t,int);
};
struct pt_readers_owner {
    struct pt_readers_control control;
    const struct pt_scheduled_span *spans;unsigned count;
};
/* session is caller-issued, nonzero and never reused across queue lifetimes.
 * In particular an allocator reusing the queue address must not reuse session.
 * Core cannot detect global session reuse without unbounded external history. */
enum pt_scheduled_result pt_readers_open(const struct pt_allocator *,
    const struct pt_scheduled_grid *,uint64_t,const struct pt_readers_backend *,
    unsigned commands,unsigned readers,struct pt_readers_output **);
/* One action per slot,<=4; strict frame ordering. reader[] has batch.count
 * elements: genuine distinct holders for TRIGGER, zero entries otherwise.
 * target[] similarly has genuine registered ACTIVE originals for CONTROL/STOP
 * and zero elements for TRIGGER. Either array may be NULL if entirely unused.
 * Declaration metadata is bounded-copied before any owner callback. Callbacks
 * still forbid editing caller inputs or releasing/changing any borrowed owner.
 * Atomic refusal preserves every input/owner/output; OK transfers command and
 * new reader holders independently. STOP closes admission conservatively even
 * when later cancelled; positive replacement issue permanently closes old keys. */
enum pt_scheduled_result pt_readers_enqueue(struct pt_readers_output *,
    const struct pt_scheduled_batch *,const struct pt_readers_key *,
    const struct pt_readers_control *,const struct pt_readers_owner *,uint64_t *);
enum pt_scheduled_result pt_readers_publish(struct pt_readers_output *,uint64_t);
enum pt_scheduled_result pt_readers_poll_command(struct pt_readers_output *,uint64_t,
    struct pt_readers_command_receipt *);
enum pt_scheduled_result pt_readers_cancel_command(struct pt_readers_output *,uint64_t,
    struct pt_readers_command_receipt *);
/* Original trigger/action resolves the persistent entry AFTER command reuse.
 * Only reader_key exposes positively actual ADOPTED+ACTIVE permission. Poll/
 * cancel may collect reserved/unadopted retirement; neither fabricates ACTIVE.
 * Pending/uncertain/malformed replies preserve out and retain possible owners.
 * Exact detached/retired envelopes can release only their corresponding domain
 * despite invalid classifications (latching failure, valid0, unchanged out).
 * Each poll/cancel performs at most one backend callback. */
enum pt_scheduled_result pt_readers_poll_reader(struct pt_readers_output *,uint64_t,unsigned,
    struct pt_readers_reader_receipt *);
enum pt_scheduled_result pt_readers_cancel_reader(struct pt_readers_output *,uint64_t,unsigned,
    struct pt_readers_reader_receipt *);
enum pt_scheduled_result pt_readers_reader_key(struct pt_readers_output *,uint64_t,unsigned,
    struct pt_readers_key *);
/* Permanently closes publication; drops local commands/readers only. Explicit
 * bounded cancel/poll collects submitted commands/readers. No auto reset/retry,
 * inferred no-reader, destructor cancellation or force-unpin. */
enum pt_scheduled_result pt_readers_stop(struct pt_readers_output *);
unsigned pt_readers_commands_held(const struct pt_readers_output *);
unsigned pt_readers_readers_held(const struct pt_readers_output *);
int pt_readers_close(struct pt_readers_output *);
#endif
