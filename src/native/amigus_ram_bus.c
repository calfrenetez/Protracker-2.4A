#include "amigus_ram_bus.h"
#include "../core/amigus_voice_plan.h"
#include <amigus/amigus.h>
#include <string.h>

#define WAVETABLE_STATUS 0x00U
#define WAVETABLE_FIFO_FULL 0x0100U
#define WAVETABLE_RAM_DATA 0x10U
#define WAVETABLE_RAM_ADDRESS 0x14U

static int registers(const struct pt_amigus_reservation *r,uintptr_t *base)
{
    const struct AmiGUS *c;
    if(!r || !r->opened || !r->reserved || r->interrupt ||
       r->resource!=PT_AMIGUS_WAVETABLE || !r->card)return 0;
    c=r->card;
    if(c->agus_TypeId!=AmiGUS_mini || c->agus_HardwareRev ||
       c->agus_FirmwareRev!=0x7ea663e7UL || !c->agus_WavetableBase)return 0;
    *base=(uintptr_t)c->agus_WavetableBase;
    return !(*base&3) && *base<=UINTPTR_MAX-0x17;
}
int pt_native_amigus_ram_bus_bind(struct pt_native_amigus_ram_bus *n,
    struct pt_amigus_reservation *r,uint32_t first,uint32_t bytes)
{
    uintptr_t base;
    if(!n || n->owner || n->faulted || !bytes || (first&3) || (bytes&3) ||
       first>=PT_AMIGUS_RAM_ADDRESS_SPACE || bytes>PT_AMIGUS_RAM_ADDRESS_SPACE-first ||
       !registers(r,&base) || r->access)return 0;
    memset(n,0,sizeof(*n));n->owner=r;n->card=r->card;n->registers=base;
    n->first=first;n->bytes=bytes;return 1;
}
int pt_native_amigus_ram_bus_owned(void *v)
{
    struct pt_native_amigus_ram_bus *n=v;uintptr_t base;
    if(!n || !n->owner)return 0;
    if(!n->faulted && registers(n->owner,&base) && n->owner->card==n->card &&
       base==n->registers)return 1;
    n->faulted=1;return 0;
}
int pt_native_amigus_ram_bus_write32(void *v,unsigned offset,uint32_t value)
{
    struct pt_native_amigus_ram_bus *n=v;
    if(!pt_native_amigus_ram_bus_owned(n))return 0;
    if(n->owner->access!=1 ||
       (offset!=WAVETABLE_RAM_ADDRESS && offset!=WAVETABLE_RAM_DATA) ||
       (offset==WAVETABLE_RAM_ADDRESS && (n->addressed || (value&3) || value<n->first ||
                         value-n->first>n->bytes-4)) ||
       (offset==WAVETABLE_RAM_DATA && !n->addressed)) {
        n->faulted=1;return 0;
    }
    n->addressed=offset==WAVETABLE_RAM_ADDRESS;
    *(volatile uint32_t *)(n->registers+offset)=value;
    /* Match the per-longword pacing read in PT4 v4.1.2.1 pt_AgusPut
       (akustikrausch/protracker4 src/sys/amigus41.asm). Inspect the same
       FIFO-full indication on every word and leave recovery to the owner. */
    if(offset==WAVETABLE_RAM_DATA &&
       (*(volatile uint16_t *)(n->registers+WAVETABLE_STATUS)&WAVETABLE_FIFO_FULL)) {
        n->faulted=1;
        return 0;
    }
    return 1;
}
int pt_native_amigus_ram_bus_clear(struct pt_native_amigus_ram_bus *n)
{
    if(!n)return 0;
    if(n->owner && (n->owner->access || n->owner->interrupt))return 0;
    memset(n,0,sizeof(*n));return 1;
}
