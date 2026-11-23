#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "routed_midi.h"

struct provider_model {
    struct pt_routed_midi_key key;
    struct pt_routed_midi_receipt receipt;
    struct pt_routed_midi_schedule *schedule;
    uint64_t now;
    uint32_t frequency;
    unsigned reads, submissions, observations, cancel_calls;
    int accept, cancel_result;
    enum pt_routed_midi_delivery delivery;
};

static uint64_t tick_for(uint64_t frame)
{
    return 100 + (frame * 709379 + 47999) / 48000;
}

static int read_clock(void *context, uint64_t *ticks, uint32_t *frequency)
{
    struct provider_model *model = context;
    ++model->reads;
    *ticks = model->now;
    *frequency = model->frequency;
    return 1;
}

static int submit(void *context, const struct pt_routed_midi_event *event)
{
    struct provider_model *model = context;
    ++model->submissions;
    assert(event->key.session == 7 && event->key.generation == 11);
    assert(event->key.serial == event->batch->serial && event->key.frame == event->batch->frame);
    assert(event->first == tick_for(event->key.frame) && event->last == tick_for(event->key.frame+1));
    assert(model->now < event->first && event->endpoint_count == 1 && !strcmp(event->endpoint[0], "synth.out"));
    assert(!pt_routed_midi_schedule_discard(model->schedule));
    assert(pt_routed_midi_schedule_submit(model->schedule) == PT_ROUTED_MIDI_INVALID);
    model->key = event->key;
    model->receipt = (struct pt_routed_midi_receipt){
        .key = event->key, .activated = event->first, .count = event->batch->count
    };
    return model->accept;
}

static enum pt_routed_midi_delivery observe(void *context, const struct pt_routed_midi_key *key,
                                            struct pt_routed_midi_receipt *receipt)
{
    struct provider_model *model = context;
    ++model->observations;
    assert(key->session == model->key.session && key->generation == model->key.generation &&
           key->serial == model->key.serial && key->frame == model->key.frame);
    *receipt = model->receipt;
    return model->delivery;
}

static int request_cancel(void *context, const struct pt_routed_midi_key *key)
{
    struct provider_model *model = context;
    assert(key->session == model->key.session && key->generation == model->key.generation &&
           key->serial == model->key.serial && key->frame == model->key.frame);
    assert(++model->cancel_calls == 1);
    assert(pt_routed_midi_schedule_request_cancel(model->schedule) == PT_ROUTED_MIDI_INVALID);
    assert(pt_routed_midi_schedule_observe(model->schedule) == PT_ROUTED_MIDI_INVALID);
    return model->cancel_result;
}

static void fixture(unsigned mode)
{
    struct pt_project project = {0};
    struct pt_routed_midi *midi = calloc(1, sizeof(*midi));
    struct pt_routed_render_plan *plan = calloc(1, sizeof(*plan));
    struct pt_routed_midi_schedule schedule = {0};
    struct provider_model model = {.schedule=&schedule, .now=99, .frequency=709379, .accept=1};
    const struct pt_routed_midi_provider api = {
        .context = &model, .context_bytes = sizeof(model), .read_clock = read_clock,
        .submit = submit, .observe = observe, .request_cancel = mode == 11 ? NULL : request_cancel
    };
    const struct pt_routed_midi_batch *stop;
    assert(midi && plan);
    pt_channels_init(&project.channels);
    assert(pt_channels_route(&project.channels, 2, PT_MIDI) == PT_CHANNEL_OK);
    project.channels.track[2].midi_channel = 6;
    strcpy(project.midi_output[2], "synth.out");
    assert(pt_routed_midi_init(midi, &project, 15));
    assert(pt_routed_midi_schedule_init(&schedule, midi, &api, 7, 11, 100, 709379, 48000));
    assert(!pt_routed_midi_schedule_init(&schedule, midi, &api, 7, 11, 100, 709379, 48000));
    assert(pt_routed_midi_schedule_request_cancel(&schedule) == PT_ROUTED_MIDI_INVALID);
    plan->midi_count = 1;
    plan->midi[0] = (struct pt_routed_midi_action){2, PT_NOTE_MIDI, 60, 100};
    assert(pt_routed_midi_prepare(midi, plan));
    if (mode == 1)
        model.now = 100;
    if (mode == 2)
        model.accept = 0;
    if (mode == 3)
        model.accept = -1;
    if (mode == 1 || mode == 2) {
        assert(pt_routed_midi_schedule_submit(&schedule) ==
               (mode == 1 ? PT_ROUTED_MIDI_LATE : PT_ROUTED_MIDI_REFUSED));
        assert(model.submissions == (mode == 2) && !midi->accepted[2].active);
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_INVALID);
        assert(pt_routed_midi_schedule_discard(&schedule) == (mode == 1));
    } else {
        assert(pt_routed_midi_schedule_submit(&schedule) ==
               (mode == 3 ? PT_ROUTED_MIDI_UNCERTAIN : PT_ROUTED_MIDI_QUEUED));
        assert(model.submissions == 1 && !midi->accepted[2].active);
        assert(!pt_routed_midi_commit(midi, 0, model.key.serial));
        pt_routed_midi_cancel(midi);
        assert(midi->pending && !pt_routed_midi_schedule_discard(&schedule));
        assert(!pt_routed_midi_prepare_stop(midi, 960));
        assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_WAITING);
        if (mode >= 8) {
            model.cancel_result = mode == 8 ? 1 : mode == 10 ? -1 : 0;
            assert(pt_routed_midi_schedule_request_cancel(&schedule) ==
                   (mode == 10 ? PT_ROUTED_MIDI_UNCERTAIN : PT_ROUTED_MIDI_WAITING));
            assert(pt_routed_midi_schedule_request_cancel(&schedule) == PT_ROUTED_MIDI_INVALID);
            assert(model.cancel_calls == (unsigned)(mode != 11));
            assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_WAITING);
            assert(midi->pending && schedule.held && !pt_routed_midi_schedule_discard(&schedule));
            assert(!pt_routed_midi_prepare_stop(midi, 960));
        }
        if (mode == 4 || mode == 8) {
            model.delivery = PT_ROUTED_MIDI_NOT_ISSUED;
            model.receipt.activated = model.receipt.count = 0;
            if (mode == 8) {
                model.receipt.key.generation++;
                assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_UNCERTAIN);
                assert(schedule.held && midi->pending);
                model.receipt.key = model.key;
            }
            assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_REFUSED);
            assert(!midi->accepted[2].active);
        } else {
            model.delivery = PT_ROUTED_MIDI_ISSUED;
            if (mode == 3) {
                model.receipt.key.serial++;
                assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_UNCERTAIN);
                model.receipt.key = model.key;
                model.receipt.count = 0;
                assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_UNCERTAIN);
                model.receipt.count = 1;
                model.receipt.activated = model.now-1;
                assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_UNCERTAIN);
                assert(!midi->accepted[2].active && !pt_routed_midi_schedule_discard(&schedule));
            }
            model.receipt.activated = mode == 6 ? tick_for(1) : mode == 7 ? model.now : tick_for(1)-1;
            assert(pt_routed_midi_schedule_observe(&schedule) ==
                   (mode == 6 || mode == 7 ? PT_ROUTED_MIDI_OUTSIDE_WINDOW : PT_ROUTED_MIDI_SETTLED));
            assert(midi->accepted[2].active);
            assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_INVALID);
        }
    }

    if (mode != 0 && mode != 5) {
        plan->frame = 960;
        assert(pt_routed_midi_prepare(midi, plan));
        assert(pt_routed_midi_schedule_submit(&schedule) == PT_ROUTED_MIDI_INVALID);
        assert(pt_routed_midi_schedule_discard(&schedule));
    }
    stop = pt_routed_midi_prepare_stop(midi, 960);
    assert(stop && stop->stop && stop->count == (unsigned)(mode == 0 || mode == 3 || (mode >= 5 && mode != 8)));
    if (stop->count)
        assert(stop->messages[0].bytes[0] == 0x85 && stop->messages[0].bytes[1] == 60);
    model.accept = 1;
    if (mode == 5) {
        model.now = 101; /* Later than the submit clock, earlier than actual activation. */
        assert(pt_routed_midi_schedule_submit(&schedule) == PT_ROUTED_MIDI_CLOCK && model.submissions == 1);
    }
    model.now = tick_for(1);
    assert(pt_routed_midi_schedule_submit(&schedule) == PT_ROUTED_MIDI_QUEUED);
    assert(pt_routed_midi_schedule_request_cancel(&schedule) == PT_ROUTED_MIDI_INVALID);
    model.delivery = PT_ROUTED_MIDI_ISSUED;
    assert(pt_routed_midi_schedule_observe(&schedule) == PT_ROUTED_MIDI_SETTLED);
    assert(midi->stopped && !midi->accepted[2].active && !schedule.held);
    assert(!pt_routed_midi_prepare_stop(midi, 1920));
    free(plan);
    free(midi);
}

int main(void)
{
    unsigned mode;
    for (mode = 0; mode < 12; ++mode)
        fixture(mode);
    puts("Routed MIDI schedule: prestart queue, exact proof, early and stale refusal, uncertain retirement and owned Stop");
    return 0;
}
