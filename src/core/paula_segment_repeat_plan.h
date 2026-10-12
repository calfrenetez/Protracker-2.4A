#ifndef PT_PAULA_SEGMENT_REPEAT_PLAN_H
#define PT_PAULA_SEGMENT_REPEAT_PLAN_H
#include "render_commands.h"
#include "paula_render_voice.h"
#include "sample_cache.h"
struct pt_sample_version;
#define PT_PAULA_SEGMENT_SOURCE_GUARDS 10U
struct pt_paula_segment_span {const void *data;size_t bytes;};
/* Descriptive snapshots taken while genuine sampler pins and signed8 Chip
 * leases are held. No field, equality result or planner success authenticates
 * ownership/currentness. The caller must independently validate the original
 * version and lease in the SAME serialized operation before dispatch.
 * descriptor is the immutable renderer PCM descriptor; master is its exact
 * copy, not a different quantized source. Mono8/16/24 masters are permitted.
 * cache_data/cache_bytes name the complete signed8, even-padded representation;
 * padding is protected, never selected as an audible frame. version and
 * cache_owner are opaque equality-only identities, never followed here.
 * Extra spans protect complete known owner/version/control allocations. Obtain
 * genuine version spans through the existing sampler API; they are guards,
 * not certificates. Unknown opaque extents additionally stay caller-disjoint.
 * Every descriptor, pin, lease and control context outlives all dependent plans
 * and actual readers. In-place edits are forbidden, not detected by PCM scans.
 */
struct pt_paula_segment_source {
    const struct pt_pcm *descriptor;struct pt_pcm master;
    const struct pt_sample_version *version;const void *sampler_owner,*project_identity,*cache_owner;
    uint64_t cache_version;unsigned generation,sample;
    struct pt_cache_lease lease;const uint8_t *cache_data;size_t cache_bytes;
    unsigned guard_count;struct pt_paula_segment_span guard[PT_PAULA_SEGMENT_SOURCE_GUARDS];
};
/* Half-open frame geometry. One mono signed8 playback frame is exactly one
 * cache byte; words are byte_length/2, never master int32 element counts.
 * No cache pointer is offset by this planner. The consumer must use checked
 * retained cache identity and these offsets only after genuine admission.
 */
struct pt_paula_segment_range {
    uint32_t frame_offset,frame_length,byte_offset,byte_length;uint16_t words;
};
enum pt_paula_segment_kind {PT_PAULA_SEGMENT_INITIAL,PT_PAULA_SEGMENT_REPROGRAM,PT_PAULA_SEGMENT_STOP_REQUIREMENTS};
enum pt_paula_segment_result {PT_PAULA_SEGMENT_OK,PT_PAULA_SEGMENT_INVALID,
    PT_PAULA_SEGMENT_UNSUPPORTED,PT_PAULA_SEGMENT_STALE,PT_PAULA_SEGMENT_ALIAS};
struct pt_paula_segment_plan {
    enum pt_paula_segment_kind kind;uint64_t frame,phase_q32;
    unsigned track,slot,source_count,write_initial,write_repeat;
    uint16_t period;uint8_t volume;
    struct pt_paula_segment_range initial,repeat;
    struct pt_paula_segment_source source[2];
};
/* Opt-in, allocation-free, metadata-only task-side plan. No sample/cache reads,
 * callbacks, owner retain/release, queue publication or hardware operations.
 * SEGMENT must be exact initial noninterpolated forward geometry with no pending
 * alternate PCM; next must equal current. REPEAT is the actual pending-source
 * state emitted by voice_set_repeat_source: preserve its exact Q32 current phase
 * and existing fetch; write_initial=0, never restart or round that fetch. The
 * next range is programmed for the existing fetch's next boundary, not assumed
 * to activate at frame or at a computed/sample-count deadline. Cross-source
 * repeats require identical source rate/bits (the current renderer permits only
 * classic mono8 cross-source handoffs; this API preserves that restriction).
 * Initial and repeat ranges may overlap or be disjoint. Even starts/lengths,
 * words1..65535, exact source extents, forward loops and mono required. No silent
 * first word, zero padding, resampling, pingpong, stereo, interpolation, restored
 * loop phase or peripheral capability is inferred. Source_count is unique
 * dependency count1/2, not reader-retirement proof or automatic ref transfer.
 * frame remains the caller's original musical boundary, not now/rebased time.
 * Inputs must be full live objects; numeric extent/alignment preflight precedes
 * every typed input read. Output protects fixed inputs, PCM capacities, complete
 * cache bytes and declared extra spans. Refusal preserves all inputs/output.
 */
enum pt_paula_segment_result pt_paula_segment_plan_build(const struct pt_render_action *,
    const struct pt_paula_segment_source *current,const struct pt_paula_segment_source *next,
    uint64_t frame,unsigned output_rate,unsigned slot,const struct pt_paula_render_caps *,
    struct pt_paula_segment_plan *);
/* Comparison only, NOT current/ACTIVE/ownership proof. Both fresh observations
 * and every original plan source remain alive. All original version, generation,
 * sample, descriptor, capacity, lease serial, owner, bytes and guards must match.
 * No former descriptor is followed: descriptor content is compared to the fresh
 * copied master snapshot. Actual sampler/lease validation stays caller-owned.
 */
int pt_paula_segment_plan_identity_equal(const struct pt_paula_segment_plan *,
    const struct pt_paula_segment_source *current,const struct pt_paula_segment_source *next);
/* Declarative terminal requirements for an unmodified initialized plan. No
 * dispatch, cancellation, STOP effect, quiet, reader retirement or unpin occurs.
 * Preserve THIS plan's original dependencies until genuine independent command/
 * reader/source proofs permit release. Earlier plans, including superseded
 * pending repeat sources, remain separately retained under their original owner;
 * this fixed two-source planner never compresses or releases that history.
 * No former PCM descriptor is read, permitting retained old-version cleanup.
 * Repeated stop planning is idempotent at the same original boundary; earlier
 * boundaries refuse. Full original capacities/control guards remain protected.
 * A never-published plan is disposed by its genuine owner; planner disposal is
 * not a release operation. Reader/backend/whole-song integration is pending.
 */
enum pt_paula_segment_result pt_paula_segment_plan_stop(const struct pt_paula_segment_plan *,
    uint64_t frame,struct pt_paula_segment_plan *);
#endif
