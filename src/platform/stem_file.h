#ifndef PT_STEM_FILE_H
#define PT_STEM_FILE_H
#include "render_file.h"
#include "stems.h"
struct pt_stem_report {struct pt_stem_plan plan;struct pt_render_report audio[16];};
/* Publish an entirely new directory after every WAV has passed byte verification.
 * Existing destination is never replaced. Any failed/cancelled batch removes
 * only its own staging. All stems retain global flow, mute/solo, pan and gain.
 * Output changes only on success; detail carries render/cancellation errors. */
enum pt_render_file_result pt_stem_file_new(const char *,const struct pt_project *,
    const struct pt_render_options *,unsigned grouped,pt_render_progress,void *,
    struct pt_stem_report *,enum pt_render_result *detail);
/* Optional bounded allocator for render working state; NULL uses legacy stack. */
enum pt_render_file_result pt_stem_file_new_allocated(const char *,const struct pt_project *,
    const struct pt_render_options *,unsigned grouped,pt_render_progress,void *,
    struct pt_stem_report *,enum pt_render_result *detail,const struct pt_allocator *);
#endif
