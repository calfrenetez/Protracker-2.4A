#ifndef PT_EDITOR_MIXED_CHECKED_H
#define PT_EDITOR_MIXED_CHECKED_H
#include "editor_mixed.h"
#include "mixed_owner_established.h"
/* Optional actual-editor lifetime for an already-established master set. Genuine
 * separate zero-init checked control; never embed in binding/editor/contexts or
 * copy/edit while adopted. Finish is adopted before first allocation and remains
 * installed on admitted failures. Ordinary binding prepare/start/service/stop
 * are the only owner/pump calls while adopted. Both engines must belong to THIS
 * editor's sampler/project; stop pre-borrow establishment first, then bind engines.
 * Whole binding (including publisher), editor and checked control are protected
 * before claiming any child. Optional single composite ordinary context extent
 * names allocator/device/pump/timer contexts; otherwise caller keeps all unlisted
 * opaque extents independently disjoint. All input controls/named spans stay
 * alive through confirmed stop. No master promotion during this source borrow.
 * Stop first confirms transport alarm/readers/counter closure, then owner close,
 * THEN zero-child control finish. Every refusal retains the hook/contexts and
 * vetoes actual edit/dispose. Completed masters remain sampler-owned.
 * The existing public established constructor keeps its original alias rules.
 * Work1..4096 is an item bound, not deadline/stack/placement certification.
 * No UI/native PLAY wiring, hardware calls, schedule/jitter allowance or audio
 * acceptance installed by this helper. Legacy synchronous bridge bind is separate. */
enum pt_mixed_owner_result pt_editor_mixed_checked_begin(struct pt_editor_mixed *,
    struct pt_mixed_established *,struct pt_paula_voices *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    struct pt_sampler_storage_span contexts,unsigned work);
#endif
