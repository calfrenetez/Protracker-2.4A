#ifndef PT_SAMPLER_PAULA_SEGMENT_READERS_H
#define PT_SAMPLER_PAULA_SEGMENT_READERS_H
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The finite segment/repeat reader profile is a HOST resource model only"
#endif
#include "sampler_paula.h"
#include "../core/scheduled_readers.h"
#include "../core/paula_segment_repeat_plan.h"
#define PT_PAULA_SEGMENT_DEPENDENCIES 3U
#define PT_PAULA_SEGMENT_COMMANDS 4U
#define PT_PAULA_SEGMENT_BACKEND_VERSION 1U
#define PT_PAULA_SEGMENT_BACKEND_REQUIRED 3U
struct pt_paula_segment_readers;
struct pt_sample_version;
/* A separately declared geometry-aware HOST contract. readers.submit must be
 * NULL: an ordinary CONTROL callback cannot silently perform a repeat-source
 * change. submit_plan must bind the exact event/full original reader domain to
 * the immutable geometry packet, preserve its original frame, and check actual
 * activation/ownership under backend exclusion. The plan is command-owned until
 * exact COMMAND_DETACHED; source/cache references belong to the full reader
 * domain until independent READER_RETIRED. No callback may edit either object,
 * reenter, call owner callbacks or infer a fetch boundary from musical frames.
 * Supplied immutable descriptor/owned-holder custody is checked across every
 * callback. A contract mutation is repaired only at saved original descriptor
 * destinations, returns UNKNOWN and latches failure; no genuine core/history
 * or resource counter is restored. Anchors end only at genuine owner release.
 * Serialized external-callback witness is unexposed and checked before any
 * owner callback reads its borrowed holder, including its pool pointer. No
 * owner functions are exposed by opaque bindings or added for negative tests.
 * Other callbacks retain the genuine scheduled_readers version2 semantics. */
struct pt_paula_segment_backend {
    struct pt_readers_backend readers;unsigned plan_version,plan_flags;
    int (*submit_plan)(void *,const struct pt_readers_event *,const struct pt_paula_segment_plan *);
};
struct pt_paula_segment_dependency {unsigned sample;struct pt_sample_version *expected;};
struct pt_paula_segment_readers_config {
    unsigned track,dependency_count;struct pt_paula_segment_dependency dependency[PT_PAULA_SEGMENT_DEPENDENCIES];
    struct pt_scheduled_grid grid;uint64_t session,frame[PT_PAULA_SEGMENT_COMMANDS];
    struct pt_paula_render_caps caps;size_t control_budget;
};
enum pt_paula_segment_readers_result {PT_PAULA_SEGMENT_READERS_OK,PT_PAULA_SEGMENT_READERS_PENDING,
    PT_PAULA_SEGMENT_READERS_INVALID,PT_PAULA_SEGMENT_READERS_CAPACITY,
    PT_PAULA_SEGMENT_READERS_STALE,PT_PAULA_SEGMENT_READERS_BUSY};
/* Internal opt-in finite one-track profile for an ALREADY validated immutable
 * project and genuine current pins held by the caller. bridge is already bound
 * through the existing sampler API; this is not an arbitrary input validator.
 * Original caller pins, bridge/project/sampler/allocator/backend contexts remain
 * alive through preparation and close. No in-place edits, owner copies/reentry.
 * Numeric fixed extent/alignment/disjoint checks precede typed new-input reads.
 * Known full version/master/cache/table extents are guarded; unknown allocator
 * context/queue extents additionally remain caller-disjoint. Open creates one
 * genuine new version2 queue with4 commands/1 reader. Admission is disjoint from
 * the old lowerer/song producer, whose SEGMENT/REPEAT refusal stays unchanged.
 * All4 immutable frames are strictly increasing; they are never rebased to now.
 * Backend capability declarations do not qualify native output/timing/quiet. */
enum pt_paula_segment_readers_result pt_paula_segment_readers_open(const struct pt_allocator *,
    struct pt_sampler_paula *,const struct pt_paula_segment_readers_config *,
    const struct pt_paula_segment_backend *,struct pt_paula_segment_readers **);
/* One metadata/pin transition OR <=256 signed8 cache bytes per step. All up to3
 * genuine independent pins and selective Chip leases are prepared before first
 * TRIGGER. Predeclared dependencies never grow/change after reader transfer.
 * Between calls generation/domain/descriptor changes refuse STALE. A callback
 * mutation/reentry latches failure; further admission refuses, while possible
 * publication retains all original obligations. Caller performs
 * explicit close for an untransferred failed preparation; no retry is implied. */
enum pt_paula_segment_readers_result pt_paula_segment_readers_step(struct pt_paula_segment_readers *,unsigned *ready);
/* Exactly SEGMENT, REPEAT, REPEAT, STOP at the configured original4 boundaries.
 * REPEAT must be the genuine renderer pending-source state; every current/next
 * descriptor must be one of the predeclared genuine dependencies. Superseded
 * pending sources remain in the immutable original reader domain. Continuing
 * actions require the full immutable key from the genuine positive key getter.
 * begin makes one sealed small command holder; enqueue transfers it. A failed
 * begin preserves output and allocates nothing. No physical boundary is proven. */
enum pt_paula_segment_readers_result pt_paula_segment_readers_begin(struct pt_paula_segment_readers *,
    uint64_t,const struct pt_render_action *,const struct pt_readers_key *,unsigned *command);
enum pt_scheduled_result pt_paula_segment_readers_enqueue(struct pt_paula_segment_readers *,unsigned,uint64_t *);
enum pt_scheduled_result pt_paula_segment_readers_publish(struct pt_paula_segment_readers *,unsigned);
enum pt_scheduled_result pt_paula_segment_readers_poll_command(struct pt_paula_segment_readers *,unsigned,
    int cancel,struct pt_readers_command_receipt *);
enum pt_scheduled_result pt_paula_segment_readers_poll_reader(struct pt_paula_segment_readers *,int cancel,
    struct pt_readers_reader_receipt *);
enum pt_scheduled_result pt_paula_segment_readers_key(struct pt_paula_segment_readers *,struct pt_readers_key *);
/* Whole core stop only closes admission/cancels genuinely local holders.
 * Published holders require explicit exact command and reader proofs. STOP
 * issue/detach, source supersession, a numerical phase or elapsed deadline never
 * disposes a dependency. Independent reader retirement AND zero command refs
 * trigger exactly-once pin/lease disposal through genuine core owner callbacks.
 * Close refuses every remaining possible reference; it never forces a lease,
 * resets a device, invents ACTIVE/quiet or calls a backend cancel implicitly. */
enum pt_scheduled_result pt_paula_segment_readers_stop(struct pt_paula_segment_readers *);
int pt_paula_segment_readers_close(struct pt_paula_segment_readers *);
#endif
