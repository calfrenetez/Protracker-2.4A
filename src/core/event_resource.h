#ifndef PT_EVENT_RESOURCE_H
#define PT_EVENT_RESOURCE_H
#include "pitch.h"

/* Navigation selects the event instrument, not a currently sounding voice.
 * This is task-side offline work: no allocator, PCM conversion, sink, MIDI send,
 * hardware access or transport action. Never call begin/step in an interrupt.
 */
enum pt_event_resource_state {
    PT_EVENT_RESOURCE_EXPLICIT, PT_EVENT_RESOURCE_RESOLVED_INHERITED,
    PT_EVENT_RESOURCE_AMBIGUOUS, PT_EVENT_RESOURCE_UNRESOLVED,
    PT_EVENT_RESOURCE_NO_RESOURCE
};
enum pt_event_resource_destination {
    PT_EVENT_RESOURCE_NONE, PT_EVENT_RESOURCE_AUDIO_MASTER,
    PT_EVENT_RESOURCE_MIDI_ROUTING
};
enum pt_event_resource_reason {
    PT_EVENT_RESOURCE_DIRECT, PT_EVENT_RESOURCE_EMPTY_EVENT,
    PT_EVENT_RESOURCE_DETACHED_PATTERN, PT_EVENT_RESOURCE_MULTIPLE_SELECTIONS,
    PT_EVENT_RESOURCE_COMPLETE_HISTORY, PT_EVENT_RESOURCE_NO_CARRY,
    PT_EVENT_RESOURCE_UNREACHABLE, PT_EVENT_RESOURCE_TICK_BUDGET
};
enum pt_event_resource_status {
    PT_EVENT_RESOURCE_OK, PT_EVENT_RESOURCE_INVALID, PT_EVENT_RESOURCE_STALE
};
struct pt_event_resource_origin {
    unsigned pattern,row,track,order,order_known,start_order;
    enum pt_flow_mode flow_mode;
};
struct pt_event_resource_result {
    enum pt_event_resource_state state;
    enum pt_event_resource_destination destination;
    enum pt_event_resource_reason reason;
    struct pt_event_resource_origin origin,source;
    unsigned instrument,route,midi_channel,source_unique;
    uint32_t ticks,visits;
};
/* Public caller-owned workspace: zero-initialize once; reset it before project
 * replacement/disposal. Borrowed project/storage must remain alive and immutable
 * until reset or the next begin. Do not alter workspace fields directly.
 * revision must change on every note/instrument/order/effect/routing edit/undo;
 * generation must change on sample metadata/storage edits and project identity.
 * Normal channel cursor selection alone is permitted. */
struct pt_event_resource_job {
    const struct pt_project *project;
    struct pt_project snapshot;
    struct pt_flow initial,flow,checkpoint;
    struct pt_pitch pitch;
    struct pt_event_resource_result result;
    struct pt_event_resource_origin last_source;
    uint64_t power,length;
    uint32_t revision,generation,limit;
    unsigned initialized,validated,ready,seen,selection,checkpoint_instrument;
};
/* Explicit instruments (including empty slots, instrument-only/portamento rows)
 * are immediate, as are empty/detached events. Begin scans metadata spans only.
 * Zero-instrument events with a known matching order context use the exact
 * engine flow+pitch carry from a reset start_order in the explicit flow_mode. Occurrence is not guessed:
 * all reachable visits are examined until F00 or an exact flow/selection cycle.
 * Conflicting carry is AMBIGUOUS; an unproved history at the limit is UNRESOLVED.
 * The MIDI destination is existing track routing controls: instrument is event
 * selection metadata, NEVER a MIDI program or an audio sample destination.
 * No allocation. INVALID preserves workspace. Inputs/outputs must be disjoint
 * from source metadata and all declared master capacities; overflow refuses.
 */
enum pt_event_resource_status pt_event_resource_begin(struct pt_event_resource_job *,
    const struct pt_project *,const struct pt_event_resource_origin *,
    uint32_t revision,uint32_t generation,uint32_t tick_limit);
/* <=256 completed engine ticks per call. First preparation after a changed
 * version uses audited pt_flow_init: its full project/PCM validation remains
 * synchronous (NOT a wall-clock bound). A same-version reset state is cached,
 * so repeated navigation does not rescan PCM. ready=1 only for a final result.
 * Wrong version/header returns STALE before walking former tables; INVALID and
 * STALE preserve caller ready. Invalid step size does not advance the job. */
enum pt_event_resource_status pt_event_resource_step(struct pt_event_resource_job *,
    uint32_t revision,uint32_t generation,unsigned ticks,unsigned *ready);
/* Refuses pending/stale/aliased outputs without changing them. */
enum pt_event_resource_status pt_event_resource_get(const struct pt_event_resource_job *,
    uint32_t revision,uint32_t generation,struct pt_event_resource_result *);
#endif
