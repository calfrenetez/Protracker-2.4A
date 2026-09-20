#ifndef PT_BOUNCE_H
#define PT_BOUNCE_H
#include "sampler.h"
#include "render.h"
/* Render a new assignable sample without intermediate files or copying the
 * rendered PCM. Project/options/PCM remain immutable until the single append
 * transaction commits. All staging is charged to the sampler's memory budget.
 * Report changes only on success; detail identifies render errors/cancellation.
 * The new slot keeps explicit output bits/rate, full volume and no loop/slices.
 * Progress accepts cancellation through final validation before publication.
 */
enum pt_edit_result pt_sampler_bounce(struct pt_sampler *,struct pt_project *,
    struct pt_pattern_history *,const struct pt_render_options *,const char *,
    pt_render_progress,void *,struct pt_render_report *,enum pt_render_result *);
#endif
