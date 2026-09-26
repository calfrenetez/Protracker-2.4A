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
/* Bounded batch path/report/measurement storage plus nested WAV and render
 * workspace. Up to three allocations coexist; caller must bound total memory.
 * All released after cleanup on every return. NULL retains legacy stack path. */
enum pt_render_file_result pt_stem_file_new_allocated(const char *,const struct pt_project *,
    const struct pt_render_options *,unsigned grouped,pt_render_progress,void *,
    struct pt_stem_report *,enum pt_render_result *detail,const struct pt_allocator *);
/* Internal alternate render engine. Each planning/output/verification pass
 * starts independently; engines own no state across stems. Requires allocator. */
enum pt_render_file_result pt_stem_file_engine_new(const char *,const struct pt_project *,
    const struct pt_render_options *,unsigned grouped,pt_render_progress,void *,
    struct pt_stem_report *,enum pt_render_result *,const struct pt_allocator *,
    const struct pt_render_file_engine *);
#endif
