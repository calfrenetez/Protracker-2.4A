#include "routed_activation.h"

static int disjoint(const void *a, size_t na, const void *b, size_t nb)
{
    const uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return a && b && na <= UINTPTR_MAX - x && nb <= UINTPTR_MAX - y &&
        (x + na <= y || y + nb <= x);
}

static int prepare_audio(const struct pt_mixed_readers_action *action, struct pt_routed_audio_program *out)
{
    out->route = action->route;
    out->slot = action->slot;
    out->kind = action->kind;
    if (action->route == PT_MIXED_READERS_PAULA) {
        out->output.paula.location = (uintptr_t)action->geometry.paula.data;
        out->output.paula.words = action->geometry.paula.words;
        out->output.paula.period = action->geometry.paula.period;
        out->output.paula.volume = action->geometry.paula.volume;
        return 1;
    }
    if (action->route != PT_MIXED_READERS_AMIGUS)
        return 0;
    const struct pt_amigus_voice_plan *voice = &action->geometry.amigus;
    switch (action->kind) {
    case PT_MIXED_READERS_TRIGGER:
        return pt_amigus_voice_trigger_registers(action->slot, voice, &out->output.amigus);
    case PT_MIXED_READERS_CONTROL:
        return pt_amigus_voice_control_registers(action->slot, voice->rate, voice->left, voice->right, &out->output.amigus);
    case PT_MIXED_READERS_STOP:
        return pt_amigus_voice_stop_registers(action->slot, &out->output.amigus);
    }
    return 0;
}

int pt_routed_activation_prepare(const struct pt_routed_midi_schedule *schedule, const struct pt_routed_midi_event *midi,
    const struct pt_mixed_readers_activation *owner, const struct pt_mixed_activation_packet *audio,
    struct pt_routed_activation_program *out)
{
    struct pt_routed_activation_program program = {0};
    const struct pt_mixed_readers_event *original;
    unsigned i;
    if (!schedule || (uintptr_t)out % _Alignof(struct pt_routed_activation_program) ||
        !disjoint(schedule, sizeof(*schedule), out, sizeof(*out)) ||
        !disjoint(schedule->midi, sizeof(*schedule->midi), out, sizeof(*out)) ||
        !pt_routed_midi_schedule_copy_packet(schedule, midi, &program.midi))
        return 0;

    original = pt_mixed_activation_publication_event(owner, audio);
    if (!original || original != schedule->audio_event || audio->ticket != program.midi.audio_ticket ||
        audio->count > PT_MIXED_READERS_ACTIONS ||
        !pt_mixed_readers_output_disjoint(original->queue, out, sizeof(*out)))
        return 0;
    program.audio_count = audio->count;
    program.paula_active = audio->expected_mask & ((1U << PT_ROUTED_PAULA_SLOTS) - 1U);
    program.amigus_active = audio->expected_mask >> PT_ROUTED_PAULA_SLOTS;
    for (i = 0; i < program.audio_count; ++i)
        if (!prepare_audio(audio->action + i, program.audio + i))
            return 0;

    *out = program;
    return 1;
}

enum {
    PAULA_DMACON = 0x096,
    PAULA_AUD0LCH = 0x0a0,
    PAULA_AUDLEN_OFFSET = 4,
    PAULA_AUDPER_OFFSET = 6,
    PAULA_AUDVOL_OFFSET = 8,
    PAULA_CHANNEL_STRIDE = 16,
    PAULA_DMA_SET = 0x8000
};

#define PAULA_CHIP_ADDRESS_SPACE UINT32_C(0x200000)

static void paula_write(struct pt_paula_dma_write *writes, unsigned *count, uint16_t reg, uint16_t value)
{
    writes[(*count)++] = (struct pt_paula_dma_write){reg, value};
}

int pt_routed_paula_dma_prepare(const struct pt_routed_activation_program *program, unsigned owned,
    struct pt_paula_dma_program *out)
{
    const unsigned channel_mask = (1U << PT_ROUTED_PAULA_SLOTS) - 1U;
    struct pt_paula_dma_program pending = {0};
    struct pt_paula_dma_write controls[PT_PAULA_DMA_FRAME_WRITES];
    unsigned controls_count = 0, seen = 0, stop = 0;

    if ((uintptr_t)out % _Alignof(struct pt_paula_dma_program) ||
        !disjoint(program, sizeof(*program), out, sizeof(*out)) ||
        (owned & ~channel_mask) || (program->paula_active & ~owned) ||
        program->audio_count > PT_MIXED_READERS_ACTIONS)
        return 0;

    for (unsigned i = 0; i < program->audio_count; ++i) {
        const struct pt_routed_audio_program *action = program->audio + i;
        if (action->route == PT_MIXED_READERS_AMIGUS)
            continue;
        if (action->route != PT_MIXED_READERS_PAULA || action->slot >= PT_ROUTED_PAULA_SLOTS)
            return 0;
        const unsigned bit = 1U << action->slot;
        const uint16_t location_register = PAULA_AUD0LCH + action->slot * PAULA_CHANNEL_STRIDE;
        const uintptr_t location = action->output.paula.location;
        const uint16_t words = action->output.paula.words;
        const uint16_t period = action->output.paula.period;
        const uint8_t volume = action->output.paula.volume;

        if (!(owned & bit) || (seen & bit))
            return 0;
        seen |= bit;
        if (action->kind == PT_MIXED_READERS_TRIGGER) {
            if ((program->paula_active & bit) || !location || (location & 1) || !words ||
                !period || volume > 64 || location >= PAULA_CHIP_ADDRESS_SPACE ||
                (uint32_t)words * 2U > PAULA_CHIP_ADDRESS_SPACE - location)
                return 0;
            paula_write(pending.preload, &pending.preload_count, location_register, (uint16_t)(location >> 16));
            paula_write(pending.preload, &pending.preload_count, location_register + 2, (uint16_t)location);
            paula_write(pending.preload, &pending.preload_count, location_register + PAULA_AUDLEN_OFFSET, words);
            paula_write(pending.preload, &pending.preload_count, location_register + PAULA_AUDPER_OFFSET, period);
            paula_write(pending.preload, &pending.preload_count, location_register + PAULA_AUDVOL_OFFSET, volume);
            pending.preload_slots |= bit;
        } else {
            if (!(program->paula_active & bit) || location || words)
                return 0;
            if (action->kind == PT_MIXED_READERS_CONTROL && period && volume <= 64) {
                paula_write(controls, &controls_count, location_register + PAULA_AUDPER_OFFSET, period);
                paula_write(controls, &controls_count, location_register + PAULA_AUDVOL_OFFSET, volume);
            } else if (action->kind == PT_MIXED_READERS_STOP && !period && !volume) {
                stop |= bit;
            } else {
                return 0;
            }
        }
    }

    if (stop)
        paula_write(pending.frame, &pending.frame_count, PAULA_DMACON, (uint16_t)stop);
    for (unsigned i = 0; i < controls_count; ++i)
        pending.frame[pending.frame_count++] = controls[i];
    if (pending.preload_slots)
        paula_write(pending.frame, &pending.frame_count, PAULA_DMACON, PAULA_DMA_SET | (uint16_t)pending.preload_slots);
    *out = pending;
    return 1;
}

int pt_routed_amigus_start_prepare(const struct pt_routed_activation_program *program, unsigned slot,
    unsigned owned, struct pt_amigus_voice_staged_start *out)
{
    const unsigned mask = (1U << PT_CHANNEL_LIMIT) - 1U;
    const struct pt_amigus_voice_register_program *start = NULL;

    if (slot >= PT_CHANNEL_LIMIT || (owned & ~mask) || !(owned & (1U << slot)) ||
        !disjoint(program, sizeof(*program), out, sizeof(*out)) ||
        (program->amigus_active & (1U << slot)) || program->audio_count > PT_MIXED_READERS_ACTIONS)
        return 0;

    for (unsigned i = 0; i < program->audio_count; ++i) {
        const struct pt_routed_audio_program *action = program->audio + i;
        if (action->route != PT_MIXED_READERS_AMIGUS || action->slot != slot)
            continue;
        if (start || action->kind != PT_MIXED_READERS_TRIGGER || action->output.amigus.write[0].value != slot)
            return 0;
        start = &action->output.amigus;
    }
    return pt_amigus_voice_split_start_registers(start, out);
}
