#include "countdown_window.h"
#include "elapsed_clock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void verify(uint32_t frequency, uint64_t epoch, uint16_t anchor)
{
    struct pt_elapsed_clock grid;
    unsigned short_frames = 0, long_frames = 0;
    assert(pt_elapsed_clock_init(&grid, frequency, 48000, epoch, 0) == PT_ELAPSED_OK);
    for (unsigned frame = 1; frame < 1000; ++frame) {
        uint64_t first, end;
        struct pt_countdown_window window;
        struct pt_countdown_bracket bracket, before;
        assert(pt_elapsed_clock_deadline(&grid, frame, &first) == PT_ELAPSED_OK);
        assert(pt_elapsed_clock_deadline(&grid, frame + 1, &end) == PT_ELAPSED_OK);
        assert(pt_countdown_window_prepare(epoch, anchor, first, end, &window));
        unsigned ticks = (unsigned)(end - first);
        assert(ticks == 14 || ticks == 15);
        short_frames += ticks == 14;
        long_frames += ticks == 15;
        assert(pt_countdown_window_status(&window, anchor) == PT_COUNTDOWN_EARLY);
        assert(pt_countdown_window_status(&window, anchor + 1) == PT_COUNTDOWN_INVALID);
        assert(pt_countdown_window_status(&window, window.end_count) == PT_COUNTDOWN_EXPIRED);
        assert(pt_countdown_window_status(&window, 0) == PT_COUNTDOWN_EXPIRED);
        assert(pt_countdown_window_bracket(&window, window.first_count, window.end_count + 1, &bracket));
        assert(bracket.first == first && bracket.last == end - 1);
        before = bracket;
        assert(!pt_countdown_window_bracket(&window, window.end_count + 1, window.first_count, &bracket));
        assert(!pt_countdown_window_bracket(&window, window.first_count + 1, window.end_count + 1, &bracket));
        assert(!pt_countdown_window_bracket(&window, window.first_count, window.end_count, &bracket));
        assert(!pt_countdown_window_bracket(&window, window.first_count, window.end_count + 1,
            (struct pt_countdown_bracket *)(void *)&window));
        assert(!memcmp(&before, &bracket, sizeof(bracket)));
    }
    assert(short_frames && long_frames);
}

int main(void)
{
    struct pt_countdown_window window, before;
    struct pt_countdown_bracket bracket;
    assert(pt_countdown_window_prepare(UINT64_MAX - 20, 25, UINT64_MAX - 15, UINT64_MAX, &window));
    assert(pt_countdown_window_bracket(&window, 20, 6, &bracket));
    assert(bracket.first == UINT64_MAX - 15 && bracket.last == UINT64_MAX - 1);
    before = window;
    assert(!pt_countdown_window_prepare(10, 20, 10, 14, &window));
    assert(!pt_countdown_window_prepare(10, 20, 14, 14, &window));
    assert(!pt_countdown_window_prepare(10, 20, 14, 30, &window));
    assert(!pt_countdown_window_prepare(10, 20, 14, 31, &window));
    assert(!pt_countdown_window_prepare(10, 0, 14, 28, &window));
    assert(!memcmp(&before, &window, sizeof(window)));
    --window.end_count;
    assert(pt_countdown_window_status(&window, 19) == PT_COUNTDOWN_INVALID);
    assert(!pt_countdown_window_bracket(&window, 19, 7, &bracket));
    verify(709379, UINT64_C(0x1fffffff0), 60000);
    verify(715909, UINT64_C(0xfffffffffff00000), 60000);
    puts("Countdown window: exact PAL and NTSC frame boundaries and original tick brackets verified");
    return 0;
}
