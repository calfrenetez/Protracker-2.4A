#ifndef PT_MIXED_QUANTIZED_AUDIT_H
#define PT_MIXED_QUANTIZED_AUDIT_H
#include "mixed_readers_plan_normalize.h"
#include "../core/render.h"

#define PT_MIXED_QAUDIT_CONTEXTS 30U
#define PT_MIXED_QAUDIT_WORK_MAX 256U
struct pt_mixed_quantized_audit;
struct pt_mixed_quantized_audit_inputs {
    const struct pt_project *project;
    const struct pt_render_options *options;
    const struct pt_paula_render_caps *caps;
    const struct pt_playback_format *format;
    const struct pt_allocator *allocator;
    const struct pt_mixed_plan_span *contexts;unsigned context_count;
    void *normalizer_storage;size_t normalizer_capacity;
    void *result_storage;size_t result_capacity;
    size_t ordinary_byte_budget;
    uint32_t revision,generation;
};
enum pt_mixed_quantized_audit_result {
    PT_MIXED_QAUDIT_INVALID=0,PT_MIXED_QAUDIT_PENDING,PT_MIXED_QAUDIT_READY,
    PT_MIXED_QAUDIT_TAKEN,PT_MIXED_QAUDIT_CANCELLED,PT_MIXED_QAUDIT_CLOSED,
    PT_MIXED_QAUDIT_REFUSED,PT_MIXED_QAUDIT_STALE,PT_MIXED_QAUDIT_ALIAS,
    PT_MIXED_QAUDIT_BUSY,PT_MIXED_QAUDIT_FAILED,PT_MIXED_QAUDIT_CAPACITY
};
enum pt_mixed_quantized_audit_phase {
    PT_MIXED_QAUDIT_SETUP_BEGIN,PT_MIXED_QAUDIT_SETUP_STEP,
    PT_MIXED_QAUDIT_SETUP_TAKE,PT_MIXED_QAUDIT_MEASURE,
    PT_MIXED_QAUDIT_NEXT,PT_MIXED_QAUDIT_CONSUME,PT_MIXED_QAUDIT_COMPLETE,
    PT_MIXED_QAUDIT_Q_BEGIN,PT_MIXED_QAUDIT_Q_STEP,PT_MIXED_QAUDIT_Q_GET,
    PT_MIXED_QAUDIT_Q_CLOSE,PT_MIXED_QAUDIT_COMMIT,PT_MIXED_QAUDIT_FINISHED
};
struct pt_mixed_quantized_audit_report {
    enum pt_mixed_quantized_audit_result result;
    enum pt_mixed_quantized_audit_phase phase;
    enum pt_render_result render_result;
    enum pt_render_setup_result setup_result;
    struct pt_mixed_plan_report normalizer;
    /* Exact consumed frames/intervals; work is charged item budget. Opaque
     * renderer measurement charges requested ticks, never claims actual ticks. */
    uint64_t frames,intervals,work;
    unsigned last_work,allocation_requests,rewinds;
    uint8_t samples[2][PT_PROJECT_SAMPLES];
};
/* Standalone geometry audit of EVERY completed genuine renderer boundary.
 * No pins, source promotion, editor hook, cache, key, queue or device. Source
 * lifetime/immutability remains the caller's responsibility; in-place edits
 * change revision/generation. Selection movement alone is permitted. READY is
 * descriptive, not a validation/reader/publication certificate. Only whole-song
 * all-track options are accepted; time/geometry refusals never add jitter.
 *
 * Query and supply three mutually disjoint aligned caller buffers. Zero their
 * USED bytes initially, keep full capacities protected/alive. Admission charges
 * all three FULL capacities plus actual setup+sequence queried control bytes,
 * with overflow checks. Exactly two genuine renderer requests, no retries.
 * Unknown complete allocator/caller contexts must be declared (max30). Fixed
 * headers/tags precede former tables; numeric captures govern stale cleanup.
 * All calls serialized/noncopyable. Fully admitted reentry faults the owner;
 * invalid/aliased external spans refuse before writable latches/callbacks.
 *
 * Private result storage contains the stable ORIGINAL Q owner slot and a
 * separate batch. It must not be embedded in a complete parent context or named
 * as child immutable context; full outer guards protect its legitimate writes.
 * Audit original owner slot remains alive and protected until exact close.
 *
 * SAME sequence take rewinds once and transfers the actual pointer. Its guarded
 * allocator STILL BORROWS THIS AUDIT WORKSPACE and original fixed controls.
 * Close refuses while transferred sequence live. Caller must close that sequence
 * first; its actual wrapped release retires the ledger under BUSY. Only then can
 * exact audit close consume its slot, even stale, without former table reads.
 * Concurrent destruction/freeing invalid allocator callbacks violate this borrow
 * contract; numeric guards do not prove residency or make that violation safe.
 */
size_t pt_mixed_quantized_audit_workspace_size(void);
size_t pt_mixed_quantized_audit_workspace_alignment(void);
size_t pt_mixed_quantized_audit_result_size(void);
size_t pt_mixed_quantized_audit_result_alignment(void);
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_begin_in_workspace(
    void *,size_t,const struct pt_mixed_quantized_audit_inputs *,
    struct pt_mixed_quantized_audit **);
/* <=work actual setup/measure/frame/Q item work, or ONE child/local phase.
 * Initial/captured metadata loops are separately finite, not a WCET/IRQ proof. */
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_step(
    struct pt_mixed_quantized_audit *,uint32_t,uint32_t,unsigned);
/* NULL checked query. Numeric reports may be obtained stale; masks zero except
 * READY. Full output guarded before stale/busy writes. No partial plan/origins. */
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_get(
    struct pt_mixed_quantized_audit *,uint32_t,uint32_t,
    struct pt_mixed_quantized_audit_report *);
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_take(
    struct pt_mixed_quantized_audit *,uint32_t,uint32_t,struct pt_render_sequence **);
/* Numeric-only mark; no child call. Close explicitly drains Q before renderer.
 * Close may consume children while returning a failure; actual NULL consumption
 * is retained. Copied owner slot refuses; NULL exact close is terminal. */
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_cancel(
    struct pt_mixed_quantized_audit *);
enum pt_mixed_quantized_audit_result pt_mixed_quantized_audit_close(
    struct pt_mixed_quantized_audit **);
#endif
