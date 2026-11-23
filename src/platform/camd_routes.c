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
