#include "routed_midi.h"
#include <string.h>

static int disjoint(const void *a, size_t na, const void *b, size_t nb)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return a && b && na <= UINTPTR_MAX - x && nb <= UINTPTR_MAX - y &&
        (x + na <= y || y + nb <= x);
}

static int collect(void *context, unsigned endpoint, const uint8_t *bytes, size_t size)
{
    struct pt_routed_midi *midi = context;
    struct pt_routed_midi_batch *batch = &midi->batch;
    struct pt_routed_midi_message *message;
    if (size != 3 || endpoint >= midi->endpoint_count || batch->count >= PT_ROUTED_MIDI_MESSAGES)
        return 0;
    message = &batch->messages[batch->count++];
    message->endpoint = (uint8_t)endpoint;
    memcpy(message->bytes, bytes, size);
    return 1;
}

int pt_routed_midi_init(struct pt_routed_midi *midi, const struct pt_project *project, uint16_t tracks)
{
    const struct pt_midi_sink sink = {midi, collect};
    unsigned track, endpoint, selected = 0;
    if (!disjoint(midi, sizeof(*midi), project, sizeof(*project)) ||
        midi->preview.sink.send || project->mode != PT_MODE_WAVETABLE || project->midi_flags ||
        pt_channels_validate(&project->channels) != PT_CHANNEL_OK ||
        tracks >> project->channels.count)
        return 0;
    for (track = 0; track < project->channels.count; ++track) {
        const char *name = project->midi_output[track];
        if ((tracks & (1U << track)) && project->channels.track[track].route == PT_MIDI) {
            if (!name[0] || !memchr(name, 0, PT_MIDI_ENDPOINT))
                return 0;
            ++selected;
        }
    }
    if (!selected)
        return 0;
    memset(midi, 0, sizeof(*midi));
    (void)pt_midi_init(&midi->preview, &sink);
    for (track = 0; track < project->channels.count; ++track) {
        const struct pt_channel *channel = &project->channels.track[track];
        const char *name = project->midi_output[track];
        if (!(tracks & (1U << track)) || channel->route != PT_MIDI)
            continue;
        for (endpoint = 0; endpoint < midi->endpoint_count; ++endpoint)
            if (!strcmp(midi->endpoint[endpoint], name))
                break;
        if (endpoint == midi->endpoint_count) {
            memcpy(midi->endpoint[endpoint], name, strlen(name) + 1);
            ++midi->endpoint_count;
            (void)pt_midi_connection(&midi->preview, endpoint, 1);
        }
        (void)pt_midi_bind(&midi->preview, track, endpoint, channel->midi_channel);
    }
    memcpy(midi->accepted, midi->preview.voices, sizeof(midi->accepted));
    return 1;
}

static int idle(const struct pt_routed_midi *midi)
{
    return midi && midi->preview.sink.context == midi && midi->endpoint_count &&
        !midi->pending && !midi->stopped && midi->batch.serial != UINT64_MAX;
}

static void begin_batch(struct pt_routed_midi *midi, uint64_t frame, unsigned stop)
{
    memcpy(midi->preview.voices, midi->accepted, sizeof(midi->accepted));
    midi->batch.frame = frame;
    midi->batch.stop = stop;
    midi->batch.count = 0;
}

const struct pt_routed_midi_batch *pt_routed_midi_prepare_stop(struct pt_routed_midi *midi, uint64_t frame)
{
    if (!idle(midi) || (midi->started && frame < midi->last_frame))
        return NULL;
    begin_batch(midi, frame, 1);
    if (pt_midi_stop(&midi->preview) != PT_MIDI_OK)
        return NULL;
    ++midi->batch.serial;
    midi->pending = 1;
    return &midi->batch;
}

const struct pt_routed_midi_batch *pt_routed_midi_prepare(struct pt_routed_midi *midi,
    const struct pt_routed_render_plan *plan)
{
    unsigned i;
    uint16_t seen = 0;
    if (!idle(midi) || !disjoint(midi, sizeof(*midi), plan, sizeof(*plan)) ||
        (midi->started && plan->frame <= midi->last_frame) || plan->midi_count > PT_MIDI_TRACKS)
        return NULL;
    if (plan->end)
        return plan->midi_count || plan->audio.count ? NULL : pt_routed_midi_prepare_stop(midi, plan->frame);
    if (!plan->midi_count)
        return NULL;
    begin_batch(midi, plan->frame, 0);
    for (i = 0; i < plan->midi_count; ++i) {
        const struct pt_routed_midi_action *action = &plan->midi[i];
        const unsigned track = action->track;
        enum pt_midi_result result;
        if (track >= PT_MIDI_TRACKS || !midi->accepted[track].bound || (seen & (1U << track)))
            return NULL;
        seen |= (uint16_t)(1U << track);
        if (action->kind == PT_NOTE_MIDI && action->velocity)
            result = pt_midi_note_on(&midi->preview, track, action->note, action->velocity);
        else if (action->kind == PT_NOTE_OFF && !action->note && !action->velocity)
            result = pt_midi_note_off(&midi->preview, track);
        else
            return NULL;
        if (result != PT_MIDI_OK)
            return NULL;
    }
    ++midi->batch.serial;
    midi->pending = 1;
    return &midi->batch;
}

int pt_routed_midi_commit(struct pt_routed_midi *midi, uint64_t frame, uint64_t serial)
{
    if (!midi || midi->preview.sink.context != midi || !midi->pending ||
        frame != midi->batch.frame || serial != midi->batch.serial)
        return 0;
    memcpy(midi->accepted, midi->preview.voices, sizeof(midi->accepted));
    midi->last_frame = frame;
    midi->started = 1;
    midi->stopped = midi->batch.stop;
    midi->pending = 0;
    return 1;
}

void pt_routed_midi_cancel(struct pt_routed_midi *midi)
{
    if (midi && midi->preview.sink.context == midi)
        midi->pending = 0;
}
