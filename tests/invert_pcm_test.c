#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "invert_pcm.h"
int main(void)
{
    int32_t original[8]={-128,-64,-1,0,1,63,64,127},copy[8],before[8];
    struct pt_pcm source={original,8,8,48000,1,8};
    struct pt_invert_pcm w,saved;struct pt_invert_loop a={0},b={0},old;
    memset(&w,0,sizeof(w));assert(pt_invert_pcm_init(&w,&source,copy,8)==PT_PCM_OK);
    assert(!memcmp(copy,original,sizeof(copy)) && w.pcm.data!=source.data);
    assert(pt_invert_loop_bind(&a,8,0,8) && pt_invert_loop_bind(&b,8,0,8));
    assert(pt_invert_loop_speed(&a,15) && pt_invert_loop_speed(&b,15));
    assert(pt_invert_pcm_update(&w,&a)==PT_PCM_OK && copy[1]==63 && original[1]==-64);
    assert(pt_invert_pcm_update(&w,&b)==PT_PCM_OK && copy[1]==-64);
    assert(pt_invert_pcm_update(&w,&a)==PT_PCM_OK && copy[2]==0);
    assert(pt_invert_pcm_reset(&w)==PT_PCM_OK && !memcmp(copy,original,sizeof(copy)));
    saved=w;memcpy(before,copy,sizeof(copy));
    assert(pt_invert_pcm_init(&w,&source,original,8)==PT_PCM_ALIAS);
    assert(!memcmp(&w,&saved,sizeof(w)) && !memcmp(copy,before,sizeof(copy)));
    assert(pt_invert_pcm_init(&w,&source,copy,7)==PT_PCM_CAPACITY);
    assert(!memcmp(&w,&saved,sizeof(w)) && !memcmp(copy,before,sizeof(copy)));
    a.end=10;old=a;assert(pt_invert_pcm_update(&w,&a)==PT_PCM_INVALID);
    assert(!memcmp(&a,&old,sizeof(a)) && !memcmp(copy,before,sizeof(copy)));
    source.bits=16;assert(pt_invert_pcm_reset(&w)==PT_PCM_INVALID);
    assert(!memcmp(&w,&saved,sizeof(w)) && !memcmp(copy,before,sizeof(copy)));
    {
        int32_t all[256],private_data[256];unsigned i,pass;
        struct pt_pcm src={all,256,256,48000,1,8};struct pt_invert_loop clock={0};
        for(i=0;i<256;++i)all[i]=(int)i-128;
        assert(pt_invert_pcm_init(&w,&src,private_data,256)==PT_PCM_OK);
        assert(pt_invert_loop_bind(&clock,256,0,256) && pt_invert_loop_speed(&clock,15));
        for(pass=0;pass<2;++pass) {
            for(i=0;i<256;++i)assert(pt_invert_pcm_update(&w,&clock)==PT_PCM_OK);
            for(i=0;i<256;++i)assert(private_data[i]==(pass?all[i]:-1-all[i]) && all[i]==(int)i-128);
        }
    }
    puts("INVERT private PCM PASS");return 0;
}
