#include "routed_frame_program.h"
#include <string.h>

static int disjoint(const void *a, size_t an, const void *b, size_t bn)
{
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return a && an <= UINTPTR_MAX - x && (!bn ||
        (b && bn <= UINTPTR_MAX - y && (x + an <= y || y + bn <= x)));
}

static int route_midi(const struct pt_routed_midi_packet *midi, const uint8_t *slots,
    unsigned slot_count, struct pt_routed_frame_program *frame)
{
    unsigned used = 0;
    if (midi->endpoint_count != slot_count || midi->count > PT_ROUTED_MIDI_MESSAGES)
        return 0;
    for (unsigned i = 0; i < slot_count; ++i) {
        const unsigned slot = slots[i];
        if (slot >= PT_MIDI_PORTS || (used & (1U << slot)))
            return 0;
        used |= 1U << slot;
    }
    for (unsigned i = 0; i < midi->count; ++i) {
        const struct pt_routed_midi_message *message = midi->messages + i;
        const unsigned status = message->bytes[0] & 0xf0;
        if (message->endpoint >= slot_count || message->bytes[1] > 127 || message->bytes[2] > 127 ||
            (status != 0x80 && (midi->stop || status != 0x90)))
            return 0;
        frame->midi[i].slot = slots[message->endpoint];
        memcpy(frame->midi[i].bytes, message->bytes, sizeof(message->bytes));
    }
    frame->midi_count = midi->count;
    return 1;
}

static int card_active_program(const struct pt_routed_audio_program *action,
    struct pt_amigus_voice_register_program *program)
{
    const struct pt_amigus_voice_register_program *source = &action->output.amigus;
    if (action->kind == PT_MIXED_READERS_STOP) {
        if (!pt_amigus_voice_stop_registers(action->slot, program))
            return 0;
    } else {
        if (action->kind != PT_MIXED_READERS_CONTROL || source->count != 5)
            return 0;
        const uint32_t rate = ((uint32_t)source->write[1].value << 16) | source->write[2].value;
        if (!pt_amigus_voice_control_registers(action->slot, rate,
                source->write[3].value, source->write[4].value, program))
            return 0;
    }
    if (source->count != program->count)
        return 0;
    for (unsigned i = 0; i < program->count; ++i)
        if (source->write[i].offset != program->write[i].offset || source->write[i].value != program->write[i].value)
            return 0;
    return 1;
}

static int route_audio(const struct pt_routed_activation_program *source, unsigned owned_paula,
    unsigned owned_card, struct pt_routed_frame_program *frame)
{
    unsigned card_seen = 0, paula_stopped = 0;
    const unsigned usable_card = (1U << PT_CHANNEL_LIMIT) - 1U;
    if ((owned_card & ~usable_card) || (source->amigus_active & ~owned_card) ||
        !pt_routed_paula_dma_prepare(source, owned_paula, &frame->paula))
        return 0;

    for (unsigned i = 0; i < source->audio_count; ++i) {
        const struct pt_routed_audio_program *action = source->audio + i;
        if (source->midi.stop && action->kind != PT_MIXED_READERS_STOP)
            return 0;
        if (action->route == PT_MIXED_READERS_PAULA) {
            if (action->kind == PT_MIXED_READERS_STOP)
                paula_stopped |= 1U << action->slot;
            continue;
        }
        if (action->route != PT_MIXED_READERS_AMIGUS || action->slot >= PT_CHANNEL_LIMIT)
            return 0;
        const unsigned bit = 1U << action->slot;
        if (!(owned_card & bit) || (card_seen & bit))
            return 0;
        card_seen |= bit;

        if (action->kind == PT_MIXED_READERS_TRIGGER) {
            struct pt_amigus_voice_staged_start staged;
            if ((source->amigus_active & bit) || action->output.amigus.write[0].value != action->slot ||
                !pt_amigus_voice_split_start_registers(&action->output.amigus, &staged))
                return 0;
            memcpy(frame->card_preload + frame->card_preload_count, staged.preload.write,
                staged.preload.count * sizeof(staged.preload.write[0]));
            frame->card_preload_count += staged.preload.count;
            frame->card_preload_slots |= bit;
            memcpy(frame->card_frame + frame->card_frame_count, staged.frame, sizeof(staged.frame));
            frame->card_frame_count += sizeof(staged.frame) / sizeof(staged.frame[0]);
        } else {
            struct pt_amigus_voice_register_program active;
            if (!(source->amigus_active & bit) || !card_active_program(action, &active))
                return 0;
            memcpy(frame->card_frame + frame->card_frame_count, active.write,
                active.count * sizeof(active.write[0]));
            frame->card_frame_count += active.count;
        }
    }

    return !source->midi.stop || (paula_stopped == source->paula_active && card_seen == source->amigus_active);
}

int pt_routed_frame_prepare(const struct pt_routed_activation_program *source, unsigned owned_paula,
    unsigned owned_card, const uint8_t *midi_slots, unsigned slot_count, struct pt_routed_frame_program *out)
{
    struct pt_routed_frame_program frame = {0};

    if (!source || slot_count > PT_MIDI_PORTS ||
        (uintptr_t)out % _Alignof(struct pt_routed_frame_program) ||
        !disjoint(out, sizeof(*out), source, sizeof(*source)) ||
        !disjoint(out, sizeof(*out), midi_slots, slot_count) ||
        !source->midi.key.session || !source->midi.key.generation || !source->midi.key.serial ||
        source->midi.first >= source->midi.last || source->midi.stop > 1 ||
        source->audio_count > PT_MIXED_READERS_ACTIONS ||
        (source->audio_count != 0) != (source->midi.audio_ticket != 0) ||
        !route_midi(&source->midi, midi_slots, slot_count, &frame) ||
        !route_audio(source, owned_paula, owned_card, &frame))
        return 0;

    frame.key = source->midi.key;
    frame.first = source->midi.first;
    frame.last = source->midi.last;
    frame.audio_ticket = source->midi.audio_ticket;
    frame.stop = source->midi.stop;
    frame.audio_count = source->audio_count;
    *out = frame;
    return 1;
}
