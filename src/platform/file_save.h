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
/* Synchronous generated content, replayed for verification. read must fill exactly
 * n bytes at offset and return1; it may be called twice for each range. Source
 * stays immutable and alive through return. Fixed workspace, mandatory allocator.
 * Callback failure cleans only staging. No replacement of existing destination. */
typedef int (*pt_file_generate)(void *,size_t,void *,size_t);
enum pt_save_result pt_file_save_generated(const char *,size_t,pt_file_generate,void *,const struct pt_allocator *);
#endif
