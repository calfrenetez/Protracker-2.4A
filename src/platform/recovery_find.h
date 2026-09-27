#ifndef PT_RECOVERY_FIND_H
#define PT_RECOVERY_FIND_H
#include "recovery_store.h"
struct pt_recovery_candidate {
    char path[PT_RECOVERY_DIRECTORY_MAX+16];
    struct pt_recovery_info info;
};
enum pt_recovery_find_result {PT_RECOVERY_NONE,PT_RECOVERY_FOUND,PT_RECOVERY_FIND_ERROR};
/* Read-only inspection of exactly 0.ptg and 1.ptg in an explicit directory.
 * Fully validates one bounded project at a time. Only matching nonzero document
 * identity and exact canonical source path with timestamp > source_timestamp are
 * candidates. Source timestamp must use the same UTC epoch as snapshot metadata;
 * this comparison is a UI hint, not proof against external source edits.
 * Corrupt/missing/incompatible files are skipped. Capacity/allocation failure
 * returns ERROR rather than hiding a potentially newer unreadable candidate.
 * NONE/ERROR leave output unchanged. No recursive scan, adoption or deletion. */
enum pt_recovery_find_result pt_recovery_find(const char *directory,uint64_t id,
    const char *source,uint64_t source_timestamp,size_t file_limit,size_t memory_budget,
    const struct pt_allocator *,struct pt_recovery_candidate *);
/* After explicit user selection, re-read and validate the candidate into separate
 * storage; reject changed metadata before touching the open document. Success
 * restores full precision, removes recovery metadata and marks the result dirty.
 * Never writes the source or snapshot; failure preserves the current document. */
enum pt_project_result pt_recovery_restore_selected(struct pt_document *,
    const struct pt_recovery_candidate *,size_t file_limit,size_t memory_budget);
#endif
