#include "amigus_voice_staged.h"

enum {
    VOICE_BANK = 0x1e, VOICE_CONTROL = 0x20, VOICE_START = 0x22, VOICE_LOOP = 0x26,
    VOICE_END = 0x2a, VOICE_RATE = 0x2e, VOICE_LEFT = 0x32, VOICE_RIGHT = 0x34
};

static uint32_t register_pair(const struct pt_amigus_voice_register_write *words)
{
    return ((uint32_t)words[0].value << 16) | words[1].value;
}

int pt_amigus_voice_split_start_registers(const struct pt_amigus_voice_register_program *trigger,
    struct pt_amigus_voice_staged_start *out)
{
    static const uint16_t parameters[] = {
        VOICE_START, VOICE_START + 2, VOICE_END, VOICE_END + 2,
        VOICE_RATE, VOICE_RATE + 2, VOICE_LEFT, VOICE_RIGHT
    };
    struct pt_amigus_voice_staged_start staged = {0};
    unsigned pos = 2;
    uintptr_t source = (uintptr_t)trigger, destination = (uintptr_t)out;

    if (!trigger || !out || destination % _Alignof(struct pt_amigus_voice_staged_start) ||
        source > UINTPTR_MAX - sizeof(*trigger) || destination > UINTPTR_MAX - sizeof(*out) ||
        (source + sizeof(*trigger) > destination && destination + sizeof(*out) > source) ||
        (trigger->count != 11 && trigger->count != 13))
        return 0;

    const struct pt_amigus_voice_register_write bank = trigger->write[0];
    const struct pt_amigus_voice_register_write stop = trigger->write[1];
    const struct pt_amigus_voice_register_write start = trigger->write[trigger->count - 1];
    const int loop = (start.value & 2) != 0;
    if (bank.offset != VOICE_BANK || bank.value >= 32 || stop.offset != VOICE_CONTROL || stop.value ||
        start.offset != VOICE_CONTROL || !(start.value & 0x8000) || (start.value & ~0x800f) ||
        ((start.value & 8) && !(start.value & 1)) || loop != (trigger->count == 13))
        return 0;
    if (loop) {
        if (trigger->write[pos].offset != VOICE_LOOP || trigger->write[pos + 1].offset != VOICE_LOOP + 2)
            return 0;
        pos += 2;
    }
    for (unsigned i = 0; i < sizeof(parameters) / sizeof(parameters[0]); ++i)
        if (trigger->write[pos + i].offset != parameters[i])
            return 0;

    const uint32_t first = register_pair(trigger->write + pos);
    const uint32_t last = register_pair(trigger->write + pos + 2);
    const uint32_t rate = register_pair(trigger->write + pos + 4);
    const uint32_t restart = loop ? register_pair(trigger->write + 2) : first;
    if (((first | last | restart) & 1) || first > last || restart > last ||
        last >= PT_AMIGUS_RAM_ADDRESS_SPACE - 2 || !rate || rate > UINT32_C(0x40000000))
        return 0;

    staged.preload.write[staged.preload.count++] = bank;
    for (unsigned i = 2; i + 1 < trigger->count; ++i)
        staged.preload.write[staged.preload.count++] = trigger->write[i];
    staged.frame[0] = bank;
    staged.frame[1] = start;
    *out = staged;
    return 1;
}
