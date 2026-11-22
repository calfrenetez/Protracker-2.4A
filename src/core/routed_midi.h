#ifndef PT_ROUTED_MIDI_H
#define PT_ROUTED_MIDI_H

#include "midi.h"
#include "render_commands.h"

#define PT_ROUTED_MIDI_MESSAGES (2U * PT_MIDI_TRACKS)

struct pt_routed_midi_message {
    uint8_t endpoint;
    uint8_t bytes[3];
};

struct pt_routed_midi_batch {
    uint64_t frame, serial;
    unsigned stop, count;
    struct pt_routed_midi_message messages[PT_ROUTED_MIDI_MESSAGES];
};

/* Caller-owned task storage. Keep one instance per song and never copy it.
 * Endpoint identities are deduplicated in first-track order, then immutable.
 * A provider must resolve and retain those exact destinations before playing.
 * None of these functions opens a link, sends bytes or reads a clock. */
struct pt_routed_midi {
    struct pt_midi preview;
    struct pt_midi_voice accepted[PT_MIDI_TRACKS];
    struct pt_routed_midi_batch batch;
    char endpoint[PT_MIDI_PORTS][PT_MIDI_ENDPOINT];
    uint64_t last_frame;
    unsigned endpoint_count, pending, started, stopped;
};

/* Initialize fresh zeroed storage from the selected MIDI tracks of a project.
 * Project and storage must be disjoint. No project pointer is retained. */
int pt_routed_midi_init(struct pt_routed_midi *, const struct pt_project *, uint16_t tracks);

/* Prepare a batch against the last accepted notes. Empty nonterminal ticks
 * require no batch; a terminal tick contains the remaining owned Note Offs.
 * Return NULL on refusal, including if another batch awaits settlement.
 * The returned pointer is borrowed until commit or cancel. */
const struct pt_routed_midi_batch *pt_routed_midi_prepare(struct pt_routed_midi *,
    const struct pt_routed_render_plan *);

/* Explicit Stop may follow a cancelled future batch. Use the actual Stop
 * frame, which may equal the frame of the last accepted batch. */
const struct pt_routed_midi_batch *pt_routed_midi_prepare_stop(struct pt_routed_midi *, uint64_t frame);

/* Commit only after the common output provider confirms acceptance of the
 * entire batch for its exact frame and preparation serial. Cancel only with
 * proof that no message was accepted. Uncertain or partially issued output
 * must be held for provider recovery before using this state again. */
int pt_routed_midi_commit(struct pt_routed_midi *, uint64_t frame, uint64_t serial);
void pt_routed_midi_cancel(struct pt_routed_midi *);

#endif
