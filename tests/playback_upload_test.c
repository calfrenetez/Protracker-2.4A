#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "playback_pcm.h"
/* Handles point to descriptors, deliberately not the simulated card bytes. */
struct device {unsigned live[4],uploads,releases,fail;size_t sizes[4];uint8_t ram[4][16];};
static void *allocate(void *ctx,size_t n)
{
    struct device *d=ctx;unsigned i;
    if(n>16)return NULL;
    for(i=0;i<4;++i)if(!d->live[i]) {d->live[i]=1;d->sizes[i]=n;return &d->live[i];}
    return NULL;
}
static unsigned index_of(struct device *d,void *p)
{unsigned i;for(i=0;i<4;++i)if(p==&d->live[i])return i;assert(0);return 0;}
static void release(void *ctx,void *p,size_t n)
{struct device *d=ctx;unsigned i=index_of(d,p);assert(d->live[i] && d->sizes[i]==n);d->live[i]=0;++d->releases;}
static int upload(void *ctx,void *p,const uint8_t *data,size_t n)
{
    struct device *d=ctx;unsigned i=index_of(d,p);assert(d->live[i] && d->sizes[i]==n);
    ++d->uploads;memcpy(d->ram[i],data,n);return !d->fail;
}
int main(void)
{
    struct device d={0};struct pt_sample_cache c;struct pt_cache_lease a,b,old,sentinel={99,123};
    int32_t master[]={8388607,-8388608,32768};int32_t original[3];
    struct pt_pcm p={master,3,3,48000,1,24};struct pt_playback_format f={16,0,1,0};
    uint8_t staging[16];unsigned i,uploads;memcpy(original,master,sizeof(master));
    pt_cache_init(&c,&d,allocate,release,12);
    assert(pt_playback_pcm_upload(&c,&p,7,1,&f,staging,16,&d,upload,&a)==PT_CACHE_LOAD);
    i=index_of(&d,pt_cache_data(&c,a));assert(!memcmp(d.ram[i],"\377\177\000\200\200\000",6));
    uploads=d.uploads;
    assert(pt_playback_pcm_upload(&c,&p,7,1,&f,NULL,0,&d,upload,&b)==PT_CACHE_HIT);
    assert(d.uploads==uploads && pt_cache_unpin(&c,b));
    old=a;pt_playback_pcm_invalidate(&c,7);assert(d.live[i] && !pt_cache_publish(&c,old));
    assert(pt_playback_pcm_upload(&c,&p,7,2,&f,staging,16,&d,upload,&b)==PT_CACHE_LOAD);
    assert(pt_cache_data(&c,old)!=pt_cache_data(&c,b));
    a=sentinel;
    assert(pt_playback_pcm_upload(&c,&p,8,1,&f,staging,16,&d,upload,&a)==PT_CACHE_CAPACITY);
    assert(a.slot==99 && a.serial==123 && d.live[i]);
    assert(pt_cache_unpin(&c,old) && !d.live[i]);
    assert(!pt_cache_clear(&c) && pt_cache_data(&c,b));assert(pt_cache_unpin(&c,b));
    assert(pt_cache_clear(&c) && c.bytes==0);
    /* Partial failed transfer cannot be hit or exposed; retry reuploads. */
    d.fail=1;
    assert(pt_playback_pcm_upload(&c,&p,7,3,&f,staging,16,&d,upload,&a)==PT_CACHE_TRANSFER);
    assert(c.bytes==0 && a.slot==99 && a.serial==123);d.fail=0;
    assert(pt_playback_pcm_upload(&c,&p,7,3,&f,staging,16,&d,upload,&a)==PT_CACHE_LOAD);
    assert(pt_cache_unpin(&c,a));
    /* Different representation replaces only unpinned data under pressure. */
    f.bits=8;f.word_pad=1;
    assert(pt_playback_pcm_upload(&c,&p,7,3,&f,staging,16,&d,upload,&b)==PT_CACHE_LOAD);
    assert(!memcmp(d.ram[index_of(&d,pt_cache_data(&c,b))],"\177\200\001\000",4));
    assert(pt_cache_unpin(&c,b));assert(pt_cache_clear(&c));
    uploads=d.uploads;
    assert(pt_playback_pcm_upload(&c,&p,7,4,&f,staging,3,&d,upload,&b)==PT_CACHE_CAPACITY);
    assert(pt_playback_pcm_upload(&c,&p,7,4,&f,(uint8_t *)master,sizeof(master),&d,upload,&b)==PT_CACHE_INVALID);
    assert(d.uploads==uploads && c.bytes==0 && !memcmp(master,original,sizeof(master)));
    for(i=0;i<4;++i)assert(!d.live[i]);
    puts("PLAYBACK UPLOAD PASS");return 0;
}
