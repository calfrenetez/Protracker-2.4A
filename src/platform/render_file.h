#ifndef PT_RENDER_FILE_H
#define PT_RENDER_FILE_H
#include "render.h"
enum pt_render_file_result { PT_RENDER_FILE_OK, PT_RENDER_FILE_INVALID,
    PT_RENDER_FILE_RENDER, PT_RENDER_FILE_BEGIN, PT_RENDER_FILE_WRITE,
    PT_RENDER_FILE_FINISH, PT_RENDER_FILE_VERIFY, PT_RENDER_FILE_PUBLISH };
/* New-file WAV publication only. Bounded streaming, no full output allocation.
 * Re-render and compare every staged byte before atomic no-replace publication.
 * Verification uses fixed-size reads with no stdio read-ahead buffer; short
 * reads/EINTR are retried, premature EOF/read errors/trailing bytes fail.
 * Project/options stay immutable during all passes. Up to signed32 file size.
 * Failure removes only owned staging. Report changes only on OK; detail gives
 * the render result (including cancellation) independently of filesystem phase.
 * Progress VERIFY reports bytes already compared as PCM frames (monotonic),
 * and also runs before opening the staged file for verification. */
enum pt_render_file_result pt_render_file_new(const char *,const struct pt_project *,
    const struct pt_render_options *,pt_render_progress,void *,struct pt_render_report *,enum pt_render_result *detail);
/* Optional bounded allocator for render working state; NULL uses legacy stack. */
enum pt_render_file_result pt_render_file_new_allocated(const char *,const struct pt_project *,
    const struct pt_render_options *,pt_render_progress,void *,struct pt_render_report *,enum pt_render_result *detail,const struct pt_allocator *);
#endif
