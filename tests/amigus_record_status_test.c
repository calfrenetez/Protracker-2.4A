#include "amigus_pcm_read.h"
#include <amigus/amigus.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void pt_test_record_status(void)
{
    uint16_t registers[0x92 / 2], before[0x92 / 2];
    struct AmiGUS card = {0};
    struct pt_amigus_reservation reservation = {0};
    struct pt_native_amigus_record_status status;
    for (unsigned i = 0; i < sizeof(registers) / sizeof(*registers); ++i)
        registers[i] = (uint16_t)(0x3200 + i * 2);
    memcpy(before, registers, sizeof(registers));
    card.agus_TypeId = AmiGUS_mini;
    card.agus_FirmwareRev = 0x7ea663e7UL;
    card.agus_PcmBase = (APTR)registers;
    reservation.card = &card;
    reservation.opened = reservation.reserved = reservation.access = 1;
    reservation.resource = PT_AMIGUS_PCM;
    assert(pt_native_amigus_pcm_record_status(&reservation, &status));
    assert(status.irq_mask == 0x3202 && status.format == 0x3280 && status.control == 0x3282);
    assert(status.volume_left == 0x3284 && status.volume_right == 0x3286);
    assert(status.watermark == 0x328a && status.pending_words == 0x3290);
    assert(!memcmp(registers, before, sizeof(registers)));
    reservation.interrupt = 1;
    status.pending_words = 0xffff;
    assert(!pt_native_amigus_pcm_record_status(&reservation, &status) && status.pending_words == 0xffff);
    reservation.interrupt = 0;
    card.agus_PcmBase = (APTR)(UINTPTR_MAX - 0x3f);
    assert(!pt_native_amigus_pcm_record_status(&reservation, &status) && status.pending_words == 0xffff);
    card.agus_PcmBase = (APTR)registers;
    reservation.access = 0;
    assert(!pt_native_amigus_pcm_record_status(&reservation, &status));
    puts("PCM RECORD STATUS PASS: seven status words, no register writes; synthetic storage only");
}
