#ifndef PT_TEST_NATIVE_TRACKER_PAULA_FORECAST_H
#define PT_TEST_NATIVE_TRACKER_PAULA_FORECAST_H

#include "../src/core/render_lookahead.h"
#include "../src/editor/mixed_readers_plan_normalize.h"
#include <stdlib.h>
#include <string.h>

enum { TRACKER_PAULA_EPOCH = 8192, TRACKER_TEST_TRACKS = 6, TRACKER_TEST_WORDS = 16 };
#ifdef PT_PAULA_TAIL_IRQ
enum { TRACKER_STOP_ROW = 2 };
#elif defined(PT_PAULA_RELOAD_IRQ)
enum { TRACKER_STOP_ROW = 3 };
#else
enum { TRACKER_STOP_ROW = 1 };
#endif

static unsigned forecast_paula, forecast_amigus, forecast_midi, forecast_f00;
static struct pt_mixed_plan_report forecast_normalizer;

static void *forecast_allocate(void *unused, size_t bytes)
{
    (void)unused;
    return malloc(bytes);
}

static void forecast_release(void *unused, void *data)
{
    (void)unused;
    free(data);
}

static int forecast_start(const struct pt_project *project, const struct pt_routed_render_plan *plan,
    const void *chip, struct pt_paula_dma_program *out)
{
    const struct pt_paula_render_caps caps = {3546895, 124, 65535};
    const struct pt_playback_format format = {8, 0, 0, 0};
    const struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS] = {{0}};
    const struct pt_mixed_plan_inputs inputs = {
        .project = project, .plan = &plan->audio, .origins = origins, .caps = &caps, .format = &format,
        .rate = PAULA_TEST_RATE, .frame = plan->frame, .revision = 1, .generation = 1
    };
    const size_t bytes = pt_mixed_plan_normalizer_workspace_size();
    void *space = calloc(1, bytes);
    struct pt_mixed_plan_quantized_batch *batch = malloc(sizeof(*batch));
    struct pt_mixed_plan_normalizer *job = NULL;
    struct pt_routed_activation_program audio = {0};
#ifdef PT_PAULA_MIXED_IRQ
    const struct pt_mixed_plan_record *card = NULL;
#endif
    int passed = 0;
    if (!space || !batch || plan->midi_count != 1 || plan->midi[0].track != 5 ||
        plan->midi[0].kind != PT_NOTE_MIDI || plan->midi[0].note != 60)
        goto finish;
    forecast_midi = plan->midi_count;
    enum pt_mixed_plan_result ready = pt_mixed_plan_normalizer_begin_quantized_in_workspace(space, bytes, &inputs, &job);
    for (unsigned i = 0; i < 64 && ready == PT_MIXED_PLAN_PENDING; ++i)
        ready = pt_mixed_plan_normalizer_step(job, 1, 1, 128);
    if (job)
        (void)pt_mixed_plan_normalizer_report(job, &forecast_normalizer);
    if (ready != PT_MIXED_PLAN_READY ||
        pt_mixed_plan_normalizer_get_quantized(job, 1, 1, batch) != PT_MIXED_PLAN_READY ||
        batch->normalized.frame != plan->frame || batch->normalized.count != 5)
        goto finish;
    for (unsigned i = 0; i < batch->normalized.count; ++i) {
        const struct pt_mixed_plan_record *record = batch->normalized.record + i;
        if (record->kind != PT_MIXED_PLAN_TRIGGER || record->sample || record->channel)
            goto finish;
        if (record->route == PT_AMIGUS && record->track == 4) {
#ifdef PT_PAULA_MIXED_IRQ
            card = record;
#endif
            ++forecast_amigus;
            continue;
        }
        if (record->route != PT_PAULA || record->slot >= 4 || record->track >= 4)
            goto finish;
        struct pt_routed_audio_program *action = audio.audio + audio.audio_count++;
        *action = (struct pt_routed_audio_program){
            .route = PT_MIXED_READERS_PAULA, .slot = record->slot, .kind = PT_MIXED_READERS_TRIGGER,
            .output.paula = {(uintptr_t)chip + record->slot * TRACKER_TEST_WORDS * 2, TRACKER_TEST_WORDS,
                             record->geometry.paula.period, record->geometry.paula.volume}
        };
    }
    forecast_paula = audio.audio_count;
    passed = forecast_paula == 4 && forecast_amigus == 1 &&
        pt_routed_paula_dma_prepare(&audio, ALL, out) && out->preload_slots == ALL;
#ifdef PT_PAULA_MIXED_IRQ
    passed = passed && mixed_forecast_start(&audio, card, plan);
#endif
finish:
    if (!pt_mixed_plan_normalizer_close(&job))
        return hold();
    free(batch);
    free(space);
    return passed;
}

static int prepare_tracker_forecast(const void *chip, struct pt_paula_dma_program *out,
    struct paula_alarm_schedule *schedule)
{
    int32_t pcm[2 * TRACKER_TEST_WORDS] = {0};
    uint16_t order = 0;
    struct pt_event events[PT_PROJECT_ROWS * TRACKER_TEST_TRACKS] = {0};
    /* A silent classic 8-bit source deliberately forecasts a segment and
     * repeat. Use a silent 16-bit master for the supported one-shot shape.
     * Its conversion to the owned mono 8-bit Chip bytes is identically zero. */
    struct pt_sample sample = {.pcm = {pcm, 2 * TRACKER_TEST_WORDS, 2 * TRACKER_TEST_WORDS, 8000, 1, 16}, .volume = 32};
    struct pt_project project = {.order_count = 1, .pattern_count = 1, .sample_count = 1,
        .bpm = 125, .speed = 2, .orders = &order, .events = events, .samples = &sample};
    const struct pt_render_options options = {.rate = PAULA_TEST_RATE, .bits = 16, .tracks = 63,
        .gain_q16 = 65536, .tick_limit = 32, .frame_limit = 10000};
    const struct pt_allocator allocator = {NULL, forecast_allocate, forecast_release};
    struct pt_render_sequence *sequence = NULL;
    struct pt_render_lookahead *lookahead = calloc(1, sizeof(*lookahead));
    struct pt_routed_render_plan *plan = malloc(sizeof(*plan));
    unsigned ready = 0;
    int started = 0, passed = 0;

    pt_channels_init(&project.channels);
    if (!lookahead || !plan || pt_channels_resize(&project.channels, TRACKER_TEST_TRACKS) != PT_CHANNEL_OK ||
        pt_channels_route(&project.channels, 5, PT_MIDI) != PT_CHANNEL_OK)
        goto finish;
#ifdef PT_PAULA_MIXED_IRQ
    memcpy(project.midi_output[5], "synth.a", sizeof("synth.a"));
    if (!mixed_forecast_open(&project))
        goto finish;
#else
    memcpy(project.midi_output[5], "test.forecast", sizeof("test.forecast"));
#endif
    for (unsigned i = 0; i < 5; ++i)
        events[i] = (struct pt_event){.kind = PT_NOTE_PERIOD, .pitch = 428, .instrument = 1};
    events[5] = (struct pt_event){.kind = PT_NOTE_MIDI, .pitch = 60, .flags = 1, .velocity = 90};
    events[TRACKER_STOP_ROW * TRACKER_TEST_TRACKS].effect = 15; /* F00 sets the original termination boundary. */

    if (pt_render_sequence_begin_routed(&project, &options, &allocator, &sequence) != PT_RENDER_OK ||
        pt_render_sequence_prepare(sequence, options.tick_limit, &ready) != PT_RENDER_OK || !ready)
        goto finish;
    for (unsigned i = 0; i < 8; ++i) {
        struct pt_render_interval interval;
        ready = 0;
        if (pt_render_sequence_next(sequence, &interval) != PT_RENDER_OK ||
            pt_render_lookahead_begin(lookahead, sequence) != PT_RENDER_OK)
            goto finish;
        for (unsigned step = 0; step < 16 && !ready; ++step)
            if (pt_render_lookahead_step_routed(lookahead, 256, plan, &ready) != PT_RENDER_OK)
                goto finish;
        if (!ready)
            goto finish;
        if (!plan->end && plan->midi_count) {
            if (started || !forecast_start(&project, plan, chip, out))
                goto finish;
            schedule->start_frame = plan->frame;
            started = 1;
        }
        for (uint32_t remaining = interval.frames; remaining;) {
            const unsigned chunk = remaining > 256 ? 256 : remaining;
            if (pt_render_sequence_consume(sequence, chunk) != PT_RENDER_OK)
                goto finish;
            remaining -= chunk;
        }
        if (pt_render_lookahead_commit(lookahead) != PT_RENDER_OK)
            goto finish;
        if (plan->end) {
            schedule->stop_frame = plan->frame;
            forecast_f00 = 1;
            passed = started && schedule->stop_frame > schedule->start_frame;
#ifdef PT_PAULA_MIXED_IRQ
            passed = passed && mixed_forecast_stop(schedule->stop_frame);
#endif
            break;
        }
    }
finish:
    if (lookahead)
        pt_render_lookahead_cancel(lookahead);
    if (sequence)
        pt_render_sequence_close(sequence);
    free(plan);
    free(lookahead);
    return passed;
}

static void report_tracker_forecast(const struct paula_alarm_schedule *schedule)
{
    if (forecast_normalizer.result != PT_MIXED_PLAN_READY)
        printf("Paula forecast normalization result=%u phase=%u reason=%u track=%u\n",
            (unsigned)forecast_normalizer.result, (unsigned)forecast_normalizer.phase,
            (unsigned)forecast_normalizer.reason, forecast_normalizer.track);
    printf("Paula tracker epoch=%lu start=%lu stop=%lu paula=%u amigus=%u midi=%u f00=%u\n",
        (unsigned long)schedule->epoch, (unsigned long)schedule->start_frame, (unsigned long)schedule->stop_frame,
        forecast_paula, forecast_amigus, forecast_midi, forecast_f00);
}
#endif
