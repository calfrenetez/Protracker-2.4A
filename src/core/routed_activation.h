#ifndef PT_ROUTED_ACTIVATION_H
#define PT_ROUTED_ACTIVATION_H

#include "mixed_readers_activation.h"
#include "routed_midi.h"

/* Delegate the genuine activation port's task publication to its routed
 * schedule. The combined provider copies the prepared audio packet and the
 * complete MIDI event before returning. There is no separate audio publish.
 * The provider's clock must observe the same original counter without
 * reentering this activation owner. Uncertainty retains both domains. */
static inline int pt_routed_activation_publish(struct pt_routed_midi_schedule *schedule,
    const struct pt_mixed_readers_activation *owner, const struct pt_mixed_activation_packet *packet)
{
    const struct pt_mixed_readers_event *event = pt_mixed_activation_publication_event(owner, packet);
    if (!event)
        return 0;
    const enum pt_routed_midi_schedule_result result = pt_routed_midi_schedule_submit_combined(schedule, event);
    if (result == PT_ROUTED_MIDI_QUEUED)
        return 1;
    return result == PT_ROUTED_MIDI_UNCERTAIN ? -1 : 0;
}

#endif
