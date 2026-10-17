#ifndef PT_RECORD_CHANNEL_H
#define PT_RECORD_CHANNEL_H
#include "record.h"

/* One complete MIDI 1.0 CHANNEL message, status included: 80..EF only;
 * Cn/Dn have two bytes, all other channel messages have three. No serial
 * framing, running status, System/SysEx, timestamps or CAMD resource operation.
 * Input channel is explicitly 1..16; target track explicitly 0..15. This filter
 * is independent of the project's MIDI OUTPUT channel. No omni/track allocation.
 *
 * Original recorder, store/context/project/history and message storage are live
 * for this serialized synchronous call. The complete 2/3-byte message is copied
 * to scalars before any recorder/store mutation; no message pointer is retained.
 * Position is the caller's authoritative monotonic sequencer Q16 row position;
 * this adapter neither reads a clock nor converts CAMD timestamps/latency.
 *
 * INVALID adapter input and valid filtered/non-note channel messages leave the
 * recorder/store untouched. Valid selected Note On/Off delegates directly to
 * pt_record_input: zero-velocity Note On is genuine Off, release velocity is not
 * recorded. Delegated orphan releases/failures retain the core's original
 * last_position and retry semantics; this is not transaction rollback.
 *
 * Does not arm, force-start, stop, cancel or reinitialize recording, or send MIDI.
 * Hold Record
 * starts only after the genuine store accepts a playable Note On. Caller routes
 * matching releases to the recorder track owning that key; changing per-call
 * track/filter does not release an old key or prove external-device silence.
 * The store still enforces actual project route/count/history constraints.
 */
enum pt_record_result pt_record_channel_message(struct pt_recorder *,uint64_t,
    unsigned target_track,unsigned input_channel,const uint8_t *,size_t);
#endif
