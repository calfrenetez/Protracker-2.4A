#ifndef PT_NATIVE_RECOVERY_H
#define PT_NATIVE_RECOVERY_H
#include "../core/recovery.h"
#include "../platform/recovery_find.h"
#define PT_NATIVE_RECOVERY_ROOT_SIZE 360
enum pt_native_recovery_media {
    PT_NATIVE_RECOVERY_MEDIA_UNKNOWN,PT_NATIVE_RECOVERY_MEDIA_FIXED,
    PT_NATIVE_RECOVERY_MEDIA_REMOVABLE
};
struct pt_native_recovery_configuration {
    char directory[PT_NATIVE_RECOVERY_ROOT_SIZE];
    uint32_t interval_seconds;
    uint8_t enabled,media,allow_removable;
};
struct pt_native_recovery {
    struct pt_allocator allocator;
    struct pt_recovery_schedule schedule;
    struct pt_recovery_store store;
    struct pt_recovery_info info;
    const struct pt_project *project;
    char root[PT_NATIVE_RECOVERY_ROOT_SIZE];
    uint64_t source_timestamp;
    uint8_t configured,bound,removable;
};
/* Zero-initialize before first use; reconfiguration refuses opened/owned/busy
 * storage or a busy schedule. The allocator is staged before resetting owner
 * metadata, so &controller->allocator is valid. Startup invalid/disabled ENV
 * retains its original disabled return0/reset behavior.
 * Explicit opt-in: existing PT24G_RECOVERY_DIR plus PT24G_RECOVERY_MEDIA set to
 * fixed or removable. This is caller/user classification, not hardware detection.
 * Unknown media stays disabled. Removable also needs PT24G_RECOVERY_REMOVABLE=1.
 * Missing interval defaults to300 seconds; PT24G_RECOVERY_SECONDS=0 disables, otherwise
 * accepts30..86400. Incomplete, empty, control-text or unreadable values refuse
 * configuration; invalid optional values never silently take defaults.
 * All storage uses caller's bounded allocator. */
int pt_native_recovery_configure(struct pt_native_recovery *,const struct pt_allocator *);
/* Task-context controller seam; callers serialize it with poll/bind/finish and
 * establish idle audio/capture/export state before a future requester/apply.
 * A zero/unconfigured owner reads as disabled, unknown media, empty directory,
 * interval300. Getter output must be disjoint from the entire controller; busy
 * or malformed state refuses without touching output. No allocation or I/O.
 * A copied draft can be cancelled simply by not calling apply. */
int pt_native_recovery_get_configuration(const struct pt_native_recovery *,
    struct pt_native_recovery_configuration *);
/* Configure initially through the existing startup API to establish its bounded
 * allocator, even when ENV leaves recovery disabled. Apply's input must be
 * disjoint from the controller. Invalid/refused apply leaves ALL owner bytes
 * unchanged. Opened/owned/busy storage or a busy schedule always refuses, even
 * for a no-op; no snapshot is discarded. Semantic no-op preserves backoff and
 * revision bookkeeping without DOS calls. Other valid changes reset ONLY the
 * schedule and configuration fields, preserving allocator, document identity,
 * bound state, project and store. No document bind or recovery prompt occurs.
 * Interval30..86400 and Boolean fields are required. Enabled settings require
 * known media, explicit removable permission and an existing canonical directory.
 * Disabled settings use enabled0/nonzero interval; unknown+empty is permitted.
 * Disabled directory text is bounded/validated but not locked; re-enable always
 * validates it. Configured disabled state can still discover snapshots read-only
 * when its nonempty directory and bound identity are present. No ENV writes. */
int pt_native_recovery_apply_configuration(struct pt_native_recovery *,
    const struct pt_native_recovery_configuration *);
/* Start a document identity from its canonical source; NULL/empty means unnamed.
 * Call only after committing a new/load/save-as document transition. The previous
 * session's exact owned snapshots are discarded; failure disables further writes
 * while retaining ownership for inspection. No old crash directory is adopted. */
int pt_native_recovery_bind(struct pt_native_recovery *,const char *source);
/* Caller serializes UI/document operations and supplies confirmed idle status. */
enum pt_recovery_tick_result pt_native_recovery_poll(struct pt_native_recovery *,
    const struct pt_project *,uint32_t revision,uint32_t saved_revision,int safe);
/* Bounded read-only scan of at most128 directory entries. Empty configuration
 * directories are never scanned. A larger directory
 * returns ERROR; never silently chooses a candidate from an incomplete scan. */
enum pt_recovery_find_result pt_native_recovery_find(struct pt_native_recovery *,struct pt_recovery_candidate *);
/* Normal confirmed close/discard; false leaves recovery copies for next launch. */
int pt_native_recovery_finish(struct pt_native_recovery *,int discard);
#endif
