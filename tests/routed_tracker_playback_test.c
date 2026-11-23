#define PT_ROUTED_COMBINED_TEST_MAIN other_combined_cases
#include "routed_midi_combined_test.c"
#include "../src/core/routed_playback.h"
#include "../src/editor/routed_preflight.h"
#include "../src/editor/sampler_mixed_readers.h"

static const struct pt_render_options options = {
    .rate = 48000, .bits = 16, .tracks = 0x801f, .gain_q16 = 65536, .tick_limit = 64, .frame_limit = 100000
};

static void tracker_project(struct trial *t, int32_t pcm[2][1024], unsigned short_card)
{
    struct pt_project *p = &t->resources->document.project;
    unsigned i, j;
    p->bpm = 125;
    p->speed = 2;
    for (i = 0; i < 2; ++i) {
        const unsigned frames = i && short_card ? 64 : 1024;
        for (j = 0; j < 1024; ++j)
            pcm[i][j] = t->resources->original[i][j % 64];
        p->samples[i].pcm = (struct pt_pcm){pcm[i], frames, frames, 8000, 1, 8};
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

static enum pt_routed_preflight_result audit_song(const struct pt_project *project,
    const struct pt_allocator *allocator, void *normalizer, size_t normalizer_bytes,
    struct pt_routed_preflight_report *report, struct pt_render_sequence **sequence)
{
    const struct pt_routed_preflight_inputs inputs = {
        .project = project, .options = options, .caps = {3546895, 124, 65535}, .format = {8, 0, 0, 0},
        .allocator = *allocator, .normalizer_workspace = normalizer, .normalizer_capacity = normalizer_bytes,
        .revision = 37, .generation = 17
    };
    const size_t bytes = pt_routed_preflight_workspace_size();
    void *workspace = calloc(1, bytes);
    struct pt_routed_preflight *audit = NULL;
    struct pt_render_sequence *second = NULL;
    enum pt_routed_preflight_result result;
    unsigned steps = 0;
    assert(workspace);
    assert(pt_routed_preflight_begin(workspace, bytes, &inputs, &audit) == PT_ROUTED_PREFLIGHT_PENDING);
    assert(!pt_routed_preflight_take(audit, 37, 17, sequence) && !*sequence);
    report->frame = UINT64_MAX;
    assert(pt_routed_preflight_step(audit, 38, 17, 73, report) == PT_ROUTED_PREFLIGHT_STALE);
    assert(report->frame == UINT64_MAX);
    do {
        assert(++steps < 1024);
        result = pt_routed_preflight_step(audit, 37, 17, 73, report);
    } while (result == PT_ROUTED_PREFLIGHT_PENDING);
    if (result == PT_ROUTED_PREFLIGHT_READY) {
        assert(pt_routed_preflight_take(audit, 37, 17, sequence) && *sequence);
        assert(!pt_routed_preflight_take(audit, 37, 17, &second) && !second);
    } else {
        assert(!pt_routed_preflight_take(audit, 37, 17, sequence) && !*sequence);
    }
    assert(pt_routed_preflight_close(&audit) && !audit);
    free(workspace);
    return result;
}

static void refuse_late_paula_loop(void)
{
    struct trial *t = trial_make(8, 8);
    struct pt_project *project = &t->resources->document.project;
    struct pt_render_sequence *sequence = NULL;
    struct pt_routed_preflight_report report = {0};
    struct allocation render_memory = {0};
    const struct pt_allocator allocator = {&render_memory, ordinary, ordinary_release};
    const size_t capacity = pt_mixed_plan_normalizer_workspace_size();
    void *normalizer = calloc(1, capacity);
    int32_t (*pcm)[1024] = malloc(2 * sizeof(*pcm));
    size_t bytes;
    uint8_t *before;
    assert(normalizer && pcm);
    tracker_project(t, pcm, 0);
    project->samples[1].loop = PT_LOOP_FORWARD;
    project->samples[1].loop_start = 8;
    project->samples[1].loop_end = 64;
    project->events[project->channels.count] = (struct pt_event){
        .kind = PT_NOTE_PERIOD, .pitch = 428, .instrument = 2
    };
    project->events[2 * project->channels.count] = (struct pt_event){.effect = 15};
    before = save(t->resources, &bytes);
    assert(audit_song(project, &allocator, normalizer, capacity, &report, &sequence) == PT_ROUTED_PREFLIGHT_REFUSED);
    assert(!sequence && report.frame == 1920 && report.audio_boundaries == 2 && report.render == PT_RENDER_OK);
    assert(report.plan.result == PT_MIXED_PLAN_REFUSED && report.plan.reason == PT_MIXED_PLAN_REASON_PAULA &&
           report.plan.track == 0);
    assert(render_memory.calls == 1 && render_memory.releases == 1);
    assert(!t->model.reads && !t->model.submits && !t->model.effects && !t->resources->card.writes && !t->resources->chips);
    same_save(t->resources, before, bytes);
    trial_drop(t);
    free(before);
    free(normalizer);
    free(pcm);
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

static void check_actions(const struct trial *t, const struct pt_mixed_plan_batch *batch,
                          const struct model_command *command)
{
    unsigned i;
    assert(command->batch.count == batch->count);
    for (i = 0; i < batch->count; ++i) {
        const struct pt_mixed_plan_record *record = batch->record + i;
        const struct pt_mixed_readers_action *action = command->batch.action + i;
        assert(action->slot == record->slot && action->route ==
            (record->route == PT_PAULA ? PT_MIXED_READERS_PAULA : PT_MIXED_READERS_AMIGUS));
        if (record->kind == PT_MIXED_PLAN_STOP) {
            assert(action->kind == PT_MIXED_READERS_STOP);
        } else if (record->route == PT_PAULA) {
            assert(action->geometry.paula.period == record->geometry.paula.period);
            assert(action->geometry.paula.volume == (record->kind == PT_MIXED_PLAN_TRIGGER ? 64 : 63));
            if (record->kind == PT_MIXED_PLAN_TRIGGER) {
                const uint8_t *data = action->geometry.paula.data;
                assert(action->geometry.paula.words == 512 && t->resources->chips == 1);
                assert(data[0] == t->resources->original[0][0] && data[1023] == t->resources->original[0][63]);
            }
        } else {
            const struct pt_amigus_voice_plan *image = &action->geometry.amigus;
            if (record->kind == PT_MIXED_PLAN_TRIGGER) {
                assert(image->rate == record->image.rate && image->left == record->image.left &&
                       image->right == record->image.right && image->end_exclusive - image->start ==
                       t->resources->document.project.samples[1].pcm.frames);
                assert(t->resources->card.ram[image->start] == t->resources->original[1][0] &&
                       t->resources->card.ram[image->end_exclusive - 1] == t->resources->original[1][63]);
            } else {
                assert(image->rate == record->geometry.amigus.rate &&
                       image->left == record->geometry.amigus.left && image->right == record->geometry.amigus.right);
            }
        }
    }
}

static enum pt_mixed_readers_result observe_retirement(void *context,
    const struct pt_mixed_readers_key *key, struct pt_mixed_readers_reader_receipt *receipt)
{
    struct combined_model *m = context;
    return pt_sampler_mixed_service_reader(m->pool, key->trigger, key->action, 0, receipt);
}

static void tracker_case(unsigned short_card, unsigned retrigger)
{
    struct allocation render_memory = {0};
    const struct pt_allocator allocator = {&render_memory, ordinary, ordinary_release};
    struct pt_routed_preflight_report audit_report = {0};
    struct trial *t = trial_make(8, 8);
    struct pt_project *project = &t->resources->document.project;
    struct pt_render_sequence *sequence = NULL;
    struct pt_render_lookahead *ahead = calloc(1, sizeof(*ahead));
    struct pt_routed_render_plan *plan = calloc(1, sizeof(*plan));
    struct pt_routed_midi *midi = calloc(1, sizeof(*midi));
    struct pt_sampler_mixed_normalized_batch *batch = calloc(1, sizeof(*batch));
    struct pt_sampler_mixed_pool *pool = NULL;
    struct pt_sampler_mixed_command_handle handle = {0};
    struct pt_sampler_mixed_reader_handle readers[5] = {{0}};
    struct pt_mixed_readers_key keys[5] = {{0}};
    struct pt_sampler_mixed_reader_handle replacement = {0};
    struct pt_mixed_readers_key original_card = {0};
    struct allocation factory_allocator = {0};
    const size_t factory_capacity = pt_sampler_mixed_workspace_size();
    void *factory_workspace = calloc(1, factory_capacity);
    enum pt_sampler_mixed_result factory_result;
    struct pt_mixed_plan_origin origins[PT_MIXED_PLAN_RECORDS] = {{0}};
    struct pt_sample_version *master[PT_PROJECT_SAMPLES] = {NULL};
    struct pt_routed_midi_schedule schedule = {0};
    struct pt_routed_playback_frame playback = {0};
    struct combined_model m = {.audio = &t->model, .schedule = &schedule};
    const struct pt_routed_midi_provider api = {
        .context = &m, .context_bytes = sizeof(m), .read_clock = combined_clock,
        .observe = observe_combined, .submit_combined = publish_combined,
        .observe_audio_retirement = observe_retirement
    };
    size_t capacity = pt_mixed_plan_normalizer_workspace_size(), saved_size;
    void *workspace = calloc(1, capacity);
    int32_t (*pcm)[1024] = malloc(2 * sizeof(*pcm));
    uint8_t *saved;
    uint64_t original = 0;
    unsigned ready = 0, boundary, idle_ticks = 0, effects = 0, i, steps;
    assert(ahead && plan && midi && batch && workspace && factory_workspace && pcm);
    tracker_project(t, pcm, short_card);
    if (retrigger) {
        project->events[4].effect = 14;
        project->events[4].parameter = 0x91;
    }
    for (i = 0; i < project->sample_count; ++i) {
        struct pt_pcm pinned;
        struct pt_sampler *sampler = &t->resources->sampler;
        assert(pt_sampler_pin(sampler, project, i, sampler->generation, &pinned, master + i) == PT_EDIT_OK);
    }
    saved = save(t->resources, &saved_size);
    assert(audit_song(project, &allocator, workspace, capacity, &audit_report, &sequence) == PT_ROUTED_PREFLIGHT_READY);
    assert(audit_report.frame == 1920 && audit_report.audio_boundaries == 2);
    assert(render_memory.calls == 1 && !render_memory.releases && render_memory.last == sequence);
    assert(!t->model.reads && !t->model.submits && !t->resources->card.writes && !t->resources->chips);
    assert(pt_routed_midi_init(midi, project, options.tracks));
    assert(pt_routed_midi_schedule_init(&schedule, midi, &api, 19, 17, 100, 709379, options.rate));
    t->config.backend = (struct pt_mixed_readers_backend){
        &m, sizeof(m), PT_MIXED_READERS_VERSION, PT_MIXED_READERS_REQUIRED,
        combined_clock, publish_audio, observe_audio, observe_reader
    };
    t->model.ticks = 99;
    open_trial(t);
    const struct pt_sampler_mixed_config config = {
        .allocator = {&factory_allocator, ordinary, ordinary_release},
        .sampler = &t->resources->sampler, .project = project, .queue = t->queue,
        .backend = &t->resources->card.cache,
        .chip_context = &t->resources->chips, .chip_allocate = chip_new, .chip_release = chip_drop,
        .control_budget = pt_sampler_mixed_pool_size() + pt_sampler_mixed_command_size() +
                          (5 + retrigger) * pt_sampler_mixed_reader_size(),
        .chip_budget = 4096, .generation = 17, .maximum_commands = 1, .maximum_readers = 5 + retrigger,
        .context_count = 5,
        .contexts = {{&factory_allocator, sizeof(factory_allocator)},
                     {&t->resources->chips, sizeof(t->resources->chips)},
                     {&t->resources->card, sizeof(t->resources->card)},
                     {&t->allocator, sizeof(t->allocator)}, {&m, sizeof(m)}}
    };
    assert(pt_sampler_mixed_open(&config, 37, factory_workspace, factory_capacity, &pool) == PT_SAMPLER_MIXED_PENDING);
    m.pool = pool;
    steps = 0;
    do {
        assert(++steps < 1024);
        factory_result = pt_sampler_mixed_advance_validation(pool, 37, 256);
    } while (factory_result == PT_SAMPLER_MIXED_PENDING);
    assert(factory_result == PT_SAMPLER_MIXED_OK && !t->model.submits && !t->resources->card.writes);

    for (boundary = 0; boundary < 3;) {
        struct pt_render_interval interval;
        const struct pt_routed_midi_batch *midi_batch;
        struct pt_mixed_readers_command_receipt receipt;
        struct pt_mixed_readers_stop_keys stop = {0};
        struct pt_mixed_plan_batch *normalized = &batch->plan.normalized;
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
        const unsigned allocations = factory_allocator.calls;
        const unsigned count = short_card && boundary ? 4 : 5;
        if (plan->end) {
            assert(plan->end == PT_RENDER_F00 + 1 && !plan->audio.count);
            assert(pt_routed_midi_schedule_stop_keys(&schedule, &stop));
            assert(stop.frame == plan->frame && stop.count == count);
            for (i = 0; i < stop.count; ++i)
                assert(keys_equal(stop.key + i, keys + i));
            ++stop.key[0].serial;
            assert(pt_sampler_mixed_begin_stop_keys(pool, 37, &stop, &handle) == PT_SAMPLER_MIXED_INVALID);
            --stop.key[0].serial;
            if (retrigger) {
                stop.key[4] = original_card;
                assert(pt_sampler_mixed_begin_stop_keys(pool, 37, &stop, &handle) == PT_SAMPLER_MIXED_INVALID);
                stop.key[4] = keys[4];
            }
            assert(!handle.address && !handle.token && factory_allocator.calls == allocations);
            assert(pt_sampler_mixed_begin_stop_keys(pool, 37, &stop, &handle) == PT_SAMPLER_MIXED_PENDING);
        } else {
            assert(!pt_routed_midi_schedule_stop_keys(&schedule, &stop) && !stop.count);
            memset(batch, 0, sizeof(*batch));
            normalize(project, plan, origins, &batch->plan, workspace, capacity);
            assert(normalized->count == count);
            for (i = 0; i < normalized->count; ++i) {
                const struct pt_mixed_plan_record *record = normalized->record + i;
                assert(record->track == i && record->sample < 2);
                if (!boundary || (retrigger && record->track == 4)) {
                    assert(record->kind == PT_MIXED_PLAN_TRIGGER);
                    batch->source[i].master = master[record->sample];
                } else {
                    assert(record->kind == PT_MIXED_PLAN_CONTROL);
                    batch->source[i].active = keys[record->track];
                }
            }
            if (!boundary) {
                ++normalized->record[4].slot;
                assert(pt_sampler_mixed_begin_normalized(pool, 37, batch, &handle) == PT_SAMPLER_MIXED_INVALID);
                --normalized->record[4].slot;
                batch->source[4].master = master[0];
                assert(pt_sampler_mixed_begin_normalized(pool, 37, batch, &handle) == PT_SAMPLER_MIXED_INVALID);
                batch->source[4].master = master[1];
            } else {
                ++batch->source[0].active.serial;
                assert(pt_sampler_mixed_begin_normalized(pool, 37, batch, &handle) == PT_SAMPLER_MIXED_INVALID);
                --batch->source[0].active.serial;
            }
            assert(!handle.address && !handle.token && factory_allocator.calls == allocations);
            assert(pt_sampler_mixed_begin_normalized(pool, 37, batch, &handle) == PT_SAMPLER_MIXED_PENDING);
        }
        steps = 0;
        do {
            assert(++steps < 64);
            factory_result = pt_sampler_mixed_advance(pool, 37, handle, NULL);
        } while (factory_result == PT_SAMPLER_MIXED_PENDING);
        assert(factory_result == PT_SAMPLER_MIXED_OK);
        if (!boundary) {
            for (i = 0; i < 5; ++i)
                assert(pt_sampler_mixed_reader(pool, handle, i, readers + i) == PT_SAMPLER_MIXED_OK);
        } else if (retrigger && boundary == 1) {
            assert(pt_sampler_mixed_reader(pool, handle, 4, &replacement) == PT_SAMPLER_MIXED_OK);
        }
        assert(pt_sampler_mixed_enqueue(pool, 37, handle, &ticket) == PT_MIXED_READERS_OK);
        m.expected_audio_count = count;
        assert(pt_sampler_mixed_publish(pool, ticket) == PT_MIXED_READERS_OK);
        const struct model_command *command = model_command(&t->model, ticket);
        if (plan->end) {
            assert(command->batch.count == stop.count);
            for (i = 0; i < stop.count; ++i) {
                assert(command->batch.action[i].kind == PT_MIXED_READERS_STOP);
                assert(keys_equal(command->key + i, stop.key + i));
            }
        } else {
            check_actions(t, normalized, command);
        }
        assert(m.submissions == boundary + 1 && t->model.effects == effects);
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_WAITING);
        fire(&t->model, ticket);
        effects += count;
        assert(pt_sampler_mixed_service_command(pool, ticket, 0, &receipt) == PT_MIXED_READERS_OK);
        record_audio_receipt(&m, &receipt, 1);
        if (!boundary) {
            original = ticket;
            for (i = 0; i < 5; ++i) {
                assert(pt_sampler_mixed_service_reader(pool, original, i, 0, NULL) == PT_MIXED_READERS_PENDING);
                assert(pt_sampler_mixed_reader_key(pool, readers[i], keys + i) == PT_MIXED_READERS_OK);
            }
        } else if (retrigger && boundary == 1) {
            struct pt_mixed_readers_key fresh = {0};
            original_card = keys[4];
            assert(pt_mixed_readers_readers_held(t->queue) == 6);
            assert(pt_sampler_mixed_reader_key(pool, readers[4], &fresh) == PT_MIXED_READERS_STALE);
            assert(pt_sampler_mixed_reader_key(pool, replacement, &fresh) == PT_MIXED_READERS_OK);
            assert(fresh.trigger == ticket && fresh.serial != original_card.serial);
            assert(fresh.route == original_card.route && fresh.slot == original_card.slot);
            assert(pt_sampler_mixed_service_reader(pool, original_card.trigger, original_card.action, 1, NULL) ==
                   PT_MIXED_READERS_OK);
            assert(pt_sampler_mixed_reader_close(pool, readers + 4));
            assert(pt_mixed_readers_readers_held(t->queue) == 5);
            readers[4] = replacement;
            keys[4] = fresh;
            replacement = (struct pt_sampler_mixed_reader_handle){0};
        }
        assert(pt_sampler_mixed_command_close(pool, &handle));
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
            memcpy(origins, normalized->next, sizeof(origins));
        if (short_card && !boundary) {
            const uint32_t before = schedule.audio_owned;
            struct model_reader *card = model_reader(&t->model, keys + 4);
            assert(card && card->state == PT_MIXED_READER_ACTIVE);
            assert(pt_routed_midi_schedule_observe_audio_retirement(&schedule, PT_MIXED_READERS_AMIGUS, 0) ==
                   PT_MIXED_READERS_PENDING && schedule.audio_owned == before);
            t->model.ticks = oracle(640);
            card->state = PT_MIXED_READER_RETIRED;
            assert(pt_routed_midi_schedule_observe_audio_retirement(&schedule, PT_MIXED_READERS_AMIGUS, 0) ==
                   PT_MIXED_READERS_OK);
            assert(schedule.audio_owned == (before & ~(UINT32_C(1) << PT_ROUTED_PAULA_SLOTS)));
            assert(pt_routed_midi_schedule_observe_audio_retirement(&schedule, PT_MIXED_READERS_AMIGUS, 0) ==
                   PT_MIXED_READERS_INVALID);
            assert(pt_sampler_mixed_reader_close(pool, readers + 4));
            memset(origins + 4, 0, sizeof(*origins));
        }
        ++boundary;
    }
    assert(idle_ticks == 1 && midi->stopped && !schedule.held && !schedule.audio_owned);
    assert(effects == (short_card ? 13 : 15) && t->model.effects == effects);
    assert(pt_routed_playback_abandon(&playback));
    assert(!pt_mixed_readers_commands_held(t->queue));
    for (i = 0; i < (short_card ? 4U : 5U); ++i) {
        assert(pt_sampler_mixed_service_reader(pool, keys[i].trigger, keys[i].action, 1, NULL) == PT_MIXED_READERS_OK);
        assert(pt_sampler_mixed_reader_close(pool, readers + i));
    }
    assert(pt_sampler_mixed_close(&pool) && !pool && !t->resources->chips);
    assert(factory_allocator.calls == factory_allocator.releases);
    assert(!pt_mixed_readers_readers_held(t->queue));
    same_save(t->resources, saved, saved_size);
    pt_render_sequence_close(sequence);
    assert(render_memory.releases == 1);
    for (i = 0; i < project->sample_count; ++i)
        pt_sampler_unpin(master[i]);
    trial_drop(t);
    free(pcm);
    free(saved);
    free(workspace);
    free(factory_workspace);
    free(batch);
    free(midi);
    free(plan);
    free(ahead);
}

int main(void)
{
    refuse_late_paula_loop();
    tracker_case(0, 0);
    tracker_case(1, 0);
    tracker_case(0, 1);
    puts("Routed tracker playback: whole-song audit, exact Stop, independent retirement and AmiGUS retrigger; SOFTWARE_ONLY");
    return 0;
}
