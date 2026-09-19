#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "slices.h"
static int32_t data[1024],before[1024];
static uint32_t markers[4096];
int main(void)
{
    struct pt_pcm pcm={data,1024,512,48000,2,24};struct pt_slice_options options={64,4,500,5};
    size_t count=99,i;uint32_t loop_start=99;
    memset(data,0,sizeof(data));
    for(i=128;i<132;++i) {data[i*2]=1000;data[i*2+1]=-1000;}
    for(i=300;i<304;++i) {data[i*2]=2000;data[i*2+1]=-2000;}
    for(i=320;i<324;++i) {data[i*2]=2000;data[i*2+1]=-2000;}
    memcpy(before,data,sizeof(data));memset(markers,0x5a,sizeof(markers));
    assert(pt_auto_slice(&pcm,&options,markers,1,&count)==PT_SLICE_CAPACITY && count==99 && markers[0]==0x5a5a5a5aUL);
    assert(pt_auto_slice(&pcm,&options,markers,4096,&count)==PT_SLICE_OK);
    assert(count==3 && markers[0]==0 && markers[1]==128 && markers[2]==300);
    assert(!memcmp(data,before,sizeof(data)) && pt_slices_valid(pcm.frames,markers,count));
    assert(pt_slice_insert(512,markers,&count,4096,200)==PT_SLICE_OK && count==4 && markers[2]==200);
    assert(pt_slice_insert(512,markers,&count,4096,200)==PT_SLICE_OK && count==4);
    assert(pt_slice_remove(512,markers,&count,2)==PT_SLICE_OK && count==3 && markers[2]==300);
    assert(pt_slice_insert(512,markers,&count,4096,512)==PT_SLICE_INVALID);
    assert(pt_slice_insert(512,markers,&count,3,400)==PT_SLICE_CAPACITY && count==3);
    assert(pt_auto_slice(&pcm,&options,(uint32_t *)data,4096,&count)==PT_SLICE_ALIAS && !memcmp(data,before,sizeof(data)));
    for(i=0;i<1024;++i)data[i]=1000;
    assert(pt_auto_slice(&pcm,&options,markers,4096,&count)==PT_SLICE_OK && count==1);
    memset(data,0,sizeof(data));assert(pt_auto_slice(&pcm,&options,markers,4096,&count)==PT_SLICE_OK && count==1);
    pcm.frames=0;assert(pt_auto_slice(&pcm,&options,NULL,0,&count)==PT_SLICE_OK && !count);
    pcm.frames=8;pcm.channels=2;
    for(i=0;i<8;++i) {data[i*2]=(int32_t)(i*100);data[i*2+1]=-(int32_t)(i*100);}
    memcpy(before,data,sizeof(data));
    assert(pt_pcm_crossfade_loop(&pcm,0,8,5,&loop_start)==PT_PCM_INVALID && loop_start==99 && !memcmp(before,data,sizeof(data)));
    assert(pt_pcm_crossfade_loop(&pcm,0,8,2,(uint32_t *)data)==PT_PCM_ALIAS);
    assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&loop_start)==PT_PCM_OK && loop_start==2);
    assert(data[12]==300 && data[13]==-300 && data[14]==100 && data[15]==-100);
    assert(!memcmp(data,before,12*sizeof(*data)) && pt_pcm_validate(&pcm)==PT_PCM_OK);
    pcm.bits=24;data[0]=-8388608;data[1]=8388607;data[12]=8388607;data[13]=-8388608;
    assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&loop_start)==PT_PCM_OK && pt_pcm_validate(&pcm)==PT_PCM_OK);
    puts("SLICES PASS: deterministic non-destructive stereo transient proposals, minimum spacing, manual markers, silence, capacity/alias safety and full-precision crossfade loops");return 0;
}
