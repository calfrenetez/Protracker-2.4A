#ifndef PT_FILE_SAVE_H
#define PT_FILE_SAVE_H
#include "safe_save.h"
/* Verify and atomically publish a NEW file only. Never replaces a destination. */
enum pt_save_result pt_file_save_new(const char *,const void *,size_t);
/* Allocates file paths/state and verification buffer using the caller budget.
 * No stdio read-ahead buffer. Release follows owned-file cleanup on all paths.
 * NULL allocator retains legacy stack behavior. */
struct pt_allocator;
enum pt_save_result pt_file_save_new_allocated(const char *,const void *,size_t,const struct pt_allocator *);
#endif
