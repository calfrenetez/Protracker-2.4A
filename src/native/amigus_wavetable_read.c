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
static uintptr_t owned_registers(struct pt_native_amigus_wavetable_read *n)
{
    uintptr_t base;
    if (!n || !n->owner || n->faulted) return 0;
    if (!registers(n->owner,&base) || n->owner->card!=n->card ||
        base!=n->registers) { n->faulted=1; return 0; }
    return base;
}

int pt_native_amigus_wavetable_read16(struct pt_native_amigus_wavetable_read *n,
                                    unsigned offset, uint16_t *value)
{
    uintptr_t base=owned_registers(n);
    if (!base || !value || offset>0x0e || (offset&1)) return 0;
    *value=*(volatile uint16_t *)(base+offset);
    return 1;
}

int pt_native_amigus_wavetable_read_selected_control(struct pt_native_amigus_wavetable_read *n, uint16_t *value)
{
    uintptr_t base=owned_registers(n);
    if (!base || !value || base>UINTPTR_MAX-(PT_AMIGUS_VOICE_CONTROL_OFFSET+sizeof(*value)-1)) return 0;
    *value=*(volatile uint16_t *)(base+PT_AMIGUS_VOICE_CONTROL_OFFSET);
    return 1;
}

int pt_native_amigus_wavetable_read_selected_voice(struct pt_native_amigus_wavetable_read *n,
                                                  struct pt_amigus_voice_snapshot *out)
{
    struct pt_amigus_voice_snapshot snapshot;
    uintptr_t base = owned_registers(n);
    if (!base || !out || base > UINTPTR_MAX - (PT_AMIGUS_VOICE_RELEASE_OFFSET + sizeof(uint16_t) - 1))
        return 0;

    const volatile uint16_t *registers = (const volatile uint16_t *)(base + PT_AMIGUS_VOICE_CONTROL_OFFSET);
    for (unsigned i = 0; i < PT_AMIGUS_VOICE_SNAPSHOT_WORDS; ++i) {
        if (owned_registers(n) != base) return 0;
        snapshot.words[i] = registers[i];
    }

    uint16_t control;
    if (!pt_native_amigus_wavetable_read_selected_control(n, &control) || control != snapshot.words[0])
        return 0;
    *out = snapshot;
    return 1;
}
