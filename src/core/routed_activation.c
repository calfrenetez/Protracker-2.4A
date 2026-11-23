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
    for (i = 0; i < program.audio_count; ++i)
        if (!prepare_audio(audio->action + i, program.audio + i))
            return 0;

    *out = program;
    return 1;
}
