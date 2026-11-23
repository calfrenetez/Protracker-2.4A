#include "../src/core/amigus_voice_plan.h"
#include "../src/native/amigus_wavetable_read.h"
#include <amigus/amigus.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { TEST_VOICE = 3, BANK_OFFSET = 0x1e };

static void apply_program(const struct pt_amigus_voice_register_program *program,
                          uint16_t *registers)
{
    assert(program->write[0].offset == BANK_OFFSET);
    assert(program->write[0].value == TEST_VOICE);
    for (unsigned i = 1; i < program->count; ++i) {
        uint16_t offset = program->write[i].offset;
        assert(offset >= PT_AMIGUS_VOICE_CONTROL_OFFSET &&
               offset <= PT_AMIGUS_VOICE_RELEASE_OFFSET && !(offset & 1));
        registers[offset / 2] = program->write[i].value;
    }
}

int main(void)
{
    /* The already selected bank is represented by ordinary host storage.
     * This fixture validates the command/snapshot pairing, not the card's
     * bank selection, completion signals, SDRAM transfers or output. */
    uint16_t registers[PT_AMIGUS_VOICE_RELEASE_OFFSET / 2 + 1] = {0};
    struct AmiGUS card = {0};
    struct pt_amigus_reservation owner = {0};
    struct pt_native_amigus_wavetable_read reader = {0};
    struct pt_amigus_voice_snapshot idle, started, stopped;
    card.agus_TypeId = AmiGUS_mini;
    card.agus_FirmwareRev = 0x7ea663e7UL;
    card.agus_WavetableBase = (APTR)registers;
    owner.card = &card;
    owner.opened = owner.reserved = owner.access = 1;
    owner.resource = PT_AMIGUS_WAVETABLE;
    assert(pt_native_amigus_wavetable_read_bind(&reader, &owner));
    assert(pt_native_amigus_wavetable_read_selected_voice(&reader, &idle));
    assert(idle.words[0] == 0);

    struct pt_sample sample = {0};
    sample.pcm.frames = 960;
    sample.pcm.bits = 8;
    sample.pcm.channels = 1;
    const struct pt_playback_format format = {8, 0, 0, 0};
    const struct pt_amigus_voice_request request = {8000, 1, 0, 4, 128};
    struct pt_amigus_voice_plan plan;
    struct pt_amigus_voice_register_program program;
    assert(pt_amigus_voice_plan_prepare(&sample, &format, &request, 0x10000, 960, &plan));
    assert(pt_amigus_voice_trigger_registers(TEST_VOICE, &plan, &program));
    assert(program.count == 11);
    assert(program.write[program.count - 1].offset == PT_AMIGUS_VOICE_CONTROL_OFFSET);
    apply_program(&program, registers);
    assert(pt_native_amigus_wavetable_read_selected_voice(&reader, &started));
    /* Capture the one-shot flag word and last-word address independently
     * from the software planner's half-open end. */
    assert(started.words[0] == 0x8000);
    assert(started.words[1] == 1 && started.words[2] == 0);
    assert(started.words[5] == 1 && started.words[6] == 958);
    assert(started.words[7] == 0x02aa && started.words[8] == 0xaaaa);
    assert(started.words[9] == 2048 && started.words[10] == 2048);

    assert(pt_amigus_voice_stop_registers(TEST_VOICE, &program));
    apply_program(&program, registers);
    assert(pt_native_amigus_wavetable_read_selected_voice(&reader, &stopped));
    assert(stopped.words[0] == 0);
    assert(!memcmp(stopped.words + 1, started.words + 1, sizeof(stopped.words) - sizeof(stopped.words[0])));
    puts("AMIGUS ONE VOICE SNAPSHOT PASS: idle/start/stop on modeled registers; no hardware completion claim");
    return 0;
}
