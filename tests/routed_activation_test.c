#define PT_MIXED_READERS_TEST_MAIN other_reader_cases
#define PT_MIXED_ACTIVATION_TEST_ENTRY static void mixed_activation_fixture(void)
#include "mixed_readers_activation_test.c"
#include "../src/core/routed_activation.h"

#ifndef PT_ROUTED_ACTIVATION_TEST_MAIN
#define PT_ROUTED_ACTIVATION_TEST_MAIN main
#define PT_ROUTED_ACTIVATION_PUBLISH(owner, ticket, stop) pt_mixed_activation_publish(owner, ticket)
#define PT_ROUTED_ACTIVATION_FIRE(owner, ticket, stop) pt_mixed_activation_fire(owner, ticket)
#define PT_ROUTED_ACTIVATION_REPORT(message) do { puts(message); fflush(stdout); } while (0)
#endif

struct routed_port_model {
    struct paired_ram *audio;
    struct pt_routed_midi_schedule *schedule;
    const struct pt_mixed_activation_packet *publication;
    struct pt_routed_midi_packet midi;
    char endpoint[PT_MIDI_PORTS][PT_MIDI_ENDPOINT];
    struct pt_routed_midi_receipt receipt;
    unsigned queued, issued, detached, publishes;
};

static struct routed_port_model *routed_port;

static int routed_clock(void *context, uint64_t *ticks, uint32_t *frequency)
{
    struct routed_port_model *p = context;
    return a_clock(p->audio, ticks, frequency);
}

static int publish_routed_packet(void *context, const struct pt_routed_midi_event *midi,
    const struct pt_mixed_readers_event *audio)
{
    struct routed_port_model *p = context;
    const struct pt_mixed_activation_packet *packet = p->publication;
    const struct pt_routed_midi_event copied_event = *midi;
    struct pt_routed_midi_packet queued = {0};
    assert(packet && !p->queued && audio == pt_mixed_activation_publication_event(packet->registration.owner, packet));
    assert(packet->first == midi->first && packet->last == midi->last && packet->frame == midi->key.frame);
    assert(midi->endpoint_count == 2 && midi->batch->count == 2);
    queued.audio_ticket = UINT64_MAX;
    assert(!pt_routed_midi_schedule_copy_packet(p->schedule, &copied_event, &queued));
    assert(queued.audio_ticket == UINT64_MAX);
    assert(pt_routed_midi_schedule_copy_packet(p->schedule, midi, &queued));
    assert(queued.audio_ticket == packet->ticket && queued.first == packet->first && queued.last == packet->last);
    assert(queued.count == 2 && queued.endpoint_count == 2 && queued.stop == midi->batch->stop);
    for (unsigned i = queued.count; i < PT_ROUTED_MIDI_MESSAGES; ++i)
        assert(!queued.messages[i].endpoint && !queued.messages[i].bytes[0] &&
               !queued.messages[i].bytes[1] && !queued.messages[i].bytes[2]);
    if (a_publish_model(p->audio, p->audio->owner, packet) != 1)
        return 0;
    p->midi = queued;
    memcpy(p->endpoint, midi->endpoint, sizeof(p->endpoint[0]) * midi->endpoint_count);
    p->receipt = (struct pt_routed_midi_receipt){.key = midi->key};
    p->issued = p->detached = 0;
    p->queued = 1;
    ++p->publishes;
    return 1;
}

static int routed_publish(void *context, struct pt_mixed_readers_activation *owner,
    const struct pt_mixed_activation_packet *packet)
{
    const struct pt_mixed_activation_packet copy = *packet;
    int result;
    assert(routed_port && context == routed_port->audio && !routed_port->publication);
    assert(!pt_routed_activation_publish(routed_port->schedule, owner, &copy));
    routed_port->publication = packet;
    result = pt_routed_activation_publish(routed_port->schedule, owner, packet);
    routed_port->publication = NULL;
    return result;
}

static int routed_commit(void *context, const struct pt_mixed_activation_packet *packet,
    struct pt_mixed_activation_actual *actual)
{
    struct routed_port_model *p = routed_port;
    int result;
    assert(p && p->audio == context && p->queued && !p->issued);
    assert(!pt_mixed_activation_publication_event(packet->registration.owner, packet));
    assert(p->midi.key.frame == packet->frame && p->midi.key.serial == p->receipt.key.serial);
    assert(!strcmp(p->endpoint[0], "synth.a") && !strcmp(p->endpoint[1], "synth.b"));
    assert(p->midi.messages[0].endpoint == 0 && p->midi.messages[1].endpoint == 1);
    assert(p->midi.messages[0].bytes[0] == (p->midi.stop ? 0x82 : 0x92));
    assert(p->midi.messages[1].bytes[0] == (p->midi.stop ? 0x85 : 0x95));
    assert(p->midi.messages[0].bytes[1] == 60 && p->midi.messages[1].bytes[1] == 67);
    assert(p->midi.messages[0].bytes[2] == (p->midi.stop ? 0 : 91));
    assert(p->midi.messages[1].bytes[2] == (p->midi.stop ? 0 : 102));
    result = a_commit_model(context, packet, actual);
    if (result == 1) {
        p->receipt.activated = p->audio->ticks;
        p->receipt.count = p->midi.count;
        p->issued = 1;
    }
    return result;
}

static enum pt_routed_midi_delivery observe_routed(void *context, const struct pt_routed_midi_key *key,
    struct pt_routed_midi_receipt *out)
{
    struct routed_port_model *p = context;
    assert(p->queued && key->serial == p->receipt.key.serial && key->frame == p->receipt.key.frame);
    if (!p->issued || !p->detached)
        return PT_ROUTED_MIDI_PENDING;
    *out = p->receipt;
    p->queued = 0;
    return PT_ROUTED_MIDI_ISSUED;
}

static void settle_routed(struct activation_case *c, struct routed_port_model *p, uint64_t ticket, uint64_t frame)
{
    struct pt_mixed_readers_command_receipt receipt;
    const struct paired_ram_command *queued = a_command(&c->ram, ticket);
    struct pt_routed_midi_packet outside = {0};
    assert(queued && !pt_mixed_activation_publication_event(c->owner, &queued->packet));
    assert(!pt_routed_midi_schedule_copy_packet(p->schedule, &p->schedule->event, &outside));
    assert(!outside.audio_ticket);
    assert(pt_routed_midi_schedule_observe(p->schedule) == PT_ROUTED_MIDI_WAITING);
    c->ram.ticks = oracle(frame) - 1;
    assert(pt_mixed_activation_fire(c->owner, ticket) == PT_MIXED_ACTIVATION_EARLY);
    assert(!p->issued && !p->detached);
    c->ram.ticks = oracle(frame);
    assert(PT_ROUTED_ACTIVATION_FIRE(c->owner, ticket, frame != 0) == PT_MIXED_ACTIVATION_COMMITTED);
    assert(p->issued && pt_routed_midi_schedule_observe(p->schedule) == PT_ROUTED_MIDI_WAITING);
    const enum pt_mixed_readers_result status = pt_mixed_activation_service_command(c->owner, ticket, 0, &receipt);
    assert(status == PT_MIXED_READERS_OK);
    assert(pt_routed_midi_schedule_audio_receipt(p->schedule, status, &receipt, &p->receipt.audio));
    p->detached = 1;
    assert(pt_routed_midi_schedule_observe(p->schedule) == PT_ROUTED_MIDI_SETTLED);
    assert(!p->queued);
}

int PT_ROUTED_ACTIVATION_TEST_MAIN(int argc, char **argv)
{
    if (argc == 2) {
        assert(!strcmp(argv[1], "regression"));
        mixed_activation_fixture();
    }
    PT_ROUTED_ACTIVATION_REPORT("Routed activation fixture entered");
    struct activation_case *c = a_make(8, 8);
    struct pt_project *project = &c->trial->resources->document.project;
    struct pt_routed_midi *midi = calloc(1, sizeof(*midi));
    struct pt_routed_render_plan *plan = calloc(1, sizeof(*plan));
    struct pt_routed_midi_schedule schedule = {0};
    struct routed_port_model model = {.audio = &c->ram, .schedule = &schedule};
    const struct pt_routed_midi_provider provider = {
        .context = &model, .context_bytes = sizeof(model), .read_clock = routed_clock,
        .observe = observe_routed, .submit_combined = publish_routed_packet
    };
    struct pt_mixed_readers_stop_keys stop = {0};
    uint64_t first, terminal;
    unsigned i;
    assert(midi && plan);
    assert(pt_channels_route(&project->channels, 14, PT_MIDI) == PT_CHANNEL_OK);
    assert(pt_channels_route(&project->channels, 15, PT_MIDI) == PT_CHANNEL_OK);
    project->channels.track[14].midi_channel = 3;
    project->channels.track[15].midi_channel = 6;
    strcpy(project->midi_output[14], "synth.a");
    strcpy(project->midi_output[15], "synth.b");
    assert(pt_routed_midi_init(midi, project, 0xc000));
    assert(pt_routed_midi_schedule_init(&schedule, midi, &provider, 19, 17, 100, 709379, 48000));
    routed_port = &model;
    c->config.port.publish = routed_publish;
    c->config.port.commit = routed_commit;
    c->ram.ticks = oracle(0) - 2;
    a_open(c);
    plan->frame = 0;
    plan->audio.count = 6;
    plan->midi_count = 2;
    plan->midi[0] = (struct pt_routed_midi_action){14, PT_NOTE_MIDI, 60, 91};
    plan->midi[1] = (struct pt_routed_midi_action){15, PT_NOTE_MIDI, 67, 102};
    assert(pt_routed_midi_prepare(midi, plan));
    first = a_enqueue(c, 6, 0, 0);
    assert(PT_ROUTED_ACTIVATION_PUBLISH(c->owner, first, 0) == PT_MIXED_READERS_OK);
    assert(model.publishes == 1 && !c->ram.effects && !model.issued);
    PT_ROUTED_ACTIVATION_REPORT("Routed activation frame zero queued");
    settle_routed(c, &model, first, 0);
    assert(c->ram.effects == 6 && midi->accepted[14].active && midi->accepted[15].active);
    assert(pt_routed_midi_prepare_stop(midi, 960));
    assert(pt_routed_midi_schedule_stop_keys(&schedule, &stop) && stop.count == 6 && stop.frame == 960);
    memset(c->trial->input, 0, sizeof(*c->trial->input));
    holder_init(c->trial->command + 1, 101);
    c->trial->input->command = control_of(c->trial->command + 1);
    c->trial->input->batch = (struct pt_mixed_readers_batch){17, stop.frame, stop.count, {{0}}};
    for (i = 0; i < stop.count; ++i) {
        struct pt_mixed_readers_action *action = c->trial->input->batch.action + i;
        action->kind = PT_MIXED_READERS_STOP;
        action->route = stop.key[i].route;
        action->slot = stop.key[i].slot;
        c->trial->input->target[i] = stop.key[i];
    }
    assert(pt_mixed_activation_enqueue(c->owner, c->trial->input, &terminal) == PT_MIXED_READERS_OK);
    assert(PT_ROUTED_ACTIVATION_PUBLISH(c->owner, terminal, 1) == PT_MIXED_READERS_OK);
    assert(model.publishes == 2 && c->ram.effects == 6);
    PT_ROUTED_ACTIVATION_REPORT("Routed activation owned Stop queued");
    settle_routed(c, &model, terminal, 960);
    assert(midi->stopped && !midi->accepted[14].active && !midi->accepted[15].active);
    assert(!c->ram.mask && !schedule.audio_owned && c->ram.effects == 12);
    for (i = 0; i < stop.count; ++i)
        assert(pt_mixed_activation_service_reader(c->owner, stop.key[i].trigger,
            stop.key[i].action, 1, NULL) == PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_close(&c->owner));
    routed_port = NULL;
    a_destroy(c);
    free(plan);
    free(midi);
    PT_ROUTED_ACTIVATION_REPORT("Routed activation: shared frame zero and exact Stop for six audio readers and two MIDI destinations; SOFTWARE_ONLY");
    return 0;
}
