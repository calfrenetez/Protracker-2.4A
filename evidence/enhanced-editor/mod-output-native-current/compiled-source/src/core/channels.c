#include <string.h>
#include "channels.h"

void pt_channels_init(struct pt_channels *state)
{
    unsigned int i;
    memset(state, 0, sizeof(*state));
    state->count = 4;
    for (i = 0; i < PT_CHANNEL_LIMIT; ++i) {
        state->track[i].route = i < 4 ? PT_PAULA : PT_AMIGUS;
        state->track[i].pan = (i % 4 == 0 || i % 4 == 3) ? 0 : 255;
        state->track[i].midi_channel = (uint8_t)(i + 1);
    }
}

enum pt_channel_result pt_channels_validate(const struct pt_channels *state)
{
    unsigned int i, paula = 0;
    if (!state || state->count < 1 || state->count > PT_CHANNEL_LIMIT ||
        state->selected >= state->count) return PT_CHANNEL_INVALID;
    for (i = 0; i < PT_CHANNEL_LIMIT; ++i) {
        const struct pt_channel *channel = &state->track[i];
        if ((channel->route != PT_PAULA && channel->route != PT_AMIGUS && channel->route != PT_MIDI) ||
            channel->muted > 1 || channel->solo > 1 || channel->group > 15 ||
            !channel->midi_channel || channel->midi_channel > 16 ||
            !memchr(channel->name, 0, PT_CHANNEL_NAME)) return PT_CHANNEL_INVALID;
        if (i < state->count && channel->route == PT_PAULA) ++paula;
    }
    return paula > 4 ? PT_CHANNEL_PAULA_LIMIT : PT_CHANNEL_OK;
}

enum pt_channel_result pt_channels_resize(struct pt_channels *state, unsigned int count)
{
    struct pt_channels next;
    enum pt_channel_result result;
    if (pt_channels_validate(state) != PT_CHANNEL_OK || !count || count > PT_CHANNEL_LIMIT)
        return PT_CHANNEL_INVALID;
    next = *state;
    next.count = (uint8_t)count;
    if (next.selected >= count) next.selected = (uint8_t)(count - 1);
    result = pt_channels_validate(&next);
    if (result == PT_CHANNEL_OK) *state = next;
    return result;
}

enum pt_channel_result pt_channels_route(struct pt_channels *state, unsigned int index,
                                        enum pt_route route)
{
    struct pt_channels next;
    enum pt_channel_result result;
    if (pt_channels_validate(state) != PT_CHANNEL_OK || index >= state->count)
        return PT_CHANNEL_INVALID;
    /* Check before narrowing: 257 must not turn into a valid Paula route. */
    if (route != PT_PAULA && route != PT_AMIGUS && route != PT_MIDI)
        return PT_CHANNEL_INVALID;
    next = *state;
    next.track[index].route = (uint8_t)route;
    result = pt_channels_validate(&next);
    if (result == PT_CHANNEL_OK) *state = next;
    return result;
}

enum pt_channel_result pt_channels_step(struct pt_channels *state, int direction)
{
    if (pt_channels_validate(state) != PT_CHANNEL_OK || (direction != 1 && direction != -1))
        return PT_CHANNEL_INVALID;
    if (direction == 1)
        state->selected = (uint8_t)((state->selected + 1) % state->count);
    else
        state->selected = state->selected ? state->selected - 1 : state->count - 1;
    return PT_CHANNEL_OK;
}

unsigned int pt_channels_page(const struct pt_channels *state)
{ return state->selected / 4; }

int pt_channel_audible(const struct pt_channels *state, unsigned int index, unsigned int available)
{
    unsigned int i;
    int any_solo = 0;
    if (pt_channels_validate(state) != PT_CHANNEL_OK || index >= state->count) return 0;
    if (!(available & state->track[index].route) || state->track[index].muted) return 0;
    for (i = 0; i < state->count; ++i) any_solo |= state->track[i].solo;
    return !any_solo || state->track[index].solo;
}

enum pt_channel_result pt_channels_paula_map(const struct pt_channels *state,
                                            const int8_t *previous, int8_t *out)
{
    int8_t next[PT_CHANNEL_LIMIT];
    unsigned int i, used = 0, previous_used = 0, slot;
    enum pt_channel_result result = pt_channels_validate(state);
    if (result != PT_CHANNEL_OK) return result;
    if (!out) return PT_CHANNEL_INVALID;
    for (i = 0; i < PT_CHANNEL_LIMIT; ++i) {
        next[i] = -1;
        if (previous && previous[i] != -1) {
            if (previous[i] < 0 || previous[i] > 3 || (previous_used & (1U << previous[i])))
                return PT_CHANNEL_INVALID;
            previous_used |= 1U << previous[i];
            if (i < state->count && state->track[i].route == PT_PAULA) {
                next[i] = previous[i];
                used |= 1U << next[i];
            }
        }
    }
    for (i = 0; i < state->count; ++i) {
        if (state->track[i].route != PT_PAULA || next[i] != -1) continue;
        for (slot = 0; slot < 4 && (used & (1U << slot)); ++slot) {}
        if (slot == 4) return PT_CHANNEL_PAULA_LIMIT;
        next[i] = (int8_t)slot;
        used |= 1U << slot;
    }
    memcpy(out, next, sizeof(next));
    return PT_CHANNEL_OK;
}

enum pt_channel_result pt_history_init(struct pt_channel_history *history,
                                       struct pt_channels *storage, unsigned int capacity,
                                       const struct pt_channels *initial)
{
    if (!history || !storage || capacity < 2 || capacity > 64 ||
        pt_channels_validate(initial) != PT_CHANNEL_OK) return PT_CHANNEL_INVALID;
    history->snapshots = storage;
    history->capacity = capacity;
    history->count = 1;
    history->cursor = 0;
    storage[0] = *initial;
    return PT_CHANNEL_OK;
}

static int history_valid(const struct pt_channel_history *history)
{
    return history && history->snapshots && history->capacity >= 2 && history->capacity <= 64 &&
           history->count >= 1 && history->count <= history->capacity && history->cursor < history->count;
}

enum pt_channel_result pt_history_commit(struct pt_channel_history *history,
                                         const struct pt_channels *state)
{
    struct pt_channels next;
    unsigned int i;
    enum pt_channel_result result = pt_channels_validate(state);
    if (result != PT_CHANNEL_OK) return result;
    if (!history_valid(history)) return PT_CHANNEL_INVALID;
    next = *state; /* Source may alias a slot shifted below. */
    if (!memcmp(&history->snapshots[history->cursor], &next, sizeof(next))) return PT_CHANNEL_OK;
    history->count = history->cursor + 1; /* Editing after undo discards redo. */
    if (history->count == history->capacity) {
        for (i = 1; i < history->count; ++i) history->snapshots[i - 1] = history->snapshots[i];
        --history->count;
    }
    history->snapshots[history->count++] = next;
    history->cursor = history->count - 1;
    return PT_CHANNEL_OK;
}

enum pt_channel_result pt_history_move(struct pt_channel_history *history, int direction,
                                       struct pt_channels *out)
{
    if (!history_valid(history) || !out || (direction != 1 && direction != -1))
        return PT_CHANNEL_INVALID;
    if ((direction == -1 && !history->cursor) ||
        (direction == 1 && history->cursor + 1 == history->count)) return PT_CHANNEL_HISTORY_END;
    if (direction < 0) --history->cursor;
    else ++history->cursor;
    *out = history->snapshots[history->cursor];
    return PT_CHANNEL_OK;
}
