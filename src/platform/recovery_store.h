#ifndef PT_RECOVERY_STORE_H
#define PT_RECOVERY_STORE_H
#include "recovery_file.h"
#define PT_RECOVERY_DIRECTORY_MAX 480
struct pt_recovery_store {
    char directory[PT_RECOVERY_DIRECTORY_MAX+1];
    char path[2][PT_RECOVERY_DIRECTORY_MAX+16];
    uint64_t document_id,timestamp;
    uint8_t opened,owned,current,busy;
};
/* Zero-initialize before use. Creates an exclusively NEW app-owned directory in
 * an existing parent; refuses every existing destination, including empty dirs.
 * The caller must keep exclusive ownership of this directory and serialize all
 * access. Do not adopt an existing recovery directory with this write owner.
 * No directory scanning, source deletion or allocation occurs here. */
enum pt_save_result pt_recovery_store_create(struct pt_recovery_store *,const char *,uint64_t);
/* Alternates two NEW file destinations. Keep the current verified snapshot until
 * the next has completely published; only then remove the old owned file.
 * Failed old-file cleanup retains at most two snapshots and is retried before
 * another write. The current good file is never removed to make room. Timestamps
 * must increase, so discovery can choose correctly if both files survive.
 * PT_SAVE_OK means the new copy is verified, even if old cleanup remains pending.
 * Existing files that this owner did not publish are never removed. */
enum pt_save_result pt_recovery_store_save(struct pt_recovery_store *,
    const struct pt_project *,const struct pt_recovery_info *,const struct pt_allocator *);
/* Explicit discard of this session's owned files and then its empty directory.
 * Caller must establish the work was saved or recovery was explicitly discarded.
 * Failure retains ownership for inspection/retry. To keep recovery for a future
 * launch, simply release the in-memory owner without calling discard. */
int pt_recovery_store_discard(struct pt_recovery_store *);
#endif
