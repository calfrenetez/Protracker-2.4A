#ifndef PT_DIAGNOSTIC_CIA_APERTURE_MODEL_H
#define PT_DIAGNOSTIC_CIA_APERTURE_MODEL_H
#include <stddef.h>
#include <stdint.h>
#include "../src/core/elapsed_clock.h"
/* Timer/software-commit diagnostic only. These synthetic keys are not actual
 * queue registration, adopted sample ownership, scheduled output or timing proof. */
#define PT_APERTURE_MAX_READS 256U
#define PT_APERTURE_MAX_TICKS 4096U
#define PT_APERTURE_SHADOW UINT32_C(0x41504552)
struct pt_aperture_key {
    uintptr_t queue;uint64_t session,generation,trigger,owner,serial;
    unsigned action,slot;
};
struct pt_aperture_control {
    struct pt_aperture_key key;uint64_t publication;
    unsigned armed,cancelled;
};
struct pt_aperture_policy {
    unsigned early_ticks,maximum_residency_ticks,maximum_reads;
};
enum pt_aperture_result {
    PT_APERTURE_READY,PT_APERTURE_COMMITTED,PT_APERTURE_INVALID,
    PT_APERTURE_STALE,PT_APERTURE_CLOCK,PT_APERTURE_EXPIRED,PT_APERTURE_EARLY,
    PT_APERTURE_READ_LIMIT,PT_APERTURE_RESIDENCY,
    PT_APERTURE_POST_WINDOW,PT_APERTURE_CHANGED_AFTER,PT_APERTURE_REENTRY
};
struct pt_aperture_state {
    struct pt_aperture_key expected;uint64_t publication;
    uint64_t frame,first,last,arm_at;
    uint32_t frequency;struct pt_aperture_policy policy;
    /* Every executed attempt records available actual samples, including
     * failure. Missing/failed reads have validity0, never predicted ticks.
     * entry=first read; before=latest precommit read; after=postcommit read.
     * A postcommit failure retains shadow/commits1 and cannot claim zero effects. */
    uint64_t entry,before,after,last_observed;
    uint32_t entry_frequency,after_frequency;
    uint64_t last_read;uint32_t last_read_frequency;
    unsigned last_read_valid,entry_valid,before_valid,after_valid,reads,commits;
    volatile uint32_t shadow;
    unsigned busy,reentry;
    enum pt_aperture_result result;
};
/* Reuses elapsed_clock on the caller's original immutable epoch. No new epoch,
 * rebase, lateness tolerance, calibrated subtraction or IRQ/timer programming.
 * Refusal preserves all output/input images. All source extents are protected. */
enum pt_aperture_result pt_aperture_prepare(const struct pt_elapsed_clock *,
    uint64_t,const struct pt_aperture_policy *,const struct pt_aperture_control *,
    struct pt_aperture_state *);
/* Bounded diagnostic read-only clock callback, never allocator/editor/owner
 * traversal. Native caller supplies the fixed IRQ-safe ReadEClock wrapper.
 * Complete callback context is disjoint from state/control; external OS/device
 * storage follows its own lifetime contract. Caller keeps immutable publication
 * and exclusion valid during the entire attempt, including the commit.
 * Invalid spans refuse before writes/callback; valid attempts run once. Nested
 * invocation latches reentry and prevents outer commit. At most maximum_reads
 * callbacks, including after-read. No automatic retry; preparation is task work.
 * Precommit stale/late/cap/residency failures commit0. Late/failed after-read or
 * postcommit change reports failure with commits1, never rollback/fake success. */
enum pt_aperture_result pt_aperture_run(struct pt_aperture_state *,
    const struct pt_aperture_control *,
    int (*)(void *,uint64_t *,uint32_t *),void *,size_t);
#endif
