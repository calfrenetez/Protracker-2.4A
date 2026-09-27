#ifndef PT_RENDER_LOOKAHEAD_H
#define PT_RENDER_LOOKAHEAD_H
#include "render_commands.h"
/* Caller-owned Fast workspace, initially zeroed. The sequence and immutable
 * project/source storage must outlive cancellation/commit; this does not pin
 * masters. Owner-thread only, no caller mutation/copies while active. Begin
 * snapshots the CURRENT pending interval's command state, including any already
 * consumed phase. Step advances only this private copy by <=256 frames, or
 * resolves one bounded command plan on a separate call. No allocation, PCM value
 * scans, device callbacks or changes to the live sequence. Mutable EFx refuses.
 *
 * Live consume may proceed concurrently in serial owner calls. Next/ordinary
 * complete/rewind invalidate old interval jobs; a stale job never commits.
 * Ready plans authorize cache preparation only, NOT an early voice start.
 * Commit requires the SAME live interval to have consumed all its frames and
 * the job to be ready, then transfers computed command state exactly once.
 * Caller remains responsible for real elapsed time, cache/voice ownership and
 * dispatching the plan at its intended deadline. No scheduler/timing guarantee.
 * Cancel is idempotent, does not dereference the owner and changes no live state.
 * On refusal/pending, output plan is unchanged; ready changes only on success.
 */
struct pt_render_lookahead {
    struct pt_render_sequence *sequence;uint64_t interval;
    struct pt_render_command_state commands;struct pt_render_plan plan;
    uint32_t remaining;unsigned ready,failed;
};
enum pt_render_result pt_render_lookahead_begin(struct pt_render_lookahead *,struct pt_render_sequence *);
enum pt_render_result pt_render_lookahead_step(struct pt_render_lookahead *,unsigned frames,struct pt_render_plan *,unsigned *ready);
enum pt_render_result pt_render_lookahead_commit(struct pt_render_lookahead *);
void pt_render_lookahead_cancel(struct pt_render_lookahead *);
#endif
