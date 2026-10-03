#ifndef PT_CHANNELS_H
#define PT_CHANNELS_H
#include <stdint.h>

#define PT_CHANNEL_LIMIT 16
#define PT_CHANNEL_NAME 16
enum pt_route { PT_PAULA = 1, PT_AMIGUS = 2, PT_MIDI = 4 };
enum pt_channel_result { PT_CHANNEL_OK, PT_CHANNEL_INVALID, PT_CHANNEL_PAULA_LIMIT,
                         PT_CHANNEL_HISTORY_END };
struct pt_channel {
    uint8_t route, pan, muted, solo, group, midi_channel;
    char name[PT_CHANNEL_NAME];
};
struct pt_channels {
    uint8_t count, selected;
    struct pt_channel track[PT_CHANNEL_LIMIT];
};
struct pt_channel_history {
    struct pt_channels *snapshots;
    unsigned int capacity, count, cursor;
};

void pt_channels_init(struct pt_channels *);
enum pt_channel_result pt_channels_validate(const struct pt_channels *);
enum pt_channel_result pt_channels_resize(struct pt_channels *, unsigned int);
enum pt_channel_result pt_channels_route(struct pt_channels *, unsigned int, enum pt_route);
enum pt_channel_result pt_channels_step(struct pt_channels *, int);
unsigned int pt_channels_page(const struct pt_channels *);
int pt_channel_audible(const struct pt_channels *, unsigned int, unsigned int);
/* Prior map may be NULL. -1 means no Paula slot. Output commits only on success.
 * Muting/solo does not discard allocations; continuing voices keep their slots. */
enum pt_channel_result pt_channels_paula_map(const struct pt_channels *,
                                            const int8_t *, int8_t *);
enum pt_channel_result pt_history_init(struct pt_channel_history *, struct pt_channels *,
                                       unsigned int, const struct pt_channels *);
enum pt_channel_result pt_history_commit(struct pt_channel_history *, const struct pt_channels *);
enum pt_channel_result pt_history_move(struct pt_channel_history *, int, struct pt_channels *);
#endif
