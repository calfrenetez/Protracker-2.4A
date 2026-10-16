#ifndef PT_PRIVATE_PREPARED_PAULA_LIFETIME_H
#define PT_PRIVATE_PREPARED_PAULA_LIFETIME_H
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME) && defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY)
#error "HOST lifetime and native TASK probe modes are mutually exclusive"
#endif
#if defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY)
#if PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY != 1
#error "The native TASK probe requires the explicit value 1"
#endif
#if !defined(__m68k__) || !defined(__amigaos__)
#error "The native TASK probe requires a genuine m68k AmigaOS compiler target"
#endif
#elif defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The unpublished prepared-source lifetime adapter is HOST-only"
#elif !defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
#error "The private HOST lifetime adapter requires its explicit opt-in"
#endif
#include "prepared_paula_sources.h"
#include "../core/document.h"
#include "../core/scheduled_readers.h"
#if defined(PT_PREPARED_PAULA_NATIVE_TASK_PROBE_ONLY)
#include <limits.h>
typedef char pt_prepared_paula_native_task_probe_abi[
    CHAR_BIT==8&&sizeof(int)==4&&sizeof(long)==4&&sizeof(void *)==4&&
    sizeof(uintptr_t)==4&&sizeof(size_t)==4&&
    sizeof(uint64_t)==8&&sizeof(uint32_t)==4&&sizeof(uint16_t)==2&&sizeof(uint8_t)==1?1:-1];
#endif

#define PT_PREPARED_PAULA_LIFETIME_CONTEXTS 16U
struct pt_prepared_paula_lifetime;
struct pt_prepared_paula_context_extent {const void *base;size_t bytes;};
struct pt_prepared_paula_lifetime_config {
    struct pt_allocator allocator;
    struct pt_scheduled_grid grid;
    uint64_t session,frame;
    size_t control_budget;
    unsigned slot,context_count;
    uint16_t period;uint8_t volume;
    struct pt_prepared_paula_context_extent context[PT_PREPARED_PAULA_LIFETIME_CONTEXTS];
};
/* Original normal-TASK serialized work only. Genuine complete accessible
 * source/backend/context objects remain alive through close or retained fault.
 * Every non-NULL allocator (source master/Chip/control) and backend context lies
 * in a genuinely complete declared extent. Length provenance/completeness is a
 * caller contract, never inferred from an address or checked flags. Borrowed
 * extents may overlap where their actual allocator contract permits an arena;
 * new controls/workspace/outputs remain disjoint. Backend controls and queue/
 * command/reader controls are disjoint from source data as the core requires.
 * No copies, reset, pointer exports, backend probes or native gate. Supplied
 * backend is the existing genuine HOST contract in HOST mode. The separate
 * native TASK probe admits a structurally complete refusal-only constructor
 * descriptor with zero backend calls; its flags infer no device or capability.
 * The original unpublished methods never call that backend. Only HOST mode
 * compiles the publish/cancel methods below, invoking the actual core/backend
 * once per admitted operation at the original frame/grid with no timing edit.
 * Native probe mode has no publication/collection API, PUBLISH source phase or
 * cancel callback authority. No ACTIVE/adoption/output/quiet authority follows.
 * Native probe callers remain original ordinary TASK, never IRQ/exception,
 * supervisor/signal/delivery or inside Disable/Enable; no SR/IPL claim is made.
 * Callers must not mutate opaque storage or borrowed inputs. Ordinary-method
 * callback reentry latches uncertainty; constructor refusal is specified below.
 * No arbitrary mutation detection or forced cleanup is promised.
 */
size_t pt_prepared_paula_lifetime_size(void);
size_t pt_prepared_paula_lifetime_alignment(void);
/* Aligned capacity>=size; zero only the owned size() prefix. Tail is untouched
 * but guarded. Construction opens the genuine EMPTY queue BEFORE a source
 * transaction. Allocator callbacks can cancel the source owner before claim:
 * no preservation of its resources is promised then. Its STORAGE still lives.
 * Fresh enqueue admission rechecks READY/currentness. No source claim in init.
 * After fixed-span/alias admission, a nonzero owned prefix returns BUSY
 * unchanged before copying inputs or
 * resetting controls, including nested init. Ordinary-method callback reentry
 * instead latches FAULT/uncertainty. No arbitrary opaque mutation detection.
 */
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_init(
    struct pt_prepared_paula_lifetime *,size_t capacity,
    struct pt_prepared_paula_sources *original_sources,
    const struct pt_prepared_paula_lifetime_config *,
    const struct pt_readers_backend *original_backend);
/* One attempt. Private reservation blocks disposal; actual lower enqueue OK
 * irreversibly transfers one C/one full3-source R even on outer fault. No supplied
 * transfer/quiet booleans. All genuine pins/leases stay in original source ledger.
 */
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_enqueue_unpublished(
    struct pt_prepared_paula_lifetime *);
/* One actual local stop only. C then R callbacks mark custody, never release
 * sampler/cache resources. No reader polling, source quiet, backend or ACTIVE.
 */
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_stop_local(
    struct pt_prepared_paula_lifetime *);
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
/* One actual publish attempt for the original queued event. The original
 * source owner stays busy/current around the real pt_readers_publish and its
 * authenticated current callbacks; no fresh source pointer/token is exported.
 * A Pending/refused/failed attempt cannot be replayed. It still retains the
 * original queue/C/R/source custody for explicit stop and separate collection.
 */
enum pt_scheduled_result pt_prepared_paula_lifetime_publish_once(
    struct pt_prepared_paula_lifetime *);
/* One genuine stop closes publication permanently. Unpublished entries may
 * consume local C then R here. Published entries keep their actual domains for
 * the separate command/reader cancel methods. Stop's completed outer return is
 * required before collection/drain; a non-OK result does not fabricate quiet.
 */
enum pt_scheduled_result pt_prepared_paula_lifetime_stop_publication(
    struct pt_prepared_paula_lifetime *);
/* At most ONE real core cancellation attempt per original domain. No supplied
 * receipt, consumed/retired boolean or quiet certificate is admitted. Pending,
 * observation or unknown/error retains actual unconsumed custody; this slice
 * exposes no retry or polling. The other domain may still be collected after
 * semantic failure. Core reader retirement can precede command detach, but its
 * actual R callback remains deferred until C references end. Before a backend
 * returns READER_RETIRED it must clear every actual persistent data/span/domain
 * and future-activation reference. Any remaining C event identity must never
 * revive or traverse that retired domain. These are genuine backend duties,
 * not facts inferred by this adapter from caller flags.
 */
enum pt_scheduled_result pt_prepared_paula_lifetime_cancel_command_once(
    struct pt_prepared_paula_lifetime *);
enum pt_scheduled_result pt_prepared_paula_lifetime_cancel_reader_once(
    struct pt_prepared_paula_lifetime *);
#endif /* HOST publication/collection only. */
/* Sticky first classified publication/collection non-OK/non-Pending result,
 * preserved through genuine
 * drain and close. Authenticated terminal good0 records BACKEND semantic failure
 * while actual releases still consume the original domain. Structural witness,
 * context/token/phase/duplicate or reentry uncertainty reports BACKEND and holds
 * resources. This scalar observation grants no ownership/currentness/quiet.
 */
enum pt_scheduled_result pt_prepared_paula_lifetime_failure(
    struct pt_prepared_paula_lifetime *);
/* <=one original resource disposal transition after actual C/R consumption AND
 * completed outer stop/collection return, closed admission, both actual core
 * held counts zero and no structural uncertainty. Semantic failure remains
 * classified but does not prevent genuine captured disposal. Repeated CLOSED
 * is observational. Release errors
 * retain original contexts; no retry of refused releases or resurrection.
 */
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_drain_step(
    struct pt_prepared_paula_lifetime *);
/* Queue close only after actual custody/drain (or before any transfer). Owner
 * storage is caller-owned, terminal/nonreusable; final allocator fault is held.
 * The raw queue pointer is consumed before the user allocator release callback.
 * If that callback faults after freeing, outer FAULT retains this aggregate and
 * contexts but does not restore the raw queue; later close never retries it.
 */
enum pt_prepared_paula_sources_result pt_prepared_paula_lifetime_close(
    struct pt_prepared_paula_lifetime *);

/* Friend interface used ONLY by the private source-ledger implementation and
 * this adapter. These are not public source/pointer/quiet certificates. Exact
 * original objects, mutual binding and original outer-call phase precede work.
 */
#define PT_PREPARED_PAULA_LIFETIME_DATA_SPANS 24U
struct pt_prepared_paula_lifetime_data {
    const uint8_t *first;size_t first_bytes;
    struct pt_scheduled_span span[PT_PREPARED_PAULA_LIFETIME_DATA_SPANS];
    unsigned count;
};
int pt_prepared_paula_lifetime_output_disjoint(const struct pt_prepared_paula_lifetime *,const void *,size_t);
int pt_prepared_paula_lifetime_source_phase(const struct pt_prepared_paula_lifetime *,const struct pt_prepared_paula_sources *,unsigned phase);
int pt_prepared_paula_sources_private_output_disjoint(struct pt_prepared_paula_sources *,const void *,size_t);
int pt_prepared_paula_sources_private_contexts(struct pt_prepared_paula_sources *,const struct pt_prepared_paula_context_extent *,unsigned);
int pt_prepared_paula_sources_private_current(struct pt_prepared_paula_sources *,struct pt_prepared_paula_lifetime *);
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_private_enqueue(struct pt_prepared_paula_sources *,struct pt_prepared_paula_lifetime *);
#if defined(PT_PREPARED_PAULA_ENABLE_HOST_LIFETIME)
enum pt_scheduled_result pt_prepared_paula_sources_private_publish(struct pt_prepared_paula_sources *,struct pt_prepared_paula_lifetime *);
enum pt_scheduled_result pt_prepared_paula_lifetime_lower_publish(struct pt_prepared_paula_lifetime *,struct pt_prepared_paula_sources *);
#endif
enum pt_prepared_paula_sources_result pt_prepared_paula_sources_private_drain(struct pt_prepared_paula_sources *,struct pt_prepared_paula_lifetime *);
enum pt_scheduled_result pt_prepared_paula_lifetime_lower_enqueue(struct pt_prepared_paula_lifetime *,struct pt_prepared_paula_sources *,const struct pt_prepared_paula_lifetime_data *);
#endif
