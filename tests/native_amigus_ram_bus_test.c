#include <amigus/amigus.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/native/amigus_ram_bus.h"
#include "../src/core/amigus_wavetable_cache.h"

int main(void)
{
    uint32_t *memory=malloc(32),snapshot[8];unsigned i;
    struct AmiGUS card={0};struct pt_amigus_reservation r={0};
    struct pt_native_amigus_ram_bus bus={0};
    assert(memory && !((uintptr_t)memory&3));
    for(i=0;i<8;++i)memory[i]=0xa5000000UL+i;
    memcpy(snapshot,memory,sizeof(snapshot));
    card.agus_TypeId=AmiGUS_mini;card.agus_FirmwareRev=0x7ea663e7UL;
    card.agus_WavetableBase=(APTR)memory;
    r.card=&card;r.opened=r.reserved=1;r.resource=PT_AMIGUS_WAVETABLE;
    /* Bind refusals must leave both context and synthetic registers untouched. */
    for(i=0;i<17;++i) {
        struct pt_amigus_reservation bad=r;struct AmiGUS changed=card;
        uint32_t first=16,bytes=16;bad.card=&changed;
        if(i==0)bad.opened=0;
        if(i==1)bad.reserved=0;
        if(i==2)bad.access=1;
        if(i==3)bad.interrupt=1;
        if(i==4)bad.resource=PT_AMIGUS_PCM;
        if(i==5)bad.card=NULL;
        if(i==6)changed.agus_TypeId=AmiGUS_Zorro2;
        if(i==7)changed.agus_HardwareRev=1;
        if(i==8)++changed.agus_FirmwareRev;
        if(i==9)changed.agus_WavetableBase=NULL;
        if(i==10)changed.agus_WavetableBase=(APTR)((uintptr_t)memory+2);
        if(i==11)changed.agus_WavetableBase=(APTR)(UINTPTR_MAX&~(uintptr_t)3);
        if(i==12)bytes=0;
        if(i==13)bytes=3;
        if(i==14)first=17;
        if(i==15)first=0x02000000UL;
        if(i==16) {first=0x01fffffcUL;bytes=8;}
        assert(!pt_native_amigus_ram_bus_bind(&bus,&bad,first,bytes));
        assert(!bus.owner && !memcmp(memory,snapshot,sizeof(snapshot)));
    }
    assert(!pt_native_amigus_ram_bus_bind(NULL,&r,16,16));
    assert(!pt_native_amigus_ram_bus_write32(NULL,0x14,16));
    assert(pt_native_amigus_ram_bus_bind(&bus,&r,16,16));
    assert(!pt_native_amigus_ram_bus_bind(&bus,&r,16,16));
    assert(pt_native_amigus_ram_bus_owned(&bus));
    assert(!pt_native_amigus_ram_bus_write32(&bus,0x14,16)); /* no access lease */
    assert(bus.faulted && !memcmp(memory,snapshot,sizeof(snapshot)));
    assert(pt_native_amigus_ram_bus_clear(&bus));
    /* Invalid address/port/pairing refuses before any store and latches. */
    for(i=0;i<7;++i) {
        unsigned offset=0x14;uint32_t value=16;
        assert(pt_native_amigus_ram_bus_bind(&bus,&r,16,16));
        assert(pt_amigus_reservation_begin(&r));
        assert(!pt_native_amigus_ram_bus_clear(&bus));
        if(i==0)offset=0x10;
        if(i==1)offset=0x00;
        if(i==2)value=12;
        if(i==3)value=32;
        if(i==4)value=17;
        if(i==5) {assert(pt_native_amigus_ram_bus_write32(&bus,0x14,16));}
        if(i==6)r.access=2;
        memcpy(snapshot,memory,sizeof(snapshot));
        assert(!pt_native_amigus_ram_bus_write32(&bus,offset,value));
        assert(bus.faulted && !pt_native_amigus_ram_bus_write32(&bus,0x14,16));
        assert(!memcmp(memory,snapshot,sizeof(snapshot)));
        r.access=1;assert(pt_amigus_reservation_end(&r));
        assert(pt_native_amigus_ram_bus_clear(&bus));
    }
    /* Lost descriptor identity cannot resume an old bus after it is restored. */
    assert(pt_native_amigus_ram_bus_bind(&bus,&r,16,16));
    assert(pt_amigus_reservation_begin(&r));
    card.agus_WavetableBase=(APTR)(memory+1);
    assert(!pt_native_amigus_ram_bus_owned(&bus));
    card.agus_WavetableBase=(APTR)memory;
    assert(!pt_native_amigus_ram_bus_write32(&bus,0x14,16));
    assert(!memcmp(memory,snapshot,sizeof(snapshot)));
    assert(pt_amigus_reservation_end(&r) && pt_native_amigus_ram_bus_clear(&bus));
    /* Losing the reservation between address and data must not issue data. */
    assert(pt_native_amigus_ram_bus_bind(&bus,&r,16,16));
    assert(pt_amigus_reservation_begin(&r));
    assert(pt_native_amigus_ram_bus_write32(&bus,0x14,16));
    memcpy(snapshot,memory,sizeof(snapshot));r.reserved=0;
    assert(!pt_native_amigus_ram_bus_write32(&bus,0x10,0x12345678UL));
    r.reserved=1;assert(!pt_native_amigus_ram_bus_write32(&bus,0x10,0x12345678UL));
    assert(!memcmp(memory,snapshot,sizeof(snapshot)));
    assert(pt_amigus_reservation_end(&r) && pt_native_amigus_ram_bus_clear(&bus));
    /* Actual cache/conversion callback chain uses these synthetic registers. */
    {
        struct pt_amigus_wavetable_cache cache={0};struct pt_cache_lease lease;
        int32_t data[3]={0x123456,-1,-8388608},original[3];
        struct pt_pcm pcm={data,3,3,48000,1,24};
        struct pt_playback_format format={16,0,0,0};
        uint8_t staging[2];uint32_t address,bytes;
        memcpy(original,data,sizeof(data));
        assert(pt_native_amigus_ram_bus_bind(&bus,&r,16,16));
        assert(pt_amigus_wavetable_cache_attach(&cache,&r,16,16,16,&bus,
            pt_native_amigus_ram_bus_owned,pt_native_amigus_ram_bus_write32));
        assert(pt_amigus_wavetable_cache_acquire(&cache,&pcm,1,1,&format,
            staging,sizeof(staging),&lease)==PT_CACHE_LOAD);
        assert(memory[0x14/4]==20 && memory[0x10/4]==0x80000000UL);
        assert(pt_amigus_wavetable_cache_location(&cache,lease,&address,&bytes));
        assert(address==16 && bytes==6 && !memcmp(original,data,sizeof(data)));
        pt_amigus_wavetable_cache_invalidate(&cache,1);
        assert(!pt_amigus_wavetable_cache_detach(&cache));
        assert(!pt_native_amigus_ram_bus_clear(&bus) && r.access==1);
        assert(pt_amigus_wavetable_cache_unpin(&cache,lease));
        assert(pt_amigus_wavetable_cache_detach(&cache));
        assert(pt_native_amigus_ram_bus_clear(&bus) && !r.access);
    }
    /* The upper25-bit address boundary fits only an entire four-byte word. */
    assert(pt_native_amigus_ram_bus_bind(&bus,&r,0x01fffffcUL,4));
    assert(pt_amigus_reservation_begin(&r));
    assert(pt_native_amigus_ram_bus_write32(&bus,0x14,0x01fffffcUL));
    assert(pt_native_amigus_ram_bus_write32(&bus,0x10,0x12345678UL));
    assert(memory[5]==0x01fffffcUL && memory[4]==0x12345678UL);
    assert(pt_amigus_reservation_end(&r) && pt_native_amigus_ram_bus_clear(&bus));
    assert(!pt_native_amigus_ram_bus_clear(NULL));
    free(memory);
    puts("NATIVE RAM BUS PASS: synthetic descriptor/paired stores, bounded region, exclusive access, latched refusal, pinned cache and intact24-bit master; no card access");
    return 0;
}
