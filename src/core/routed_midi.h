#ifndef PT_ROUTED_MIDI_H
#define PT_ROUTED_MIDI_H

#include "elapsed_clock.h"
#include "midi.h"
#include "mixed_scheduled_readers.h"
#include "render_commands.h"

#define PT_ROUTED_MIDI_MESSAGES (2U * PT_MIDI_TRACKS)

struct pt_routed_midi_message {
    uint8_t endpoint;
    uint8_t bytes[3];
};

struct pt_routed_midi_batch {
    uint64_t frame, serial;
    unsigned stop, count, needs_audio;
    struct pt_routed_midi_message messages[PT_ROUTED_MIDI_MESSAGES];
};

struct pt_routed_midi_schedule;

/* Caller-owned task storage. Keep one instance per song and never copy it.
 * Endpoint identities are deduplicated in first-track order, then immutable.
 * A provider must resolve and retain those exact destinations before playing.
 * The planner opens no link, sends no bytes and reads no clock. */
struct pt_routed_midi {
    struct pt_midi preview;
    struct pt_midi_voice accepted[PT_MIDI_TRACKS];
    struct pt_routed_midi_batch batch;
    char endpoint[PT_MIDI_PORTS][PT_MIDI_ENDPOINT];
    uint64_t last_frame;
    unsigned endpoint_count, pending, started, stopped;
    struct pt_routed_midi_schedule *schedule;
};

/* Initialize fresh zeroed storage from the selected MIDI tracks of a project.
 * Project and storage must be disjoint. No project pointer is retained. */
int pt_routed_midi_init(struct pt_routed_midi *, const struct pt_project *, uint16_t tracks);

/* Prepare a batch against the last accepted notes. Empty nonterminal ticks
 * require no batch. A combined provider also prepares audio-only boundaries.
 * A terminal tick contains the remaining owned Note Offs.
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

enum pt_routed_midi_schedule_result {
    PT_ROUTED_MIDI_QUEUED, PT_ROUTED_MIDI_WAITING, PT_ROUTED_MIDI_SETTLED,
    PT_ROUTED_MIDI_REFUSED, PT_ROUTED_MIDI_INVALID, PT_ROUTED_MIDI_CLOCK,
    PT_ROUTED_MIDI_LATE, PT_ROUTED_MIDI_OUTSIDE_WINDOW, PT_ROUTED_MIDI_UNCERTAIN,
    PT_ROUTED_MIDI_PARTIAL
};

enum pt_routed_midi_delivery {
    PT_ROUTED_MIDI_PENDING, PT_ROUTED_MIDI_NOT_ISSUED, PT_ROUTED_MIDI_ISSUED, PT_ROUTED_MIDI_UNKNOWN
};

struct pt_routed_midi_key { uint64_t session, generation, frame, serial; };

struct pt_routed_midi_event {
    struct pt_routed_midi_key key;
    uint64_t first, last;
    const struct pt_routed_midi_batch *batch;
    const char (*endpoint)[PT_MIDI_ENDPOINT];
    unsigned endpoint_count;
};

struct pt_routed_audio_receipt {
    const struct pt_mixed_readers_output *queue;
    uint64_t ticket, first_issued, last_issued;
    unsigned count;
    enum pt_mixed_readers_command delivery;
};

struct pt_routed_midi_receipt {
    struct pt_routed_midi_key key;
    uint64_t activated;
    unsigned count;
    struct pt_routed_audio_receipt audio;
};

/* Serialized task-side provider. The clock and actual activation proof must
 * use the same counter domain as the audio scheduler. Submit returns 1 for a
 * whole batch queued before its window, 0 only if nothing was queued, or
 * another value if uncertain. ISSUED and NOT_ISSUED both prove complete
 * retirement of references.
 * ISSUED attests whole-batch activation at the returned tick. A provider that
 * cannot meet these conditions must refuse. Borrowed data stays live until
 * confirmed refusal or a valid retirement proof. No provider edits or reentry. */
struct pt_routed_midi_provider {
    void *context;
    size_t context_bytes;
    int (*read_clock)(void *, uint64_t *, uint32_t *);
    int (*submit)(void *, const struct pt_routed_midi_event *);
    enum pt_routed_midi_delivery (*observe)(void *, const struct pt_routed_midi_key *,
                                             struct pt_routed_midi_receipt *);
    /* Optional cancellation request for the exact queued batch. Return 1 when
     * requested, 0 if refused, or -1 if the request outcome is unknown.
     * The key is borrowed during the call only. Every result still requires
     * an independent terminal observe receipt. */
    int (*request_cancel)(void *, const struct pt_routed_midi_key *);
    /* Alternative to submit. One provider accepts both legs or neither.
     * Return values follow submit. Copy any needed audio command values in
     * the call. The actual audio owner keeps its readers and resolves their
     * quiet separately. Observe reports both legs under the original key,
     * attesting the total audio action count and the first and last issue
     * ticks. Non-issue reports zero actions and ticks. Return PENDING while
     * awaiting either proof. */
    int (*submit_combined)(void *, const struct pt_routed_midi_event *,
                          const struct pt_mixed_readers_event *);
};

struct pt_routed_midi_schedule {
    struct pt_routed_midi *midi;
    struct pt_routed_midi_provider provider;
    struct pt_elapsed_clock grid;
    struct pt_routed_midi_event event;
    const struct pt_mixed_readers_output *audio_queue;
    uint64_t audio_ticket, last_audio_ticket;
    uint64_t last_read;
    unsigned audio_count, busy, held, clock_seen, faulted, cancel_requested;
};

/* Bind a zeroed schedule before preparing the first batch. The session must
 * be fresh. Epoch is the future start of frame zero, using the same
 * epoch/frequency/rate as audio. Once
 * bound, only this schedule may settle the planner. Keep both objects alive
 * through confirmed retirement and release of all owned notes. */
int pt_routed_midi_schedule_init(struct pt_routed_midi_schedule *, struct pt_routed_midi *,
    const struct pt_routed_midi_provider *, uint64_t session, uint64_t generation,
    uint64_t epoch, uint32_t frequency, uint32_t rate);

/* A single pre-deadline submit. A late request or uncertain provider result
 * inhibits further notes. An unsubmitted batch can be discarded explicitly.
 * After uncertainty, only a matching terminal proof can settle the batch;
 * then only an owned Stop may be submitted. A valid out-of-window issue
 * receipt records the actual owned notes for Stop and returns OUTSIDE_WINDOW.
 * It never qualifies the timing. No immediate-send fallback. */
enum pt_routed_midi_schedule_result pt_routed_midi_schedule_submit(struct pt_routed_midi_schedule *);
/* Call from the audio owner's publication path with its original event.
 * Session, generation, frame and both counter bounds must match. The first
 * audio event binds its queue for the session. Tickets must advance. NULL is an
 * explicit MIDI-only batch for the same combined provider. With an audio
 * leg, both terminal receipts must agree on the original identity. Two issues
 * inside the same logical frame settle. One issue is PARTIAL and blocks new
 * notes while retaining any actual MIDI notes for Stop. A terminal Stop is
 * complete only when both legs are settled by the common provider. A MIDI
 * batch with zero messages follows the audio receipt when audio is present. */
enum pt_routed_midi_schedule_result pt_routed_midi_schedule_submit_combined(
    struct pt_routed_midi_schedule *, const struct pt_mixed_readers_event *);
enum pt_routed_midi_schedule_result pt_routed_midi_schedule_observe(struct pt_routed_midi_schedule *);
/* Begin draining a queued non-Stop batch before an owned Stop. Request
 * cancellation at most once and keep the batch borrowed through observe,
 * including if the request is refused or activation has already won. Without
 * a cancellation callback, wait for the original batch's terminal receipt. */
enum pt_routed_midi_schedule_result pt_routed_midi_schedule_request_cancel(struct pt_routed_midi_schedule *);
int pt_routed_midi_schedule_discard(struct pt_routed_midi_schedule *);

#endif
