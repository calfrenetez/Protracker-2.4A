#ifndef PT_RENDER_INVERT_FILE_H
#define PT_RENDER_INVERT_FILE_H
#include "stem_file.h"
/* Explicit bounded EFx whole-song/pattern WAV conversion. Uses the strict
 * render_invert subset and repeats rendering from fresh private samples for
 * measurement, output and byte-for-byte verification before no-replace publish. */
enum pt_render_file_result pt_render_invert_file_new(const char *,const struct pt_project *,
    const struct pt_render_options *,pt_render_progress,void *,struct pt_render_report *,
    enum pt_render_result *,size_t sample_budget,const struct pt_allocator *);
/* Selected/grouped stems retain all channels' shared EFx mutations. Each stem
 * begins from master PCM; the budget is per pass, never multiplied by stems. */
enum pt_render_file_result pt_render_invert_stems_new(const char *,const struct pt_project *,
    const struct pt_render_options *,unsigned grouped,pt_render_progress,void *,
    struct pt_stem_report *,enum pt_render_result *,size_t sample_budget,const struct pt_allocator *);
#endif
