#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "channels.h"

int main(void)
{
    struct pt_channels s, before, slots[3];
    struct pt_channel_history history;
    int8_t map[16], next[16];
    unsigned int i, n;
    pt_channels_init(&s);
    assert(s.count == 4 && pt_channels_validate(&s) == PT_CHANNEL_OK);
    for (i = 0; i < 4; ++i) assert(s.track[i].route == PT_PAULA);
    assert(pt_channels_resize(&s, 16) == PT_CHANNEL_OK);
    before = s;
    assert(pt_channels_route(&s, 4, PT_PAULA) == PT_CHANNEL_PAULA_LIMIT);
    assert(!memcmp(&s, &before, sizeof(s)));
    assert(pt_channels_route(&s, 4, (enum pt_route)3) == PT_CHANNEL_INVALID);
    assert(pt_channels_route(&s, 4, (enum pt_route)257) == PT_CHANNEL_INVALID);
    assert(pt_channels_paula_map(&s, NULL, map) == PT_CHANNEL_OK);
    for (i = 0; i < 4; ++i) assert(map[i] == (int)i);
    assert(pt_channels_route(&s, 1, PT_MIDI) == PT_CHANNEL_OK);
    assert(pt_channels_route(&s, 15, PT_PAULA) == PT_CHANNEL_OK);
    assert(pt_channels_paula_map(&s, map, next) == PT_CHANNEL_OK);
    assert(next[0] == 0 && next[2] == 2 && next[3] == 3 && next[15] == 1 && next[1] == -1);
    memcpy(map, next, sizeof(map));
    for (n = 1; n <= 16; ++n) {
        assert(pt_channels_resize(&s, n) == PT_CHANNEL_OK);
        s.selected = 0;
        for (i = 0; i < n; ++i) {
            assert(s.selected == i && pt_channels_page(&s) == i / 4);
            assert(pt_channels_step(&s, 1) == PT_CHANNEL_OK);
        }
        assert(!s.selected);
        assert(pt_channels_step(&s, -1) == PT_CHANNEL_OK && s.selected == n - 1);
    }
    assert(pt_channel_audible(&s, 0, PT_PAULA));
    assert(!pt_channel_audible(&s, 1, PT_PAULA));
    assert(!pt_channel_audible(&s, 4, PT_PAULA));
    assert(s.track[1].route == PT_MIDI && s.track[4].route == PT_AMIGUS);
    s.track[4].solo = 1;
    assert(!pt_channel_audible(&s, 0, 7));
    assert(pt_channel_audible(&s, 4, 7));
    s.track[4].muted = 1;
    assert(!pt_channel_audible(&s, 4, 7));
    assert(pt_channels_paula_map(&s, map, next) == PT_CHANNEL_OK && !memcmp(map, next, 16));
    map[0] = map[15];
    memset(next, 0x7e, 16);
    assert(pt_channels_paula_map(&s, map, next) == PT_CHANNEL_INVALID && next[0] == 0x7e);
    pt_channels_init(&s);
    assert(pt_history_init(&history, slots, 3, &s) == PT_CHANNEL_OK);
    assert(pt_history_move(&history, -1, &before) == PT_CHANNEL_HISTORY_END);
    assert(pt_channels_resize(&s, 8) == PT_CHANNEL_OK);
    assert(pt_history_commit(&history, &s) == PT_CHANNEL_OK);
    assert(pt_channels_resize(&s, 12) == PT_CHANNEL_OK);
    assert(pt_history_commit(&history, &s) == PT_CHANNEL_OK);
    assert(pt_channels_resize(&s, 16) == PT_CHANNEL_OK);
    assert(pt_history_commit(&history, &s) == PT_CHANNEL_OK);
    assert(history.count == 3 && slots[0].count == 8);
    assert(pt_history_move(&history, -1, &s) == PT_CHANNEL_OK && s.count == 12);
    assert(pt_history_move(&history, -1, &s) == PT_CHANNEL_OK && s.count == 8);
    assert(pt_history_move(&history, 1, &s) == PT_CHANNEL_OK && s.count == 12);
    s.track[0].pan = 123;
    assert(pt_history_commit(&history, &s) == PT_CHANNEL_OK);
    assert(pt_history_move(&history, 1, &s) == PT_CHANNEL_HISTORY_END);
    assert(pt_history_move(&history, -1, &s) == PT_CHANNEL_OK && s.track[0].pan == 0);
    assert(pt_history_move(&history, 1, &s) == PT_CHANNEL_OK && s.track[0].pan == 123);
    puts("CHANNELS PASS: 1-16 pages, exclusive routes, stable Paula slots, absence, mute/solo, undo/redo");
    return 0;
}
