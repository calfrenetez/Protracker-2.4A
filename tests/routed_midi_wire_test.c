#include "../src/core/routed_midi_wire.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    struct pt_routed_frame_program frame = {
        .key = {1, 1, 0, 1}, .first = 32, .last = 47, .midi_count = 1, .midi = {{2, {0x95, 60, 90}}}
    };
    struct pt_routed_midi_wire out;
    assert(pt_routed_midi_wire_prepare(&frame, 2, &out));
    assert(out.word[0] == 0x195 && out.word[1] == 0x13c && out.word[2] == 0x15a);
    const struct pt_routed_midi_wire first = out;
    assert(!pt_routed_midi_wire_prepare(&frame, 1, &out));
    frame.midi_count = 2;
    assert(!pt_routed_midi_wire_prepare(&frame, 2, &out));
    frame.midi_count = 1;
    frame.stop = 1;
    assert(!pt_routed_midi_wire_prepare(&frame, 2, &out));
    frame.midi[0].bytes[0] = 0x85;
    frame.midi[0].bytes[1] = 128;
    assert(!pt_routed_midi_wire_prepare(&frame, 2, &out));
    assert(!memcmp(&first, &out, sizeof(first)));
    frame.midi[0].bytes[1] = 60;
    frame.midi[0].bytes[2] = 0;
    assert(pt_routed_midi_wire_prepare(&frame, 2, &out));
    assert(out.word[0] == 0x185 && out.word[1] == 0x13c && out.word[2] == 0x100);
    return 0;
}
