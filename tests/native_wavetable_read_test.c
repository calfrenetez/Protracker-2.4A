#include "../src/native/amigus_wavetable_read.h"
#include <amigus/amigus.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct AmiGUS card={0};
    struct pt_amigus_reservation r={0};
    struct pt_native_amigus_wavetable_read n={0};
    uint16_t registers[8],copy[8],value;
    unsigned i;
    for (i=0;i<8;++i) registers[i]=(uint16_t)(0xa500+i);
    memcpy(copy,registers,sizeof(copy));
    card.agus_TypeId=AmiGUS_mini; card.agus_FirmwareRev=0x7ea663e7UL;
    card.agus_WavetableBase=(APTR)registers;
    r.card=&card; r.opened=r.reserved=r.access=1; r.resource=PT_AMIGUS_WAVETABLE;
    assert(pt_native_amigus_wavetable_read_bind(&n,&r));
    assert(!pt_native_amigus_wavetable_read_bind(&n,&r));
    for (i=0;i<8;++i) {
        value=0; assert(pt_native_amigus_wavetable_read16(&n,i*2,&value));
        assert(value==registers[i]);
    }
    for (i=0;i<256;++i) if (i>14 || (i&1)) {
        value=0x1234;
        assert(!pt_native_amigus_wavetable_read16(&n,i,&value) && value==0x1234);
    }
    assert(!pt_native_amigus_wavetable_read16(&n,0,NULL));
    assert(!pt_native_amigus_wavetable_read16(NULL,0,&value));
    assert(!pt_native_amigus_wavetable_read_bind(NULL,&r));
    assert(!memcmp(registers,copy,sizeof(copy)));
    for (i=0;i<16;++i) {
        struct pt_amigus_reservation bad=r;
        struct AmiGUS changed=card;
        struct pt_native_amigus_wavetable_read fresh={0}, active={0};
        assert(pt_native_amigus_wavetable_read_bind(&active,&r));
        if (i==0) bad.opened=0;
        if (i==1) bad.reserved=0;
        if (i==2) bad.access=0;
        if (i==3) bad.access=2;
        if (i==4) bad.interrupt=1;
        if (i==5) bad.resource=PT_AMIGUS_PCM;
        if (i==6) bad.card=NULL;
        if (i==7) changed.agus_TypeId=AmiGUS_Zorro2;
        if (i==8) changed.agus_HardwareRev=1;
        if (i==9) changed.agus_FirmwareRev++;
        if (i==10) changed.agus_WavetableBase=NULL;
        if (i==11) changed.agus_WavetableBase=(APTR)((unsigned char *)registers+1);
        if (i==12) changed.agus_WavetableBase=(APTR)(UINTPTR_MAX-1);
        /* Valid but different descriptor/base: cannot redirect a bound reader. */
        if (i==13) changed.agus_WavetableBase=(APTR)copy;
        if (i>=7) bad.card=&changed;
        if (i<13) assert(!pt_native_amigus_wavetable_read_bind(&fresh,&bad));
        if (i==14) {fresh.faulted=1; assert(!pt_native_amigus_wavetable_read_bind(&fresh,&r));}
        if (i==15) {fresh.registers=1; assert(!pt_native_amigus_wavetable_read_bind(&fresh,&r));}
        r=bad;
        value=0x1234;
        assert(!pt_native_amigus_wavetable_read16(&active,0,&value) && value==0x1234 && active.faulted);
        /* Even restoring the owner cannot resume a faulted reader. */
        r.card=&card; r.opened=r.reserved=r.access=1; r.interrupt=0; r.resource=PT_AMIGUS_WAVETABLE;
        assert(!pt_native_amigus_wavetable_read16(&active,0,&value));
        assert(!memcmp(registers,copy,sizeof(copy)));
    }
    puts("WAVETABLE READ PASS: eight global reads, all unsafe offsets refused, descriptor/lease loss latched, no writes; synthetic storage only");
    return 0;
}
