#ifndef PT_RECOVERY_FILE_H
#define PT_RECOVERY_FILE_H
#include "project_file.h"
#include "project_import.h"
#define PT_RECOVERY_SOURCE_MAX 255
#define PT_RECOVERY_EXTENSION 0x52435652UL /* RCVR, optional project chunk */
struct pt_recovery_info {
    uint64_t document_id,revision,saved_revision,timestamp;
    char source[PT_RECOVERY_SOURCE_MAX+1]; /* Empty for an unnamed document. */
};
/* Explicit NEW recovery destination, never an overwrite. Serial synchronous
 * master snapshot: project/metadata stay immutable through return. A small
 * allocator-owned extension table wraps the existing streamed project saver;
 * PCM is neither copied into a second whole-file buffer nor reduced in precision.
 * document_id is a nonzero caller-assigned document identity; revision must differ from
 * saved_revision (undo may make it numerically smaller). timestamp is a nonzero caller-supplied UTC time.
 * These metadata values identify a candidate, not proof the original is unchanged.
 * Caller chooses location and scheduling; no automatic writes are enabled here.
 * Existing RCVR extensions and exhausted extension capacity are refused. */
enum pt_save_result pt_recovery_file_save(const char *,const struct pt_project *,
    const struct pt_recovery_info *,const struct pt_allocator *);
/* Inspect a completely validated project. Exactly one version1 RCVR chunk is
 * required; malformed/duplicate/unknown-version records leave output unchanged. */
int pt_recovery_project_info(const struct pt_project *,struct pt_recovery_info *);
/* Explicit recovery into a staged document with matching nonzero identity.
 * Failure preserves document and info; success strips only the RCVR extension,
 * retains all original project/master data, and marks the recovered document
 * dirty. Never writes to or deletes the original or the recovery file. The UI
 * must separately offer recovery, check source freshness and obtain selection. */
enum pt_project_result pt_recovery_file_load(struct pt_document *,const char *,
    uint64_t expected_document,size_t file_limit,size_t memory_budget,
    struct pt_recovery_info *);
#endif
