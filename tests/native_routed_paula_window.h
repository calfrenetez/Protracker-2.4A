#ifndef PT_TEST_NATIVE_ROUTED_PAULA_WINDOW_H
#define PT_TEST_NATIVE_ROUTED_PAULA_WINDOW_H

/* Each transition uses a fresh local countdown to measure the same sample
 * frame width. This test does not share a song epoch across transitions. */
enum { PAULA_TEST_FRAME = 512, PAULA_TEST_RATE = 48000, PAULA_WRITE_MARKER = 0x4752 };

struct paula_window_raw {
    uint16_t first, end, attempts, marker;
    uint8_t before[3], after[3];
};

typedef char paula_window_size[(sizeof(struct paula_window_raw) == 14) ? 1 : -1];
typedef char paula_marker_offset[(offsetof(struct paula_window_raw, marker) == 6) ? 1 : -1];
typedef char paula_before_offset[(offsetof(struct paula_window_raw, before) == 8) ? 1 : -1];
typedef char paula_after_offset[(offsetof(struct paula_window_raw, after) == 11) ? 1 : -1];

struct paula_window_observation {
    struct pt_countdown_window window;
    struct pt_cia_clock_pair anchor_sample;
    struct paula_window_raw raw;
    uint32_t hz;
    unsigned stage, chip, timer, anchor, status, command, dma_before, dma_after, in_frame;
};

static int prepare_paula_window(struct paula_window_observation *obs, const struct pt_elapsed_clock *grid,
    unsigned frame)
{
    uint64_t first, end;
    if (pt_elapsed_clock_deadline(grid, frame, &first) != PT_ELAPSED_OK ||
        pt_elapsed_clock_deadline(grid, frame + 1, &end) != PT_ELAPSED_OK ||
        !pt_countdown_window_prepare(0, obs->anchor, first, end, &obs->window))
        return 0;
    obs->raw.first = obs->window.first_count;
    obs->raw.end = obs->window.end_count;
    return 1;
}

static int paula_window_inside(const struct paula_window_observation *obs)
{
    struct pt_countdown_bracket bracket;
    return obs->status == 1 && obs->raw.marker == PAULA_WRITE_MARKER &&
        pt_countdown_window_bracket(&obs->window, pt_cia_stable_count(obs->raw.before),
            pt_cia_stable_count(obs->raw.after), &bracket);
}

static struct paula_window_observation timed[2];
static unsigned timed_count;
static volatile ULONG timer_strays;

#ifndef PT_PAULA_TIMED_IRQ
extern int pt_cia_local_paula_dma(volatile UBYTE *, volatile UBYTE *, struct paula_window_raw *, uint32_t);

static int approach_window(const struct pt_diagnostic_cia *timer, uint16_t first)
{
    for (unsigned i = 0; i < 16384; ++i) {
        uint8_t raw[3];
        raw[0] = *timer->high;
        raw[1] = *timer->low;
        raw[2] = *timer->high;
        unsigned sample = pt_cia_stable_count(raw);
        if (sample && sample <= (unsigned)first + 80)
            return 1;
    }
    return 0;
}

static int apply_grouped_transition(const struct pt_native_paula_reservation *owner,
    const struct pt_paula_dma_write *word, unsigned before, unsigned after)
{
    struct pt_native_eclock clock = {0};
    struct pt_diagnostic_cia timer = {0};
    struct pt_elapsed_clock grid;
    uint64_t ticks;
    if (timed_count >= sizeof(timed) / sizeof(timed[0]))
        return 0;
    struct paula_window_observation *obs = timed + timed_count++;
    obs->stage = stage;
    obs->command = word->value;
    obs->status = 3;
    const unsigned expected = before ? before : SET_BITS | after;
    if (word->offset != DMA_SET_CLEAR || word->value != expected || !owns_channels(owner) ||
        !pt_native_eclock_open(&clock) || !pt_native_eclock_read(&clock, &ticks, &obs->hz) ||
        (obs->hz != 709379 && obs->hz != 715909) ||
        !pt_diagnostic_cia_acquire(&timer, pt_cia_direct_clock_stray, (void *)&timer_strays))
        goto finish;
    obs->chip = timer.chip;
    obs->timer = timer.bit;
    if (!pt_cia_start_masked_countdown(&timer))
        goto finish;
    Disable();
    pt_cia_direct_clock_pair(timer.high, timer.low, &obs->anchor_sample);
    snapshot(owner, "before-registers", 0, NULL);
    Enable();
    obs->anchor = pt_cia_stable_count(obs->anchor_sample.after);
    if (obs->anchor_sample.marker != 0x2452 || !obs->anchor ||
        pt_elapsed_clock_init(&grid, obs->hz, PAULA_TEST_RATE, 0, 0) != PT_ELAPSED_OK ||
        !prepare_paula_window(obs, &grid, PAULA_TEST_FRAME))
        goto finish;
    if (!approach_window(&timer, obs->raw.first) || !pt_diagnostic_cia_current(&timer))
        goto finish;
    Disable();
    obs->dma_before = *custom_word(DMA_READ);
    if ((obs->dma_before & (DMA_MASTER | ALL)) == (DMA_MASTER | before) && owns_channels(owner)) {
        obs->status = pt_cia_local_paula_dma(timer.high, timer.low, &obs->raw, word->value);
        obs->dma_after = *custom_word(DMA_READ);
        if (obs->raw.marker)
            snapshot(owner, "after-registers", 1, word);
    }
    Enable();
    obs->in_frame = paula_window_inside(obs);

finish:
    if (!pt_diagnostic_cia_close(&timer))
        return hold();
    pt_native_eclock_close(&clock);
    return obs->in_frame && (obs->dma_after & (DMA_MASTER | ALL)) == (DMA_MASTER | after) &&
        !timer_strays && !timer.held && !clock.opened && owns_channels(owner);
}
#endif

static void report_windows(void)
{
    for (unsigned i = 0; i < timed_count; ++i) {
        const struct paula_window_observation *obs = timed + i;
        const struct paula_window_raw *raw = &obs->raw;
        const uint8_t *anchor = obs->anchor_sample.after;
        printf("Paula window %u stage=%u hz=%lu chip=%u timer=%u anchor=%02x%02x%02x count=%u/%u attempts=%u "
            "status=%u rawbefore=%02x%02x%02x rawafter=%02x%02x%02x mark=%04x "
            "command=%04x dma=%04x/%04x in_frame=%u\n",
            i, obs->stage, (unsigned long)obs->hz, obs->chip, obs->timer,
            (unsigned)anchor[0], (unsigned)anchor[1], (unsigned)anchor[2],
            (unsigned)raw->first, (unsigned)raw->end, (unsigned)raw->attempts, obs->status,
            (unsigned)raw->before[0], (unsigned)raw->before[1], (unsigned)raw->before[2],
            (unsigned)raw->after[0], (unsigned)raw->after[1], (unsigned)raw->after[2],
            (unsigned)raw->marker, obs->command, obs->dma_before, obs->dma_after, obs->in_frame);
    }
    printf("Paula windows closed stray=%lu rows=%u\n", (unsigned long)timer_strays, timed_count);
}
#endif
