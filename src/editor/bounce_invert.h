#ifndef PT_BOUNCE_INVERT_H
#define PT_BOUNCE_INVERT_H
#include "bounce.h"
/* Explicit bounded offline EFx bounce. All source masters and history remain
 * unchanged until the generated sample commits; undo removes only the new slot.
 * Private sample budget is separate from sampler output/history accounting;
 * both use the sampler allocator. Selection retains all shared EFx clocks. */
enum pt_edit_result pt_sampler_bounce_invert(struct pt_sampler *,struct pt_project *,
    struct pt_pattern_history *,const struct pt_render_options *,const char *,
    pt_render_progress,void *,struct pt_render_report *,enum pt_render_result *,size_t sample_budget);
#endif
