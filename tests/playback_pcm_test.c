#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "playback_pcm.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p,size_t n) {(void)ctx;(void)n;free(p);}
int main(void)
{
    int32_t data[]={-8388608,8388607,-32768,32768,-128,128,0,65536,8388607,-8388608};
    int32_t original[10];struct pt_pcm p={data,10,5,48000,2,24};
    struct pt_playback_format f={8,0,0,1};uint8_t out[16],old[16];size_t n;
    struct pt_sample_cache cache;struct pt_cache_lease a,b,c;
    const uint8_t left8[]={128,255,0,0,127,0},right8[]={127,1,0,1,128,0};
    const uint8_t left16[]={128,0,255,128,255,255,0,0,127,255};
    memcpy(original,data,sizeof(data));memset(out,0xaa,sizeof(out));memcpy(old,out,sizeof(out));
    assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_OK && n==6);
    assert(pt_playback_pcm_pack(&p,&f,out,5)==PT_PCM_CAPACITY && !memcmp(out,old,sizeof(out)));
    assert(pt_playback_pcm_pack(&p,&f,(uint8_t *)data,sizeof(data))==PT_PCM_ALIAS);
    assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_OK && !memcmp(out,left8,6) && out[6]==0xaa);
    f.channel=1;assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_OK && !memcmp(out,right8,6));
    f.bits=16;f.channel=0;assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_OK && !memcmp(out,left16,10));
    f.little_endian=1;assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_OK);
    {unsigned i;for(i=0;i<10;i+=2)assert(out[i]==left16[i+1] && out[i+1]==left16[i]);}
    assert(!memcmp(data,original,sizeof(data)) && p.bits==24 && p.frames==5 && p.rate==48000);
    /* Distinct representations coexist without rewriting the true24 master. */
    pt_cache_init(&cache,NULL,allocate,release,64);f=(struct pt_playback_format){8,0,0,1};
    assert(pt_playback_pcm_acquire(&cache,&p,7,1,&f,&a)==PT_CACHE_LOAD);
    assert(!memcmp(pt_cache_data(&cache,a),left8,6));
    f.bits=16;assert(pt_playback_pcm_acquire(&cache,&p,7,1,&f,&b)==PT_CACHE_LOAD);
    assert(!memcmp(pt_cache_data(&cache,b),left16,10));
    assert(pt_playback_pcm_acquire(&cache,&p,7,1,&f,&c)==PT_CACHE_HIT);assert(pt_cache_unpin(&cache,c));
    pt_playback_pcm_invalidate(&cache,7);assert(!pt_cache_publish(&cache,a));
    assert(pt_playback_pcm_acquire(&cache,&p,7,2,&f,&c)==PT_CACHE_LOAD);
    assert(pt_cache_unpin(&cache,a));assert(pt_cache_unpin(&cache,b));assert(pt_cache_unpin(&cache,c));
    assert(pt_cache_clear(&cache) && !cache.bytes);
    {int32_t values[]={-128,-1,0,1,127};struct pt_pcm eight={values,5,5,8287,1,8};
     const uint8_t expected[]={128,0,255,0,0,0,1,0,127,0};f=(struct pt_playback_format){16,0,0,0};
     assert(pt_playback_pcm_pack(&eight,&f,out,16)==PT_PCM_OK && !memcmp(out,expected,10));}
    f.channel=2;assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_INVALID);
    f.channel=0;f.bits=24;assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_INVALID);
    f.bits=8;data[0]=-8388609;memcpy(old,out,sizeof(out));
    assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_INVALID && !memcmp(out,old,sizeof(out)));
    data[0]=original[0];p.frames=0;p.data=NULL;
    assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_OK && !n);
    assert(pt_playback_pcm_pack(&p,&f,NULL,0)==PT_PCM_OK);
    puts("PLAYBACK PCM PASS: true24 master preservation, signed rounding/clipping, 8/16 bit endian/channel/padding, coexistence, revision invalidation and refusal");
    return 0;
}
