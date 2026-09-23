#ifndef PT_FILE_LOAD_H
#define PT_FILE_LOAD_H
#include "document.h"
enum pt_load_result {PT_LOAD_OK,PT_LOAD_INVALID,PT_LOAD_IO,PT_LOAD_LIMIT,PT_LOAD_MEMORY};
/* Seekable files only. On failure outputs are unchanged. On success the caller
 * owns bytes through allocator, including a one-byte allocation for empty files.
 * Detects size changes during reading, not same-size concurrent edits. */
enum pt_load_result pt_file_load(const char *path,size_t limit,
    const struct pt_allocator *allocator,uint8_t **bytes,size_t *size);
#endif
