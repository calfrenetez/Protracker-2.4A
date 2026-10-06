#ifndef PT_EDITOR_MIXED_READERS_PREPARE_H
#define PT_EDITOR_MIXED_READERS_PREPARE_H
#include "editor_mixed.h"
#include "sampler_mixed_readers.h"
#include "sampler_internal.h"
#include "../core/mixed_readers_activation.h"

#define PT_EDITOR_MIXED_READERS_ORDINARY 37U
#define PT_EDITOR_MIXED_READERS_CHIP 32U
#define PT_EDITOR_MIXED_READERS_GUARDS 8192U
struct pt_editor_mixed_readers_prepare;
/* Controller-issued identity, never a factory handle, queue key or ACTIVE
 * certificate. A consumed registration cannot authorize a reused slot. */
struct pt_editor_mixed_command_ref {unsigned slot;uint64_t serial;};
struct pt_editor_mixed_reader_ref {unsigned slot;uint64_t serial;};
enum pt_editor_mixed_readers_result {
    PT_EDITOR_MIXED_READERS_INVALID=0,PT_EDITOR_MIXED_READERS_PENDING,
    PT_EDITOR_MIXED_READERS_OPEN,PT_EDITOR_MIXED_READERS_CAPACITY,
    PT_EDITOR_MIXED_READERS_CANCELLED,PT_EDITOR_MIXED_READERS_STALE,
    PT_EDITOR_MIXED_READERS_FAULT,PT_EDITOR_MIXED_READERS_CLOSED
};
enum pt_editor_mixed_readers_phase {
    PT_EDITOR_MIXED_READERS_EMPTY=0,PT_EDITOR_MIXED_READERS_ACTIVATION,
    PT_EDITOR_MIXED_READERS_FACTORY,PT_EDITOR_MIXED_READERS_VALIDATION,
    PT_EDITOR_MIXED_READERS_REQUESTS,PT_EDITOR_MIXED_READERS_FAILED,
    PT_EDITOR_MIXED_READERS_FINISHED
};
struct pt_editor_mixed_readers_prepare_inputs {
    struct pt_editor_mixed *binding;
    struct pt_amigus_wavetable_cache *backend;
    struct pt_mixed_activation_config activation;
    void *chip_context;
    void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
    size_t factory_budget,chip_budget;
    /* Complete immutable ordinary callback/context aggregate, containing this
     * complete controller and original allocator/Chip/port/reservation/bus
     * contexts. Editor, binding, card cache/reservation and sources are separate.
     * Unknown opaque pointee sizes remain the genuine caller's obligation. */
    struct pt_sampler_storage_span contexts;
    void *activation_workspace;size_t activation_capacity;
    void *factory_workspace;size_t factory_capacity;
};
struct pt_editor_mixed_readers_request {
    enum pt_mixed_readers_kind kind;unsigned track,sample,channel;
    /* TRIGGER keeps this already-current genuine master pinned by its caller
     * through preparation/cancellation. CONTROL/STOP use only reader, whose
     * exact positive original ACTIVE key is freshly obtained by the controller.
     * Zero-init inactive members, including reader for a TRIGGER. */
    struct pt_sample_version *expected;
    struct pt_editor_mixed_reader_ref reader;
    union {
        struct {uint16_t period;uint8_t volume;} paula;
        struct {unsigned bits,little_endian;
            struct pt_amigus_voice_request trigger;
            uint32_t rate;uint16_t left,right;} amigus;
    } geometry;
};
struct pt_editor_mixed_command_record {
    struct pt_sampler_mixed_command_handle handle;
    uint64_t serial,ticket;
    unsigned count,transferred;
    unsigned reader[PT_SAMPLER_MIXED_ACTIONS];
};
struct pt_editor_mixed_reader_record {
    struct pt_sampler_mixed_reader_handle handle;
    uint64_t serial,ticket;
    unsigned action,track,sample,channel;
};
struct pt_editor_mixed_readers_callback {struct pt_editor_mixed_readers_prepare *owner;};
/* Opt-in request-driven task-side editor barrier, not a song producer or live
 * backend. Require genuine already-established immutable 8/16/24 masters and
 * an attached idle mixed binding. Zero-init/noncopyable controller, immutable
 * inputs and complete context/workspace/source storage survive until terminal
 * close. Install the private editor hook BEFORE the first callback/allocation.
 * Two genuine ordinary activation allocations + factory pool/2C/32R total
 * at most37 ordinary and32 separately sized Chip entries. Full capacities,
 * unused sources and unpublished/retired allocations guard every external
 * request/output before writable faults, entry or callbacks. Local child slots
 * do not weaken those outer guards; original enqueue success alone transfers.
 * Ordinary and Chip callbacks are serialized through complete ledgers; known
 * allocation aliases are neither initialized nor released as fresh ownership.
 * Fresh callbacks returning stale/faulting storage are disposed exactly once.
 * Changes between calls must change editor revision or sampler generation;
 * fixed identity/header/tag checks precede any former table traversal. Valid
 * channel-selection movement is allowed. NULL receipt service drains stale
 * captured ownership without reading former source arrays. No READY token.
 *
 * One advance performs one construction phase OR <=work validator items. One
 * batch advance performs one factory action/transition OR <=256 derived bytes.
 * Publication/services keep literal original frame/grid and use the original
 * separately injected whole-paired port. Exact copied-only activation_fire is
 * called independently on the borrowed owner: NEVER an editor-walking fire
 * wrapper. This controller installs no timer, musical STOP, PLAY, MMIO or audio.
 *
 * Cancel/close only cancels untransferred factory work and locally stops the
 * unpublished queue. No backend polling, force release, retry or shifted clock.
 * Consumed child slots (even close0+NULL) are terminal, never retried. Retained
 * untransferred R handles close before C handles. Exact command detach AND exact
 * reader retirement are independent; submitted/uncertain domains veto edit,
 * undo, route mutation and dispose until explicit services prove both. Pool and
 * handles close before activation; the hook/contexts remain through pending
 * source_close and later explicit read-only source_quiet. Only actual positive
 * complete consumption clears the hook. Terminal calls read only this control.
 * Host software proof is not native/IRQ/placement/WCET/stack/timing/device/voice
 * stop/audio/listening acceptance. No raw underlying ownership calls while bound.
 */
struct pt_editor_mixed_readers_prepare {
    const struct pt_editor_mixed_readers_prepare_inputs *inputs;
    struct pt_editor_mixed_readers_prepare_inputs saved;
    struct pt_editor *editor;struct pt_project *project;
    struct pt_project project_header;struct pt_sampler sampler_header;
    struct pt_allocator allocator;
    struct pt_editor_mixed_readers_callback callback;
    struct pt_mixed_readers_activation *activation;
    struct pt_mixed_readers_output *queue;
    struct pt_sampler_mixed_pool *pool;
    struct pt_sampler_storage_span guards[PT_EDITOR_MIXED_READERS_GUARDS];
    unsigned guard_count;
    struct pt_sampler_storage_span ordinary[PT_EDITOR_MIXED_READERS_ORDINARY];
    struct pt_sampler_storage_span chip[PT_EDITOR_MIXED_READERS_CHIP];
    struct pt_editor_mixed_command_record command[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_reader_record reader[PT_SAMPLER_MIXED_READERS];
    uint64_t serial;uint32_t revision,generation;
    unsigned phase,busy,closing,close_call,reentries,hook;
    enum pt_editor_mixed_readers_result result,first_error;
};
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_begin(
    struct pt_editor_mixed_readers_prepare *,const struct pt_editor_mixed_readers_prepare_inputs *);
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_get(struct pt_editor_mixed_readers_prepare *);
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_advance_validation(
    struct pt_editor_mixed_readers_prepare *,unsigned);
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_batch_begin(
    struct pt_editor_mixed_readers_prepare *,uint64_t,const struct pt_editor_mixed_readers_request *,unsigned,
    struct pt_editor_mixed_command_ref *);
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_batch_advance(
    struct pt_editor_mixed_readers_prepare *,struct pt_editor_mixed_command_ref);
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_enqueue(
    struct pt_editor_mixed_readers_prepare *,struct pt_editor_mixed_command_ref,uint64_t *);
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_reader_reference(
    struct pt_editor_mixed_readers_prepare *,struct pt_editor_mixed_command_ref,unsigned,struct pt_editor_mixed_reader_ref *);
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_publish(
    struct pt_editor_mixed_readers_prepare *,struct pt_editor_mixed_command_ref);
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_service_command(
    struct pt_editor_mixed_readers_prepare *,struct pt_editor_mixed_command_ref,unsigned,struct pt_mixed_readers_command_receipt *);
enum pt_mixed_readers_result pt_editor_mixed_readers_prepare_service_reader(
    struct pt_editor_mixed_readers_prepare *,struct pt_editor_mixed_reader_ref,unsigned,struct pt_mixed_readers_reader_receipt *);
enum pt_editor_mixed_readers_result pt_editor_mixed_readers_prepare_cancel(struct pt_editor_mixed_readers_prepare *);
int pt_editor_mixed_readers_prepare_close(struct pt_editor_mixed_readers_prepare *);
#endif
