#ifndef PT_TEST_NATIVE_ROUTED_PAULA_ALARM_H
#define PT_TEST_NATIVE_ROUTED_PAULA_ALARM_H

#include "native_cia_alarm_lifecycle.h"

enum { PAULA_STOP_FRAME = 1280, PAULA_ALARM_LEAD = 128, PAULA_ALARM_SPINS = 65536 };
extern void pt_cia_local_paula_alarm_irq(void);

struct paula_alarm_schedule {
    uint64_t epoch, start_frame, stop_frame;
};

#ifdef PT_PAULA_MIXED_IRQ
#include "native_tracker_mixed_words.h"
extern void pt_cia_local_mixed_alarm_irq(void);
#endif

/* The IRQ checks the still-pending request itself, since ADCMD_LOCK can
 * complete on loss of its allocation. It uses no audio.device call. */
_Static_assert(NT_MESSAGE == 5 &&
    offsetof(struct IOAudio, ioa_Request.io_Message.mn_Node.ln_Type) == 8 &&
    offsetof(struct IOAudio, ioa_Request.io_Unit) == 24 &&
    offsetof(struct IOAudio, ioa_AllocKey) == 32, "Paula IRQ lock observation layout");

struct paula_alarm_row {
    struct pt_cia_clock_pair arm;
    unsigned lead_count, delay, calls;
#ifdef PT_PAULA_WAIT_IRQ
    struct pt_cia_clock_pair wait_before, wait_after;
    unsigned alarm_signal_only;
#ifdef PT_PAULA_TAIL_IRQ
    unsigned before_attempts, after_attempts;
#endif
#endif
#ifdef PT_PAULA_RELOAD_IRQ
    unsigned issued_reload;
#endif
};

static struct paula_alarm_row armed_at[2];
static unsigned alarm_chip, alarm_timer, alarm_calls, alarm_strays, alarm_closed;

#ifdef PT_PAULA_RELOAD_IRQ
enum { CLOCK_RELOAD_MARGIN = 4096 };
static struct {
    struct Task *task;
    ULONG signal;
    volatile ULONG reloads;
} routed_clock;
_Static_assert(offsetof(__typeof__(routed_clock), signal) == 4 &&
    offsetof(__typeof__(routed_clock), reloads) == 8, "Owned CIA reload interrupt layout");
extern void pt_cia_reload_irq(void);
extern void pt_cia_local_mixed_reload_irq(void);
#ifdef PT_PAULA_TAIL_IRQ
enum { MIXED_TAIL_FIRST = 13, MIXED_TAIL_SPAN = 7 };
extern void pt_cia_local_mixed_tail_irq(void);

static unsigned sample_wait_pair(const struct pt_diagnostic_cia *clock, struct pt_cia_clock_pair *point)
{
    for (unsigned attempt = 1; attempt <= 4; ++attempt) {
        pt_cia_direct_clock_pair(clock->high, clock->low, point);
        if (point->marker == 0x2452 && pt_cia_stable_count(point->after))
            return attempt;
    }
    return 0;
}
#endif

#ifndef PT_PAULA_TAIL_IRQ
static int capture_epoch(struct paula_window_observation *obs, const struct pt_diagnostic_cia *clock,
    const struct pt_elapsed_clock *grid, uint64_t frame, unsigned reload,
    struct pt_countdown_observation *timeline)
{
    if (routed_clock.reloads != reload || !pt_diagnostic_cia_current(clock))
        return 0;
    pt_cia_direct_clock_pair(clock->high, clock->low, &obs->anchor_sample);
    obs->anchor = pt_cia_stable_count(obs->anchor_sample.after);
    if (routed_clock.reloads != reload || obs->anchor_sample.marker != 0x2452 ||
        obs->anchor <= CLOCK_RELOAD_MARGIN)
        return 0;
    if (!reload)
        timeline->count = obs->anchor;
    else if (!pt_countdown_extend(timeline, reload, obs->anchor))
        return 0;
    obs->anchor_tick = timeline->tick;
    obs->reload = reload;
    obs->chip = clock->chip;
    obs->timer = clock->bit;
    obs->hz = grid->frequency;
    return prepare_paula_window(obs, grid, frame) && obs->raw.end > CLOCK_RELOAD_MARGIN &&
        obs->raw.first < UINT16_MAX - CLOCK_RELOAD_MARGIN;
}
#endif
#endif

static unsigned sample_count(const struct pt_diagnostic_cia *clock)
{
    uint8_t raw[3];
    raw[0] = *clock->high;
    raw[1] = *clock->low;
    raw[2] = *clock->high;
    return pt_cia_stable_count(raw);
}

static int run_irq_word(const struct pt_native_paula_reservation *owner, struct local_alarm *alarm,
    const struct pt_diagnostic_cia *clock, const struct pt_paula_dma_write *word, unsigned before, unsigned after)
{
    struct paula_window_observation *obs = timed + timed_count;
    struct paula_alarm_row *receipt = armed_at + timed_count++;
    struct local_alarm_data *irq = &alarm->data;
    obs->stage = stage;
    obs->command = word->value;
    obs->status = 4;
    receipt->lead_count = (unsigned)obs->raw.first + PAULA_ALARM_LEAD;

    Disable();
    snapshot(owner, "before-registers", 0, NULL);
    obs->dma_before = *custom_word(DMA_READ);
    if (word->offset != DMA_SET_CLEAR || word->value != (before ? before : SET_BITS | after) ||
        irq->armed || irq->entry || irq->calls != alarm->delivered || irq->stray ||
        !pt_diagnostic_cia_current(clock) || !owns_channels(owner) ||
        owner->lock->ioa_Request.io_Message.mn_Node.ln_Type != NT_MESSAGE ||
        (obs->dma_before & (DMA_MASTER | ALL)) != (DMA_MASTER | before)) {
        Enable();
        return 0;
    }

    pt_cia_direct_clock_pair(clock->high, clock->low, &receipt->arm);
    unsigned probe = pt_cia_stable_count(receipt->arm.after);
    if (probe <= receipt->lead_count) {
        Enable();
        return 0;
    }
    receipt->delay = probe - receipt->lead_count;
#ifdef PT_PAULA_WAIT_IRQ
    SetSignal(0, irq->signal);
#endif
    irq->audio_lock = owner->lock;
    irq->allocation_key = (UWORD)owner->key;
    irq->dma_before = DMA_MASTER | before;
    irq->command = word->value;
    irq->entry = &obs->raw;
    irq->result = UINT32_MAX;
#ifdef PT_PAULA_RELOAD_IRQ
    if (routed_clock.reloads != obs->reload) {
        irq->entry = NULL;
        Enable();
        return 0;
    }
    irq->reload = &routed_clock.reloads;
    irq->expected_reload = obs->reload;
    irq->observed_reload = UINT32_MAX;
#endif
    irq->armed = 1;
    int armed = pt_diagnostic_cia_arm(&alarm->timer, receipt->delay);
    if (!armed)
        irq->armed = 0;
    Enable();
    if (!armed)
        return 0;

#ifdef PT_PAULA_WAIT_IRQ
#ifdef PT_PAULA_TAIL_IRQ
    receipt->before_attempts = sample_wait_pair(clock, &receipt->wait_before);
#else
    pt_cia_direct_clock_pair(clock->high, clock->low, &receipt->wait_before);
#endif
    ULONG wait_mask = irq->signal | alarm->deadline_signal | SIGBREAKF_CTRL_C;
#ifdef PT_PAULA_RELOAD_IRQ
    wait_mask |= routed_clock.signal;
#endif
    ULONG wake = Wait(wait_mask);
#ifdef PT_PAULA_TAIL_IRQ
    if (stage == 6) {
        receipt->alarm_signal_only = (wake == irq->signal || wake == (irq->signal | routed_clock.signal)) &&
            ((wake & routed_clock.signal) ||
             Wait(routed_clock.signal | alarm->deadline_signal | SIGBREAKF_CTRL_C) == routed_clock.signal) &&
            routed_clock.reloads == 1;
    } else
#endif
        receipt->alarm_signal_only = wake == irq->signal;
#ifdef PT_PAULA_TAIL_IRQ
    receipt->after_attempts = sample_wait_pair(clock, &receipt->wait_after);
#else
    pt_cia_direct_clock_pair(clock->high, clock->low, &receipt->wait_after);
#endif
#else
    for (unsigned i = 0; i < PAULA_ALARM_SPINS && irq->armed; ++i) {
        unsigned now = sample_count(clock);
        if ((now && now <= obs->raw.end) || !(*clock->control & 1U))
            break;
    }
#endif
    Disable();
    receipt->calls = irq->calls;
#ifdef PT_PAULA_RELOAD_IRQ
    receipt->issued_reload = irq->observed_reload;
#endif
    if (irq->armed || irq->calls != alarm->delivered + 1 || irq->stray || irq->result > 3) {
        /* Keep the vector and its original data live until it can be closed.
         * A pending notification after this point must issue no write. */
        irq->armed = 0;
        Enable();
        return 0;
    }
    ++alarm->delivered;
    irq->entry = NULL;
    __asm__ volatile ("" ::: "memory");
    obs->status = irq->result;
    obs->dma_after = *custom_word(DMA_READ);
    if (obs->raw.marker)
        snapshot(owner, "after-registers", 1, word);
    Enable();
    obs->in_frame = paula_window_inside(obs);
#ifdef PT_PAULA_RELOAD_IRQ
    unsigned required_generation = obs->reload;
#ifdef PT_PAULA_TAIL_IRQ
    if (stage == 6)
        ++required_generation;
#endif
    if (receipt->issued_reload != obs->reload || routed_clock.reloads != required_generation)
        return 0;
#endif
#ifdef PT_PAULA_WAIT_IRQ
    const unsigned entering = pt_cia_stable_count(receipt->wait_before.after);
    const unsigned waking = pt_cia_stable_count(receipt->wait_after.after);
    if (!receipt->alarm_signal_only || entering <= obs->raw.first || entering >= probe || !waking)
        return 0;
#ifdef PT_PAULA_TAIL_IRQ
    if (!receipt->before_attempts || !receipt->after_attempts ||
        (stage != 6 && waking > pt_cia_stable_count(obs->raw.after)))
        return 0;
#else
    if (waking > pt_cia_stable_count(obs->raw.after))
        return 0;
#endif
#endif
#ifdef PT_PAULA_MIXED_IRQ
    if (!mixed_words_issued(&obs->raw))
        return 0;
#endif
    return obs->in_frame && (obs->dma_after & (DMA_MASTER | ALL)) == (DMA_MASTER | after) &&
        owns_channels(owner);
}

static int observe_started_fetches(const struct pt_native_paula_reservation *owner,
    const struct pt_diagnostic_cia *clock, unsigned cutoff)
{
    for (unsigned i = 0; i < PAULA_ALARM_SPINS; ++i) {
        unsigned sample = sample_count(clock);
        if ((sample && sample <= cutoff) || !(*clock->control & 1U))
            return 0;
        if ((*custom_word(INT_REQUEST_READ) & AUDIO_INTERRUPTS) == AUDIO_INTERRUPTS) {
            const struct trace_row row = snapshot(owner, "after-start-fetches", 0, NULL);
            return row.owned && (row.dma & ALL) == ALL && (row.request & AUDIO_INTERRUPTS) == AUDIO_INTERRUPTS;
        }
    }
    return 0;
}

static int run_irq_transitions(const struct pt_native_paula_reservation *owner,
    struct pt_routed_activation_program *input, const void *chip, const struct pt_paula_dma_write *start,
    struct paula_alarm_schedule *schedule)
{
    struct pt_paula_dma_program stop;
    struct pt_native_eclock timebase = {0};
    struct pt_diagnostic_cia clock = {0};
    struct local_alarm alarm = {.signal = -1};
    struct pt_elapsed_clock grid;
#ifdef PT_PAULA_RELOAD_IRQ
    struct pt_countdown_observation timeline = {0};
    BYTE reload_signal = -1;
#else
    struct pt_cia_clock_pair anchor;
#endif
    uint64_t now;
    uint32_t hz;
    int passed = 0;
#ifdef PT_PAULA_TAIL_IRQ
    void (*handler)(void) = pt_cia_local_mixed_tail_irq;
#elif defined(PT_PAULA_RELOAD_IRQ)
    void (*handler)(void) = pt_cia_local_mixed_reload_irq;
#elif defined(PT_PAULA_MIXED_IRQ)
    void (*handler)(void) = pt_cia_local_mixed_alarm_irq;
#else
    void (*handler)(void) = pt_cia_local_paula_alarm_irq;
#endif
    if (!prepare(input, chip, ALL, ALL, PT_MIXED_READERS_STOP, &stop) || stop.frame_count != 1 ||
        !pt_native_eclock_open(&timebase) || !pt_native_eclock_read(&timebase, &now, &hz) ||
        (hz != 709379 && hz != 715909) || now > UINT64_MAX - hz ||
#ifndef PT_PAULA_TAIL_IRQ
        pt_elapsed_clock_init(&grid, hz, PAULA_TEST_RATE, schedule->epoch, 0) != PT_ELAPSED_OK ||
#ifdef PT_PAULA_MIXED_IRQ
        !mixed_prepare_words(&grid, start, stop.frame) ||
#endif
#endif
#ifdef PT_PAULA_RELOAD_IRQ
        !(routed_clock.task = FindTask(NULL)) || (reload_signal = AllocSignal(-1)) < 0 || reload_signal >= 32)
        goto finish;
    routed_clock.signal = 1UL << reload_signal;
    if ((routed_clock.signal & SIGBREAKF_CTRL_C) ||
        !pt_diagnostic_cia_acquire(&clock, pt_cia_reload_irq, &routed_clock) ||
        !open_alarm(&alarm, &clock, now + hz, handler) ||
        (routed_clock.signal & (alarm.data.signal | alarm.deadline_signal)))
        goto finish;
#else
        !pt_diagnostic_cia_acquire(&clock, pt_cia_direct_clock_stray, (void *)&timer_strays) ||
        !open_alarm(&alarm, &clock, now + hz, handler))
        goto finish;
#endif

    alarm_chip = alarm.timer.chip;
    alarm_timer = alarm.timer.bit;
#ifndef PT_PAULA_WAIT_IRQ
    /* On a single CPU this short diagnostic excludes allocation by other
     * tasks. The busy waits leave hardware IRQs enabled and cannot reschedule
     * the caller. The handler also checks the original audio lock and DMA. */
    Forbid();
#endif
#ifdef PT_PAULA_TAIL_IRQ
    if (pt_diagnostic_cia_arm_mode(&clock, UINT16_MAX, 1)) {
        struct pt_cia_clock_pair anchor;
        uint64_t stop_offset, first, end;
        pt_cia_direct_clock_pair(clock.high, clock.low, &anchor);
        timeline.count = pt_cia_stable_count(anchor.after);
        if (anchor.marker != 0x2452 || routed_clock.reloads ||
            pt_elapsed_clock_init(&grid, hz, PAULA_TEST_RATE, 0, 0) != PT_ELAPSED_OK ||
            pt_elapsed_clock_deadline(&grid, schedule->stop_frame, &stop_offset) != PT_ELAPSED_OK ||
            stop_offset + MIXED_TAIL_FIRST >= timeline.count)
            goto finish;
        schedule->epoch = timeline.count - stop_offset - MIXED_TAIL_FIRST;
        if (pt_elapsed_clock_init(&grid, hz, PAULA_TEST_RATE, schedule->epoch, 0) != PT_ELAPSED_OK ||
            !mixed_prepare_words(&grid, start, stop.frame))
            goto finish;
        for (unsigned i = 0; i < 2; ++i) {
            timed[i].anchor_sample = anchor;
            timed[i].anchor = timeline.count;
            timed[i].chip = clock.chip;
            timed[i].timer = clock.bit;
            timed[i].hz = hz;
        }
        if (!prepare_paula_window(timed, &grid, schedule->start_frame) ||
            pt_elapsed_clock_deadline(&grid, schedule->stop_frame, &first) != PT_ELAPSED_OK ||
            pt_elapsed_clock_deadline(&grid, schedule->stop_frame + 1, &end) != PT_ELAPSED_OK ||
            end <= (uint64_t)timeline.count + 1 ||
            !pt_countdown_window_intersect(&timeline, first, end, MIXED_TAIL_SPAN, &timed[1].window) ||
            timed[1].window.first_count != MIXED_TAIL_FIRST || timed[1].window.end_count != 1)
            goto finish;
        timed[1].raw.first = timed[1].window.first_count;
        timed[1].raw.end = timed[1].window.end_count;
        if (run_irq_word(owner, &alarm, &clock, start, 0, ALL)) {
            Delay(1);
            if (!routed_clock.reloads && observe_started_fetches(owner, &clock, MIXED_TAIL_FIRST + 512)) {
                stage = 6;
                passed = run_irq_word(owner, &alarm, &clock, stop.frame, ALL, 0);
            }
        }
    }
#elif defined(PT_PAULA_RELOAD_IRQ)
    if (pt_diagnostic_cia_arm_mode(&clock, UINT16_MAX, 1) &&
        capture_epoch(timed, &clock, &grid, schedule->start_frame, 0, &timeline) &&
        run_irq_word(owner, &alarm, &clock, start, 0, ALL)) {
        Delay(1);
        if (!routed_clock.reloads && observe_started_fetches(owner, &clock, CLOCK_RELOAD_MARGIN) &&
            Wait(routed_clock.signal | alarm.deadline_signal | SIGBREAKF_CTRL_C) == routed_clock.signal &&
            capture_epoch(timed + 1, &clock, &grid, schedule->stop_frame, 1, &timeline)) {
            stage = 6;
            passed = run_irq_word(owner, &alarm, &clock, stop.frame, ALL, 0);
        }
    }
#else
    if (pt_cia_start_masked_countdown(&clock)) {
        Disable();
        pt_cia_direct_clock_pair(clock.high, clock.low, &anchor);
        Enable();
        for (unsigned i = 0; i < 2; ++i) {
            timed[i].anchor_sample = anchor;
            timed[i].anchor = pt_cia_stable_count(anchor.after);
            timed[i].chip = clock.chip;
            timed[i].timer = clock.bit;
            timed[i].hz = hz;
        }
        if (anchor.marker == 0x2452 && timed[0].anchor &&
            prepare_paula_window(timed, &grid, schedule->start_frame) &&
            prepare_paula_window(timed + 1, &grid, schedule->stop_frame) &&
            run_irq_word(owner, &alarm, &clock, start, 0, ALL)) {
#ifdef PT_PAULA_WAIT_IRQ
            Delay(1);
#endif
            if (observe_started_fetches(owner, &clock, (unsigned)timed[1].raw.first + 512)) {
                stage = 6;
                passed = run_irq_word(owner, &alarm, &clock, stop.frame, ALL, 0);
            }
        }
    }
    if (!pt_diagnostic_cia_close(&alarm.timer)) {
#ifndef PT_PAULA_WAIT_IRQ
        Permit();
#endif
        return hold();
    }
#ifndef PT_PAULA_WAIT_IRQ
    Permit();
#endif

#endif

finish:
    if (!close_alarm(&alarm) || !pt_diagnostic_cia_close(&clock))
        return hold();
#ifdef PT_PAULA_RELOAD_IRQ
    if (reload_signal >= 0 && reload_signal < 32) {
        SetSignal(0, routed_clock.signal);
        FreeSignal(reload_signal);
    }
    passed = passed && routed_clock.reloads == 1;
#endif
    pt_native_eclock_close(&timebase);
    alarm_calls = alarm.data.calls;
    alarm_strays = alarm.data.stray;
    alarm_closed = 1;
    return passed && alarm_calls == 2 && !alarm_strays && !timer_strays &&
        !alarm.timer.held && !clock.held && !timebase.opened;
}

static void report_irq_transitions(void)
{
    for (unsigned i = 0; i < timed_count; ++i) {
        const struct paula_alarm_row *row = armed_at + i;
        const uint8_t *raw = row->arm.after;
        printf("Paula alarm %u sample=%02x%02x%02x lead=%u delay=%u calls=%u\n", i,
            (unsigned)raw[0], (unsigned)raw[1], (unsigned)raw[2], row->lead_count, row->delay, row->calls);
#ifdef PT_PAULA_WAIT_IRQ
        const uint8_t *before = row->wait_before.after, *after = row->wait_after.after;
        printf("Paula wait %u before=%02x%02x%02x after=%02x%02x%02x alarm=%u\n", i,
            (unsigned)before[0], (unsigned)before[1], (unsigned)before[2],
            (unsigned)after[0], (unsigned)after[1], (unsigned)after[2], row->alarm_signal_only);
#ifdef PT_PAULA_TAIL_IRQ
        printf("Paula tail samples %u before=%u after=%u\n", i, row->before_attempts, row->after_attempts);
#endif
#endif
    }
#ifdef PT_PAULA_RELOAD_IRQ
    for (unsigned i = 0; i < timed_count; ++i)
        printf("Paula reload %u tick=%lu planned=%u issued=%u\n", i,
            (unsigned long)timed[i].anchor_tick, timed[i].reload, armed_at[i].issued_reload);
    printf("Paula reload clock callbacks=%lu\n", (unsigned long)routed_clock.reloads);
#endif
    printf("Paula alarm closed=%u chip=%u timer=%u calls=%u stray=%u\n",
        alarm_closed, alarm_chip, alarm_timer, alarm_calls, alarm_strays);
}
#endif
