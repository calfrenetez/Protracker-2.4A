#ifndef PT_CAMD_ROUTES_H
#define PT_CAMD_ROUTES_H

#include "camd_control.h"

struct pt_camd_routes {
    unsigned count;
    uint8_t slot[PT_MIDI_PORTS];
};

/* Resolve a complete project-order destination list against the original
 * serialized control owner. Each destination needs an observed receiver and
 * an existing sender link for that exact name. On refusal, out is unchanged.
 * The result is valid until the owner refreshes or changes its links. It
 * contains no link handles and grants no delivery or timing authority. */
int pt_camd_routes_resolve(const struct pt_camd_control *,
    const char (*names)[PT_MIDI_ENDPOINT], unsigned count, struct pt_camd_routes *out);

#endif
