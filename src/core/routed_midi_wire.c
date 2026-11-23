#include "routed_midi_wire.h"

int pt_routed_midi_wire_prepare(const struct pt_routed_frame_program *frame, unsigned slot,
    struct pt_routed_midi_wire *out)
{
    if (!frame || !out || slot >= PT_MIDI_PORTS || frame->midi_count != 1 || frame->stop > 1 ||
        !frame->key.session || !frame->key.generation || !frame->key.serial || frame->first >= frame->last)
        return 0;
    const struct pt_routed_frame_midi *message = frame->midi;
    const unsigned status = message->bytes[0] & 0xf0U;
    if (message->slot != slot || (status != 0x80 && (frame->stop || status != 0x90)) ||
        message->bytes[1] > 127 || message->bytes[2] > 127)
        return 0;
    const struct pt_routed_midi_wire prepared = {{0x100U | message->bytes[0],
        0x100U | message->bytes[1], 0x100U | message->bytes[2]}};
    *out = prepared;
    return 1;
}
