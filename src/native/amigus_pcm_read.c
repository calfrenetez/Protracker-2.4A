#include "amigus_pcm_read.h"
#include <amigus/amigus.h>

static int pcm_address(const struct pt_amigus_reservation *r, uintptr_t *address)
{
    const struct AmiGUS *card;
    if (!r || !r->opened || !r->reserved || r->access != 1 ||
        r->interrupt || r->resource != PT_AMIGUS_PCM || !r->card) return 0;
    card = r->card;
    if (card->agus_TypeId != AmiGUS_mini || card->agus_HardwareRev != 0 ||
        card->agus_FirmwareRev != 0x7ea663e7UL || !card->agus_PcmBase) return 0;
    *address = (uintptr_t)card->agus_PcmBase;
    return !(*address & 1) && *address <= UINTPTR_MAX - 0x12;
}
int pt_native_amigus_pcm_read16(const struct pt_amigus_reservation *r,
                               unsigned offset, uint16_t *value)
{
    uintptr_t address;
    if (!value || !pcm_address(r,&address)) return 0;
    if (offset != 0x00 && offset != 0x02 && offset != 0x04 &&
        offset != 0x06 && offset != 0x10) return 0;
    *value = *(volatile uint16_t *)(address+offset);
    return 1;
}
int pt_native_amigus_quiesce_begin(struct pt_native_amigus_quiesce *q,
                                  const struct pt_amigus_reservation *r)
{
    uintptr_t address;
    if (!q || q->owner || q->requested || q->confirmed || !pcm_address(r,&address)) return 0;
    q->owner=r;
    /* Pinned Mini map: bit15-clear writes clear only the selected IRQ bits. */
    *(volatile uint16_t *)(address+0x06)=0;
    *(volatile uint16_t *)(address+0x00)=7;
    *(volatile uint16_t *)(address+0x02)=7;
    *(volatile uint16_t *)(address+0x08)=0;
    q->requested=1;
    return 1;
}
int pt_native_amigus_quiesce_poll(struct pt_native_amigus_quiesce *q)
{
    uint16_t rate,mask,used;
    if (!q || !q->owner || !q->requested) return -1;
    if (!pt_native_amigus_pcm_read16(q->owner,0x06,&rate) ||
        !pt_native_amigus_pcm_read16(q->owner,0x02,&mask) ||
        !pt_native_amigus_pcm_read16(q->owner,0x10,&used)) {
        q->confirmed=0; return -1;
    }
    q->confirmed=!(rate&0x8000) && !(mask&7) && !used;
    return q->confirmed ? 1 : 0;
}
int pt_native_amigus_pcm_fifo_probe32(const struct pt_amigus_reservation *r,
                                    uint32_t value)
{
    uintptr_t address;
    uint16_t rate,mask,used;
    if (!pcm_address(r,&address) ||
        !pt_native_amigus_pcm_read16(r,0x06,&rate) ||
        !pt_native_amigus_pcm_read16(r,0x02,&mask) ||
        !pt_native_amigus_pcm_read16(r,0x10,&used) ||
        (rate&0x8000) || (mask&7) || used>4 || (used&1)) return 0;
    /* Native big-endian MOVE.L at the documented adjacent16-bit data ports.
     * Physical pending-word increments still require separate qualification. */
    *(volatile uint32_t *)(address+0x0c)=value;
    return 1;
}
