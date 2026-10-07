#ifndef PT_EDITOR_MIXED_CAUSAL_SOURCE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_SOURCE_INTERNAL_H
#include "editor_mixed_causal_prepare.h"
#include "editor_mixed_readers_source_internal.h"
#include "../core/mixed_scheduled_readers_internal.h"

/* Private owner-thread seam, not a public producer/validation certificate.
 * The constructor installs the controller's existing static editor hook before
 * any external master establishment callback. Zero/noncopyable controller and
 * ORIGINAL separately mutable publisher, immutable input descriptor, complete
 * context aggregate and every full declared extent survive final positive
 * prepare_close/mixed_stop. No supplied callback, READY flag or quiet assertion.
 * contexts contains the whole controller plus ordinary callback contexts; it
 * excludes the original publisher and declared mutable result/output buffers.
 * preparation is its exact original future input, inside contexts or separate;
 * its values are admitted only by activate. Immutable spans guard outputs and
 * allocation returns. Mutable spans guard allocation returns only: original
 * request/ref/result publishers can remain writable by their genuine APIs.
 * All nonempty declared extents are pairwise apart and apart from contexts,
 * preparation, this input, publisher and all full project/sampler capacities.
 * Unused span entries are semantic zero. Fourteen is a total caller-span bound,
 * not a permission to truncate an outer parent or omit its unused capacity. */
struct pt_editor_mixed_causal_source_inputs {
    struct pt_editor_mixed *binding;
    struct pt_sampler_storage_span contexts;
    const struct pt_editor_mixed_causal_prepare_inputs *preparation;
    /* Exact full roles, each matching one complete immutable span. Future
     * preparation fields may be unset here; activation later must match both
     * workspace roles exactly and keep full backend/reservation objects inside
     * the fixed whole backend parent. No generic immutable reuse permission. */
    struct pt_sampler_storage_span causal_workspace,factory_workspace,backend_parent;
    unsigned immutable_count,mutable_count;
    struct pt_sampler_storage_span immutable[PT_EDITOR_MIXED_SOURCE_SPANS];
    struct pt_sampler_storage_span mutable[PT_EDITOR_MIXED_SOURCE_SPANS];
};
struct pt_editor_mixed_causal_source_borrow {
    struct pt_editor_mixed_causal_prepare *address;
    uint64_t serial;
};
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_source_begin(
    struct pt_editor_mixed_causal_prepare *,const struct pt_editor_mixed_causal_source_inputs *,
    struct pt_editor_mixed_causal_source_borrow *original);
/* Require actual producer establishment/cancellation and independent current
 * pins before activate; no argument claims them and this seam does not certify
 * them. Genuine factory validation follows. No callback/allocation here.
 * Retain original numeric guards and append actual current storage before the
 * one legitimate snapshot refresh. Failure retains early hook/borrow adoption. */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_source_activate(
    struct pt_editor_mixed_causal_prepare *,const struct pt_editor_mixed_causal_source_borrow *original,
    const struct pt_editor_mixed_causal_prepare_inputs *original_inputs);
/* Numeric read-only child-consumption query, never work/READY authority. */
int pt_editor_mixed_causal_source_children_closed(struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original);
/* Separate exclusion around actual external source child calls. Enter after
 * cancellation/fault is cleanup-only by protocol, not work permission. Retain
 * actual child outputs/NULL consumption before leave. Invalid/copied entry
 * never clears a live latch. Leave permits exact original cleanup after stale
 * fixed tags and never traverses former source arrays. Controller public calls
 * retain their own busy guard and must not be wrapped by source_enter. */
int pt_editor_mixed_causal_source_enter(const struct pt_editor_mixed_causal_source_borrow *original);
int pt_editor_mixed_causal_source_leave(const struct pt_editor_mixed_causal_source_borrow *original);
/* Close external sequence/Q/audit/establishment owners, independently consume
 * controller children/source quiet and unpin masters ONCE while borrow vetoes
 * mutation, then close this exact original publisher. Actual consumed zero is
 * terminal even when a child returned failure. Borrow release keeps hook and
 * storage alive until separate final positive prepare_close/mixed_stop. */
int pt_editor_mixed_causal_source_borrow_close(struct pt_editor_mixed_causal_source_borrow *original);
/* Private owner-thread diagnostic values only. No writable output parameter,
 * caller READY/ACTIVE/quiet assertion or child ownership/work authority. Queries
 * never fault, change latches, refresh/capture storage or invoke callbacks. The
 * genuine caller stores returned values in its own separately admitted outputs.
 * ZERO describes a readable zero publisher pair without touching controller;
 * it does not authenticate prior issuance, final closure or a copied publisher.
 * Positive scopes retain all original fixed controls through actual hook close. */
struct pt_editor_mixed_source_observation pt_editor_mixed_causal_source_observe(
    struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original);
/* Numeric captured-only disjointness, not allocation provenance/ownership.
 * full_bytes is the actual complete callback request/returned capacity. This
 * predicate cannot authenticate an unknown allocation's caller-supplied length.
 * No source descriptor, former/current table, master body or queue is followed.
 * Newly promoted capacities join numeric guards at activate only; genuine
 * establish/sampler guards protect earlier promotion. Only establish metadata
 * precedes promotion; the two audit allocations follow that one refresh.
 * External retired extents remain in the producer's separate request ledger. */
int pt_editor_mixed_causal_source_allocation_disjoint(
    struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original,
    const void *candidate,size_t full_bytes);
/* Keep original refs actually issued by this controller scope. Slot/serial
 * cannot authenticate arbitrary past issuance after a record is cleared.
 * ABSENT prunes only that genuinely retained ref after actual NULL consumption
 * or later serial reuse; it certifies neither independent proof nor source quiet.
 * Partial records refuse INVALID. No handle address or child key is followed. */
enum pt_editor_mixed_source_registration_observation
pt_editor_mixed_causal_source_command_registration(
    struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original,
    struct pt_editor_mixed_command_ref);
enum pt_editor_mixed_source_registration_observation
pt_editor_mixed_causal_source_reader_registration(
    struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original,
    struct pt_editor_mixed_reader_ref);
/* Separate bounded genuine TASK operation, unlike the four inert queries.
 * Exact original held borrow/ref and active work scope precede normal controller
 * exclusion and a genuine factory/queue holder-current call. Existing failure,
 * cancellation, staleness and callback reentry are terminal refusals. PENDING
 * only describes clean not-yet-active custody now. OK is not a key, lasting
 * ACTIVE/READY authority, quiet/proof or permission to skip fresh batch admission.
 * Do not wrap this controller operation in source_enter. No output is written. */
enum pt_mixed_readers_result pt_editor_mixed_causal_source_reader_readiness(
    struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original,
    struct pt_editor_mixed_reader_ref original_reader);
/* Genuine bounded task operation, including original cleanup after sticky
 * cancellation/stale failure. Exact borrow/ref and complete captured local
 * handle span precede callbacks. Normal errors remain errors while the by-value
 * bit preserves actual consumed retirement, including genuine local unpublished
 * queue retirement with no backend R call. It asserts no envelope validity.
 * R-first bit does not consume C or
 * the retained factory owner: only its later actual NULL clears registration.
 * No receipt/key/validity/ACTIVE/quiet/source-stop/native certificate. No former
 * source arrays are followed. Do not wrap this controller call in source_enter. */
struct pt_mixed_reader_retirement pt_editor_mixed_causal_source_retire_original_reader(
    struct pt_editor_mixed_causal_prepare *,
    const struct pt_editor_mixed_causal_source_borrow *original,
    struct pt_editor_mixed_reader_ref original_reader,unsigned cancel);
#endif
