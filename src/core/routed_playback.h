#ifndef PT_ROUTED_PLAYBACK_H
#define PT_ROUTED_PLAYBACK_H

#include "render_lookahead.h"
#include "routed_midi.h"

/* Task-owned association between one live lookahead and its combined batch.
 * Start zeroed. Retain the original lookahead, sequence, schedule and MIDI
 * owner until this association has been committed or abandoned. */
struct pt_routed_playback_frame {
    const struct pt_routed_playback_frame *owner;
    struct pt_render_sequence *sequence;
    struct pt_render_lookahead *lookahead;
    struct pt_routed_midi_schedule *schedule;
    uint64_t interval, frame, serial;
};

/* Snapshot an already-ready forecast into disjoint caller scratch and prepare
 * its MIDI batch. A genuinely idle forecast needs no output submission. The
 * caller normalizes and prepares audio, then uses its original publication
 * callback to submit the combined batch. An end with active audio voices
 * requires a combined audio Stop. No clock or output calls here. */
enum pt_render_result pt_routed_playback_prepare(struct pt_routed_playback_frame *,
    struct pt_render_lookahead *, struct pt_routed_midi_schedule *, struct pt_routed_render_plan *);

/* Advance the live lookahead only after its source interval was consumed and
 * its exact combined batch has settled. Early, partial, stale or late proof
 * preserves the original binding and forecast. */
enum pt_render_result pt_routed_playback_commit(struct pt_routed_playback_frame *);

/* Discard this local association. The caller still owns the lookahead and all
 * preparations, cancellation, retirement and Stop for any scheduled output. */
int pt_routed_playback_abandon(struct pt_routed_playback_frame *);

#endif
