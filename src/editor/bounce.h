#ifndef PT_BOUNCE_H
#define PT_BOUNCE_H
#include "sampler.h"
#include "render.h"
/* Render a new assignable sample without intermediate files or copying the
 * rendered PCM. Project/options/PCM remain immutable until the single append
 * transaction commits. Output/history staging is charged to the sampler budget.
 * Measurement/mixer workspace uses its allocator and is released before commit;
 * the caller allocator must bound total memory, including temporary workspace.
 * Workspace refusal reports MEMORY/CAPACITY without changing samples or redo.
 * Report changes only on success; detail identifies render errors/cancellation.
 * The new slot keeps explicit output bits/rate, full volume and no loop/slices.
 * Progress accepts cancellation through final validation before publication.
 */
enum pt_edit_result pt_sampler_bounce(struct pt_sampler *,struct pt_project *,
    struct pt_pattern_history *,const struct pt_render_options *,const char *,
    pt_render_progress,void *,struct pt_render_report *,enum pt_render_result *);
#endif
