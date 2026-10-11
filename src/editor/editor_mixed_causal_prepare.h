#ifndef PT_EDITOR_MIXED_CAUSAL_PREPARE_H
#define PT_EDITOR_MIXED_CAUSAL_PREPARE_H
#include "editor_mixed.h"
#include "sampler_mixed_readers.h"
#include "sampler_internal.h"
#include "../core/mixed_readers_causal.h"

#include "editor_mixed_readers_prepare.h"
#include "sampler_mixed_causal_internal.h"
struct pt_editor_mixed_causal_prepare;
struct pt_editor_mixed_causal_stop_prepare;
struct pt_editor_mixed_causal_source_inputs;
struct pt_editor_mixed_causal_source_borrow;
struct pt_editor_mixed_causal_prepare_inputs {
    struct pt_editor_mixed *binding;
    struct pt_amigus_wavetable_cache *backend;
    struct pt_mixed_causal_config causal;
    /* Task-only one-shot binding of the exact genuinely opened/borrowed original
     * registration. Copy it; do not retain its pointer or acquire a source first.
     * 1 accepted,0 no references,-1 unknown; raw outcome remains diagnostic.
     * Snapshot/reentry fault retains owner/contexts even after actual1 or0.
     * Source close/quiet still provide their own independent lifetime proofs. */
    int (*bind_original)(void *,const struct pt_mixed_causal_registration *);
    void *chip_context;
    void *(*chip_allocate)(void *,size_t);
    void (*chip_release)(void *,void *,size_t);
    size_t factory_budget,chip_budget;
    /* Complete immutable ordinary callback/context aggregate, containing this
     * complete controller and original allocator/Chip/port/reservation/bus
     * contexts. Editor, binding, card cache/reservation and sources are separate.
     * Unknown opaque pointee sizes remain the genuine caller's obligation. */
    struct pt_sampler_storage_span contexts;
    void *causal_workspace;size_t causal_capacity;
    void *factory_workspace;size_t factory_capacity;
};
struct pt_editor_mixed_causal_callback {struct pt_editor_mixed_causal_prepare *owner;};
/* Private opt-in exactly-one pure-TRIGGER pair task-side editor barrier, not whole-song or live
 * backend. Require genuine already-established immutable 8/16/24 masters and
 * an attached idle mixed binding. Zero-init/noncopyable controller, immutable
 * inputs and complete context/workspace/source storage survive until terminal
 * close. Install the private editor hook BEFORE the first callback/allocation.
 * Two genuine ordinary causal allocations + factory pool/2C/32R total
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
 * separately injected whole-paired port. Exact copied-only causal_fire is
 * called independently on the borrowed owner: NEVER an editor-walking fire
 * wrapper. This controller installs no timer, musical STOP, PLAY, MMIO or audio.
 *
 * Cancel/close only cancels untransferred factory work and locally stops the
 * unpublished queue. No backend polling, force release, retry or shifted clock.
 * Consumed child slots (even close0+NULL) are terminal, never retried. Retained
 * untransferred R handles close before C handles. Exact command detach AND exact
 * reader retirement are independent; submitted/uncertain domains veto edit,
 * undo, route mutation and dispose until explicit services prove both. Pool and
 * handles close before causal owner; the hook/contexts remain through pending
 * source_close and later explicit read-only source_quiet. Only actual positive
 * complete consumption clears the hook. Terminal calls read only this control.
 * Binding is one-shot after actual successful open/borrow and before factory;
 * third batches/CONTROL/STOP/rolling refill refuse without conversion/truncation.
 * Host software proof is not native/IRQ/placement/WCET/stack/timing/device/voice
 * stop/audio/listening acceptance. No raw underlying ownership calls while bound.
 */
struct pt_editor_mixed_causal_prepare {
    const struct pt_editor_mixed_causal_prepare_inputs *inputs;
    struct pt_editor_mixed_causal_prepare_inputs saved;
    struct pt_editor *editor;struct pt_project *project;
    struct pt_project project_header;struct pt_sampler sampler_header;
    struct pt_allocator allocator;
    struct pt_editor_mixed_causal_callback callback;
    struct pt_mixed_causal_owner *causal;
    struct pt_mixed_readers_output *queue;
    struct pt_sampler_mixed_pool *pool;
    struct pt_sampler_mixed_causal_binding factory_binding;
    unsigned original_binding_called,original_binding_confirmed,prepared_batches;
    int original_binding_outcome;
    struct pt_sampler_storage_span guards[PT_EDITOR_MIXED_READERS_GUARDS];
    unsigned guard_count;
    struct pt_sampler_storage_span ordinary[PT_EDITOR_MIXED_READERS_ORDINARY];
    struct pt_sampler_storage_span chip[PT_EDITOR_MIXED_READERS_CHIP];
    struct pt_editor_mixed_command_record command[PT_SAMPLER_MIXED_COMMANDS];
    struct pt_editor_mixed_reader_record reader[PT_SAMPLER_MIXED_READERS];
    uint64_t serial;uint32_t revision,generation;
    unsigned phase,busy,closing,close_call,reentries,hook;
    enum pt_editor_mixed_readers_result result,first_error;
    /* Private early-source ownership. Never a READY/validation/quiet token.
     * The internal constructor alone issues the exact original publisher and
     * serial; complete mutable spans guard allocations, not legitimate outputs.
     * Standalone begin leaves this appended scope zero and keeps its old path. */
    const struct pt_editor_mixed_causal_source_inputs *source_inputs;
    const struct pt_editor_mixed_causal_prepare_inputs *source_preparation;
    struct pt_editor_mixed_causal_source_borrow *source_publisher;
    struct pt_editor_mixed *source_binding;
    struct pt_sampler_storage_span source_contexts;
    struct pt_sampler_storage_span source_causal_workspace,source_factory_workspace,source_backend_parent;
    struct pt_sampler_storage_span source_immutable[PT_EDITOR_MIXED_SOURCE_SPANS];
    struct pt_sampler_storage_span source_mutable[PT_EDITOR_MIXED_SOURCE_SPANS];
    uint64_t source_serial;
    unsigned source_mode,source_held,source_released,source_activated,source_busy;
    unsigned source_cancel_requested,source_drained,source_publisher_guard;
    unsigned source_immutable_count,source_mutable_count;
    /* Internal constructor-issued complete-wrapper identity; default/SOURCE
     * paths leave this zero. Never a public STOP capability or READY flag. */
    struct pt_editor_mixed_causal_stop_prepare *stop_owner;
};
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_begin(
    struct pt_editor_mixed_causal_prepare *,const struct pt_editor_mixed_causal_prepare_inputs *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_get(struct pt_editor_mixed_causal_prepare *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_advance_validation(
    struct pt_editor_mixed_causal_prepare *,unsigned);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_batch_begin(
    struct pt_editor_mixed_causal_prepare *,uint64_t,const struct pt_editor_mixed_readers_request *,unsigned,
    struct pt_editor_mixed_command_ref *);
/* Count1..16; guard the WHOLE original fixed batch before any field read or
 * writable entry/fault. Unused action/ref fields are semantically zero and
 * unused levels are LEGACY/0/0. Active LEGACY requires levels0/0; QUANTIZED is
 * AmiGUS TRIGGER only with nested legacy volume/pan0. Paula/CONTROL/STOP use
 * LEGACY/0/0 in the shared request type, but this facade refuses CONTROL/STOP
 * before resource allocation. No supplied READY/key.
 * Both this entry and legacy batch_begin guard their original complete declared
 * input and command-ref output against allocator returns throughout callbacks.
 * Two captured guard slots must remain free; exactly this invocation's pair
 * retires by verified compaction preserving all later guards before publication.
 * All existing genuine handle/transfer/proof/cancel/close/hook contracts apply.
 * Queried factory workspace/pool/command sizes, not old hardcoded bytes, apply.
 */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_batch_begin_quantized(
    struct pt_editor_mixed_causal_prepare *,uint64_t,
    const struct pt_editor_mixed_readers_quantized_batch *,struct pt_editor_mixed_command_ref *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_batch_advance(
    struct pt_editor_mixed_causal_prepare *,struct pt_editor_mixed_command_ref);
enum pt_mixed_readers_result pt_editor_mixed_causal_prepare_enqueue(
    struct pt_editor_mixed_causal_prepare *,struct pt_editor_mixed_command_ref,uint64_t *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_reader_reference(
    struct pt_editor_mixed_causal_prepare *,struct pt_editor_mixed_command_ref,unsigned,struct pt_editor_mixed_reader_ref *);
enum pt_mixed_readers_result pt_editor_mixed_causal_prepare_publish(
    struct pt_editor_mixed_causal_prepare *,struct pt_editor_mixed_command_ref);
enum pt_mixed_readers_result pt_editor_mixed_causal_prepare_service_command(
    struct pt_editor_mixed_causal_prepare *,struct pt_editor_mixed_command_ref,unsigned,struct pt_mixed_readers_command_receipt *);
enum pt_mixed_readers_result pt_editor_mixed_causal_prepare_service_reader(
    struct pt_editor_mixed_causal_prepare *,struct pt_editor_mixed_reader_ref,unsigned,struct pt_mixed_readers_reader_receipt *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_prepare_cancel(struct pt_editor_mixed_causal_prepare *);
int pt_editor_mixed_causal_prepare_close(struct pt_editor_mixed_causal_prepare *);
#endif
