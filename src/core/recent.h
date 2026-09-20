#ifndef PT_RECENT_H
#define PT_RECENT_H
#include <stddef.h>
#include <stdint.h>
#define PT_RECENT_LIMIT 10
#define PT_RECENT_PATH 1024
#define PT_RECENT_BYTES (16+PT_RECENT_LIMIT*(PT_RECENT_PATH+1))
struct pt_recent {char path[PT_RECENT_LIMIT][PT_RECENT_PATH];uint8_t count;};
enum pt_recent_result {PT_RECENT_OK,PT_RECENT_INVALID,PT_RECENT_CAPACITY,PT_RECENT_CORRUPT};
void pt_recent_init(struct pt_recent *);
/* Call only after successful open/save, with a resolved path. ASCII case-folded
 * equality follows Amiga filenames; spelling of the newest success is retained.
 * No filesystem probes. Failed operations preserve state; path may alias list. */
enum pt_recent_result pt_recent_remember(struct pt_recent *,const char *);
enum pt_recent_result pt_recent_remove(struct pt_recent *,unsigned);
/* Versioned big-endian lengths and CRC32. Encode output must not alias list;
 * no writes to output/written until validation/capacity checks pass. Decode
 * stages the complete list and replaces state only after all checks pass. */
enum pt_recent_result pt_recent_encode(const struct pt_recent *,void *,size_t,size_t *written);
enum pt_recent_result pt_recent_decode(struct pt_recent *,const void *,size_t);
#endif
