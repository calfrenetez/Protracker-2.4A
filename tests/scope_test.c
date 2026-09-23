#include <assert.h>
#include <string.h>
#include "scope.h"
int main(void)
{
    int8_t data[18],a[81],b[81];struct pt_scope_phase phase={0};unsigned i;
    for(i=0;i<16;++i)data[i]=(int8_t)((int)i-8);
    data[16]=data[17]=0;
    pt_scope_wave(&phase,a,data,18,0,16,4,7,1,0,1,1,125,1000);
    assert(a[0]==0); /* 20 samples: initial 16, then four into loop. */
    pt_scope_wave(&phase,b,data,18,0,16,4,7,1,0,2,1,125,1000);
    assert(b[0]==-1 && memcmp(a,b,sizeof(a)));
    pt_scope_wave(&phase,b,data,18,0,16,4,7,2,2,2,1,125,1000);
    assert(b[0]==-8); /* Same sample retrigger starts its attack again. */
    pt_scope_wave(&phase,a,data,18,0,16,4,7,3,3,3,2,125,1000);
    assert(a[16]==-8 && b[16]==-7); /* Higher period stretches the waveform. */
    pt_scope_wave(&phase,a,data,18,0,16,16,2,4,0,1,1,125,1000);
    for(i=0;i<81;++i)assert(a[i]==0); /* Finished one-shot: silent DMA guard. */
    pt_scope_wave(&phase,a,data,18,17,2,0,2,5,0,0,1,125,1000);
    for(i=0;i<81;++i)assert(a[i]==0); /* Bounds checked before sampling. */
    pt_scope_wave(&phase,a,data,18,0,16,17,2,5,0,0,1,125,1000);
    for(i=0;i<81;++i)assert(a[i]==0);
    pt_scope_wave(&phase,a,data,18,0,16,4,7,5,0,0,0,125,1000);
    for(i=0;i<81;++i)assert(a[i]==0);
    /* Compare loop stepping with direct indexing across arbitrary loop sizes,
       short repeat words and steps larger than the loop. */
    {
        unsigned repeat,period,tick,j;
        for(repeat=1;repeat<=16;++repeat)for(period=1;period<900;period+=31)for(tick=0;tick<40;tick+=3) {
            uint64_t pos=((uint64_t)tick*3546895*5*65536)/(250UL*period);
            uint64_t step=((uint64_t)3546895<<16)/(period*16000UL);
            struct pt_scope_phase fresh={0};
            pt_scope_wave(&fresh,a,data,18,0,16,1,repeat,1,0,tick,period,125,3546895);
            for(j=0;j<81;++j,pos+=step) {
                unsigned offset=pos<(16UL<<16)?(unsigned)(pos>>16):1+(unsigned)(((pos-(16UL<<16))%((uint64_t)repeat<<16))>>16);
                assert(a[j]==data[offset]);
            }
        }
    }
    return 0;
}
