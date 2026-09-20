#ifndef PT_RECENT_FILE_H
#define PT_RECENT_FILE_H
#include "../core/recent.h"
/* App-owned prefix; parent directory already exists. Two alternating slots
 * keep the previous valid list if a write is interrupted. Single writer only.
 * Load probes exactly two preferences files, never the listed project paths.
 * Failure preserves the caller's list. Save rereads and verifies its output. */
int pt_recent_file_load(const char *prefix,struct pt_recent *);
int pt_recent_file_save(const char *prefix,const struct pt_recent *);
#endif
