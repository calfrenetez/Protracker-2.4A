#ifndef PT_RECENT_FILE_H
#define PT_RECENT_FILE_H
#include "../core/recent.h"
#include "../core/document.h"
/* App-owned prefix; parent directory already exists. Two alternating slots
 * keep the previous valid list if a write is interrupted. Single writer only.
 * Load probes exactly two preferences files, never the listed project paths.
 * Failure preserves the caller's list. Save rereads and verifies its output. */
int pt_recent_file_load(const char *prefix,struct pt_recent *);
int pt_recent_file_save(const char *prefix,const struct pt_recent *);
/* Required allocator owns one workspace per call, released on every exit. */
int pt_recent_file_load_allocated(const char *,struct pt_recent *,const struct pt_allocator *);
int pt_recent_file_save_allocated(const char *,const struct pt_recent *,const struct pt_allocator *);
#endif
