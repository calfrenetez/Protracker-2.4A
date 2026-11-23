#ifndef PT_ROUTED_MIDI_WIRE_H
#define PT_ROUTED_MIDI_WIRE_H

#include "routed_frame_program.h"

struct pt_routed_midi_wire {
    uint16_t word[3];
};

/* Encode one complete note for one explicitly selected UART endpoint.
 * Frame activation and transmit drain require separate observations by the
 * original port owner. Refusal leaves the destination unchanged. */
int pt_routed_midi_wire_prepare(const struct pt_routed_frame_program *, unsigned slot,
    struct pt_routed_midi_wire *);

#endif
