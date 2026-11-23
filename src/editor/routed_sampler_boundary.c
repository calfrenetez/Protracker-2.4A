#include <string.h>
#include "routed_sampler_boundary.h"

static int available(const struct pt_routed_sampler_boundary *boundary,
    const struct pt_routed_sampler_config *config, const struct pt_routed_midi_schedule *schedule)
{
    return boundary && !boundary->owner && config && config->pool && config->project &&
        config->masters && config->origins && schedule && config->generation == schedule->event.key.generation &&
        config->rate == schedule->grid.rate && schedule->provider.submit_combined;
}

static uint64_t serial(const struct pt_routed_sampler_boundary *boundary)
{
    return boundary->stop_serial ? boundary->stop_serial : boundary->playback.serial;
}

static enum pt_routed_sampler_result prepare_stop(struct pt_routed_sampler_boundary *boundary)
{
    if (!pt_routed_midi_schedule_stop_keys(boundary->schedule, &boundary->stop))
        boundary->phase = PT_ROUTED_SAMPLER_REFUSED;
    else
        boundary->phase = boundary->stop.count ? PT_ROUTED_SAMPLER_PENDING : PT_ROUTED_SAMPLER_READY;
    return boundary->phase;
}

enum pt_routed_sampler_result pt_routed_sampler_begin(struct pt_routed_sampler_boundary *boundary,
    const struct pt_routed_sampler_config *config, struct pt_render_lookahead *lookahead,
    struct pt_routed_midi_schedule *schedule)
{
    if (!available(boundary, config, schedule) || !lookahead ||
        !pt_render_sequence_routed_source_matches(lookahead->sequence, config->project, config->rate) ||
        pt_routed_playback_prepare(&boundary->playback, lookahead, schedule, &boundary->plan) != PT_RENDER_OK)
        return PT_ROUTED_SAMPLER_INVALID;

    boundary->owner = boundary;
    boundary->config = config;
    boundary->schedule = schedule;
    if (boundary->plan.end)
        return prepare_stop(boundary);
    boundary->phase = PT_ROUTED_SAMPLER_REFUSED;
    if (boundary->plan.audio.count) {
        boundary->inputs = (struct pt_mixed_plan_inputs){
            .project = config->project, .plan = &boundary->plan.audio, .origins = config->origins,
            .caps = &config->caps, .format = &config->format, .rate = config->rate,
            .frame = boundary->plan.frame, .revision = config->revision, .generation = config->generation
        };
        if (pt_mixed_plan_normalizer_begin_quantized_in_workspace(config->workspace, config->workspace_bytes,
                &boundary->inputs, &boundary->normalizer) != PT_MIXED_PLAN_PENDING)
            return boundary->phase;
    }
    boundary->phase = boundary->normalizer ? PT_ROUTED_SAMPLER_PENDING : PT_ROUTED_SAMPLER_READY;
    return boundary->phase;
}

enum pt_routed_sampler_result pt_routed_sampler_begin_stop(struct pt_routed_sampler_boundary *boundary,
    const struct pt_routed_sampler_config *config, struct pt_routed_midi_schedule *schedule, uint64_t frame)
{
    const struct pt_routed_midi_batch *batch;
    if (!available(boundary, config, schedule) || schedule->held || schedule->busy || !schedule->midi ||
        schedule->midi->schedule != schedule || frame == UINT64_MAX)
        return PT_ROUTED_SAMPLER_INVALID;
    batch = pt_routed_midi_prepare_stop(schedule->midi, frame);
    if (!batch)
        return PT_ROUTED_SAMPLER_INVALID;
    boundary->owner = boundary;
    boundary->config = config;
    boundary->schedule = schedule;
    boundary->stop_serial = batch->serial;
    return prepare_stop(boundary);
}

static int bind_sources(struct pt_routed_sampler_boundary *boundary)
{
    const struct pt_mixed_plan_batch *plan = &boundary->batch.plan.normalized;
    unsigned i;
    for (i = 0; i < plan->count; ++i) {
        const struct pt_mixed_plan_record *record = plan->record + i;
        if (record->kind == PT_MIXED_PLAN_TRIGGER) {
            boundary->batch.source[i].master = boundary->config->masters[record->sample];
        } else {
            const unsigned route = record->route == PT_PAULA ? PT_MIXED_READERS_PAULA : PT_MIXED_READERS_AMIGUS;
            const struct pt_mixed_readers_key *key = pt_routed_midi_schedule_audio_key(
                boundary->schedule, route, record->slot);
            if (!key)
                return 0;
            boundary->batch.source[i].active = *key;
        }
    }
    return 1;
}

enum pt_routed_sampler_result pt_routed_sampler_step(struct pt_routed_sampler_boundary *boundary,
    uint32_t revision, uint32_t generation, unsigned work)
{
    const struct pt_routed_sampler_config *config;
    enum pt_sampler_mixed_result factory;
    if (!boundary || boundary->owner != boundary || !(config = boundary->config) ||
        !work || work > PT_MIXED_PLAN_WORK_MAX)
        return PT_ROUTED_SAMPLER_INVALID;
    if (revision != config->revision || generation != config->generation)
        boundary->phase = PT_ROUTED_SAMPLER_STALE;
    if (boundary->phase != PT_ROUTED_SAMPLER_PENDING)
        return boundary->phase;

    if (boundary->normalizer) {
        const enum pt_mixed_plan_result plan = pt_mixed_plan_normalizer_step(
            boundary->normalizer, revision, generation, work);
        if (plan == PT_MIXED_PLAN_PENDING)
            return boundary->phase;
        if (plan != PT_MIXED_PLAN_READY || pt_mixed_plan_normalizer_get_quantized(boundary->normalizer,
                revision, generation, &boundary->batch.plan) != PT_MIXED_PLAN_READY ||
            !pt_mixed_plan_normalizer_close(&boundary->normalizer) || !bind_sources(boundary))
            boundary->phase = plan == PT_MIXED_PLAN_STALE ? PT_ROUTED_SAMPLER_STALE : PT_ROUTED_SAMPLER_REFUSED;
        return boundary->phase;
    }

    if (boundary->command.address) {
        factory = pt_sampler_mixed_advance(config->pool, revision, boundary->command, NULL);
        if (factory == PT_SAMPLER_MIXED_OK)
            boundary->phase = PT_ROUTED_SAMPLER_READY;
    } else if (boundary->stop.count) {
        factory = pt_sampler_mixed_begin_stop_keys(config->pool, revision, &boundary->stop, &boundary->command);
    } else {
        factory = pt_sampler_mixed_begin_normalized(config->pool, revision, &boundary->batch, &boundary->command);
    }
    if (factory != PT_SAMPLER_MIXED_OK && factory != PT_SAMPLER_MIXED_PENDING)
        boundary->phase = factory == PT_SAMPLER_MIXED_STALE ? PT_ROUTED_SAMPLER_STALE : PT_ROUTED_SAMPLER_REFUSED;
    return boundary->phase;
}

int pt_routed_sampler_publish(struct pt_routed_sampler_boundary *boundary, uint32_t revision, uint32_t generation)
{
    const struct pt_routed_sampler_config *config;
    if (!boundary || boundary->owner != boundary || !(config = boundary->config) ||
        boundary->phase != PT_ROUTED_SAMPLER_READY || !serial(boundary) || boundary->publish_attempted)
        return 0;
    if (revision != config->revision || generation != config->generation) {
        boundary->phase = PT_ROUTED_SAMPLER_STALE;
        return 0;
    }
    boundary->publish_attempted = 1;
    if (boundary->command.address) {
        if (pt_sampler_mixed_enqueue(config->pool, revision, boundary->command, &boundary->ticket) != PT_MIXED_READERS_OK)
            return 0;
        boundary->published = pt_sampler_mixed_publish(config->pool, boundary->ticket) == PT_MIXED_READERS_OK;
    } else {
        boundary->published = pt_routed_midi_schedule_submit_combined(boundary->schedule, NULL) == PT_ROUTED_MIDI_QUEUED;
    }
    return boundary->published;
}

enum pt_render_result pt_routed_sampler_commit(struct pt_routed_sampler_boundary *boundary,
    uint32_t revision, uint32_t generation)
{
    enum pt_render_result result;
    if (!boundary || boundary->owner != boundary || boundary->phase != PT_ROUTED_SAMPLER_READY ||
        revision != boundary->config->revision || generation != boundary->config->generation ||
        boundary->command.address || boundary->command.token || (serial(boundary) && !boundary->published) ||
        (boundary->ticket && boundary->schedule->audio_ticket != boundary->ticket))
        return PT_RENDER_INVALID;
    if (boundary->stop_serial)
        result = pt_routed_midi_schedule_settled(boundary->schedule, boundary->stop.frame, boundary->stop_serial) ?
            PT_RENDER_OK : PT_RENDER_INVALID;
    else
        result = pt_routed_playback_commit(&boundary->playback);
    if (result == PT_RENDER_OK) {
        const size_t bytes = sizeof(boundary->batch.plan.normalized.next);
        if (boundary->stop_serial || boundary->plan.end)
            memset(boundary->config->origins, 0, bytes);
        else if (boundary->plan.audio.count)
            memcpy(boundary->config->origins, boundary->batch.plan.normalized.next, bytes);
        memset(boundary, 0, sizeof(*boundary));
    }
    return result;
}

int pt_routed_sampler_abandon(struct pt_routed_sampler_boundary *boundary)
{
    struct pt_routed_midi_schedule *schedule;
    if (!boundary || boundary->owner != boundary)
        return 0;
    schedule = boundary->schedule;
    if (schedule->held || schedule->busy || !pt_mixed_plan_normalizer_close(&boundary->normalizer))
        return 0;
    if (boundary->command.address &&
        (boundary->ticket || pt_sampler_mixed_cancel(boundary->config->pool, boundary->command) != PT_SAMPLER_MIXED_OK ||
         !pt_sampler_mixed_command_close(boundary->config->pool, &boundary->command)))
        return 0;
    if (schedule->midi->pending && (schedule->midi->batch.serial != serial(boundary) ||
                                   !pt_routed_midi_schedule_discard(schedule)))
        return 0;
    memset(boundary, 0, sizeof(*boundary));
    return 1;
}
