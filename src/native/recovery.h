#ifndef PT_NATIVE_RECOVERY_H
#define PT_NATIVE_RECOVERY_H
#include "../core/recovery.h"
#include "../platform/recovery_find.h"
struct pt_native_recovery {
    struct pt_allocator allocator;
    struct pt_recovery_schedule schedule;
    struct pt_recovery_store store;
    struct pt_recovery_info info;
    const struct pt_project *project;
    char root[360];
    uint64_t source_timestamp;
    uint8_t configured,bound,removable;
};
/* Zero-initialize before first use; reconfiguration refuses an open store.
 * Explicit opt-in: existing PT24G_RECOVERY_DIR plus PT24G_RECOVERY_MEDIA set to
 * fixed or removable. This is caller/user classification, not hardware detection.
 * Unknown media stays disabled. Removable also needs PT24G_RECOVERY_REMOVABLE=1.
 * Missing interval defaults to300 seconds; PT24G_RECOVERY_SECONDS=0 disables, otherwise
 * accepts30..86400. Incomplete, empty, control-text or unreadable values refuse
 * configuration; invalid optional values never silently take defaults.
 * All storage uses caller's bounded allocator. */
int pt_native_recovery_configure(struct pt_native_recovery *,const struct pt_allocator *);
/* Start a document identity from its canonical source; NULL/empty means unnamed.
 * Call only after committing a new/load/save-as document transition. The previous
 * session's exact owned snapshots are discarded; failure disables further writes
 * while retaining ownership for inspection. No old crash directory is adopted. */
int pt_native_recovery_bind(struct pt_native_recovery *,const char *source);
/* Caller serializes UI/document operations and supplies confirmed idle status. */
enum pt_recovery_tick_result pt_native_recovery_poll(struct pt_native_recovery *,
    const struct pt_project *,uint32_t revision,uint32_t saved_revision,int safe);
/* Bounded read-only scan of at most128 directory entries. A larger directory
 * returns ERROR; never silently chooses a candidate from an incomplete scan. */
enum pt_recovery_find_result pt_native_recovery_find(struct pt_native_recovery *,struct pt_recovery_candidate *);
/* Normal confirmed close/discard; false leaves recovery copies for next launch. */
int pt_native_recovery_finish(struct pt_native_recovery *,int discard);
#endif
