#ifndef PT_ROUTED_ACTIVATION_H
#define PT_ROUTED_ACTIVATION_H

#include "amigus_voice_staged.h"
#include "mixed_readers_activation.h"
#include "routed_midi.h"

struct pt_routed_audio_program {
    unsigned route, slot;
    enum pt_mixed_readers_kind kind;
    union {
        struct { uintptr_t location; uint16_t words, period; uint8_t volume; } paula;
        struct pt_amigus_voice_register_program amigus;
    } output;
};

struct pt_routed_activation_program {
    struct pt_routed_midi_packet midi;
    unsigned audio_count, paula_active, amigus_active;
    struct pt_routed_audio_program audio[PT_MIXED_READERS_ACTIONS];
};

/* Delegate the genuine activation port's task publication to its routed
 * schedule. The combined provider copies the prepared audio packet and the
 * complete MIDI event before returning. There is no separate audio publish.
 * The provider's clock must observe the same original counter without
 * reentering this activation owner. Uncertainty retains both domains. */
static inline int pt_routed_activation_publish(struct pt_routed_midi_schedule *schedule,
    const struct pt_mixed_readers_activation *owner, const struct pt_mixed_activation_packet *packet)
{
    const struct pt_mixed_readers_event *event = pt_mixed_activation_publication_event(owner, packet);
    if (!event)
        return 0;
    const enum pt_routed_midi_schedule_result result = pt_routed_midi_schedule_submit_combined(schedule, event);
    if (result == PT_ROUTED_MIDI_QUEUED)
        return 1;
    return result == PT_ROUTED_MIDI_UNCERTAIN ? -1 : 0;
}

/* Prepare the audio program and accompanying MIDI bytes inside the original
 * audio publication callback. The provider owns the output, which stays
 * unchanged on refusal. Paula locations reference held sample memory. The
 * original owners still govern reader, endpoint and device lifetimes. */
int pt_routed_activation_prepare(const struct pt_routed_midi_schedule *, const struct pt_routed_midi_event *,
    const struct pt_mixed_readers_activation *, const struct pt_mixed_activation_packet *,
    struct pt_routed_activation_program *);

#define PT_PAULA_DMA_PRELOAD_WRITES (5U * PT_ROUTED_PAULA_SLOTS)
#define PT_PAULA_DMA_FRAME_WRITES (2U * PT_ROUTED_PAULA_SLOTS)

struct pt_paula_dma_write { uint16_t offset, value; };
struct pt_paula_dma_program {
    unsigned preload_count, frame_count, preload_slots;
    struct pt_paula_dma_write preload[PT_PAULA_DMA_PRELOAD_WRITES];
    struct pt_paula_dma_write frame[PT_PAULA_DMA_FRAME_WRITES];
};

/* Encode the prepared Paula leg for the classic custom registers. An inactive
 * owned channel permits preload of its location, length, period and volume.
 * At the frame, clear the selected Stop channels, apply controls and set the
 * selected start channels. Leave master DMA and all other channels alone.
 * Refuse active-slot retriggers and any action outside the owned mask. The
 * caller still verifies the held Chip-RAM samples, period caps, channel
 * exclusivity, register state, preload retirement and activation timing.
 * No writes or allocation. The destination is unchanged on refusal. */
int pt_routed_paula_dma_prepare(const struct pt_routed_activation_program *, unsigned owned_slots,
    struct pt_paula_dma_program *);

/* Resolve one exact inactive trigger from the prepared audio program. The
 * caller supplies the voices held under its original card reservation. The
 * active snapshot blocks prestaging an existing reader. Real voice quiet,
 * unchanged registers and retained program ownership require separate proof. */
int pt_routed_amigus_start_prepare(const struct pt_routed_activation_program *, unsigned slot,
    unsigned owned_voices, struct pt_amigus_voice_staged_start *);

#endif
