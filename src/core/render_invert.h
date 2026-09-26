#ifndef PT_RENDER_INVERT_H
#define PT_RENDER_INVERT_H
#include "render.h"
/* Explicit offline classic EFx path. sample_budget bounds all additional private
 * sample descriptors and PCM; ordinary renderer workspace uses the same caller
 * allocator separately. All channels retain shared mutation clocks regardless of
 * audio selection/mute/solo. Requires whole mono8 one-shots or forward loops
 * (at least four loop frames), and no interpolation/slices. Private one-shots
 * start with a cleared first word and retain the classic two-frame DMA repeat,
 * even when EFx makes it audible; original master bytes are preserved.
 * Unknown/unsupported inputs fail before sink calls. Masters are never edited.
 * This does not enable EFx in the queued Studio/hardware sequence API. */
enum pt_render_result pt_render_invert_stream(const struct pt_project *,const struct pt_render_options *,
    pt_render_sink,void *,pt_render_progress,void *,struct pt_render_report *,
    size_t sample_budget,const struct pt_allocator *);
#endif
