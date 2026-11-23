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
    midi->batch.needs_audio = 0;
}

const struct pt_routed_midi_batch *pt_routed_midi_prepare_stop(struct pt_routed_midi *midi, uint64_t frame)
{
    if (!idle(midi) || (midi->started && frame < midi->last_frame))
        return NULL;
    begin_batch(midi, frame, 1);
    midi->batch.needs_audio = midi->schedule && midi->schedule->audio_owned;
    if (pt_midi_stop(&midi->preview) != PT_MIDI_OK)
        return NULL;
    ++midi->batch.serial;
    midi->pending = 1;
    return &midi->batch;
}

const struct pt_routed_midi_batch *pt_routed_midi_prepare(struct pt_routed_midi *midi,
    const struct pt_routed_render_plan *plan)
{
    unsigned i, audio;
    uint16_t seen = 0;
    if (!idle(midi) || !disjoint(midi, sizeof(*midi), plan, sizeof(*plan)) ||
        (midi->started && plan->frame <= midi->last_frame) || plan->midi_count > PT_MIDI_TRACKS)
        return NULL;
    if (plan->end)
        return plan->midi_count || plan->audio.count ? NULL : pt_routed_midi_prepare_stop(midi, plan->frame);
    audio = midi->schedule && midi->schedule->provider.submit_combined && plan->audio.count;
    if ((!plan->midi_count && !audio) || (audio && plan->audio.count > PT_RENDER_ACTIONS))
        return NULL;
    begin_batch(midi, plan->frame, 0);
    midi->batch.needs_audio = audio;
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

static int commit(struct pt_routed_midi *midi, uint64_t frame, uint64_t serial)
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

int pt_routed_midi_commit(struct pt_routed_midi *midi, uint64_t frame, uint64_t serial)
{
    return midi && !midi->schedule && commit(midi, frame, serial);
}

void pt_routed_midi_cancel(struct pt_routed_midi *midi)
{
    if (midi && midi->preview.sink.context == midi && !midi->schedule)
        midi->pending = 0;
}

int pt_routed_midi_schedule_init(struct pt_routed_midi_schedule *schedule, struct pt_routed_midi *midi,
    const struct pt_routed_midi_provider *provider, uint64_t session, uint64_t generation,
    uint64_t epoch, uint32_t frequency, uint32_t rate)
{
    struct pt_elapsed_clock grid;
    if (!disjoint(schedule, sizeof(*schedule), midi, sizeof(*midi)) ||
        !disjoint(schedule, sizeof(*schedule), provider, sizeof(*provider)) ||
        !disjoint(midi, sizeof(*midi), provider, sizeof(*provider)) ||
        schedule->midi || !idle(midi) || midi->started || midi->schedule || !session || !generation ||
        !provider->read_clock || ((provider->submit == NULL) == (provider->submit_combined == NULL)) ||
        (provider->observe_audio_retirement && !provider->submit_combined) ||
        !provider->observe || frequency < rate ||
        pt_elapsed_clock_init(&grid, frequency, rate, epoch, 0) != PT_ELAPSED_OK)
        return 0;
    if ((provider->context == NULL) != (provider->context_bytes == 0) ||
        (provider->context &&
         (!disjoint(schedule, sizeof(*schedule), provider->context, provider->context_bytes) ||
          !disjoint(midi, sizeof(*midi), provider->context, provider->context_bytes))))
        return 0;
    schedule->midi = midi;
    schedule->provider = *provider;
    schedule->grid = grid;
    schedule->event.key.session = session;
    schedule->event.key.generation = generation;
    schedule->event.endpoint = (const char (*)[PT_MIDI_ENDPOINT])midi->endpoint;
    schedule->event.endpoint_count = midi->endpoint_count;
    midi->schedule = schedule;
    return 1;
}

static int schedule_ready(const struct pt_routed_midi_schedule *schedule)
{
    return schedule && schedule->midi && schedule->midi->schedule == schedule && !schedule->busy;
}

static unsigned audio_slot(unsigned route, unsigned slot)
{
    if (route == PT_MIXED_READERS_PAULA && slot < PT_ROUTED_PAULA_SLOTS)
        return slot;
    if (route == PT_MIXED_READERS_AMIGUS && slot < PT_CHANNEL_LIMIT)
        return PT_ROUTED_PAULA_SLOTS + slot;
    return PT_ROUTED_AUDIO_SLOTS;
}

static int same_audio_key(const struct pt_mixed_readers_key *a, const struct pt_mixed_readers_key *b)
{
    return a->queue == b->queue && a->session == b->session && a->generation == b->generation &&
        a->trigger == b->trigger && a->owner == b->owner && a->serial == b->serial &&
        a->action == b->action && a->route == b->route && a->slot == b->slot;
}

const struct pt_mixed_readers_key *pt_routed_midi_schedule_audio_key(
    const struct pt_routed_midi_schedule *schedule, unsigned route, unsigned route_slot)
{
    const unsigned slot = audio_slot(route, route_slot);
    if (!schedule_ready(schedule) || schedule->held || slot == PT_ROUTED_AUDIO_SLOTS ||
        !(schedule->audio_owned & (UINT32_C(1) << slot)))
        return NULL;
    return schedule->audio_active + slot;
}

int pt_routed_midi_schedule_stop_keys(const struct pt_routed_midi_schedule *schedule,
    struct pt_mixed_readers_stop_keys *out)
{
    struct pt_mixed_readers_stop_keys keys = {0};
    unsigned slot;
    if (!schedule_ready(schedule) || schedule->held || !schedule->provider.submit_combined ||
        !schedule->midi->pending || !schedule->midi->batch.stop ||
        (uintptr_t)out % _Alignof(struct pt_mixed_readers_stop_keys) ||
        !disjoint(schedule, sizeof(*schedule), out, sizeof(*out)) ||
        !disjoint(schedule->midi, sizeof(*schedule->midi), out, sizeof(*out)) ||
        (schedule->provider.context && !disjoint(schedule->provider.context,
            schedule->provider.context_bytes, out, sizeof(*out))))
        return 0;
    keys.frame = schedule->midi->batch.frame;
    for (slot = 0; slot < PT_ROUTED_AUDIO_SLOTS; ++slot) {
        if (!(schedule->audio_owned & (UINT32_C(1) << slot)))
            continue;
        if (keys.count == PT_MIXED_READERS_ACTIONS)
            return 0;
        keys.key[keys.count++] = schedule->audio_active[slot];
    }
    *out = keys;
    return 1;
}

static void drop_audio_key(struct pt_routed_midi_schedule *schedule, unsigned slot)
{
    memset(schedule->audio_active + slot, 0, sizeof(*schedule->audio_active));
    schedule->audio_owned &= ~(UINT32_C(1) << slot);
}

enum pt_mixed_readers_result pt_routed_midi_schedule_observe_audio_retirement(
    struct pt_routed_midi_schedule *schedule, unsigned route, unsigned route_slot)
{
    const unsigned slot = audio_slot(route, route_slot);
    struct pt_mixed_readers_reader_receipt receipt = {0};
    struct pt_mixed_readers_key key;
    enum pt_mixed_readers_result result;
    if (!schedule_ready(schedule) || schedule->held || schedule->midi->pending ||
        !schedule->provider.observe_audio_retirement || slot == PT_ROUTED_AUDIO_SLOTS ||
        !(schedule->audio_owned & (UINT32_C(1) << slot)))
        return PT_MIXED_READERS_INVALID;
    key = schedule->audio_active[slot];
    schedule->busy = 1;
    result = schedule->provider.observe_audio_retirement(schedule->provider.context, &key, &receipt);
    schedule->busy = 0;
    if (result == PT_MIXED_READERS_PENDING)
        return result;
    if (result != PT_MIXED_READERS_OK) {
        schedule->faulted = 1;
        return result;
    }
    if (receipt.domain != PT_MIXED_READER_DOMAIN || receipt.state != PT_MIXED_READER_RETIRED ||
        receipt.adoption != PT_MIXED_ADOPTED || !receipt.reference ||
        receipt.observed > receipt.issued || receipt.issued > schedule->last_read ||
        !same_audio_key(&receipt.key, &key)) {
        schedule->faulted = 1;
        return PT_MIXED_READERS_BACKEND;
    }
    drop_audio_key(schedule, slot);
    return PT_MIXED_READERS_OK;
}

static int audio_matches(const struct pt_routed_midi_schedule *schedule,
    const struct pt_mixed_readers_event *audio, uint64_t first, uint64_t last,
    struct pt_routed_audio_change *changes, int recheck)
{
    unsigned i;
    uint32_t seen = 0;
    if (!disjoint(schedule, sizeof(*schedule), audio, sizeof(*audio)) ||
        !disjoint(schedule->midi, sizeof(*schedule->midi), audio, sizeof(*audio)) ||
        !audio->queue || (schedule->audio_queue && schedule->audio_queue != audio->queue) ||
        audio->ticket <= schedule->last_audio_ticket || !audio->command_owner || !audio->batch.count ||
        audio->batch.count > PT_MIXED_READERS_ACTIONS || audio->session != schedule->event.key.session ||
        audio->batch.generation != schedule->event.key.generation ||
        audio->batch.frame != schedule->midi->batch.frame || audio->first != first || audio->last != last)
        return 0;
    for (i = 0; i < audio->batch.count; ++i) {
        const struct pt_mixed_readers_action *action = audio->batch.action + i;
        const struct pt_mixed_readers_domain *domain = audio->reader[i];
        const struct pt_mixed_readers_key *key;
        unsigned slot = audio_slot(action->route, action->slot);
        uint32_t bit;
        if (slot == PT_ROUTED_AUDIO_SLOTS || !domain ||
            !disjoint(schedule, sizeof(*schedule), domain, sizeof(*domain)) ||
            !disjoint(schedule->midi, sizeof(*schedule->midi), domain, sizeof(*domain)))
            return 0;
        bit = UINT32_C(1) << slot;
        key = &domain->key;
        if ((seen & bit) || key->route != action->route || key->slot != action->slot ||
            key->queue != audio->queue || key->session != audio->session ||
            key->generation != audio->batch.generation || !key->owner || !key->serial ||
            (schedule->midi->batch.stop && action->kind != PT_MIXED_READERS_STOP))
            return 0;
        if (action->kind == PT_MIXED_READERS_TRIGGER) {
            if (key->trigger != audio->ticket || key->action != i)
                return 0;
        } else if ((action->kind != PT_MIXED_READERS_CONTROL && action->kind != PT_MIXED_READERS_STOP) ||
                   !(schedule->audio_owned & bit) || !same_audio_key(key, schedule->audio_active + slot)) {
            return 0;
        }
        if (recheck) {
            if (changes[i].kind != action->kind || !same_audio_key(&changes[i].key, key))
                return 0;
        } else {
            changes[i] = (struct pt_routed_audio_change){*key, action->kind};
        }
        seen |= bit;
    }
    return !schedule->midi->batch.stop || seen == schedule->audio_owned;
}

static enum pt_routed_midi_schedule_result submit_batch(struct pt_routed_midi_schedule *schedule,
    const struct pt_mixed_readers_event *audio, int combined)
{
    struct pt_routed_midi_event *event;
    const struct pt_routed_midi_batch *batch;
    const struct pt_mixed_readers_output *queue = NULL;
    struct pt_routed_audio_change changes[PT_MIXED_READERS_ACTIONS];
    struct pt_mixed_readers_binding binding = {0};
    uint64_t ticket = 0, owner = 0;
    uint64_t now = 0, first, last;
    uint32_t frequency = 0;
    unsigned count = 0;
    int result;
    if (!schedule_ready(schedule) || schedule->held || !schedule->midi->pending ||
        combined != (schedule->provider.submit_combined != NULL))
        return PT_ROUTED_MIDI_INVALID;
    batch = &schedule->midi->batch;
    if ((schedule->faulted && !batch->stop) || (batch->needs_audio && !audio))
        return PT_ROUTED_MIDI_INVALID;
    if (batch->frame == UINT64_MAX ||
        pt_elapsed_clock_deadline(&schedule->grid, batch->frame, &first) != PT_ELAPSED_OK ||
        pt_elapsed_clock_deadline(&schedule->grid, batch->frame + 1, &last) != PT_ELAPSED_OK ||
        first >= last) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_CLOCK;
    }
    if (audio) {
        if (!audio_matches(schedule, audio, first, last, changes, 0))
            return PT_ROUTED_MIDI_INVALID;
        queue = audio->queue;
        ticket = audio->ticket;
        owner = audio->command_owner;
        count = audio->batch.count;
        binding = audio->binding;
    }
    schedule->busy = 1;
    result = schedule->provider.read_clock(schedule->provider.context, &now, &frequency);
    schedule->busy = 0;
    if (result != 1 || frequency != schedule->grid.frequency ||
        (schedule->clock_seen && now < schedule->last_read)) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_CLOCK;
    }
    schedule->clock_seen = 1;
    schedule->last_read = now;
    if (now >= first) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_LATE;
    }
    if (audio && (audio->queue != queue || audio->ticket != ticket ||
                  audio->command_owner != owner || audio->batch.count != count ||
                  audio->binding.context != binding.context || audio->binding.context_bytes != binding.context_bytes ||
                  !audio_matches(schedule, audio, first, last, changes, 1))) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_INVALID;
    }
    event = &schedule->event;
    event->key.frame = batch->frame;
    event->key.serial = batch->serial;
    event->first = first;
    event->last = last;
    event->batch = batch;
    schedule->audio_ticket = ticket;
    schedule->audio_count = count;
    schedule->audio_owner = owner;
    schedule->audio_event = audio;
    schedule->audio_binding = binding;
    if (audio) {
        schedule->audio_queue = queue;
        schedule->last_audio_ticket = ticket;
        memcpy(schedule->audio_pending, changes, count * sizeof(*changes));
    }
    schedule->cancel_requested = 0;
    schedule->settled = 0;
    schedule->busy = 1;
    if (combined)
        result = schedule->provider.submit_combined(schedule->provider.context, event, audio);
    else
        result = schedule->provider.submit(schedule->provider.context, event);
    schedule->busy = 0;
    if (result == 1) {
        schedule->held = 1;
        return PT_ROUTED_MIDI_QUEUED;
    }
    schedule->faulted = 1;
    if (result == 0) {
        schedule->midi->pending = 0;
        return PT_ROUTED_MIDI_REFUSED;
    }
    schedule->held = 1;
    return PT_ROUTED_MIDI_UNCERTAIN;
}

enum pt_routed_midi_schedule_result pt_routed_midi_schedule_submit(struct pt_routed_midi_schedule *schedule)
{
    return submit_batch(schedule, NULL, 0);
}

enum pt_routed_midi_schedule_result pt_routed_midi_schedule_submit_combined(
    struct pt_routed_midi_schedule *schedule, const struct pt_mixed_readers_event *audio)
{
    return submit_batch(schedule, audio, 1);
}

int pt_routed_midi_schedule_audio_receipt(const struct pt_routed_midi_schedule *schedule,
    enum pt_mixed_readers_result result, const struct pt_mixed_readers_command_receipt *receipt,
    struct pt_routed_audio_receipt *out)
{
    struct pt_routed_audio_receipt proof = {0};
    unsigned i, issued;
    if (result != PT_MIXED_READERS_OK || !schedule_ready(schedule) || !schedule->held || !schedule->audio_ticket ||
        !receipt || !disjoint(schedule, sizeof(*schedule), out, sizeof(*out)) ||
        !disjoint(schedule->midi, sizeof(*schedule->midi), out, sizeof(*out)) ||
        !disjoint(receipt, sizeof(*receipt), out, sizeof(*out)) ||
        receipt->domain != PT_MIXED_COMMAND_DOMAIN || receipt->queue != schedule->audio_queue ||
        receipt->ticket != schedule->audio_ticket || receipt->owner != schedule->audio_owner ||
        receipt->session != schedule->event.key.session || receipt->generation != schedule->event.key.generation ||
        receipt->event != schedule->audio_event || receipt->binding.context != schedule->audio_binding.context ||
        receipt->binding.context_bytes != schedule->audio_binding.context_bytes ||
        !receipt->count || receipt->count > PT_MIXED_READERS_ACTIONS || receipt->count != schedule->audio_count)
        return 0;
    proof.queue = receipt->queue;
    proof.ticket = receipt->ticket;
    proof.delivery = receipt->action[0].command;
    issued = proof.delivery == PT_MIXED_COMMAND_ISSUED || proof.delivery == PT_MIXED_COMMAND_CANCELLED_AFTER;
    if (!issued && proof.delivery != PT_MIXED_COMMAND_CANCELLED_BEFORE)
        return 0;
    for (i = 0; i < receipt->count; ++i) {
        const struct pt_mixed_readers_action_receipt *action = receipt->action + i;
        if (action->command != proof.delivery || !same_audio_key(&action->key, &schedule->audio_pending[i].key))
            return 0;
        if (issued) {
            if (action->observed < schedule->event.first || action->issued < action->observed ||
                action->issued >= schedule->event.last)
                return 0;
            if (!i || action->issued < proof.first_issued)
                proof.first_issued = action->issued;
            if (action->issued > proof.last_issued)
                proof.last_issued = action->issued;
        } else if (action->issued || action->observed) {
            return 0;
        }
    }
    proof.count = issued ? receipt->count : 0;
    *out = proof;
    return 1;
}

static int same_key(const struct pt_routed_midi_key *a, const struct pt_routed_midi_key *b)
{
    return a->session == b->session && a->generation == b->generation &&
        a->frame == b->frame && a->serial == b->serial;
}

enum pt_routed_midi_schedule_result pt_routed_midi_schedule_request_cancel(struct pt_routed_midi_schedule *schedule)
{
    struct pt_routed_midi_key key;
    int result;
    if (!schedule_ready(schedule) || !schedule->held || schedule->event.batch->stop ||
        schedule->cancel_requested)
        return PT_ROUTED_MIDI_INVALID;
    schedule->cancel_requested = 1;
    schedule->faulted = 1;
    if (!schedule->provider.request_cancel)
        return PT_ROUTED_MIDI_WAITING;
    key = schedule->event.key;
    schedule->busy = 1;
    result = schedule->provider.request_cancel(schedule->provider.context, &key);
    schedule->busy = 0;
    return result == 0 || result == 1 ? PT_ROUTED_MIDI_WAITING : PT_ROUTED_MIDI_UNCERTAIN;
}

static int audio_receipt_valid(const struct pt_routed_midi_schedule *schedule,
                               const struct pt_routed_audio_receipt *audio, int issued)
{
    const int no_effect = !audio->count && !audio->first_issued && !audio->last_issued;
    if (audio->ticket != schedule->audio_ticket)
        return 0;
    if (!schedule->audio_ticket)
        return !audio->queue && audio->delivery == PT_MIXED_COMMAND_WAITING && no_effect;
    return audio->queue == schedule->audio_queue &&
        ((issued && audio->count == schedule->audio_count && audio->first_issued >= schedule->last_read &&
          audio->first_issued <= audio->last_issued) ||
         (audio->delivery == PT_MIXED_COMMAND_CANCELLED_BEFORE && no_effect));
}

static void accept_audio(struct pt_routed_midi_schedule *schedule)
{
    unsigned i;
    for (i = 0; i < schedule->audio_count; ++i) {
        const struct pt_routed_audio_change *change = schedule->audio_pending + i;
        unsigned slot = audio_slot(change->key.route, change->key.slot);
        uint32_t bit = UINT32_C(1) << slot;
        if (change->kind == PT_MIXED_READERS_TRIGGER) {
            schedule->audio_active[slot] = change->key;
            schedule->audio_owned |= bit;
        } else if (change->kind == PT_MIXED_READERS_STOP) {
            drop_audio_key(schedule, slot);
        }
    }
}

enum pt_routed_midi_schedule_result pt_routed_midi_schedule_observe(struct pt_routed_midi_schedule *schedule)
{
    struct pt_routed_midi_receipt receipt = {0};
    const struct pt_routed_midi_event *event;
    enum pt_routed_midi_delivery delivery;
    int midi_issued, audio_issued, silent_midi;
    if (!schedule_ready(schedule) || !schedule->held)
        return PT_ROUTED_MIDI_INVALID;
    event = &schedule->event;
    schedule->busy = 1;
    delivery = schedule->provider.observe(schedule->provider.context, &event->key, &receipt);
    schedule->busy = 0;
    if (delivery == PT_ROUTED_MIDI_PENDING)
        return PT_ROUTED_MIDI_WAITING;
    midi_issued = delivery == PT_ROUTED_MIDI_ISSUED;
    audio_issued = receipt.audio.delivery == PT_MIXED_COMMAND_ISSUED ||
                   receipt.audio.delivery == PT_MIXED_COMMAND_CANCELLED_AFTER;
    silent_midi = schedule->audio_ticket && !event->batch->count;
    if (!same_key(&receipt.key, &event->key) || !audio_receipt_valid(schedule, &receipt.audio, audio_issued) ||
        (midi_issued ? receipt.count != event->batch->count || receipt.activated < schedule->last_read :
                       delivery != PT_ROUTED_MIDI_NOT_ISSUED || receipt.count || receipt.activated) ||
        ((silent_midi ? audio_issued : midi_issued) &&
         !commit(schedule->midi, event->key.frame, event->key.serial))) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_UNCERTAIN;
    }
    if (audio_issued)
        accept_audio(schedule);
    if (event->batch->stop && schedule->audio_owned)
        schedule->midi->stopped = 0;
    schedule->held = 0;
    schedule->audio_event = NULL;
    schedule->audio_owner = 0;
    schedule->audio_binding = (struct pt_mixed_readers_binding){0};
    schedule->midi->pending = 0;
    if (midi_issued || audio_issued)
        schedule->last_read = receipt.activated > receipt.audio.last_issued ? receipt.activated : receipt.audio.last_issued;
    if (schedule->audio_ticket && !silent_midi && midi_issued != audio_issued) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_PARTIAL;
    }
    if (!(silent_midi ? audio_issued : midi_issued)) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_REFUSED;
    }
    if ((!silent_midi && (receipt.activated < event->first || receipt.activated >= event->last)) ||
        (audio_issued && (receipt.audio.first_issued < event->first || receipt.audio.last_issued >= event->last))) {
        schedule->faulted = 1;
        return PT_ROUTED_MIDI_OUTSIDE_WINDOW;
    }
    schedule->settled = 1;
    return PT_ROUTED_MIDI_SETTLED;
}

int pt_routed_midi_schedule_settled(const struct pt_routed_midi_schedule *schedule, uint64_t frame, uint64_t serial)
{
    return schedule_ready(schedule) && schedule->settled && !schedule->held && !schedule->midi->pending &&
        serial && schedule->event.key.frame == frame && schedule->event.key.serial == serial &&
        schedule->midi->last_frame == frame && schedule->midi->batch.serial == serial;
}

int pt_routed_midi_schedule_discard(struct pt_routed_midi_schedule *schedule)
{
    if (!schedule_ready(schedule) || schedule->held || !schedule->midi->pending)
        return 0;
    schedule->midi->pending = 0;
    return 1;
}
