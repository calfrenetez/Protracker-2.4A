#define PT_ROUTED_COMBINED_TEST_MAIN other_combined_cases
#include "routed_midi_combined_test.c"
#include "../src/core/amigus_trigger_levels.h"
#include "../src/core/routed_playback.h"
#include "../src/editor/mixed_readers_plan_normalize.h"

static void tracker_project(struct trial *t, int32_t pcm[2][1024])
{
    struct pt_project *p = &t->resources->document.project;
    unsigned i, j;
    p->bpm = 125;
    p->speed = 2;
    for (i = 0; i < 2; ++i) {
        for (j = 0; j < 1024; ++j)
            pcm[i][j] = t->resources->original[i][j % 64];
        p->samples[i].pcm = (struct pt_pcm){pcm[i], 1024, 1024, 8000, 1, 8};
    }
    for (i = 0; i < 5; ++i) {
        p->channels.track[i].pan = i == 0 || i == 3 ? 0 : 255;
        p->events[i] = (struct pt_event){.kind = PT_NOTE_PERIOD, .pitch = 428,
            .instrument = i == 4 ? 2 : 1, .effect = 10, .parameter = 1};
    }
    assert(pt_channels_route(&p->channels, 15, PT_MIDI) == PT_CHANNEL_OK);
    p->channels.track[15].midi_channel = 6;
    strcpy(p->midi_output[15], "synth.out");
    p->events[15] = (struct pt_event){.kind = PT_NOTE_MIDI, .pitch = 60, .flags = 1, .velocity = 100};
    p->events[p->channels.count] = (struct pt_event){.effect = 15};
}

static void normalize(const struct pt_project *project, const struct pt_routed_render_plan *plan,
    const struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS],
    struct pt_mixed_plan_quantized_batch *batch, void *workspace, size_t capacity)
{
    const struct pt_paula_render_caps caps = {3546895, 124, 65535};
    const struct pt_playback_format format = {8, 0, 0, 0};
    const struct pt_mixed_plan_inputs inputs = {
        .project = project, .plan = &plan->audio, .origins = origins, .caps = &caps, .format = &format,
        .rate = 48000, .frame = plan->frame, .revision = 37, .generation = 17
    };
    struct pt_mixed_plan_normalizer *job = NULL;
    enum pt_mixed_plan_result result;
    unsigned steps = 0;
    assert(pt_mixed_plan_normalizer_begin_quantized_in_workspace(workspace, capacity, &inputs, &job) == PT_MIXED_PLAN_PENDING);
    do {
        assert(++steps < 64);
        result = pt_mixed_plan_normalizer_step(job, 37, 17, 7);
    } while (result == PT_MIXED_PLAN_PENDING);
    assert(result == PT_MIXED_PLAN_READY);
    assert(pt_mixed_plan_normalizer_get_quantized(job, 37, 17, batch) == PT_MIXED_PLAN_READY);
    assert(pt_mixed_plan_normalizer_close(&job) && !job);
}

static void prepare_trigger(struct trial *t, const struct pt_mixed_plan_record *record,
                            const struct pt_mixed_plan_trigger_levels *levels, unsigned i)
{
    struct pt_mixed_readers_action *action = t->input->batch.action + i;
    struct holder *reader = t->reader + i;
    struct resources *resources = t->resources;
    struct pt_mixed_readers_owner *owner = t->input->reader + i;
    assert(record->kind == PT_MIXED_PLAN_TRIGGER && record->channel == 0);
    *owner = source(reader, resources, action->route, record->slot, record->sample, 1000 + i);
    reader->queue = t->queue;
    action->kind = PT_MIXED_READERS_TRIGGER;
    if (action->route == PT_MIXED_READERS_PAULA) {
        const uint8_t *data;
        size_t length;
        assert(record->track == record->slot && levels->mode == PT_MIXED_PLAN_LEVEL_LEGACY);
        assert(pt_sampler_paula_location(&resources->paula, record->track, reader->lease, &data, &length));
        assert(length == 1024 && record->geometry.paula.volume == 64);
        action->geometry.paula.data = data;
        action->geometry.paula.words = (uint16_t)(length / 2);
        action->geometry.paula.period = record->geometry.paula.period;
        action->geometry.paula.volume = record->geometry.paula.volume;
    } else {
        const struct pt_playback_format format = {record->geometry.amigus.bits, record->channel,
                                                  record->geometry.amigus.little_endian, 0};
        const struct pt_amigus_trigger_levels_request request = {
            record->geometry.amigus.trigger, levels->left, levels->right
        };
        const struct pt_sample *sample = resources->document.project.samples + record->sample;
        assert(record->track == 4 && levels->mode == PT_MIXED_PLAN_LEVEL_QUANTIZED);
        assert(pt_amigus_trigger_levels_prepare(sample, &format, &request, owner->card.address,
                                                owner->card.logical_bytes, &action->geometry.amigus));
        assert(action->geometry.amigus.rate == record->image.rate &&
               action->geometry.amigus.left == record->image.left && action->geometry.amigus.right == record->image.right);
    }
}

static uint64_t publish_plan(struct trial *t, const struct pt_mixed_plan_quantized_batch *batch,
                             uint64_t original, unsigned command)
{
    unsigned i;
    uint64_t ticket;
    assert(batch->normalized.count == 5);
    memset(t->input, 0, sizeof(*t->input));
    holder_init(t->command + command, 100 + command);
    t->input->command = control_of(t->command + command);
    t->input->batch.frame = batch->normalized.frame;
    t->input->batch.generation = 17;
    t->input->batch.count = batch->normalized.count;
    for (i = 0; i < batch->normalized.count; ++i) {
        const struct pt_mixed_plan_record *record = batch->normalized.record + i;
        struct pt_mixed_readers_action *action = t->input->batch.action + i;
        action->route = record->route == PT_PAULA ? PT_MIXED_READERS_PAULA : PT_MIXED_READERS_AMIGUS;
        action->slot = record->slot;
        if (!original) {
            prepare_trigger(t, record, batch->levels + i, i);
            continue;
        }
        assert(record->kind == PT_MIXED_PLAN_CONTROL);
        assert(pt_mixed_readers_reader_key(t->queue, original, i, t->input->target + i) == PT_MIXED_READERS_OK);
        action->kind = PT_MIXED_READERS_CONTROL;
        if (action->route == PT_MIXED_READERS_PAULA) {
            action->geometry.paula.period = record->geometry.paula.period;
            action->geometry.paula.volume = record->geometry.paula.volume;
            assert(action->geometry.paula.volume == 63);
        } else {
            action->geometry.amigus.rate = record->geometry.amigus.rate;
            action->geometry.amigus.left = record->geometry.amigus.left;
            action->geometry.amigus.right = record->geometry.amigus.right;
        }
    }
    assert(pt_mixed_readers_enqueue(t->queue, t->input, &ticket) == PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_publish(t->queue, ticket) == PT_MIXED_READERS_OK);
    return ticket;
}

int main(void)
{
    const struct pt_render_options options = {
        .rate = 48000, .bits = 16, .tracks = 0x801f, .gain_q16 = 65536, .tick_limit = 64, .frame_limit = 100000
    };
    const struct pt_allocator allocator = {NULL, master_alloc, master_free};
    struct trial *t = trial_make(8, 8);
    struct pt_project *project = &t->resources->document.project;
    struct pt_render_sequence *sequence = NULL;
    struct pt_render_lookahead *ahead = calloc(1, sizeof(*ahead));
    struct pt_routed_render_plan *plan = calloc(1, sizeof(*plan));
    struct pt_routed_midi *midi = calloc(1, sizeof(*midi));
    struct pt_mixed_plan_quantized_batch *batch = calloc(1, sizeof(*batch));
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS] = {{0}};
    struct pt_sample_version *master[2] = {NULL, NULL};
    struct pt_routed_midi_schedule schedule = {0};
    struct pt_routed_playback_frame playback = {0};
    struct combined_model m = {.audio = &t->model, .schedule = &schedule};
    const struct pt_routed_midi_provider api = {
        .context = &m, .context_bytes = sizeof(m), .read_clock = combined_clock,
        .observe = observe_combined, .submit_combined = publish_combined
    };
    size_t capacity = pt_mixed_plan_normalizer_workspace_size(), saved_size;
    void *workspace = calloc(1, capacity);
    int32_t (*pcm)[1024] = malloc(2 * sizeof(*pcm));
    uint8_t *saved;
    uint64_t original = 0;
    unsigned ready = 0, boundary, idle_ticks = 0, i;
    assert(ahead && plan && midi && batch && workspace && pcm);
    tracker_project(t, pcm);
    for (i = 0; i < 2; ++i) {
        struct pt_pcm pinned;
        struct pt_sampler *sampler = &t->resources->sampler;
        assert(pt_sampler_pin(sampler, project, i, sampler->generation, &pinned, master + i) == PT_EDIT_OK);
    }
    saved = save(t->resources, &saved_size);
    assert(pt_render_sequence_begin_routed(project, &options, &allocator, &sequence) == PT_RENDER_OK);
    do {
        assert(pt_render_sequence_prepare(sequence, 2, &ready) == PT_RENDER_OK);
    } while (!ready);
    assert(pt_routed_midi_init(midi, project, options.tracks));
    assert(pt_routed_midi_schedule_init(&schedule, midi, &api, 19, 17, 100, 709379, options.rate));
    t->config.backend = (struct pt_mixed_readers_backend){
        &m, sizeof(m), PT_MIXED_READERS_VERSION, PT_MIXED_READERS_REQUIRED,
        combined_clock, publish_audio, observe_audio, observe_reader
    };
    t->model.ticks = 99;
    open_trial(t);
    for (boundary = 0; boundary < 3;) {
        struct pt_render_interval interval;
        const struct pt_routed_midi_batch *midi_batch;
        uint64_t ticket;
        uint32_t remaining;
        assert(pt_render_sequence_next(sequence, &interval) == PT_RENDER_OK);
        assert(pt_render_lookahead_begin(ahead, sequence) == PT_RENDER_OK);
        ready = 0;
        do {
            assert(pt_render_lookahead_step_routed(ahead, 73, plan, &ready) == PT_RENDER_OK);
        } while (!ready);
        assert(pt_routed_playback_prepare(&playback, ahead, &schedule, plan) == PT_RENDER_OK);
        if (!plan->end && !plan->audio.count && !plan->midi_count) {
            assert(!boundary && !interval.frames && !plan->frame && ++idle_ticks == 1);
            assert(!midi->pending && !m.submissions);
            assert(pt_routed_playback_commit(&playback) == PT_RENDER_OK);
            continue;
        }
        assert(plan->frame == boundary * UINT64_C(960));
        assert(!!plan->end == (boundary == 2) && !!plan->midi_count == (boundary == 0));
        midi_batch = &midi->batch;
        assert(midi->pending && midi_batch->serial == playback.serial && midi_batch->count == (unsigned)(boundary != 1));
        assert(pt_routed_playback_commit(&playback) == PT_RENDER_INVALID);
        assert(pt_routed_midi_schedule_submit_combined(&schedule, NULL) == PT_ROUTED_MIDI_INVALID);
        if (plan->end) {
            assert(plan->end == PT_RENDER_F00 + 1 && !plan->audio.count);
            ticket = publish_stop(t, original, plan->frame, boundary);
        } else {
            normalize(project, plan, origins, batch, workspace, capacity);
            ticket = publish_plan(t, batch, original, boundary);
            if (!original)
                original = ticket;
        }
        assert(m.submissions == boundary + 1 && t->model.effects == boundary * 5);
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_WAITING);
        settle_audio(t, &m, ticket, 1, 0);
        settle_midi(&m, boundary != 1);
        remaining = interval.frames;
        while (remaining) {
            uint32_t chunk = remaining > 73 ? 73 : remaining;
            assert(pt_render_sequence_consume(sequence, chunk) == PT_RENDER_OK);
            remaining -= chunk;
        }
        assert(pt_routed_playback_commit(&playback) == PT_RENDER_INVALID);
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_SETTLED);
        assert(midi->accepted[15].active == (boundary != 2));
        assert(!pt_routed_midi_schedule_settled(&schedule, plan->frame, playback.serial + 1));
        assert(pt_routed_playback_commit(&playback) == PT_RENDER_OK);
        assert(pt_routed_playback_commit(&playback) == PT_RENDER_INVALID);
        if (!plan->end)
            memcpy(origins, batch->normalized.next, sizeof(origins));
        ++boundary;
    }
    assert(idle_ticks == 1 && midi->stopped && !schedule.held && t->model.effects == 15);
    assert(pt_routed_playback_abandon(&playback));
    assert(!pt_mixed_readers_commands_held(t->queue));
    for (i = 0; i < 5; ++i)
        assert(pt_mixed_readers_service_reader(t->queue, original, i, 1, NULL) == PT_MIXED_READERS_OK);
    same_save(t->resources, saved, saved_size);
    pt_render_sequence_close(sequence);
    for (i = 0; i < 2; ++i)
        pt_sampler_unpin(master[i]);
    trial_drop(t);
    free(pcm);
    free(saved);
    free(workspace);
    free(batch);
    free(midi);
    free(plan);
    free(ahead);
    puts("Routed tracker playback: normalized attack, audio-only CONTROL and owned F00 Stop at 0/960/1920; SOFTWARE_ONLY");
    return 0;
}
