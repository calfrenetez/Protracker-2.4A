#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "paula_preview.h"
struct cancellation {unsigned calls,stop;};
static int progress(void *ctx,uint32_t done,uint32_t total)
{struct cancellation *c=ctx;assert(done<=total);return ++c->calls<c->stop;}
static void cancellation_tests(void)
{
    int32_t source_data[128],original[128],data[256];unsigned i;struct cancellation c;
    struct pt_pcm source={source_data,128,64,24000,2,24},out={data,256,128,48000,2,8};
    for(i=0;i<128;++i)source_data[i]=(int32_t)i*1000;
    memcpy(original,source_data,sizeof(original));data[0]=12345;
    c=(struct cancellation){0,1};
    assert(pt_paula_preview_prepare_progress(&source,&out,progress,&c)==PT_PCM_CANCELLED && data[0]==12345);
    c=(struct cancellation){0,2};
    assert(pt_paula_preview_prepare_progress(&source,&out,progress,&c)==PT_PCM_CANCELLED && c.calls==2);
    assert(!memcmp(original,source_data,sizeof(original)));
    c=(struct cancellation){0,999};assert(pt_paula_preview_prepare_progress(&source,&out,progress,&c)==PT_PCM_OK);
    out.rate=source.rate;out.frames=source.frames;c=(struct cancellation){0,2};
    assert(pt_paula_preview_prepare_progress(&source,&out,progress,&c)==PT_PCM_CANCELLED);
    assert(!memcmp(original,source_data,sizeof(original)));
}
int main(void)
{
    int32_t master[8]={8388607,-8388608,32768,-32768,1,-1,65536,-65536},original[8];
    int32_t data[32],reference[32];struct pt_pcm source={master,8,8,48000,1,24};
    struct pt_pcm dest={data,32,0,24000,1,8},expected={reference,32,4,24000,1,24},from;
    uint32_t n,a,b;memcpy(original,master,sizeof(master));
    assert(pt_paula_preview_frames(&source,dest.rate,&n)==PT_PCM_OK && n==4);dest.frames=n;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_OK);
    assert(pt_pcm_resample_filtered(&source,&expected)==PT_PCM_OK);
    from=expected;expected.bits=8;assert(pt_pcm_convert(&from,&expected)==PT_PCM_OK);
    assert(!memcmp(data,reference,n*sizeof(int32_t)) && !memcmp(master,original,sizeof(master)));
    a=99;b=100;
    assert(pt_paula_preview_loop(&source,24000,0,8,&a,&b)==PT_PCM_OK && a==0 && b==4);
    a=99;b=100;
    assert(pt_paula_preview_loop(&source,24000,2,6,&a,&b)==PT_PCM_CAPACITY && a==99 && b==100);
    assert(pt_paula_preview_loop(&source,24000,8,9,&a,&b)==PT_PCM_INVALID);
    assert(pt_paula_preview_loop(&source,24000,0,8,&a,&a)==PT_PCM_INVALID);
    source.rate=24000;source.frames=7;
    assert(pt_paula_preview_loop(&source,24000,1,7,&a,&b)==PT_PCM_OK && a==2 && b==6);
    assert(pt_paula_preview_loop(&source,48000,1,7,&a,&b)==PT_PCM_OK && a==2 && b==14);
    assert(!memcmp(master,original,sizeof(master)));
    source.frames=5;source.rate=24000;
    assert(pt_paula_preview_frames(&source,24000,&n)==PT_PCM_OK && n==6);dest.frames=n;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_OK && data[5]==0 && data[0]==127 && data[1]==-128);
    assert(!memcmp(master,original,sizeof(master)));
    dest.data=master;assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_ALIAS);
    dest.data=data;dest.capacity=5;data[0]=42;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_CAPACITY && data[0]==42);
    dest.capacity=32;source.channels=2;source.frames=4;
    assert(pt_paula_preview_frames(&source,24000,&n)==PT_PCM_OK && n==4);
    dest.channels=2;dest.frames=n;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_OK);
    assert(data[0]==127 && data[1]==-128 && data[2]==1 && data[3]==-1);
    source.frames=3;dest.frames=4;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_OK && data[6]==0 && data[7]==0);
    dest.capacity=7;assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_CAPACITY);
    dest.capacity=32;dest.data=master+4;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_ALIAS);
    dest.data=data;source.frames=4;source.rate=48000;dest.frames=2;
    assert(pt_paula_preview_prepare(&source,&dest)==PT_PCM_OK);
    expected.channels=2;expected.frames=2;expected.bits=24;
    assert(pt_pcm_resample_filtered(&source,&expected)==PT_PCM_OK);
    from=expected;expected.bits=8;assert(pt_pcm_convert(&from,&expected)==PT_PCM_OK);
    assert(!memcmp(data,reference,4*sizeof(int32_t)) && !memcmp(master,original,sizeof(master)));
    source.channels=1;source.frames=8;source.rate=1;
    assert(pt_paula_preview_frames(&source,192000,&n)==PT_PCM_CAPACITY);
    source.rate=192000;assert(pt_paula_preview_frames(&source,1,&n)==PT_PCM_CAPACITY);
    cancellation_tests();puts("PAULA PREVIEW PASS: filtered mono rate conversion, precision, padding, bounds and master preservation");return 0;
}
