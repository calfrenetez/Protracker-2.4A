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
    return 0;
}
