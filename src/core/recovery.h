#ifndef PT_RECOVERY_H
#define PT_RECOVERY_H
#include <stdint.h>
/* Task-context policy only. The caller chooses the destination and establishes
 * that it is persistent, non-removable storage (or explicitly allowed removable
 * storage). Unknown/RAM media must not be called persistent. No I/O or allocation
 * occurs in this owner. Reset it when the document identity changes. */
struct pt_recovery_policy {
    uint32_t interval_seconds;
    uint8_t enabled,allow_removable;
};
struct pt_recovery_schedule {
    struct pt_recovery_policy policy;
    uint64_t since,observed,snapshot_revision,snapshot_saved_revision;
    uint8_t armed,have_snapshot,busy;
};
enum pt_recovery_tick_result {
    PT_RECOVERY_SKIPPED,PT_RECOVERY_SAVED,PT_RECOVERY_FAILED,PT_RECOVERY_INVALID
};
/* Zero-initialize schedule before first use. Nonzero interval required even when disabled. Invalid/reentrant configuration
 * preserves the existing schedule. Successful configuration starts a new interval. */
int pt_recovery_configure(struct pt_recovery_schedule *,const struct pt_recovery_policy *);
/* Revisions are identities, not ordered numbers: undo may make revision smaller
 * than saved_revision. Both are supplied by the same current document journal.
 * now is monotonic seconds; backwards time restarts the interval. Continuous
 * edits do not postpone a due snapshot; clean/disabled/ineligible media disarm.
 * safe_to_write must be false during playback, capture or another transaction.
 * The synchronous callback must leave masters/journal immutable; exactly1 means
 * successful verified publication. Failure backs off for a full interval and
 * never replaces the last successful revision. Reentrant ticks are refused. */
enum pt_recovery_tick_result pt_recovery_tick(struct pt_recovery_schedule *,
    uint64_t now,uint64_t revision,uint64_t saved_revision,
    int persistent,int removable,int safe_to_write,int (*snapshot)(void *),void *);
#endif
