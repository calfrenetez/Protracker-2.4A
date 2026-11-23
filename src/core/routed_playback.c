#include <string.h>
#include "routed_playback.h"

static int apart(const void *a, size_t na, const void *b, size_t nb)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return a && b && na <= UINTPTR_MAX - x && nb <= UINTPTR_MAX - y &&
        (x + na <= y || y + nb <= x);
}

enum pt_render_result pt_routed_playback_prepare(struct pt_routed_playback_frame *frame,
    struct pt_render_lookahead *lookahead, struct pt_routed_midi_schedule *schedule,
    struct pt_routed_render_plan *plan)
{
    const struct pt_routed_midi_batch *batch = NULL;
    struct pt_routed_midi *midi;
    enum pt_render_result result;
    unsigned ready = 0, i;
    if (!frame || !lookahead || !schedule || !plan || !lookahead->ready || !lookahead->sequence ||
        !schedule->provider.submit_combined || schedule->held || schedule->busy ||
        !(midi = schedule->midi) || midi->schedule != schedule || midi->pending || midi->stopped ||
        !apart(frame, sizeof(*frame), lookahead, sizeof(*lookahead)) ||
        !apart(frame, sizeof(*frame), schedule, sizeof(*schedule)) ||
        !apart(frame, sizeof(*frame), midi, sizeof(*midi)) ||
        !apart(frame, sizeof(*frame), plan, sizeof(*plan)) ||
        !apart(lookahead, sizeof(*lookahead), schedule, sizeof(*schedule)) ||
        !apart(lookahead, sizeof(*lookahead), midi, sizeof(*midi)) ||
        !apart(plan, sizeof(*plan), schedule, sizeof(*schedule)) ||
        !apart(plan, sizeof(*plan), midi, sizeof(*midi)) ||
        !pt_render_sequence_output_disjoint(lookahead->sequence, frame, sizeof(*frame)) ||
        !pt_render_sequence_output_disjoint(lookahead->sequence, schedule, sizeof(*schedule)) ||
        !pt_render_sequence_output_disjoint(lookahead->sequence, midi, sizeof(*midi)) || frame->owner)
        return PT_RENDER_INVALID;
    if (schedule->provider.context &&
        (!apart(frame, sizeof(*frame), schedule->provider.context, schedule->provider.context_bytes) ||
         !apart(plan, sizeof(*plan), schedule->provider.context, schedule->provider.context_bytes)))
        return PT_RENDER_INVALID;
    result = pt_render_lookahead_step_routed(lookahead, 1, plan, &ready);
    if (result != PT_RENDER_OK)
        return result;
    if (!ready || (schedule->faulted && !plan->end))
        return PT_RENDER_INVALID;
    if (plan->audio.count || plan->midi_count || plan->end) {
        batch = pt_routed_midi_prepare(midi, plan);
        if (!batch)
            return PT_RENDER_INVALID;
        if (plan->end)
            for (i = 0; i < PT_CHANNEL_LIMIT; ++i)
                if (lookahead->commands.voice[i].active)
                    midi->batch.needs_audio = 1;
    }
    *frame = (struct pt_routed_playback_frame){frame, lookahead->sequence, lookahead, schedule,
                                              lookahead->interval, plan->frame, batch ? batch->serial : 0};
    return PT_RENDER_OK;
}

enum pt_render_result pt_routed_playback_commit(struct pt_routed_playback_frame *frame)
{
    enum pt_render_result result;
    if (!frame || frame->owner != frame || !frame->lookahead || frame->lookahead->sequence != frame->sequence ||
        frame->lookahead->interval != frame->interval ||
        (frame->serial && !pt_routed_midi_schedule_settled(frame->schedule, frame->frame, frame->serial)))
        return PT_RENDER_INVALID;
    result = pt_render_lookahead_commit(frame->lookahead);
    if (result == PT_RENDER_OK)
        memset(frame, 0, sizeof(*frame));
    return result;
}

int pt_routed_playback_abandon(struct pt_routed_playback_frame *frame)
{
    if (!frame || (frame->owner && frame->owner != frame))
        return 0;
    memset(frame, 0, sizeof(*frame));
    return 1;
}
