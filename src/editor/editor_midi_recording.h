#ifndef PT_EDITOR_MIDI_RECORDING_H
#define PT_EDITOR_MIDI_RECORDING_H
#include "editor.h"
#include "../core/record_channel.h"

struct pt_editor_midi_recording;
enum pt_editor_midi_record_result {
    PT_EDITOR_RECORD_OK, PT_EDITOR_RECORD_IGNORED, PT_EDITOR_RECORD_INVALID,
    PT_EDITOR_RECORD_TIME, PT_EDITOR_RECORD_CAPACITY, PT_EDITOR_RECORD_COLLISION,
    PT_EDITOR_RECORD_STORE_FAILED, PT_EDITOR_RECORD_CONFLICT,
    PT_EDITOR_RECORD_BUSY, PT_EDITOR_RECORD_HELD, PT_EDITOR_RECORD_ALIAS
};
struct pt_editor_midi_record_status {
    unsigned attached,armed,started,active,target_track,input_channel;
    uint64_t origin,last_position;
};

/* One allocation-free, stopped, serialized editor take. This is not an input
 * source, clock, output/audition owner or native frontend enrollment. The caller
 * supplies authoritative Q16 sequencer positions and complete channel messages.
 * The original editor/project/history/allocator contexts and separate owner
 * storage remain live and stationary until matched detach succeeds. No copying,
 * direct recorder/history/source mutation, reinitialization or returned stack
 * lifetime is permitted. Reentry refuses BUSY; queries/hooks do no Stop/cancel.
 *
 * begin needs aligned zero storage of at least size() bytes. Only size() bytes
 * are scanned/initialized; the full capacity is protected as a numeric span and
 * the tail is preserved. It must be disjoint from known editor/project/master/
 * history storage, settings and the complete ORIGINAL sampler allocator context
 * whose actual extent is supplied. Other opaque callback contexts and transitive
 * allocations remain a caller disjointness/lifetime obligation, not a heap census.
 * Any nonzero owned prefix, after fixed span/alias admission, refuses BUSY.
 *
 * A take fixes one MIDI-routed track and input filter1..16. It exclusively uses
 * the existing empty change barrier and recording query. Existing callbacks OR
 * context-only slots refuse unchanged; audio capture is never overridden. Armed
 * Hold, failed Stop and the stopped-but-not-detached interval all veto generic
 * editor change/dispose and report recording-busy. Only this take's own genuine
 * recorder/store writes its normal per-event journal records. There is no barrier
 * bypass API or callback action. Initial begin requires idle workflow state;
 * later ordinary read-only usage scans/navigation and deferred Apply intent may
 * coexist. They do not poison Stop/detach or get erased by this adapter. Existing
 * workflow active()/prepare_change gates still prevent actual publication while
 * enrolled; genuine recorded revisions stale an older preview. Unexpected real
 * workflow busy/transaction, listed source/journal identity or hook drift is
 * retained HELD; this is not a census of all UI flags.
 * Other direct APIs (notably source_load_with)
 * which do not consult prepare_change remain forbidden to the caller while
 * enrolled. The unchanged editor refusal text may mention playback.
 *
 * stop delegates to genuine pt_record_stop; failures retain armed obligations.
 * cancel explicitly drops LOCAL recorder ownership, never external MIDI/audio
 * or audition ownership, and does not undo accepted notes. Neither detaches.
 * detach refuses armed/active/unfinished state and clears only the two matched
 * original callback/context slots. Drift latches HELD, retains the whole original
 * domain, and permits no forced cleanup/retry/reset. After detach get reads only
 * this owner's remaining local state; it does not touch former editor sources.
 * All aliases must end before caller zero/reuse of successfully detached storage;
 * pointer/self checks do not authenticate a later same-address lifetime.
 */
size_t pt_editor_midi_recording_size(void);
size_t pt_editor_midi_recording_alignment(void);
enum pt_editor_midi_record_result pt_editor_midi_recording_begin(
    struct pt_editor_midi_recording *,size_t capacity,struct pt_editor *,
    size_t allocator_context_bytes,const struct pt_record_settings *,
    uint64_t position,unsigned target_track,unsigned input_channel);
enum pt_editor_midi_record_result pt_editor_midi_recording_message(
    struct pt_editor_midi_recording *,uint64_t,const uint8_t *,size_t);
enum pt_editor_midi_record_result pt_editor_midi_recording_stop(
    struct pt_editor_midi_recording *,uint64_t);
enum pt_editor_midi_record_result pt_editor_midi_recording_cancel(
    struct pt_editor_midi_recording *);
enum pt_editor_midi_record_result pt_editor_midi_recording_detach(
    struct pt_editor_midi_recording *);
enum pt_editor_midi_record_result pt_editor_midi_recording_get(
    struct pt_editor_midi_recording *,struct pt_editor_midi_record_status *);
#endif
