#include "amigus_pcm_read.h"
#include <amigus/amigus.h>

int pt_native_amigus_pcm_read16(const struct pt_amigus_reservation *r,
                               unsigned offset, uint16_t *value)
{
    const struct AmiGUS *card;
    uintptr_t address;
    if (!r || !value || !r->opened || !r->reserved || r->access != 1 ||
        r->interrupt || r->resource != PT_AMIGUS_PCM || !r->card) return 0;
    if (offset != 0x00 && offset != 0x02 && offset != 0x04 &&
        offset != 0x06 && offset != 0x10) return 0;
    card = r->card;
    if (card->agus_TypeId != AmiGUS_mini || card->agus_HardwareRev != 0 ||
        card->agus_FirmwareRev != 0x7ea663e7UL || !card->agus_PcmBase) return 0;
    address = (uintptr_t)card->agus_PcmBase;
    if ((address & 1) || address > UINTPTR_MAX - 0x12) return 0;
    *value = *(volatile uint16_t *)(address + offset);
    return 1;
}
