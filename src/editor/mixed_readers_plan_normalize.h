#ifndef PT_MIXED_READERS_PLAN_NORMALIZE_H
#define PT_MIXED_READERS_PLAN_NORMALIZE_H
#include "../core/render_commands.h"
#include "../core/paula_render_voice.h"
#include "../core/amigus_voice_plan.h"

#define PT_MIXED_PLAN_RECORDS 16U
#define PT_MIXED_PLAN_CONTEXTS 32U
#define PT_MIXED_PLAN_WORK_MAX 256U
#define PT_MIXED_PLAN_GAIN_CANDIDATES 16449U
struct pt_mixed_plan_normalizer;
struct pt_mixed_plan_span {const void *data;size_t bytes;};
/* Logical timeline identity ONLY. Never a pin, ACTIVE key, reader reference,
 * validation certificate or permission to publish. Empty origins are all zero. */
struct pt_mixed_plan_origin {
    unsigned present,sample,channel;uint64_t frame;
};
struct pt_mixed_plan_inputs {
    const struct pt_project *project;
    const struct pt_render_plan *plan;
    const struct pt_mixed_plan_origin *origins; /* Complete16-entry vector. */
    const struct pt_paula_render_caps *caps;
    const struct pt_playback_format *format;
    const struct pt_mixed_plan_span *contexts;unsigned context_count;
    unsigned rate;uint64_t frame;uint32_t revision,generation;
};
enum pt_mixed_plan_kind {PT_MIXED_PLAN_TRIGGER,PT_MIXED_PLAN_CONTROL,PT_MIXED_PLAN_STOP};
struct pt_mixed_plan_record {
    enum pt_mixed_plan_kind kind;unsigned track,route,slot,sample,channel;
    unsigned first_action,control_action; /* UINT_MAX means no folded control. */
    union {
        struct {uint16_t period;uint8_t volume;} paula;
        struct {unsigned bits,little_endian;
            struct pt_amigus_voice_request trigger;
            uint32_t rate;uint16_t left,right;} amigus;
    } geometry;
    /* Semantic origin-zero oracle for card TRIGGER only. Numeric description,
     * NEVER an allocated card range, reservation or physical address proof. */
    struct pt_amigus_voice_plan image;
};
struct pt_mixed_plan_batch {
    uint64_t frame;unsigned count;
    struct pt_mixed_plan_record record[PT_MIXED_PLAN_RECORDS];
    struct pt_mixed_plan_origin next[PT_MIXED_PLAN_RECORDS];
    uint8_t samples[2][PT_PROJECT_SAMPLES];
};
enum pt_mixed_plan_result {
    PT_MIXED_PLAN_INVALID=0,PT_MIXED_PLAN_PENDING,PT_MIXED_PLAN_READY,
    PT_MIXED_PLAN_REFUSED,PT_MIXED_PLAN_STALE,PT_MIXED_PLAN_ALIAS
};
enum pt_mixed_plan_phase {
    PT_MIXED_PLAN_SHAPES,PT_MIXED_PLAN_ORIGINS,PT_MIXED_PLAN_RECORD,
    PT_MIXED_PLAN_SOURCE,PT_MIXED_PLAN_GEOMETRY,PT_MIXED_PLAN_GAINS,
    PT_MIXED_PLAN_COMPLETE
};
enum pt_mixed_plan_reason {
    PT_MIXED_PLAN_REASON_NONE,PT_MIXED_PLAN_REASON_COUNT,
    PT_MIXED_PLAN_REASON_ROUTE,PT_MIXED_PLAN_REASON_KIND,
    PT_MIXED_PLAN_REASON_DUPLICATE,PT_MIXED_PLAN_REASON_FOLD,
    PT_MIXED_PLAN_REASON_ORIGIN,PT_MIXED_PLAN_REASON_SOURCE,
    PT_MIXED_PLAN_REASON_PAULA,PT_MIXED_PLAN_REASON_AMIGUS,
    PT_MIXED_PLAN_REASON_GAINS
};
struct pt_mixed_plan_report {
    enum pt_mixed_plan_result result;enum pt_mixed_plan_phase phase;
    enum pt_mixed_plan_reason reason;
    unsigned action,track,kind,last_work,actions_scanned,candidates;
    uint64_t work;
};
/* Private allocation-free geometry adapter. Borrow genuine readable, already
 * semantically validated immutable project/PCM descriptors. It neither proves
 * that precondition nor scans/pins/promotes/changes PCM. No callbacks, queues,
 * sampler owners, factory handles, timer, device, audio or full-song audit.
 *
 * Query/caller-allocate aligned Fast workspace; zero its USED bytes initially.
 * The COMPLETE supplied capacity and complete immutable inputs/context spans
 * must be disjoint; inputs/plan/origins/caps/format/context declarations and all
 * source storage remain alive/immutable through close. Unknown external control
 * capacities belong in contexts. Context declarations are guards, not authority.
 * Do not embed this workspace inside a declared complete parent context span.
 * Calls are serialized/noncopyable; in-place source edits change actual revision
 * or generation. Valid selection-cursor movement alone is permitted. Header/tag
 * checks precede former descriptor walks. All external publication and local
 * converter outputs are guarded before writes; full spare PCM/tails are guarded.
 * The known original owner-pointer slot remains alive and holds the genuine
 * job until close; its entire pointer extent is protected from writable outputs.
 * Initial/fixed metadata guards are finite separately from charged step items.
 *
 * Only singleton TRIGGER/CONTROL/STOP or TRIGGER+one identical final CONTROL
 * (except step) reduces. Factory-fresh Paula map/card slots are preserved.
 * Normalized source/channel/geometry and tentative origins are descriptive only:
 * a later genuine owner must pin current masters and obtain exact original ACTIVE
 * keys itself. READY never authorizes cache/upload/enqueue/publication. Unsupported
 * shapes/geometry refuse; no skipped actions, splitting, approximation or jitter.
 */
size_t pt_mixed_plan_normalizer_workspace_size(void);
size_t pt_mixed_plan_normalizer_workspace_alignment(void);
enum pt_mixed_plan_result pt_mixed_plan_normalizer_begin_in_workspace(
    void *,size_t,const struct pt_mixed_plan_inputs *,struct pt_mixed_plan_normalizer **);
/* work1..256 charges a raw action/origin/source/record/conversion or one exact
 * volume/pan candidate. This bound is not a latency, IRQ or native stack proof. */
enum pt_mixed_plan_result pt_mixed_plan_normalizer_step(
    struct pt_mixed_plan_normalizer *,uint32_t revision,uint32_t generation,unsigned work);
/* Writes the ENTIRE batch only on READY. Otherwise unchanged. NULL is a checked
 * query. Full16 result/tentative vectors and all64 input action slots are guarded. */
enum pt_mixed_plan_result pt_mixed_plan_normalizer_get(
    struct pt_mixed_plan_normalizer *,uint32_t revision,uint32_t generation,struct pt_mixed_plan_batch *);
/* Numeric-capture-only diagnostic query, safe after source expiration. No partial
 * batch/origins/masks. NULL checked query. Does not walk former source arrays. */
enum pt_mixed_plan_result pt_mixed_plan_normalizer_report(
    struct pt_mixed_plan_normalizer *,struct pt_mixed_plan_report *);
/* Local cancellation/consumption only: captured numeric guards, no former table
 * walks, pin release, voice STOP, backend polling or callbacks. Clears USED job
 * bytes and consumes ONLY the exact original owner slot toNULL; a copied non-NULL
 * owner slot refuses. NULL is idempotently terminal. Supplied extra
 * capacity is caller-owned and unchanged. No external source ownership is freed. */
int pt_mixed_plan_normalizer_close(struct pt_mixed_plan_normalizer **);
#endif
