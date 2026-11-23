#ifndef PT_TEST_NATIVE_TRACKER_MIXED_WORDS_H
#define PT_TEST_NATIVE_TRACKER_MIXED_WORDS_H

#include "../src/core/routed_frame_program.h"
#include "../src/editor/mixed_readers_plan_normalize.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "routed_camd_fixture.h"

#ifdef PT_PAULA_MIDI_WIRE
#include "native_midi_loopback.h"

enum { MIXED_LOOPBACK_SLOT = 1, MIXED_RECEIVE_SLACK = 5000 };
static struct pt_routed_midi_wire mixed_wire[2];
static struct pt_test_midi_receipt mixed_receipts[2];
static struct pt_test_midi_uart mixed_uart;
#endif

/* The forecast ledger is private. Card words go to RAM and the port is
 * a loopback only in the explicitly enabled serial fixture. */
static struct pt_routed_activation_program mixed_source[2];
static struct pt_routed_midi *mixed_midi_forecast;
static struct fixture_camd *mixed_camd;

static int mixed_forecast_open(const struct pt_project *project)
{
    mixed_midi_forecast = calloc(1, sizeof(*mixed_midi_forecast));
    mixed_camd = calloc(1, sizeof(*mixed_camd));
    if (!mixed_midi_forecast || !mixed_camd || !pt_routed_midi_init(mixed_midi_forecast, project, 63) ||
        mixed_midi_forecast->endpoint_count != 1)
        return 0;
    fixture_camd_start(mixed_camd, (const char (*)[PT_MIDI_ENDPOINT])mixed_midi_forecast->endpoint, 1);
    return 1;
}

static void mixed_forecast_close(void)
{
    if (mixed_camd && mixed_camd->lease.owner)
        fixture_camd_finish(mixed_camd);
    free(mixed_camd);
    free(mixed_midi_forecast);
}

static int mixed_forecast_midi(struct pt_routed_activation_program *program,
    const struct pt_routed_midi_batch *batch, unsigned index)
{
    if (!batch || batch->count != 1)
        return 0;
    program->midi = (struct pt_routed_midi_packet){
        .key = {1, 1, batch->frame, batch->serial}, .audio_ticket = index + 1, .count = batch->count,
        .endpoint_count = mixed_midi_forecast->endpoint_count, .stop = batch->stop
    };
    program->midi.messages[0] = batch->messages[0];
    return pt_routed_midi_commit(mixed_midi_forecast, batch->frame, batch->serial);
}

static int mixed_forecast_start(const struct pt_routed_activation_program *paula,
    const struct pt_mixed_plan_record *card, const struct pt_routed_render_plan *plan)
{
    struct pt_routed_activation_program *program = mixed_source;
    if (!card || paula->audio_count != 4 || card->slot >= PT_CHANNEL_LIMIT)
        return 0;
    *program = *paula;
    struct pt_routed_audio_program *voice = program->audio + program->audio_count++;
    *voice = (struct pt_routed_audio_program){
        .route = PT_MIXED_READERS_AMIGUS, .slot = card->slot, .kind = PT_MIXED_READERS_TRIGGER
    };
    return pt_amigus_voice_trigger_registers(card->slot, &card->image, &voice->output.amigus) &&
        mixed_forecast_midi(program, pt_routed_midi_prepare(mixed_midi_forecast, plan), 0);
}

static int mixed_forecast_stop(uint64_t frame)
{
    struct pt_routed_activation_program *program = mixed_source + 1;
    const struct pt_routed_activation_program *start = mixed_source;
    program->audio_count = start->audio_count;
    for (unsigned i = 0; i < start->audio_count; ++i) {
        const struct pt_routed_audio_program *original = start->audio + i;
        struct pt_routed_audio_program *action = program->audio + i;
        *action = (struct pt_routed_audio_program){
            .route = original->route, .slot = original->slot, .kind = PT_MIXED_READERS_STOP
        };
        if (action->route == PT_MIXED_READERS_PAULA)
            program->paula_active |= 1U << action->slot;
        else {
            program->amigus_active |= 1U << action->slot;
            if (!pt_amigus_voice_stop_registers(action->slot, &action->output.amigus))
                return 0;
        }
    }
    return mixed_forecast_midi(program, pt_routed_midi_prepare_stop(mixed_midi_forecast, frame), 1);
}

static int mixed_prepare_words(const struct pt_elapsed_clock *grid, const struct pt_paula_dma_write *start,
    const struct pt_paula_dma_write *stop)
{
    struct pt_camd_routes routes;
    if (!pt_camd_routes_snapshot_for(&mixed_camd->lease,
            (const char (*)[PT_MIDI_ENDPOINT])mixed_midi_forecast->endpoint,
            mixed_midi_forecast->endpoint_count, &routes))
        return 0;
    for (unsigned i = 0; i < 2; ++i) {
        struct pt_routed_activation_program *program = mixed_source + i;
        struct pt_routed_frame_program frame;
        struct paula_window_raw *raw = &timed[i].raw;
        const uint64_t number = program->midi.key.frame;
        const struct pt_paula_dma_write *paula = i ? stop : start;
        const unsigned card = mixed_source[1].amigus_active;
        if (number == UINT64_MAX ||
            pt_elapsed_clock_deadline(grid, number, &program->midi.first) != PT_ELAPSED_OK ||
            pt_elapsed_clock_deadline(grid, number + 1, &program->midi.last) != PT_ELAPSED_OK ||
            !pt_routed_frame_prepare(program, ALL, card, routes.slot, routes.count, &frame) ||
            frame.paula.frame_count != 1 || frame.paula.frame[0].offset != paula->offset ||
            frame.paula.frame[0].value != paula->value || frame.card_frame_count != 2 ||
            frame.card_frame[0].offset != 0x1e || frame.card_frame[1].offset != 0x20 ||
            frame.midi_count != 1)
            return 0;
#ifdef PT_PAULA_MIDI_WIRE
        if (!pt_routed_midi_wire_prepare(&frame, MIXED_LOOPBACK_SLOT, mixed_wire + i))
            return 0;
        raw->wire_first = mixed_wire[i].word[0];
#endif
        raw->bank = frame.card_frame[0].value;
        raw->control = frame.card_frame[1].value;
        const struct pt_routed_frame_midi *message = frame.midi;
        raw->midi = (uint32_t)message->slot << 24 | (uint32_t)message->bytes[0] << 16 |
            (uint32_t)message->bytes[1] << 8 | message->bytes[2];
        raw->observed_bank = raw->observed_control = UINT16_MAX;
        raw->observed_midi = UINT32_MAX;
    }
    return 1;
}

#ifdef PT_PAULA_MIDI_WIRE
static void receive_mixed_wire(const struct pt_diagnostic_cia *clock, unsigned i)
{
    struct paula_window_raw *raw = &timed[i].raw;
    struct pt_test_midi_receipt *receipt = &mixed_receipts[i];
    if (raw->end <= MIXED_RECEIVE_SLACK ||
        !serial_receive(clock, raw->end - MIXED_RECEIVE_SLACK, mixed_wire + i, receipt))
        return;
    raw->observed_midi = (uint32_t)MIXED_LOOPBACK_SLOT << 24 | (uint32_t)receipt->bytes[0] << 16 |
        (uint32_t)receipt->bytes[1] << 8 | receipt->bytes[2];
}
#endif

static int mixed_words_issued(const struct paula_window_raw *raw)
{
    return raw->observed_bank == raw->bank && raw->observed_control == raw->control &&
        raw->observed_midi == raw->midi;
}

static void report_mixed_words(void)
{
    for (unsigned i = 0; i < timed_count; ++i) {
        const struct paula_window_raw *raw = &timed[i].raw;
#ifdef PT_PAULA_MIDI_WIRE
        printf("Mixed IRQ wire %u bank=%04lx/%04x control=%04lx/%04x midi=%08lx/%08lx\n", i,
#else
        printf("Mixed IRQ RAM %u bank=%04lx/%04x control=%04lx/%04x midi=%08lx/%08lx\n", i,
#endif
            (unsigned long)raw->bank, (unsigned)raw->observed_bank, (unsigned long)raw->control,
            (unsigned)raw->observed_control, (unsigned long)raw->midi, (unsigned long)raw->observed_midi);
#ifdef PT_PAULA_MIDI_WIRE
        const struct pt_routed_midi_wire *wire = &mixed_wire[i];
        const struct pt_test_midi_receipt *receipt = &mixed_receipts[i];
        printf("Mixed UART %u tx=%03x/%03x/%03x rx=%02x%02x%02x received=%u idle=%u\n", i,
            (unsigned)wire->word[0], (unsigned)wire->word[1], (unsigned)wire->word[2],
            (unsigned)receipt->bytes[0], (unsigned)receipt->bytes[1], (unsigned)receipt->bytes[2],
            receipt->received, receipt->idle);
#ifdef PT_PAULA_MIDI_WIRE_CLOCK
        printf("Mixed UART readback %u samples=", i);
        for (unsigned byte = 0; byte < 3; ++byte) {
            const uint8_t *raw = receipt->readback[byte].after;
            printf("%s%02x%02x%02x", byte ? "/" : "", (unsigned)raw[0], (unsigned)raw[1], (unsigned)raw[2]);
        }
        putchar('\n');
#endif
#endif
    }
#ifdef PT_PAULA_MIDI_WIRE
    printf("Mixed UART cleanup restored=%u request=%04x/%04x uart=%04x period=%u\n",
        mixed_uart.restored, (unsigned)mixed_uart.original_requests, (unsigned)mixed_uart.final_requests,
        mixed_uart.status, mixed_uart.period);
#endif
}
#endif
