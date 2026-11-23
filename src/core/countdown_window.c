#include "countdown_window.h"

int pt_countdown_extend(struct pt_countdown_observation *observation, uint32_t reloads, uint16_t count)
{
    if (!observation || reloads < observation->reloads)
        return 0;
    uint64_t elapsed = ((uint64_t)(reloads - observation->reloads) << 16) + observation->count;
    if (elapsed < count || elapsed - count > UINT64_MAX - observation->tick)
        return 0;
    observation->tick += elapsed - count;
    observation->reloads = reloads;
    observation->count = count;
    return 1;
}

int pt_countdown_window_prepare(uint64_t anchor_tick, uint16_t anchor_count,
    uint64_t first, uint64_t end, struct pt_countdown_window *out)
{
    if (!out || anchor_tick >= first || first >= end || end - anchor_tick >= anchor_count)
        return 0;
    *out = (struct pt_countdown_window){
        .first = first,
        .end = end,
        .anchor_count = anchor_count,
        .first_count = anchor_count - (uint16_t)(first - anchor_tick),
        .end_count = anchor_count - (uint16_t)(end - anchor_tick),
    };
    return 1;
}

static int valid(const struct pt_countdown_window *window)
{
    return window && window->first < window->end &&
        window->anchor_count > window->first_count && window->first_count > window->end_count &&
        window->end_count && window->end - window->first == (uint64_t)(window->first_count - window->end_count);
}

static enum pt_countdown_status classify(const struct pt_countdown_window *window, uint16_t count)
{
    if (count > window->anchor_count)
        return PT_COUNTDOWN_INVALID;
    if (count > window->first_count)
        return PT_COUNTDOWN_EARLY;
    if (count <= window->end_count)
        return PT_COUNTDOWN_EXPIRED;
    return PT_COUNTDOWN_INSIDE;
}

enum pt_countdown_status pt_countdown_window_status(const struct pt_countdown_window *window, uint16_t count)
{
    return valid(window) ? classify(window, count) : PT_COUNTDOWN_INVALID;
}

int pt_countdown_window_bracket(const struct pt_countdown_window *window, uint16_t first, uint16_t last,
    struct pt_countdown_bracket *out)
{
    const uintptr_t source = (uintptr_t)window, dest = (uintptr_t)out;

    if (!window || !out || dest % _Alignof(struct pt_countdown_bracket) ||
        source > UINTPTR_MAX - sizeof(*window) || dest > UINTPTR_MAX - sizeof(*out) ||
        (source + sizeof(*window) > dest && dest + sizeof(*out) > source) ||
        !valid(window) || last > first ||
        classify(window, first) != PT_COUNTDOWN_INSIDE || classify(window, last) != PT_COUNTDOWN_INSIDE)
        return 0;

    *out = (struct pt_countdown_bracket){
        window->first + (window->first_count - first),
        window->first + (window->first_count - last)
    };
    return 1;
}
