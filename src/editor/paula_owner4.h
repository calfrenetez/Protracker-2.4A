#ifndef PT_PAULA_OWNER4_H
#define PT_PAULA_OWNER4_H
#include "paula_segment_sequence.h"
#include "paula_segment_future.h"
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The fixed-four checked-stop owner is a serialized HOST proposal only"
#endif
/* Additive Source proposal; no native/frontend admission. Valid immutable
 * project, three genuine CURRENT caller pins and complete opaque callback
 * extents are borrowing preconditions. All callers serialize all entry/mutation
 * paths and retain storage/contexts until positive close or deliberate HOLD.
 * This facade does not hook direct sampler/pattern/cache/teardown APIs. */
struct pt_paula_owner4;
enum pt_paula_owner4_result {
    PT_OWNER4_OPEN, PT_OWNER4_STOPPING, PT_OWNER4_CLOSED,
    PT_OWNER4_REFUSED, PT_OWNER4_HOLD
};
enum pt_paula_owner4_retire_order {
    PT_OWNER4_COMMAND_FIRST, PT_OWNER4_READER_FIRST
};
size_t pt_paula_owner4_size(void);
size_t pt_paula_owner4_alignment(void);
/* Internally constructs the exact future_init backend and matching real
 * sequence. No existing producer/future handles or pair certificate accepted.
 * Clean numeric refusal leaves outputs/storage untouched. After any constructor
 * begins, failure returns HOLD with an outer anchor: a failed sequence_begin
 * can leave an unreturned partial workspace, which must NOT be cast into an
 * owner, closed, freed, zeroed or silently retried. Storage is noncopyable.
 * Each allocator entry captures live headers and rejects callback metadata changes
 * before following a changed pointer; original source anchors stay retained.
 * All input structs and known source extents are disjoint from the three
 * complete controls and output. Allocator's opaque context remains caller-
 * disjoint from every control/child allocation and project/version/cache span. */
enum pt_paula_owner4_result pt_paula_owner4_construct(
    void *,size_t,void *,size_t,void *,size_t,
    const struct pt_allocator *,struct pt_sampler_paula *,
    const struct pt_paula_segment_sequence_config *,
    const struct pt_paula_future_ops *,uint32_t,
    enum pt_paula_owner4_retire_order,struct pt_paula_owner4 **);
enum pt_paula_segment_sequence_result pt_paula_owner4_step(
    struct pt_paula_owner4 *,uint32_t,unsigned);
enum pt_scheduled_result pt_paula_owner4_publish(struct pt_paula_owner4 *,uint32_t);
enum pt_paula_future_result pt_paula_owner4_deliver(struct pt_paula_owner4 *,uint64_t);
enum pt_scheduled_result pt_paula_owner4_poll_command(struct pt_paula_owner4 *,unsigned);
/* Closes admission first; once-only real sequence cancellation and future stop
 * request. Cancellation FAILED is deliberate and never converted to quiet.
 * One explicit advance performs <=one service/probe/disposal/close operation,
 * with four fixed originals and no loop/rebase/fifth command. Unknown callback,
 * restore, core fault or partial constructor retains anchors as sticky HOLD.
 * Future dispose must be positive BEFORE genuine producer close. */
enum pt_paula_owner4_result pt_paula_owner4_request_stop(struct pt_paula_owner4 *);
enum pt_paula_owner4_result pt_paula_owner4_advance_stop(struct pt_paula_owner4 *);
/* Observation only, never persistent READY/quiet/currentness or native authority.
 * CLOSED scopes this owner; genuine caller pins and other owners remain live
 * until their separately balanced release, and all edit paths remain excluded. */
enum pt_paula_owner4_result pt_paula_owner4_status(const struct pt_paula_owner4 *);
#endif
