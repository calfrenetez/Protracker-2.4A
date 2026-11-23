#ifndef PT_ROUTED_FRAME_PROGRAM_H
#define PT_ROUTED_FRAME_PROGRAM_H

#include "routed_activation.h"

#define PT_ROUTED_CARD_PRELOAD_WRITES (PT_CHANNEL_LIMIT * (PT_AMIGUS_VOICE_REGISTER_WRITES - 2U))
#define PT_ROUTED_CARD_FRAME_WRITES (PT_CHANNEL_LIMIT * 5U)

struct pt_routed_frame_midi { uint8_t slot, bytes[3]; };

struct pt_routed_frame_program {
    struct pt_routed_midi_key key;
    uint64_t first, last, audio_ticket;
    unsigned stop, audio_count, midi_count;
    struct pt_paula_dma_program paula;
    unsigned card_preload_slots, card_preload_count, card_frame_count;
    struct pt_amigus_voice_register_write card_preload[PT_ROUTED_CARD_PRELOAD_WRITES];
    struct pt_amigus_voice_register_write card_frame[PT_ROUTED_CARD_FRAME_WRITES];
    struct pt_routed_frame_midi midi[PT_ROUTED_MIDI_MESSAGES];
};

/* Lower one prepared provider publication into ordered Paula/Card writes
 * and MIDI bytes bound to the original destination slots. The caller
 * supplies the exact owned channel/voice masks and the complete project-order
 * MIDI mapping from its original destination owner. None of those slots
 * grants device authority by itself. Active retriggers refuse this preload
 * path. Terminal Stop covers the original active audio masks.
 *
 * Complete before the first frame tick. Stage only independently quiet,
 * still-owned slots. Keep all audio leases, MIDI links, window and command
 * storage through actual receipt, cancellation and retirement. A successful
 * encoding does not decide whether the native backend can meet the window.
 * No allocation or effects. Refusal leaves the entire destination unchanged. */
int pt_routed_frame_prepare(const struct pt_routed_activation_program *,
    unsigned owned_paula, unsigned owned_card, const uint8_t *midi_slots, unsigned slot_count,
    struct pt_routed_frame_program *);

#endif
