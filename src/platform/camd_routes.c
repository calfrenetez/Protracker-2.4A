#include "camd_routes.h"
#include <string.h>

static int disjoint(const void *a, size_t a_bytes, const void *b, size_t b_bytes)
{
    uintptr_t first = (uintptr_t)a, second = (uintptr_t)b;

    return a && a_bytes <= UINTPTR_MAX - first &&
        (!b_bytes || (b && b_bytes <= UINTPTR_MAX - second &&
                      (first + a_bytes <= second || second + b_bytes <= first)));
}

int pt_camd_routes_resolve(const struct pt_camd_control *control,
    const char (*names)[PT_MIDI_ENDPOINT], unsigned count, struct pt_camd_routes *out)
{
    struct pt_camd_routes resolved = {0};
    const struct pt_midi_endpoints *registry;
    unsigned used = 0;

    if (count > PT_MIDI_PORTS || !control ||
        !disjoint(out, sizeof(*out), control, sizeof(*control)) ||
        !disjoint(out, sizeof(*out), names, count * sizeof(*names)) ||
        pt_camd_control_state(control) != PT_CAMD_READY ||
        !disjoint(out, sizeof(*out), control->context, control->context_bytes))
        return 0;

    registry = pt_camd_control_registry(control);
    for (unsigned index = 0; index < count; ++index) {
        const char *end = memchr(names[index], 0, sizeof(names[index]));
        if (!end)
            return 0;
        int slot = pt_midi_endpoints_find(registry, names[index], (size_t)(end - names[index]) + 1);
        if (slot < 0 || (used & (1U << slot)) ||
            !pt_midi_endpoints_available(registry, (unsigned)slot) ||
            !control->links[slot] || strcmp(control->link_names[slot].identity, names[index]))
            return 0;
        used |= 1U << slot;
        resolved.slot[index] = (uint8_t)slot;
    }

    resolved.count = count;
    *out = resolved;
    return 1;
}

int pt_camd_routes_borrow(struct pt_camd_control *control,
    const char (*names)[PT_MIDI_ENDPOINT], unsigned count, struct pt_camd_route_lease *lease)
{
    struct pt_camd_routes resolved;

    if (!count || count > PT_MIDI_PORTS || pt_camd_control_state(control) != PT_CAMD_READY ||
        control->route_lease || (uintptr_t)lease % _Alignof(struct pt_camd_route_lease) ||
        !disjoint(lease, sizeof(*lease), control, sizeof(*control)) ||
        !disjoint(lease, sizeof(*lease), control->context, control->context_bytes) ||
        !disjoint(lease, sizeof(*lease), names, count * sizeof(*names)) || lease->owner ||
        !pt_camd_routes_resolve(control, names, count, &resolved))
        return 0;

    *lease = (struct pt_camd_route_lease){.owner = control, .routes = resolved};
    control->route_lease = lease;
    return 1;
}

int pt_camd_routes_snapshot(const struct pt_camd_route_lease *lease, struct pt_camd_routes *out)
{
    if (!lease || pt_camd_control_state(lease->owner) != PT_CAMD_READY)
        return 0;
    const struct pt_camd_control *control = lease->owner;
    if (control->route_lease != lease || !lease->routes.count || lease->routes.count > PT_MIDI_PORTS ||
        (uintptr_t)out % _Alignof(struct pt_camd_routes) ||
        !disjoint(out, sizeof(*out), lease, sizeof(*lease)) ||
        !disjoint(out, sizeof(*out), control, sizeof(*control)) ||
        !disjoint(out, sizeof(*out), control->context, control->context_bytes))
        return 0;

    const struct pt_midi_endpoints *registry = pt_camd_control_registry(control);
    for (unsigned i = 0; i < lease->routes.count; ++i) {
        unsigned slot = lease->routes.slot[i];
        if (!pt_midi_endpoints_available(registry, slot) || !control->links[slot])
            return 0;
    }
    *out = lease->routes;
    return 1;
}

int pt_camd_routes_release(struct pt_camd_route_lease *lease)
{
    if (!lease || pt_camd_control_state(lease->owner) != PT_CAMD_READY ||
        lease->owner->route_lease != lease)
        return 0;
    lease->owner->route_lease = NULL;
    *lease = (struct pt_camd_route_lease){0};
    return 1;
}
