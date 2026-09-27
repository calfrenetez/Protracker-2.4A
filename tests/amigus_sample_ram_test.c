#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amigus_sample_ram.h"
#include "playback_pcm.h"
struct fixture {
    struct pt_amigus_sample_ram arena;
    struct pt_sample_cache cache;
    uint8_t memory[2048];
    uint32_t address;
    unsigned calls,fail;
    int owned,fail_result;
};
static int owned(void *p) {return ((struct fixture *)p)->owned;}
static int write32(void *p,unsigned reg,uint32_t value)
{
    struct fixture *f=p;unsigned i;assert(f->owned);++f->calls;
    if(reg==0x14)f->address=value;
    else {
        assert(reg==0x10 && !(f->address&3) && f->address<=sizeof(f->memory)-4);
        for(i=0;i<4;++i)f->memory[f->address+i]=(uint8_t)(value>>(24-8*i));
    }
    /* Failure may follow a partial/complete bus write. Resource stays unpublished. */
    return f->fail && f->calls==f->fail?f->fail_result:1;
}
static void init(struct fixture *f,unsigned capacity)
{
    memset(f,0,sizeof(*f));memset(f->memory,0xcc,sizeof(f->memory));f->owned=1;
    assert(pt_amigus_sample_ram_init(&f->arena,32,capacity,f,owned,write32));
    pt_cache_init(&f->cache,&f->arena,pt_amigus_sample_ram_allocate,pt_amigus_sample_ram_release,capacity);
    assert(!f->calls);
}
static unsigned live(struct fixture *f)
{
    unsigned i,n=0;for(i=0;i<PT_CACHE_SLOTS;++i)n+=!!f->arena.block[i].reserved;return n;
}
static void allocation(struct fixture *f)
{
    void *a,*b,*c,*d;unsigned i;void *blocks[PT_CACHE_SLOTS];struct pt_amigus_sample_ram before;
    init(f,40);before=f->arena;
    assert(!pt_amigus_sample_ram_init(&f->arena,1,40,f,owned,write32));
    assert(!pt_amigus_sample_ram_init(&f->arena,0xfffffffc,8,f,owned,write32));
    assert(!pt_amigus_sample_ram_init(&f->arena,0x02000000,4,f,owned,write32));
    assert(!pt_amigus_sample_ram_init(&f->arena,0x01fffffc,8,f,owned,write32));
    assert(!memcmp(&before,&f->arena,sizeof(before)));
    assert(pt_amigus_sample_ram_init(&f->arena,0x01fffffc,4,f,owned,write32));
    a=pt_amigus_sample_ram_allocate(&f->arena,4);assert(a && ((struct pt_amigus_ram_block *)a)->address==0x01fffffc);
    assert(!pt_amigus_sample_ram_allocate(&f->arena,1));
    pt_amigus_sample_ram_release(&f->arena,a,4);init(f,40);
    assert(!pt_amigus_sample_ram_allocate(&f->arena,0));
    assert(!pt_amigus_sample_ram_allocate(&f->arena,SIZE_MAX));
    a=pt_amigus_sample_ram_allocate(&f->arena,9);b=pt_amigus_sample_ram_allocate(&f->arena,9);
    c=pt_amigus_sample_ram_allocate(&f->arena,9);assert(a && b && c);
    assert(((struct pt_amigus_ram_block *)a)->address==32);
    assert(((struct pt_amigus_ram_block *)b)->address==44);
    assert(((struct pt_amigus_ram_block *)c)->address==56);
    pt_amigus_sample_ram_release(&f->arena,b,9);
    assert(!pt_amigus_sample_ram_allocate(&f->arena,13)); /* Fragmented16 bytes. */
    d=pt_amigus_sample_ram_allocate(&f->arena,12);assert(d==b);
    assert(((struct pt_amigus_ram_block *)c)->address==56); /* No relocation. */
    pt_amigus_sample_ram_release(&f->arena,d,11);assert(live(f)==3);
    pt_amigus_sample_ram_release(&f->arena,d,12);
    pt_amigus_sample_ram_release(&f->arena,a,9);pt_amigus_sample_ram_release(&f->arena,c,9);
    assert(!live(f) && !f->calls);
    init(f,256);
    for(i=0;i<PT_CACHE_SLOTS;++i) {blocks[i]=pt_amigus_sample_ram_allocate(&f->arena,1);assert(blocks[i]);}
    assert(!pt_amigus_sample_ram_allocate(&f->arena,1));
    for(i=0;i<PT_CACHE_SLOTS;++i)pt_amigus_sample_ram_release(&f->arena,blocks[i],1);
    assert(!live(f));
    f->owned=0;assert(!pt_amigus_sample_ram_allocate(&f->arena,4));assert(!f->calls);
}
static void conversion(struct fixture *f)
{
    int32_t data[258],original[258];uint8_t staging[256],expected[258];
    struct pt_pcm p={data,258,129,48000,2,24};struct pt_playback_format format;
    unsigned i,bits,channel,endian,k;const unsigned partitions[]={1,3,17,256};
    for(i=0;i<258;++i)data[i]=(i%3==0)?8388607:(i%3==1)?-8388608:257;
    memcpy(original,data,sizeof(data));
    for(bits=8;bits<=16;bits+=8)for(channel=0;channel<2;++channel)
    for(endian=0;endian<2;++endian)for(k=0;k<4;++k) {
        size_t size;uint32_t address,length;struct pt_cache_lease lease,hit;unsigned calls;
        if(partitions[k]<bits/8)continue;
        init(f,1024);format=(struct pt_playback_format){bits,channel,endian,0};
        assert(pt_playback_pcm_size(&p,&format,&size)==PT_PCM_OK);
        assert(pt_playback_pcm_pack(&p,&format,expected,sizeof(expected))==PT_PCM_OK);
        assert(pt_playback_pcm_upload_chunks(&f->cache,&p,1,1,&format,staging,partitions[k],&f->arena,pt_amigus_sample_ram_write,&lease)==PT_CACHE_LOAD);
        assert(pt_amigus_sample_ram_location(&f->arena,pt_cache_data(&f->cache,lease),&address,&length));
        assert(address==32 && length==size && !memcmp(f->memory+address,expected,size));
        for(i=0;i<32;++i)assert(f->memory[i]==0xcc);
        for(i=(unsigned)size;i<(size+3)/4*4;++i)assert(f->memory[address+i]==0);
        assert(f->memory[address+(size+3)/4*4]==0xcc);
        calls=f->calls;
        assert(pt_playback_pcm_upload_chunks(&f->cache,&p,1,1,&format,NULL,0,&f->arena,pt_amigus_sample_ram_write,&hit)==PT_CACHE_HIT);
        assert(f->calls==calls && hit.serial==lease.serial);
        assert(!pt_cache_clear(&f->cache));assert(pt_cache_unpin(&f->cache,hit));assert(live(f)==1);
        assert(pt_cache_unpin(&f->cache,lease));assert(!live(f) && !f->cache.bytes);
        assert(!memcmp(data,original,sizeof(data)));
    }
}
static enum pt_cache_result load(struct fixture *f,unsigned identity,unsigned version,unsigned frames,struct pt_cache_lease *lease)
{
    int32_t data[16];uint8_t staging[7];unsigned i;
    struct pt_pcm p={data,16,frames,48000,1,8};struct pt_playback_format format={8,0,0,0};
    for(i=0;i<16;++i)data[i]=(int32_t)(i+version);
    return pt_playback_pcm_upload_chunks(&f->cache,&p,identity,version,&format,staging,sizeof(staging),&f->arena,pt_amigus_sample_ram_write,lease);
}
static void lifecycle(struct fixture *f)
{
    struct pt_cache_lease a,b,c;uint8_t original[8];uint32_t address,size;
    init(f,16);assert(load(f,1,1,5,&a)==PT_CACHE_LOAD);memcpy(original,f->memory+32,8);
    assert(load(f,2,1,5,&b)==PT_CACHE_LOAD);assert(pt_cache_unpin(&f->cache,b));
    assert(load(f,3,1,9,&c)==PT_CACHE_CAPACITY); /* Rounded physical capacity. */
    assert(live(f)==1 && !memcmp(original,f->memory+32,8));
    assert(load(f,3,1,5,&c)==PT_CACHE_LOAD);assert(pt_cache_unpin(&f->cache,c));
    pt_playback_pcm_invalidate(&f->cache,1);
    assert(load(f,1,2,5,&b)==PT_CACHE_LOAD); /* Old generation stays pinned. */
    assert(!memcmp(original,f->memory+32,8));assert(pt_cache_data(&f->cache,a));
    assert(!pt_cache_clear(&f->cache));assert(pt_cache_unpin(&f->cache,b));assert(live(f)==1);
    assert(pt_cache_unpin(&f->cache,a));assert(!live(f));
    init(f,16);assert(load(f,1,1,5,&a)==PT_CACHE_LOAD);assert(pt_cache_unpin(&f->cache,a));
    assert(load(f,1,2,5,&b)==PT_CACHE_LOAD); /* Equal-sized allocation refill. */
    assert(pt_amigus_sample_ram_location(&f->arena,pt_cache_data(&f->cache,b),&address,&size));
    assert(address==32 && f->memory[address]==2);assert(pt_cache_unpin(&f->cache,b));
    f->owned=0;assert(!pt_amigus_sample_ram_location(&f->arena,&f->arena.block[0],&address,&size));
    assert(pt_cache_clear(&f->cache));assert(!live(f));
}
static void tails(struct fixture *f)
{
    unsigned n,i;const uint8_t data[]={0x80,0x01,0x7f,0xab,0xcd,0xef,0x23};
    for(n=1;n<=sizeof(data);++n) {
        void *p;uint32_t address,size;
        init(f,8);p=pt_amigus_sample_ram_allocate(&f->arena,n);assert(p);
        for(i=0;i<n;++i) {
            assert(pt_amigus_sample_ram_write(&f->arena,p,i,data+i,1));
            assert(pt_amigus_sample_ram_location(&f->arena,p,&address,&size)==(i+1==n));
        }
        assert(address==32 && size==n && !memcmp(f->memory+32,data,n));
        for(i=n;i<(n+3)/4*4;++i)assert(f->memory[32+i]==0);
        assert(f->memory[32+(n+3)/4*4]==0xcc);
        assert(f->calls==2*((n+3)/4));
        pt_amigus_sample_ram_release(&f->arena,p,n);assert(!live(f));
    }
}
static void failures(struct fixture *f)
{
    unsigned fail;struct pt_cache_lease out,sentinel={31,999};uint32_t address,size;uint8_t bytes[257]={0};void *p;
    for(fail=1;fail<=8;++fail) {
        init(f,32);f->fail=fail;out=sentinel;
        assert(load(f,1,1,13,&out)==PT_CACHE_TRANSFER);
        assert(out.slot==sentinel.slot && out.serial==sentinel.serial && !live(f) && !f->cache.bytes);
        f->fail=0;assert(load(f,1,1,13,&out)==PT_CACHE_LOAD);
        assert(pt_cache_unpin(&f->cache,out) && pt_cache_clear(&f->cache));assert(!live(f));
    }
    init(f,512);p=pt_amigus_sample_ram_allocate(&f->arena,257);assert(p);
    assert(!pt_amigus_sample_ram_write(&f->arena,p,0,bytes,257));assert(!f->calls);
    assert(!pt_amigus_sample_ram_location(&f->arena,p,&address,&size));
    assert(pt_amigus_sample_ram_write(&f->arena,p,0,bytes,1));
    assert(!pt_amigus_sample_ram_write(&f->arena,p,2,bytes,1));
    assert(!pt_amigus_sample_ram_write(&f->arena,p,1,bytes,1)); /* Poisoned. */
    assert(pt_amigus_sample_ram_write(&f->arena,p,0,bytes,1));
    f->owned=0;assert(!pt_amigus_sample_ram_write(&f->arena,p,1,bytes,1));assert(!f->calls);
    pt_amigus_sample_ram_release(&f->arena,p,257);assert(!live(f));
}
static void invalid_callback_results(struct fixture *f)
{
    const int statuses[]={-1,2};unsigned k,fail;
    struct pt_cache_lease out;uint32_t address,size;void *p;uint8_t data[4]={1,2,3,4};
    for(k=0;k<2;++k) {
        init(f,32);f->owned=statuses[k];
        assert(!pt_amigus_sample_ram_allocate(&f->arena,4) && !f->calls);
        f->owned=1;p=pt_amigus_sample_ram_allocate(&f->arena,4);assert(p);
        assert(pt_amigus_sample_ram_write(&f->arena,p,0,data,4));
        f->owned=statuses[k];address=size=999;
        assert(!pt_amigus_sample_ram_location(&f->arena,p,&address,&size));
        assert(address==999 && size==999);
        assert(!pt_amigus_sample_ram_write(&f->arena,p,0,data,4));
        assert(f->calls==2);pt_amigus_sample_ram_release(&f->arena,p,4);
        for(fail=1;fail<=2;++fail) {
            init(f,32);f->fail=fail;f->fail_result=statuses[k];out=(struct pt_cache_lease){31,999};
            assert(load(f,1,1,4,&out)==PT_CACHE_TRANSFER);
            assert(out.serial==999 && f->calls==fail && !live(f) && !f->cache.bytes);
            f->fail=0;assert(load(f,1,1,4,&out)==PT_CACHE_LOAD);
            assert(pt_cache_unpin(&f->cache,out) && pt_cache_clear(&f->cache));
        }
    }
}
int main(void)
{
    struct fixture *f=malloc(sizeof(*f));assert(f);
    allocation(f);conversion(f);lifecycle(f);tails(f);failures(f);invalid_callback_results(f);free(f);
    puts("AMIGUS SAMPLE RAM PASS: bounded addresses, exact uploads, pinned eviction, failures and master preservation; fake bus only");return 0;
}
