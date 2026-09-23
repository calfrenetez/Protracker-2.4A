#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "playback_pcm.h"
/* Handles point to descriptors, deliberately not the simulated card bytes. */
struct device {unsigned live[4],uploads,releases,fail,fail_chunk;size_t next_offset,max_chunk;size_t sizes[4];uint8_t ram[4][16];};
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
static int write_chunk(void *ctx,void *p,size_t offset,const uint8_t *data,size_t n)
{
    struct device *d=ctx;unsigned i=index_of(d,p);
    assert(d->live[i] && offset==d->next_offset && n && n<=d->max_chunk);
    assert(offset<=d->sizes[i] && n<=d->sizes[i]-offset);
    memcpy(d->ram[i]+offset,data,n);d->next_offset+=n;++d->uploads;
    if(d->fail_chunk && --d->fail_chunk==0)return 0;
    return 1;
}
static void chunk_tests(void)
{
    struct device d={0};struct pt_sample_cache c;struct pt_cache_lease a,b;
    int32_t data[]={8388607,-8388608,-32768,32768,1,-1,32768,-32768,65536,-65536};
    int32_t original[10];struct pt_pcm p={data,10,5,48000,2,24};
    struct pt_playback_format f;uint8_t staging[17],expected[16];
    unsigned bits,channel,endian,pad,capacity,uploads;size_t bytes;
    memcpy(original,data,sizeof(data));pt_cache_init(&c,&d,allocate,release,16);
    for(bits=8;bits<=16;bits+=8)for(channel=0;channel<2;++channel)
    for(endian=0;endian<2;++endian)for(pad=0;pad<2;++pad)
    for(capacity=bits/8;capacity<=11;++capacity) {
        f=(struct pt_playback_format){bits,channel,endian,pad};
        assert(pt_playback_pcm_size(&p,&f,&bytes)==PT_PCM_OK);
        assert(pt_playback_pcm_pack(&p,&f,expected,sizeof(expected))==PT_PCM_OK);
        d.next_offset=0;d.max_chunk=capacity;memset(staging,0xa5,sizeof(staging));
        assert(pt_playback_pcm_upload_chunks(&c,&p,1,1,&f,staging,capacity,&d,write_chunk,&a)==PT_CACHE_LOAD);
        assert(d.next_offset==bytes && staging[capacity]==0xa5);
        assert(!memcmp(d.ram[index_of(&d,pt_cache_data(&c,a))],expected,bytes));
        uploads=d.uploads;
        assert(pt_playback_pcm_upload_chunks(&c,&p,1,1,&f,NULL,0,&d,write_chunk,&b)==PT_CACHE_HIT);
        assert(d.uploads==uploads);assert(pt_cache_unpin(&c,b));
        assert(pt_cache_unpin(&c,a) && pt_cache_clear(&c));
    }
    f=(struct pt_playback_format){16,1,1,0};d.max_chunk=3;d.next_offset=0;d.fail_chunk=2;
    a=(struct pt_cache_lease){99,123};
    assert(pt_playback_pcm_upload_chunks(&c,&p,1,2,&f,staging,3,&d,write_chunk,&a)==PT_CACHE_TRANSFER);
    assert(c.bytes==0 && a.slot==99 && a.serial==123 && d.next_offset==4);
    d.next_offset=0;
    assert(pt_playback_pcm_upload_chunks(&c,&p,1,2,&f,staging,3,&d,write_chunk,&a)==PT_CACHE_LOAD);
    assert(pt_cache_unpin(&c,a) && pt_cache_clear(&c));uploads=d.uploads;
    assert(pt_playback_pcm_upload_chunks(&c,&p,1,3,&f,staging,1,&d,write_chunk,&a)==PT_CACHE_CAPACITY);
    assert(pt_playback_pcm_upload_chunks(&c,&p,1,3,&f,(uint8_t *)(data+8),3,&d,write_chunk,&a)==PT_CACHE_INVALID);
    assert(c.bytes==0 && d.uploads==uploads && !memcmp(data,original,sizeof(data)));
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
    chunk_tests();puts("PLAYBACK UPLOAD PASS: bounded chunks and transactional device resources");return 0;
}
