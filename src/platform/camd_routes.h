#ifndef PT_CAMD_ROUTES_H
#define PT_CAMD_ROUTES_H

#include "camd_control.h"

struct pt_camd_routes {
    unsigned count;
    uint8_t slot[PT_MIDI_PORTS];
};

struct pt_camd_route_lease {
    struct pt_camd_control *owner;
    struct pt_camd_routes routes;
};

/* Resolve a complete project-order destination list against the original
 * serialized control owner. Each destination needs an observed receiver and
 * an existing sender link for that exact name. On refusal, out is unchanged.
 * The result is valid until the owner refreshes or changes its links. It
 * contains no link handles and grants no delivery or timing authority. */
int pt_camd_routes_resolve(const struct pt_camd_control *,
    const char (*names)[PT_MIDI_ENDPOINT], unsigned count, struct pt_camd_routes *out);

/* Task-side, serialized lease for the original links used by future frame
 * publications. Zero the lease before use. While it is live, the owner can
 * refresh availability but cannot add, retire or close links. A refresh that
 * loses a receiver blocks new snapshots without releasing any original link.
 * Snapshot refuses without modifying out. No MIDI is sent by these calls. */
int pt_camd_routes_borrow(struct pt_camd_control *, const char (*names)[PT_MIDI_ENDPOINT],
    unsigned count, struct pt_camd_route_lease *);
int pt_camd_routes_snapshot(const struct pt_camd_route_lease *, struct pt_camd_routes *);
/* Call only when every frame and the Stop/cleanup using these slots has been
 * settled by its original provider. This release does not prove delivery or
 * device quiet. Copies and moved leases have no authority. */
int pt_camd_routes_release(struct pt_camd_route_lease *);

#endif
