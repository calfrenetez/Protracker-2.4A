#include "amigus_wavetable_read.h"
#include <amigus/amigus.h>

static int registers(const struct pt_amigus_reservation *r, uintptr_t *base)
{
    const struct AmiGUS *c;
    if (!r || !r->opened || !r->reserved || r->access!=1 || r->interrupt ||
        r->resource!=PT_AMIGUS_WAVETABLE || !r->card) return 0;
    c=r->card;
    if (c->agus_TypeId!=AmiGUS_mini || c->agus_HardwareRev ||
        c->agus_FirmwareRev!=0x7ea663e7UL || !c->agus_WavetableBase) return 0;
    *base=(uintptr_t)c->agus_WavetableBase;
    return !(*base&1) && *base<=UINTPTR_MAX-0x0f;
}
int pt_native_amigus_wavetable_read_bind(struct pt_native_amigus_wavetable_read *n,
                                       const struct pt_amigus_reservation *r)
{
    uintptr_t base;
    if (!n || n->owner || n->card || n->registers || n->faulted ||
        !registers(r,&base)) return 0;
    n->owner=r; n->card=r->card; n->registers=base;
    return 1;
}
int pt_native_amigus_wavetable_read16(struct pt_native_amigus_wavetable_read *n,
                                    unsigned offset, uint16_t *value)
{
    uintptr_t base;
    if (!n || !n->owner || n->faulted) return 0;
    if (!registers(n->owner,&base) || n->owner->card!=n->card ||
        base!=n->registers) { n->faulted=1; return 0; }
    if (!value || offset>0x0e || (offset&1)) return 0;
    *value=*(volatile uint16_t *)(base+offset);
    return 1;
}
