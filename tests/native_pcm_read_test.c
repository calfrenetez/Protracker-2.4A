#include "amigus_pcm_read.h"
#include <amigus/amigus.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    struct AmiGUS card;
    struct pt_amigus_reservation r;
    uint16_t registers[9], copy[9], value;
    unsigned i;
    memset(&card,0,sizeof(card)); memset(&r,0,sizeof(r));
    for (i=0;i<9;++i) registers[i]=(uint16_t)(0xa500+i);
    memcpy(copy,registers,sizeof(copy));
    card.agus_TypeId=AmiGUS_mini; card.agus_FirmwareRev=0x7ea663e7UL;
    card.agus_PcmBase=(APTR)registers;
    r.card=&card; r.opened=r.reserved=r.access=1; r.resource=PT_AMIGUS_PCM;
    for (i=0;i<5;++i) {
        unsigned offset=i==4 ? 0x10 : i*2;
        value=0; assert(pt_native_amigus_pcm_read16(&r,offset,&value));
        assert(value==registers[offset/2]);
    }
    for (i=0;i<14;++i) {
        struct pt_amigus_reservation bad=r;
        struct AmiGUS changed=card;
        unsigned offset=0x06;
        bad.card=&changed;
        if (i==0) bad.opened=0;
        if (i==1) bad.reserved=0;
        if (i==2) bad.access=0;
        if (i==3) bad.access=2;
        if (i==4) bad.interrupt=1;
        if (i==5) bad.resource=PT_AMIGUS_WAVETABLE;
        if (i==6) bad.card=NULL;
        if (i==7) changed.agus_TypeId=AmiGUS_Zorro2;
        if (i==8) changed.agus_HardwareRev=1;
        if (i==9) changed.agus_FirmwareRev++;
        if (i==10) changed.agus_PcmBase=NULL;
        if (i==11) changed.agus_PcmBase=(APTR)((unsigned char *)registers+1);
        if (i==12) changed.agus_PcmBase=(APTR)(UINTPTR_MAX-1);
        if (i==13) offset=0x0c; /* Data port must never be read. */
        value=0x1234;
        assert(!pt_native_amigus_pcm_read16(&bad,offset,&value) && value==0x1234);
    }
    assert(!pt_native_amigus_pcm_read16(NULL,0x06,&value));
    assert(!pt_native_amigus_pcm_read16(&r,0x06,NULL));
    assert(!memcmp(registers,copy,sizeof(copy)));
    puts("PCM READ PASS: five status reads, sixteen refusal guards, no memory writes");
    {
        struct pt_native_amigus_quiesce q={0};
        struct pt_amigus_reservation bad=r;
        bad.access=0;
        assert(!pt_native_amigus_quiesce_begin(&q,&bad));
        assert(!q.owner && !q.requested && !memcmp(registers,copy,sizeof(copy)));
        assert(pt_native_amigus_quiesce_begin(&q,&r));
        assert(registers[0]==7 && registers[1]==7 && registers[3]==0 && registers[4]==0);
        for (i=0;i<9;++i) if(i!=0 && i!=1 && i!=3 && i!=4) assert(registers[i]==copy[i]);
        assert(!pt_native_amigus_quiesce_begin(&q,&r));
        memcpy(copy,registers,sizeof(copy));
        assert(pt_native_amigus_quiesce_poll(&q)==0 && !q.confirmed);
        assert(!memcmp(registers,copy,sizeof(copy))); /* Pending poll writes nothing. */
        registers[1]=0x50; registers[8]=0; /* Simulated device clear/empty readback. */
        assert(pt_native_amigus_quiesce_poll(&q)==1 && q.confirmed);
        memcpy(copy,registers,sizeof(copy));
        r.access=0;
        assert(pt_native_amigus_quiesce_poll(&q)==-1 && !q.confirmed);
        assert(!memcmp(registers,copy,sizeof(copy)));
    }
    puts("PCM QUIESCE PASS: four exact disable/reset stores, one-shot begin, read-only pending/lost-owner polls; synthetic memory only");
    return 0;
}
