#define PT_MIXED_READERS_TEST_MAIN original_mixed_readers_cases
#include "mixed_scheduled_readers_test.c"
#include "routed_midi.h"

struct combined_model {
    struct model *audio;
    struct pt_routed_midi_schedule *schedule;
    struct pt_routed_midi_receipt receipt;
    enum pt_routed_midi_delivery delivery;
    unsigned submissions, cancels, checked_admission;
};

static int combined_clock(void *context, uint64_t *ticks, uint32_t *frequency)
{
    struct combined_model *m = context;
    return model_clock(m->audio, ticks, frequency);
}

static int publish_combined(void *context, const struct pt_routed_midi_event *midi,
                            const struct pt_mixed_readers_event *audio)
{
    struct combined_model *m = context;
    assert(midi->first == oracle(midi->key.frame) && midi->last == oracle(midi->key.frame + 1));
    assert(m->audio->ticks < midi->first && midi->endpoint_count == 1);
    assert(!strcmp(midi->endpoint[0], "synth.out"));
    m->receipt = (struct pt_routed_midi_receipt){.key = midi->key, .count = midi->batch->count};
    m->delivery = PT_ROUTED_MIDI_PENDING;
    ++m->submissions;
    if (midi->batch->count) {
        assert(midi->batch->count == 1 && midi->batch->messages[0].endpoint == 0);
        assert(midi->batch->messages[0].bytes[0] == (midi->batch->stop ? 0x85 : 0x95));
        assert(midi->batch->messages[0].bytes[1] == 60);
    }
    if (!audio) {
        assert(midi->batch->stop);
        return 1;
    }
    assert(audio->session == midi->key.session && audio->batch.generation == midi->key.generation);
    assert(audio->batch.frame == midi->key.frame && audio->first == midi->first && audio->last == midi->last);
    assert(audio->batch.count == 5);
    m->receipt.audio.queue = audio->queue;
    m->receipt.audio.ticket = audio->ticket;
    return model_submit(m->audio, audio);
}

static int publish_audio(void *context, const struct pt_mixed_readers_event *audio)
{
    struct combined_model *m = context;
    struct pt_mixed_readers_event wrong = *audio;
    unsigned reads = m->audio->reads, submissions = m->submissions;
    enum pt_routed_midi_schedule_result result;
    if (!m->checked_admission) {
        ++wrong.first;
        assert(pt_routed_midi_schedule_submit_combined(m->schedule, &wrong) == PT_ROUTED_MIDI_INVALID);
        wrong = *audio;
        ++wrong.batch.generation;
        assert(pt_routed_midi_schedule_submit_combined(m->schedule, &wrong) == PT_ROUTED_MIDI_INVALID);
        m->checked_admission = 1;
    } else {
        wrong.ticket = m->receipt.audio.ticket;
        assert(pt_routed_midi_schedule_submit_combined(m->schedule, &wrong) == PT_ROUTED_MIDI_INVALID);
        wrong = *audio;
        wrong.queue = (const struct pt_mixed_readers_output *)m;
        assert(pt_routed_midi_schedule_submit_combined(m->schedule, &wrong) == PT_ROUTED_MIDI_INVALID);
    }
    assert(m->audio->reads == reads && m->submissions == submissions);
    result = pt_routed_midi_schedule_submit_combined(m->schedule, audio);
    assert(result == PT_ROUTED_MIDI_QUEUED);
    return 1;
}

static enum pt_mixed_readers_reply observe_audio(void *context, uint64_t ticket,
    unsigned cancel, struct pt_mixed_readers_command_receipt *receipt)
{
    struct combined_model *m = context;
    return model_command_service(m->audio, ticket, cancel, receipt);
}

static enum pt_mixed_readers_reply observe_reader(void *context, const struct pt_mixed_readers_domain *reader,
    unsigned cancel, struct pt_mixed_readers_reader_receipt *receipt)
{
    struct combined_model *m = context;
    return model_reader_service(m->audio, reader, cancel, receipt);
}

static enum pt_routed_midi_delivery observe_combined(void *context, const struct pt_routed_midi_key *key,
                                                    struct pt_routed_midi_receipt *receipt)
{
    struct combined_model *m = context;
    assert(key->session == m->receipt.key.session && key->generation == m->receipt.key.generation &&
           key->frame == m->receipt.key.frame && key->serial == m->receipt.key.serial);
    *receipt = m->receipt;
    return m->delivery;
}

static int cancel_combined(void *context, const struct pt_routed_midi_key *key)
{
    struct combined_model *m = context;
    assert(++m->cancels == 1 && key->serial == m->receipt.key.serial);
    return 1;
}

static void settle_audio(struct trial *t, struct combined_model *m, uint64_t ticket,
                          unsigned fire_audio, unsigned cancel)
{
    struct pt_mixed_readers_command_receipt receipt;
    unsigned i;
    if (fire_audio)
        fire(&t->model, ticket);
    assert(pt_mixed_readers_service_command(t->queue, ticket, cancel, &receipt) == PT_MIXED_READERS_OK);
    assert(receipt.queue == m->receipt.audio.queue && receipt.ticket == m->receipt.audio.ticket);
    m->receipt.audio.delivery = receipt.action[0].command;
    if (fire_audio) {
        m->receipt.audio.count = receipt.count;
        m->receipt.audio.first_issued = receipt.action[0].issued;
        m->receipt.audio.last_issued = receipt.action[0].issued;
        for (i = 1; i < receipt.count; ++i) {
            uint64_t issued = receipt.action[i].issued;
            if (issued < m->receipt.audio.first_issued)
                m->receipt.audio.first_issued = issued;
            if (issued > m->receipt.audio.last_issued)
                m->receipt.audio.last_issued = issued;
        }
    }
}

static void settle_midi(struct combined_model *m, unsigned issue)
{
    m->delivery = issue ? PT_ROUTED_MIDI_ISSUED : PT_ROUTED_MIDI_NOT_ISSUED;
    m->receipt.activated = issue ? oracle(m->receipt.key.frame) + 1 : 0;
    if (issue)
        m->audio->ticks = m->receipt.activated;
    else
        m->receipt.count = 0;
}

static uint64_t publish_stop(struct trial *t, uint64_t first)
{
    unsigned i;
    uint64_t stop;
    memset(t->input, 0, sizeof(*t->input));
    holder_init(t->command + 1, 101);
    t->input->command = control_of(t->command + 1);
    t->input->batch.generation = 17;
    t->input->batch.frame = 960;
    t->input->batch.count = 5;
    for (i = 0; i < 5; ++i) {
        struct pt_mixed_readers_key *key = &t->input->target[i];
        assert(pt_mixed_readers_reader_key(t->queue, first, i, key) == PT_MIXED_READERS_OK);
        t->input->batch.action[i].route = key->route;
        t->input->batch.action[i].slot = key->slot;
        t->input->batch.action[i].kind = PT_MIXED_READERS_STOP;
    }
    assert(pt_mixed_readers_enqueue(t->queue, t->input, &stop) == PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_publish(t->queue, stop) == PT_MIXED_READERS_OK);
    return stop;
}

enum combined_case { BOTH_ISSUE, AUDIO_ONLY, MIDI_ONLY, BOTH_CANCEL, WRONG_RECEIPT,
                     LATE_AUDIO_RECEIPT, STOP_AUDIO_ONLY, AUDIO_FRAME, AUDIO_FRAME_CANCEL };

static void combined_case(enum combined_case which)
{
    struct trial *t = trial_make(8, 8);
    struct pt_project *project = &t->resources->document.project;
    struct pt_routed_render_plan *plan = calloc(1, sizeof(*plan));
    struct pt_routed_midi *midi = calloc(1, sizeof(*midi));
    struct pt_routed_midi_schedule schedule = {0};
    struct combined_model m = {.audio = &t->model, .schedule = &schedule};
    const struct pt_routed_midi_provider api = {
        .context = &m, .context_bytes = sizeof(m), .read_clock = combined_clock,
        .observe = observe_combined, .request_cancel = cancel_combined, .submit_combined = publish_combined
    };
    const struct pt_routed_midi_batch *stop;
    unsigned audio_frame = which == AUDIO_FRAME || which == AUDIO_FRAME_CANCEL;
    unsigned audio_issued = which != MIDI_ONLY && which != BOTH_CANCEL && which != AUDIO_FRAME_CANCEL;
    unsigned midi_issued = which != AUDIO_ONLY && which != BOTH_CANCEL && !audio_frame;
    unsigned cancel = which == MIDI_ONLY || which == BOTH_CANCEL || which == AUDIO_ONLY || which == AUDIO_FRAME_CANCEL;
    unsigned i;
    uint64_t first;
    assert(plan && midi);
    assert(pt_channels_route(&project->channels, 15, PT_MIDI) == PT_CHANNEL_OK);
    project->channels.track[15].midi_channel = 6;
    strcpy(project->midi_output[15], "synth.out");
    assert(pt_routed_midi_init(midi, project, 1U << 15));
    assert(pt_routed_midi_schedule_init(&schedule, midi, &api, 19, 17, 100, 709379, 48000));
    t->config.backend = (struct pt_mixed_readers_backend){
        &m, sizeof(m), PT_MIXED_READERS_VERSION, PT_MIXED_READERS_REQUIRED,
        combined_clock, publish_audio, observe_audio, observe_reader
    };
    t->model.ticks = 99;
    open_trial(t);
    assert(!pt_routed_midi_prepare(midi, plan));
    plan->audio.count = 5;
    if (!audio_frame) {
        plan->midi_count = 1;
        plan->midi[0] = (struct pt_routed_midi_action){15, PT_NOTE_MIDI, 60, 100};
    }
    assert(pt_routed_midi_prepare(midi, plan));
    assert(pt_routed_midi_schedule_submit(&schedule) == PT_ROUTED_MIDI_INVALID);
    assert(pt_routed_midi_schedule_submit_combined(&schedule, NULL) == PT_ROUTED_MIDI_INVALID);
    trigger_input(t, 5, 0, 0);
    assert(pt_mixed_readers_enqueue(t->queue, t->input, &first) == PT_MIXED_READERS_OK);
    assert(pt_mixed_readers_publish(t->queue, first) == PT_MIXED_READERS_OK);
    assert(m.submissions == 1 && t->model.submits == 1 && !t->model.effects && !midi->accepted[15].active);
    assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_WAITING);
    if (cancel) {
        assert(pt_routed_midi_schedule_request_cancel(&schedule) == PT_ROUTED_MIDI_WAITING && m.cancels == 1);
        assert(pt_routed_midi_schedule_request_cancel(&schedule) == PT_ROUTED_MIDI_INVALID);
    }
    settle_audio(t, &m, first, audio_issued, cancel);
    settle_midi(&m, midi_issued);
    if (which == WRONG_RECEIPT) {
        ++m.receipt.audio.ticket;
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_UNCERTAIN);
        assert(schedule.held && midi->pending && !midi->accepted[15].active);
        assert(!pt_routed_midi_prepare_stop(midi, 960));
        --m.receipt.audio.ticket;
        --m.receipt.audio.count;
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_UNCERTAIN);
        assert(schedule.held && midi->pending && !midi->accepted[15].active);
        ++m.receipt.audio.count;
    }
    if (which == LATE_AUDIO_RECEIPT) {
        m.audio->ticks = oracle(1);
        m.receipt.audio.last_issued = m.audio->ticks;
    }
    assert(pt_routed_midi_schedule_observe(&schedule) ==
           (which == LATE_AUDIO_RECEIPT ? PT_ROUTED_MIDI_OUTSIDE_WINDOW :
            audio_frame ? (audio_issued ? PT_ROUTED_MIDI_SETTLED : PT_ROUTED_MIDI_REFUSED) :
            audio_issued != midi_issued ? PT_ROUTED_MIDI_PARTIAL :
            midi_issued ? PT_ROUTED_MIDI_SETTLED : PT_ROUTED_MIDI_REFUSED));
    assert(midi->accepted[15].active == midi_issued);
    if (audio_frame)
        assert(midi->started == audio_issued);
    assert(t->model.effects == 5 * audio_issued && pt_mixed_readers_readers_held(t->queue) == 5);

    stop = pt_routed_midi_prepare_stop(midi, 960);
    assert(stop && stop->count == midi_issued);
    if (audio_issued) {
        uint64_t stop_ticket = publish_stop(t, first);
        settle_audio(t, &m, stop_ticket, 1, 0);
    } else {
        assert(pt_routed_midi_schedule_submit_combined(&schedule, NULL) == PT_ROUTED_MIDI_QUEUED);
    }
    settle_midi(&m, which != STOP_AUDIO_ONLY && which != AUDIO_FRAME);
    if (which == STOP_AUDIO_ONLY) {
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_PARTIAL);
        assert(!midi->stopped && midi->accepted[15].active);
        stop = pt_routed_midi_prepare_stop(midi, 1920);
        assert(stop && stop->count == 1);
        assert(pt_routed_midi_schedule_submit_combined(&schedule, NULL) == PT_ROUTED_MIDI_QUEUED);
        settle_midi(&m, 1);
    }
    assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_SETTLED);
    assert(midi->stopped && !midi->accepted[15].active && !schedule.held);
    for (i = 0; i < 5; ++i)
        assert(pt_mixed_readers_service_reader(t->queue, first, i, 1, NULL) == PT_MIXED_READERS_OK);
    trial_drop(t);
    free(plan);
    free(midi);
}

int main(void)
{
    combined_case(BOTH_ISSUE);
    combined_case(AUDIO_ONLY);
    combined_case(MIDI_ONLY);
    combined_case(BOTH_CANCEL);
    combined_case(WRONG_RECEIPT);
    combined_case(LATE_AUDIO_RECEIPT);
    combined_case(STOP_AUDIO_ONLY);
    combined_case(AUDIO_FRAME);
    combined_case(AUDIO_FRAME_CANCEL);
    puts("Combined schedule: exact Paula, AmiGUS and MIDI admission, partial receipts and owned Stop; SOFTWARE_ONLY");
    return 0;
}
