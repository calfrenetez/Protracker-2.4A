#define PT_MIXED_READERS_TEST_MAIN other_reader_cases
#define PT_MIXED_ACTIVATION_TEST_ENTRY static void mixed_activation_fixture(void)
#include "mixed_readers_activation_test.c"
#include "../src/core/routed_activation.h"
#include "../src/core/routed_frame_program.h"

#ifndef PT_ROUTED_ACTIVATION_TEST_MAIN
#define PT_ROUTED_ACTIVATION_TEST_MAIN main
#define PT_ROUTED_ACTIVATION_PUBLISH(owner, ticket, stop) pt_mixed_activation_publish(owner, ticket)
#define PT_ROUTED_ACTIVATION_FIRE(owner, ticket, stop) pt_mixed_activation_fire(owner, ticket)
#define PT_ROUTED_ACTIVATION_REPORT(message) do { puts(message); fflush(stdout); } while (0)
#endif
#ifndef PT_ROUTED_ACTIVATION_TEST_CONTROL
#define PT_ROUTED_ACTIVATION_TEST_CONTROL 1
#endif
#if PT_ROUTED_ACTIVATION_TEST_CONTROL
#include "routed_camd_fixture.h"
#endif

struct routed_port_model {
    struct paired_ram *audio;
    struct pt_routed_midi_schedule *schedule;
    const struct pt_mixed_activation_packet *publication;
    struct pt_routed_activation_program program;
    char endpoint[PT_MIDI_PORTS][PT_MIDI_ENDPOINT];
    struct pt_routed_midi_receipt receipt;
    unsigned queued, issued, detached, publishes;
#if PT_ROUTED_ACTIVATION_TEST_CONTROL
    struct fixture_camd *camd, *reversed;
#endif
};

static struct routed_port_model *routed_port;

static void verify_paula_dma_words(const struct pt_paula_dma_write *actual,
    const struct pt_paula_dma_write *expected, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        assert(actual[i].offset == expected[i].offset);
        assert(actual[i].value == expected[i].value);
    }
}

static void verify_paula_dma_preparation(void)
{
    struct pt_routed_activation_program input = {
        .audio_count = 5, .paula_active = 6,
        .audio = {
            {PT_MIXED_READERS_PAULA, 3, PT_MIXED_READERS_TRIGGER, {.paula = {0x1ffffe, 1, 500, 64}}},
            {PT_MIXED_READERS_PAULA, 1, PT_MIXED_READERS_STOP, {.paula = {0}}},
            {PT_MIXED_READERS_PAULA, 2, PT_MIXED_READERS_CONTROL, {.paula = {0, 0, 400, 32}}},
            {PT_MIXED_READERS_PAULA, 0, PT_MIXED_READERS_TRIGGER, {.paula = {0x01fffc, 3, 600, 1}}},
            {PT_MIXED_READERS_AMIGUS, 21, PT_MIXED_READERS_TRIGGER, {.amigus = {0}}},
        }
    };
    static const struct pt_paula_dma_write preload[] = {
        {0x0d0, 0x001f}, {0x0d2, 0xfffe}, {0x0d4, 1}, {0x0d6, 500}, {0x0d8, 64},
        {0x0a0, 0x0001}, {0x0a2, 0xfffc}, {0x0a4, 3}, {0x0a6, 600}, {0x0a8, 1}
    };
    static const struct pt_paula_dma_write on_frame[] = {
        {0x096, 0x0002}, {0x0c6, 400}, {0x0c8, 32}, {0x096, 0x8009}
    };
    struct pt_paula_dma_program output;
    unsigned char previous[sizeof(output)];

    assert(pt_routed_paula_dma_prepare(&input, 15, &output));
    assert(output.preload_count == 10 && output.frame_count == 4 && output.preload_slots == 9);
    verify_paula_dma_words(output.preload, preload, output.preload_count);
    verify_paula_dma_words(output.frame, on_frame, output.frame_count);
    memcpy(previous, &output, sizeof(previous));
    assert(!pt_routed_paula_dma_prepare(&input, 7, &output));
    assert(!pt_routed_paula_dma_prepare(&input, 31, &output));

    input.paula_active = 14;
    assert(!pt_routed_paula_dma_prepare(&input, 15, &output));
    input.paula_active = 6;
    input.audio[0].output.paula.words = 2;
    assert(!pt_routed_paula_dma_prepare(&input, 15, &output));
    input.audio[0].output.paula.words = 1;
    input.audio[0].output.paula.location = 0x1fffff;
    assert(!pt_routed_paula_dma_prepare(&input, 15, &output));
    input.audio[0].output.paula.location = 0x1ffffe;
    input.audio[3].slot = 3;
    assert(!pt_routed_paula_dma_prepare(&input, 15, &output));
    input.audio[3].slot = 0;
    input.audio[1].output.paula.volume = 1;
    assert(!pt_routed_paula_dma_prepare(&input, 15, &output));
    input.audio[1].output.paula.volume = 0;
    input.audio[2].output.paula.period = 0;
    assert(!pt_routed_paula_dma_prepare(&input, 15, &output));
    input.audio[2].output.paula.period = 400;
    assert(!pt_routed_paula_dma_prepare(&input, 15, (struct pt_paula_dma_program *)(void *)&input));
    assert(!memcmp(previous, &output, sizeof(previous)));

    for (unsigned slot = 0; slot < 4; ++slot) {
        input.audio[slot] = (struct pt_routed_audio_program){
            PT_MIXED_READERS_PAULA, slot, PT_MIXED_READERS_TRIGGER, {.paula = {0x1002, 2, 400, 32}}
        };
    }
    input.audio_count = 4;
    input.paula_active = 0;
    assert(pt_routed_paula_dma_prepare(&input, 15, &output));
    assert(output.preload_count == PT_PAULA_DMA_PRELOAD_WRITES && output.frame_count == 1);
    assert(output.preload_slots == 15 && output.frame[0].offset == 0x096 && output.frame[0].value == 0x800f);
    for (unsigned slot = 0; slot < 4; ++slot) {
        input.audio[slot].kind = PT_MIXED_READERS_CONTROL;
        input.audio[slot].output.paula.location = 0;
        input.audio[slot].output.paula.words = 0;
    }
    input.paula_active = 15;
    assert(pt_routed_paula_dma_prepare(&input, 15, &output));
    assert(!output.preload_slots && !output.preload_count && output.frame_count == PT_PAULA_DMA_FRAME_WRITES);
    for (unsigned slot = 0; slot < 4; ++slot) {
        assert(output.frame[slot * 2].offset == 0x0a6 + slot * 16 && output.frame[slot * 2].value == 400);
        assert(output.frame[slot * 2 + 1].offset == 0x0a8 + slot * 16 && output.frame[slot * 2 + 1].value == 32);
    }
}

static void verify_amigus_staged_start(void)
{
    struct pt_amigus_voice_plan voice = {0x10002, 0x10004, 0x20000, 0x12345678, 0x800f, 4321, 1234};
    struct pt_amigus_voice_register_program source;
    struct pt_amigus_voice_staged_start output;
    unsigned char prior[sizeof(output)];

    assert(pt_amigus_voice_trigger_registers(31, &voice, &source));
    assert(pt_amigus_voice_split_start_registers(&source, &output));
    assert(output.preload.count == 11 && output.preload.write[0].offset == 0x1e && output.preload.write[0].value == 31);
    for (unsigned i = 1; i < output.preload.count; ++i) {
        assert(output.preload.write[i].offset == source.write[i + 1].offset);
        assert(output.preload.write[i].value == source.write[i + 1].value);
    }
    assert(output.frame[0].offset == 0x1e && output.frame[0].value == 31);
    assert(output.frame[1].offset == 0x20 && output.frame[1].value == 0x800f);
    voice.control = 0x8000;
    assert(pt_amigus_voice_trigger_registers(0, &voice, &source));
    assert(pt_amigus_voice_split_start_registers(&source, &output));
    assert(output.preload.count == 9 && output.preload.write[1].offset == 0x22);
    assert(output.frame[0].value == 0 && output.frame[1].value == 0x8000);

    memcpy(prior, &output, sizeof(prior));
    assert(!pt_amigus_voice_split_start_registers(&source, (struct pt_amigus_voice_staged_start *)(void *)&source));
    source.write[3].offset = 0x20;
    assert(!pt_amigus_voice_split_start_registers(&source, &output));
    source.write[3].offset = 0x24;
    source.write[5].value = 3;
    assert(!pt_amigus_voice_split_start_registers(&source, &output));
    assert(pt_amigus_voice_stop_registers(0, &source));
    assert(!pt_amigus_voice_split_start_registers(&source, &output));
    assert(!memcmp(prior, &output, sizeof(prior)));
}

static void verify_frame_program(void)
{
    static const uint8_t original_order[] = {9, 2}, duplicate[] = {9, 9};
    const struct pt_amigus_voice_plan voice = {0x10002, 0x10004, 0x20000, 0x12345678, 0x800f, 4321, 1234};
    struct pt_routed_activation_program source = {
        .midi = {.key = {100, 17, 960, 2}, .first = 12100, .last = 12115, .audio_ticket = 3,
                 .count = 2, .endpoint_count = 2, .messages = {{1, {0x91, 62, 43}}, {0, {0x90, 60, 127}}}},
        .audio_count = 4,
        .amigus_active = 8,
        .audio = {
            {PT_MIXED_READERS_AMIGUS, 2, PT_MIXED_READERS_TRIGGER, {.amigus = {0}}},
            {PT_MIXED_READERS_PAULA, 0, PT_MIXED_READERS_TRIGGER, {.paula = {0x1004, 5, 400, 32}}},
            {PT_MIXED_READERS_AMIGUS, 3, PT_MIXED_READERS_CONTROL, {.amigus = {0}}},
            {PT_MIXED_READERS_AMIGUS, 7, PT_MIXED_READERS_TRIGGER, {.amigus = {0}}},
        },
    };
    struct pt_routed_frame_program frame;
    unsigned char before[sizeof(frame)];

    assert(pt_amigus_voice_trigger_registers(2, &voice, &source.audio[0].output.amigus));
    assert(pt_amigus_voice_control_registers(3, 1, 244, 356, &source.audio[2].output.amigus));
    assert(pt_amigus_voice_trigger_registers(7, &voice, &source.audio[3].output.amigus));
    assert(pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    assert(frame.key.session == 100 && frame.key.generation == 17 && frame.key.frame == 960 && frame.key.serial == 2);
    assert(frame.first == 12100 && frame.last == 12115 && frame.audio_ticket == 3 && frame.audio_count == 4);
    assert(!frame.stop && frame.paula.preload_slots == 1 && frame.paula.preload_count == 5);
    assert(frame.paula.frame_count == 1 && frame.paula.frame[0].offset == 0x096 && frame.paula.frame[0].value == 0x8001);
    assert(frame.card_preload_slots == 0x84 && frame.card_preload_count == 22 && frame.card_frame_count == 9);
    assert(frame.card_preload[0].offset == 0x1e && frame.card_preload[0].value == 2);
    assert(frame.card_preload[11].offset == 0x1e && frame.card_preload[11].value == 7);
    for (unsigned i = 0; i < frame.card_preload_count; ++i)
        assert(frame.card_preload[i].offset != 0x20);
    assert(frame.card_frame[0].value == 2 && frame.card_frame[1].value == 0x800f);
    assert(frame.card_frame[2].value == 3 && frame.card_frame[6].value == 356);
    assert(frame.card_frame[7].value == 7 && frame.card_frame[8].value == 0x800f);
    assert(frame.midi_count == 2 && frame.midi[0].slot == 2 && frame.midi[1].slot == 9);
    assert(!memcmp(frame.midi[0].bytes, source.midi.messages[0].bytes, 3));
    assert(!memcmp(frame.midi[1].bytes, source.midi.messages[1].bytes, 3));

    memcpy(before, &frame, sizeof(before));
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, duplicate, 2, &frame));
    assert(!pt_routed_frame_prepare(&source, 15, 0x0c, original_order, 2, &frame));
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 1, &frame));
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, frame.midi[0].bytes, 2, &frame));
    const unsigned saved = source.amigus_active;
    source.amigus_active |= 4;
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    source.amigus_active = saved;
    source.audio[2].output.amigus.write[3].offset = 0x20;
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    assert(pt_amigus_voice_control_registers(3, 1, 244, 356, &source.audio[2].output.amigus));
    source.midi.messages[0].bytes[2] = 128;
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    source.midi.messages[0].bytes[2] = 43;
    source.midi.audio_ticket = 0;
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    source.midi.audio_ticket = 3;
    source.midi.stop = 1;
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    for (unsigned i = 0; i < 2; ++i)
        source.midi.messages[i].bytes[0] -= 0x10;
    source.paula_active = 1;
    source.audio[1].kind = PT_MIXED_READERS_STOP;
    memset(&source.audio[1].output.paula, 0, sizeof(source.audio[1].output.paula));
    source.audio_count = 2;
    source.audio[0] = source.audio[2];
    source.audio[0].kind = PT_MIXED_READERS_STOP;
    assert(pt_amigus_voice_stop_registers(3, &source.audio[0].output.amigus));
    source.amigus_active |= 4;
    assert(!pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    assert(!memcmp(before, &frame, sizeof(before)));
    source.amigus_active = 8;
    assert(pt_routed_frame_prepare(&source, 15, 0x8c, original_order, 2, &frame));
    assert(frame.stop && !frame.card_preload_count && frame.card_frame_count == 2);
    assert(frame.card_frame[0].value == 3 && !frame.card_frame[1].value);
    assert(!frame.paula.preload_count && frame.paula.frame_count == 1 && frame.paula.frame[0].value == 1);
    assert(frame.midi_count == 2 && frame.midi[0].bytes[0] == 0x81 && frame.midi[1].bytes[0] == 0x80);

    source.midi.stop = 0;
    source.audio_count = PT_MIXED_READERS_ACTIONS;
    source.midi.count = PT_ROUTED_MIDI_MESSAGES;
    source.amigus_active = source.paula_active = 0;
    for (unsigned i = 0; i < source.audio_count; ++i) {
        source.audio[i] = (struct pt_routed_audio_program){.route = PT_MIXED_READERS_AMIGUS,
            .kind = PT_MIXED_READERS_TRIGGER, .slot = i};
        assert(pt_amigus_voice_trigger_registers(i, &voice, &source.audio[i].output.amigus));
    }
    for (unsigned i = 0; i < source.midi.count; ++i)
        source.midi.messages[i] = (struct pt_routed_midi_message){i % 2, {0x91, i, 80}};
    assert(pt_routed_frame_prepare(&source, 0, 0xffff, original_order, 2, &frame));
    assert(frame.card_preload_count == PT_ROUTED_CARD_PRELOAD_WRITES &&
           frame.card_frame_count == 2 * source.audio_count && frame.midi_count == PT_ROUTED_MIDI_MESSAGES);
    source.amigus_active = 0xffff;
    for (unsigned i = 0; i < source.audio_count; ++i) {
        source.audio[i].kind = PT_MIXED_READERS_CONTROL;
        assert(pt_amigus_voice_control_registers(i, 1, i, 64, &source.audio[i].output.amigus));
    }
    assert(pt_routed_frame_prepare(&source, 0, 0xffff, original_order, 2, &frame));
    assert(!frame.card_preload_count && frame.card_frame_count == PT_ROUTED_CARD_FRAME_WRITES);
}

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
    struct pt_routed_activation_program queued = {0};
    assert(packet && !p->queued && audio == pt_mixed_activation_publication_event(packet->registration.owner, packet));
    queued.midi.audio_ticket = UINT64_MAX;
    assert(!pt_routed_activation_prepare(p->schedule, &copied_event, packet->registration.owner, packet, &queued));
    assert(!pt_routed_activation_prepare(p->schedule, midi, NULL, NULL, &queued));
    assert(queued.midi.audio_ticket == UINT64_MAX);
    assert(pt_routed_activation_prepare(p->schedule, midi, packet->registration.owner, packet, &queued));
    assert(queued.midi.audio_ticket == packet->ticket && queued.midi.first == packet->first && queued.midi.last == packet->last);
    assert(queued.midi.key.frame == packet->frame && queued.audio_count == packet->count);
#if PT_ROUTED_ACTIVATION_TEST_CONTROL
    {
        struct pt_camd_routes routes = {.count = 99};
        assert(!pt_camd_routes_snapshot_for(&p->reversed->lease, midi->endpoint, midi->endpoint_count, &routes));
        assert(routes.count == 99);
        assert(!pt_camd_routes_snapshot_for(&p->camd->lease, midi->endpoint, midi->endpoint_count - 1, &routes));
        assert(routes.count == 99);
        assert(pt_camd_routes_snapshot_for(&p->camd->lease, midi->endpoint, midi->endpoint_count, &routes));
        assert(routes.count == 2 && routes.slot[0] == 1 && routes.slot[1] == 0);
        assert(pt_camd_control_close_step(&p->camd->owner) == PT_CAMD_IN_USE);
        assert(queued.paula_active == (packet->frame == 0 ? 0U : 15U));
        assert(queued.amigus_active == (packet->frame == 0 ? 0U : 3U));
        if (packet->frame != 0) {
            struct pt_routed_frame_program frame;
            assert(pt_routed_frame_prepare(&queued, 15, 3, routes.slot, routes.count, &frame));
            assert(frame.audio_ticket == packet->ticket && frame.key.frame == packet->frame);
            assert(frame.first == packet->first && frame.last == packet->last &&
                   frame.midi_count == queued.midi.count && frame.stop == queued.midi.stop);
            assert(!frame.paula.preload_count && !frame.card_preload_count);
            if (frame.stop) {
                assert(frame.paula.frame_count == 1 && frame.card_frame_count == 4);
                assert(frame.midi_count == 2 && frame.midi[0].slot == 1 && frame.midi[1].slot == 0);
            } else {
                assert(frame.paula.frame_count == 8 && frame.card_frame_count == 10);
            }

            struct pt_paula_dma_program paula;
            assert(pt_routed_paula_dma_prepare(&queued, 15, &paula) && !paula.preload_count);
            if (queued.midi.stop)
                assert(paula.frame_count == 1 && paula.frame[0].offset == 0x096 && paula.frame[0].value == 15);
            else
                assert(paula.frame_count == PT_PAULA_DMA_FRAME_WRITES);
        }
    }
#endif
    assert(queued.midi.count == midi->batch->count && queued.midi.endpoint_count == 2 && queued.midi.stop == midi->batch->stop);
    for (unsigned i = queued.midi.count; i < PT_ROUTED_MIDI_MESSAGES; ++i)
        assert(!queued.midi.messages[i].endpoint && !queued.midi.messages[i].bytes[0] &&
               !queued.midi.messages[i].bytes[1] && !queued.midi.messages[i].bytes[2]);
    for (unsigned i = 0; i < packet->count; ++i) {
        const struct pt_mixed_readers_action *action = packet->action + i;
        const struct pt_routed_audio_program *program = queued.audio + i;
        const struct pt_amigus_voice_register_program *registers = &program->output.amigus;
        assert(program->route == action->route && program->slot == action->slot && program->kind == action->kind);
        if (action->route == PT_MIXED_READERS_PAULA) {
            assert(program->output.paula.location == (uintptr_t)action->geometry.paula.data);
            assert(program->output.paula.words == action->geometry.paula.words);
            assert(program->output.paula.period == action->geometry.paula.period &&
                   program->output.paula.volume == action->geometry.paula.volume);
        } else if (action->kind == PT_MIXED_READERS_TRIGGER) {
            const struct pt_amigus_voice_plan *voice = &action->geometry.amigus;
            const unsigned end = (voice->control & 2) ? 6 : 4;
            assert(registers->write[end].offset == 0x2a && registers->write[end + 1].offset == 0x2c);
            assert((uint32_t)registers->write[end].value * 65536 + registers->write[end + 1].value ==
                   voice->end_exclusive - 2);
            assert(registers->write[registers->count - 1].value == voice->control);
            if (PT_ROUTED_ACTIVATION_TEST_CONTROL) {
                struct pt_amigus_voice_staged_start staged;
                const unsigned owned = 1U << action->slot;
                assert(pt_routed_amigus_start_prepare(&queued, action->slot, owned, &staged));
                assert(staged.preload.count == registers->count - 2);
                assert(staged.preload.write[0].value == action->slot);
                assert(staged.frame[0].offset == 0x1e && staged.frame[0].value == action->slot);
                assert(staged.frame[1].offset == 0x20 && staged.frame[1].value == voice->control);
                const unsigned captured = staged.preload.count;
                assert(!pt_routed_amigus_start_prepare(&queued, action->slot, 0, &staged));
                queued.amigus_active |= owned;
                assert(!pt_routed_amigus_start_prepare(&queued, action->slot, owned, &staged));
                queued.amigus_active &= ~owned;
                assert(staged.preload.count == captured);
            }
        } else if (action->kind == PT_MIXED_READERS_CONTROL) {
            assert(registers->count == 5 && registers->write[1].offset == 0x2e && registers->write[2].offset == 0x30);
            assert(registers->write[3].offset == 0x32 && registers->write[4].offset == 0x34);
            assert(registers->write[3].value == action->geometry.amigus.left &&
                   registers->write[4].value == action->geometry.amigus.right);
        } else {
            assert(action->kind == PT_MIXED_READERS_STOP);
            assert(registers->count == 2 && registers->write[0].value == action->slot && !registers->write[1].value);
        }
    }
    if (a_publish_model(p->audio, p->audio->owner, packet) != 1)
        return 0;
    p->program = queued;
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
    const struct pt_routed_midi_packet *midi = &p->program.midi;
    assert(midi->key.frame == packet->frame && midi->key.serial == p->receipt.key.serial);
    assert(!strcmp(p->endpoint[0], "synth.a") && !strcmp(p->endpoint[1], "synth.b"));
    if (midi->count) {
        assert(midi->count == 2 && midi->messages[0].endpoint == 0 && midi->messages[1].endpoint == 1);
        assert(midi->messages[0].bytes[0] == (midi->stop ? 0x82 : 0x92));
        assert(midi->messages[1].bytes[0] == (midi->stop ? 0x85 : 0x95));
        assert(midi->messages[0].bytes[1] == 60 && midi->messages[1].bytes[1] == 67);
        assert(midi->messages[0].bytes[2] == (midi->stop ? 0 : 91));
        assert(midi->messages[1].bytes[2] == (midi->stop ? 0 : 102));
    }
    result = a_commit_model(context, packet, actual);
    if (result == 1) {
        p->receipt.activated = midi->count ? p->audio->ticks : 0;
        p->receipt.count = midi->count;
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
    return p->receipt.count ? PT_ROUTED_MIDI_ISSUED : PT_ROUTED_MIDI_NOT_ISSUED;
}

static void settle_routed(struct activation_case *c, struct routed_port_model *p, uint64_t ticket, uint64_t frame)
{
    struct pt_mixed_readers_command_receipt receipt;
    const struct paired_ram_command *queued = a_command(&c->ram, ticket);
    struct pt_routed_activation_program outside = {0};
    assert(queued && !pt_mixed_activation_publication_event(c->owner, &queued->packet));
    assert(!pt_routed_activation_prepare(p->schedule, &p->schedule->event, c->owner, &queued->packet, &outside));
    assert(!outside.midi.audio_ticket);
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

static void exercise_control(struct activation_case *c, struct routed_port_model *model,
    struct pt_routed_render_plan *plan)
{
    uint64_t ticket;
    struct pt_mixed_readers_inputs *inputs = c->trial->input;
    struct pt_routed_midi *midi = model->schedule->midi;
    plan->frame = 960;
    plan->midi_count = 0;
    const struct pt_routed_midi_batch *batch = pt_routed_midi_prepare(midi, plan);
    assert(batch && !batch->count && batch->needs_audio && !batch->stop);
    memset(inputs, 0, sizeof(*inputs));
    holder_init(c->trial->command + 1, 101);
    inputs->command = control_of(c->trial->command + 1);
    inputs->batch = (struct pt_mixed_readers_batch){17, plan->frame, 6, {{0}}};
    for (unsigned i = 0; i < inputs->batch.count; ++i) {
        const unsigned route = i < 4 ? PT_MIXED_READERS_PAULA : PT_MIXED_READERS_AMIGUS;
        const unsigned slot = i < 4 ? i : i - 4;
        const struct pt_mixed_readers_key *key = pt_routed_midi_schedule_audio_key(model->schedule, route, slot);
        struct pt_mixed_readers_action *action = inputs->batch.action + i;
        assert(key);
        inputs->target[i] = *key;
        action->kind = PT_MIXED_READERS_CONTROL;
        action->route = route;
        action->slot = slot;
        if (route == PT_MIXED_READERS_PAULA) {
            action->geometry.paula.period = 400;
            action->geometry.paula.volume = 32;
        } else {
            action->geometry.amigus.rate = 0x123456;
            action->geometry.amigus.left = 2048;
            action->geometry.amigus.right = 1024;
        }
    }
    assert(pt_mixed_activation_enqueue(c->owner, inputs, &ticket) == PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_publish(c->owner, ticket) == PT_MIXED_READERS_OK);
    assert(model->publishes == 2 && c->ram.effects == 6);
    PT_ROUTED_ACTIVATION_REPORT("Routed activation controls queued");
    settle_routed(c, model, ticket, plan->frame);
    assert(c->ram.effects == 12 && midi->accepted[14].active && midi->accepted[15].active);
    for (unsigned i = 4; i < inputs->batch.count; ++i) {
        const struct paired_ram_reader *reader = a_reader(&c->ram, inputs->target + i);
        assert(reader && reader->active && reader->action.geometry.amigus.rate == 0x123456);
        assert(reader->action.geometry.amigus.left == 2048 && reader->action.geometry.amigus.right == 1024);
    }
}

int PT_ROUTED_ACTIVATION_TEST_MAIN(int argc, char **argv)
{
    if (argc == 2) {
        assert(!strcmp(argv[1], "regression"));
        mixed_activation_fixture();
    }
    PT_ROUTED_ACTIVATION_REPORT("Routed activation fixture entered");
    if (PT_ROUTED_ACTIVATION_TEST_CONTROL) {
        verify_paula_dma_preparation();
        verify_amigus_staged_start();
        verify_frame_program();
        PT_ROUTED_ACTIVATION_REPORT("Mixed frame program preserves writes and destination slots");
        PT_ROUTED_ACTIVATION_REPORT("Paula DMA preload and exact frame words verified");
    }
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
    const uint64_t stop_frame = PT_ROUTED_ACTIVATION_TEST_CONTROL ? 1920 : 960;
    const unsigned stop_command = PT_ROUTED_ACTIVATION_TEST_CONTROL ? 2 : 1;
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
#if PT_ROUTED_ACTIVATION_TEST_CONTROL
    model.camd = calloc(1, sizeof(*model.camd));
    model.reversed = calloc(1, sizeof(*model.reversed));
    assert(model.camd && model.reversed);
    fixture_camd_start(model.camd, (const char (*)[PT_MIDI_ENDPOINT])midi->endpoint);
    fixture_camd_start(model.reversed, fixture_camd_clusters);
#endif
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
    if (PT_ROUTED_ACTIVATION_TEST_CONTROL)
        PT_ROUTED_ACTIVATION_REPORT("AmiGUS inactive voice preload and reselect verified");
    settle_routed(c, &model, first, 0);
    assert(c->ram.effects == 6 && midi->accepted[14].active && midi->accepted[15].active);
#if PT_ROUTED_ACTIVATION_TEST_CONTROL
    {
        struct pt_camd_routes routes = {.count = 99};
        const void *original = model.camd->owner.links[1];
        model.camd->links.receivers = 1;
        assert(pt_camd_control_refresh(&model.camd->owner) == PT_CAMD_OK);
        assert(!pt_camd_routes_snapshot_for(&model.camd->lease, schedule.event.endpoint, 2, &routes));
        assert(routes.count == 99 && model.camd->owner.links[1] == original);
        model.camd->links.receivers = 3;
        assert(pt_camd_control_refresh(&model.camd->owner) == PT_CAMD_OK);
        assert(pt_camd_routes_snapshot_for(&model.camd->lease, schedule.event.endpoint, 2, &routes));
        assert(routes.slot[0] == 1 && model.camd->owner.links[1] == original);
    }
#endif
    if (PT_ROUTED_ACTIVATION_TEST_CONTROL)
        exercise_control(c, &model, plan);
    assert(pt_routed_midi_prepare_stop(midi, stop_frame));
    assert(pt_routed_midi_schedule_stop_keys(&schedule, &stop) && stop.count == 6 && stop.frame == stop_frame);
    memset(c->trial->input, 0, sizeof(*c->trial->input));
    holder_init(c->trial->command + stop_command, 100 + stop_command);
    c->trial->input->command = control_of(c->trial->command + stop_command);
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
    assert(model.publishes == stop_command + 1 && c->ram.effects == 6 * stop_command);
    PT_ROUTED_ACTIVATION_REPORT("Routed activation owned Stop queued");
    settle_routed(c, &model, terminal, stop_frame);
    assert(midi->stopped && !midi->accepted[14].active && !midi->accepted[15].active);
    assert(!c->ram.mask && !schedule.audio_owned && c->ram.effects == 6 * (stop_command + 1));
    for (i = 0; i < stop.count; ++i)
        assert(pt_mixed_activation_service_reader(c->owner, stop.key[i].trigger,
            stop.key[i].action, 1, NULL) == PT_MIXED_READERS_OK);
    assert(pt_mixed_activation_close(&c->owner));
#if PT_ROUTED_ACTIVATION_TEST_CONTROL
    fixture_camd_finish(model.camd);
    fixture_camd_finish(model.reversed);
    free(model.camd);
    free(model.reversed);
    PT_ROUTED_ACTIVATION_REPORT("Routed CAMD endpoints matched the original publication and retained Stop");
#endif
    routed_port = NULL;
    a_destroy(c);
    free(plan);
    free(midi);
    PT_ROUTED_ACTIVATION_REPORT("Routed activation: shared frame zero and exact Stop for six audio readers and two MIDI destinations; SOFTWARE_ONLY");
    return 0;
}
