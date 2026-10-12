#ifndef PT_PAULA_DELIVERY4_TASK_H
#define PT_PAULA_DELIVERY4_TASK_H
#include "paula_segment_future.h"
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "Delivery4 is an unqualified TASK snapshot proposal, not a native backend"
#endif
#define PT_PAULA_DELIVERY4_ACTIONS 4U
#define PT_PAULA_DELIVERY4_DEPENDENCIES 3U
/* Ordinary caller-owned storage. Copying a snapshot transfers no pin, lease,
 * queue command, output reader, timer reference or activation authority.
 * Its genuine upstream reader holds every source/cache/guard until independent
 * retirement; this structure and all contexts remain alive through final restore.
 * It describes exactly the original typed packet, without packet32/rolling casts.
 * The proposed native holder would contain exactly four once-only action slots
 * and four independent mailboxes. No native ABI or concurrency is implemented. */
struct pt_paula_delivery4_action {
    struct pt_paula_future_packet packet;
    const struct pt_readers_domain *domain;
    uint32_t frequency;
    unsigned publication;
};
struct pt_paula_delivery4_actions {
    struct pt_paula_delivery4_action action[PT_PAULA_DELIVERY4_ACTIONS];
};
/* The native successor needs an explicitly owned three-dependency domain,
 * not only the one/two sources selected by each action. In particular a
 * superseded pending repeat source stays retained until reader retirement.
 * This proposal leaves that ownership in the genuine existing reader pool. */
struct pt_paula_delivery4_observation {
    uint64_t ticket,observed,issued;
    unsigned publication;
    enum pt_paula_future_result result;
};
/* Proposed mailbox payload only. No flag, payload or result proves adoption,
 * quiet, detachment, currentness or successful restoration. No mailbox writer,
 * reader, callback, native atomic or acknowledgment is implemented here. */
struct pt_paula_delivery4_mailboxes {
    struct pt_paula_delivery4_observation slot[PT_PAULA_DELIVERY4_ACTIONS];
};
enum pt_paula_delivery4_snapshot_result {
    PT_PAULA_DELIVERY4_SNAPSHOT_OK,PT_PAULA_DELIVERY4_SNAPSHOT_STALE,
    PT_PAULA_DELIVERY4_SNAPSHOT_INVALID,PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS
};
enum pt_paula_delivery4_window {
    PT_PAULA_DELIVERY4_BAD_WINDOW,PT_PAULA_DELIVERY4_OLD_TICKET,
    PT_PAULA_DELIVERY4_EARLY,PT_PAULA_DELIVERY4_IN_WINDOW,PT_PAULA_DELIVERY4_LATE
};
/* TASK only, serialized, ALREADY validated immutable project and already held
 * genuine expected pins plus original leases. Not an arbitrary-input validator.
 * Runs fresh existing current/version/route/lease/descriptor checks in THIS
 * call; temporary successful master pins are balanced exactly once. No cache
 * acquisition, allocation, conversion, publication, persistent retain or release
 * of any upstream owner. No callbacks or edits may interleave. dependency[0..2] and expected[0..2] are the full three genuine
 * predeclared dependency observations/pins, including superseded sources.
 * They come from the genuine reader owner; no public getter or binding is added.
 * Refusal preserves out and packet. Success is an ordinary fresh metadata
 * snapshot, NOT currentness sealed across calls and NOT permission to dispatch.
 * All known complete project tables, PCM capacities, slices, extension payloads
 * and source/control extents protect writes. All three original guard lists are
 * compared with their actual held version spans plus bridge/sampler/project;
 * unknown opaque
 * context extents additionally stay caller-disjoint. Caller supplies original
 * grid/four frames. Initial sampler edit generation zero is permitted. */
enum pt_paula_delivery4_snapshot_result pt_paula_delivery4_task_snapshot(
    struct pt_sampler_paula *,const struct pt_paula_future_packet *,
    const struct pt_readers_domain *,
    const struct pt_paula_segment_source dependency[PT_PAULA_DELIVERY4_DEPENDENCIES],
    struct pt_sample_version *const expected[PT_PAULA_DELIVERY4_DEPENDENCIES],
    const struct pt_scheduled_grid *,
    const uint64_t frame[PT_PAULA_DELIVERY4_ACTIONS],unsigned publication,
    struct pt_paula_delivery4_action *);
/* Pure comparison of supplied clock observation, not a clock acquisition,
 * delivery, activation grant, rearm, rebase or quiet proof. The later native
 * seam must authenticate its actual clock source and full delivery currentness
 * separately. No storage is mutated and no callback or owner API is called. */
enum pt_paula_delivery4_window pt_paula_delivery4_window_observe(
    const struct pt_paula_delivery4_action *,uint64_t ticket,
    uint64_t observed,uint32_t frequency);
#endif
