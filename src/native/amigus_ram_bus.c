#include "amigus_ram_bus.h"
#include "../core/amigus_voice_plan.h"
#include <amigus/amigus.h>
#include <string.h>

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
       (offset!=0x14 && offset!=0x10) ||
       (offset==0x14 && (n->addressed || (value&3) || value<n->first ||
                         value-n->first>n->bytes-4)) ||
       (offset==0x10 && !n->addressed)) {
        n->faulted=1;return 0;
    }
    n->addressed=offset==0x14;
    *(volatile uint32_t *)(n->registers+offset)=value;
    return 1;
}
int pt_native_amigus_ram_bus_clear(struct pt_native_amigus_ram_bus *n)
{
    if(!n)return 0;
    if(n->owner && (n->owner->access || n->owner->interrupt))return 0;
    memset(n,0,sizeof(*n));return 1;
}
