#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "playback_internal.h"
/* Handles point to descriptors, deliberately not the simulated card bytes. */
struct device {unsigned live[4],uploads,releases,fail,fail_chunk;size_t next_offset,max_chunk;size_t sizes[4];uint8_t ram[4][1024];};
static void *allocate(void *ctx,size_t n)
{
    struct device *d=ctx;unsigned i;
    if(n>1024)return NULL;
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
static unsigned incremental;
static enum pt_cache_result job_upload(struct pt_sample_cache *c,const struct pt_pcm *p,uint32_t identity,uint64_t version,
    const struct pt_playback_format *f,uint8_t *staging,size_t capacity,void *context,
    int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out)
{
    struct pt_playback_upload_job job={0};struct pt_cache_lease result={99,123};
    struct device *d=context;unsigned writes=d->uploads;enum pt_cache_result r;
    r=incremental==1?pt_playback_upload_begin(&job,c,p,identity,version,f,context,write,&result):
        pt_playback_upload_begin_prepared(&job,c,p,identity,version,f,context,write,&result);
    assert(d->uploads==writes);
    while(r==PT_CACHE_PENDING) {
        size_t before=job.offset;writes=d->uploads;
        assert(result.slot==99 && result.serial==123 && c->entry[job.lease.slot].valid==0);
        assert(!pt_cache_trim(c,SIZE_MAX));
        r=pt_playback_upload_step(&job,staging,capacity,&result);
        assert(d->uploads<=writes+1);
        if(r==PT_CACHE_PENDING)assert(job.offset>before && job.offset-before<=PT_PLAYBACK_UPLOAD_CHUNK);
    }
    assert(!job.cache);pt_playback_upload_cancel(&job);
    if(r==PT_CACHE_LOAD || r==PT_CACHE_HIT)*out=result;
    else assert(result.slot==99 && result.serial==123);
    return r;
}
static void chunk_tests(unsigned prepared,unsigned source_bits)
{
    struct device d={0};struct pt_sample_cache c;struct pt_cache_lease a,b;
    int32_t data[]={8388607,-8388608,-32768,32768,1,-1,32768,-32768,65536,-65536};
    int32_t original[10];struct pt_pcm p={data,10,5,48000,2,24};
    struct pt_playback_format f;uint8_t staging[17],expected[16];
    unsigned bits,channel,endian,pad,capacity,uploads,i;size_t bytes;
    enum pt_cache_result (*upload_chunks)(struct pt_sample_cache *,const struct pt_pcm *,uint32_t,uint64_t,
        const struct pt_playback_format *,uint8_t *,size_t,void *,
        int (*)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *)=
        incremental?job_upload:prepared?pt_playback_pcm_upload_prepared:pt_playback_pcm_upload_chunks;
    p.bits=source_bits;for(i=0;i<10;++i)data[i]/=(int32_t)1<<(24-source_bits);
    assert(pt_pcm_validate(&p)==PT_PCM_OK);
    memcpy(original,data,sizeof(data));pt_cache_init(&c,&d,allocate,release,16);
    for(bits=8;bits<=16;bits+=8)for(channel=0;channel<2;++channel)
    for(endian=0;endian<2;++endian)for(pad=0;pad<2;++pad)
    for(capacity=bits/8;capacity<=11;++capacity) {
        f=(struct pt_playback_format){bits,channel,endian,pad};
        assert(pt_playback_pcm_size(&p,&f,&bytes)==PT_PCM_OK);
        assert(pt_playback_pcm_pack(&p,&f,expected,sizeof(expected))==PT_PCM_OK);
        d.next_offset=0;d.max_chunk=capacity;memset(staging,0xa5,sizeof(staging));
        assert(upload_chunks(&c,&p,1,1,&f,staging,capacity,&d,write_chunk,&a)==PT_CACHE_LOAD);
        assert(d.next_offset==bytes && staging[capacity]==0xa5);
        assert(!memcmp(d.ram[index_of(&d,pt_cache_data(&c,a))],expected,bytes));
        uploads=d.uploads;
        assert(upload_chunks(&c,&p,1,1,&f,NULL,0,&d,write_chunk,&b)==PT_CACHE_HIT);
        assert(d.uploads==uploads);assert(pt_cache_unpin(&c,b));
        assert(pt_cache_unpin(&c,a) && pt_cache_clear(&c));
    }
    f=(struct pt_playback_format){16,1,1,0};d.max_chunk=3;d.next_offset=0;d.fail_chunk=2;
    a=(struct pt_cache_lease){99,123};
    assert(upload_chunks(&c,&p,1,2,&f,staging,3,&d,write_chunk,&a)==PT_CACHE_TRANSFER);
    assert(c.bytes==0 && a.slot==99 && a.serial==123 && d.next_offset==4);
    d.next_offset=0;
    assert(upload_chunks(&c,&p,1,2,&f,staging,3,&d,write_chunk,&a)==PT_CACHE_LOAD);
    assert(pt_cache_unpin(&c,a) && pt_cache_clear(&c));uploads=d.uploads;
    assert(upload_chunks(&c,&p,1,3,&f,staging,1,&d,write_chunk,&a)==PT_CACHE_CAPACITY);
    assert(upload_chunks(&c,&p,1,3,&f,(uint8_t *)(data+8),3,&d,write_chunk,&a)==PT_CACHE_INVALID);
    assert(c.bytes==0 && d.uploads==uploads && !memcmp(data,original,sizeof(data)));
}
static void job_refusal(void)
{
    struct device d={0};struct pt_sample_cache c;struct pt_playback_upload_job job={0};
    struct pt_cache_lease out={99,123},old,busy;struct pt_playback_format f={8,0,0,1};
    int32_t data[5]={-128,-1,0,1,127};struct pt_pcm p={data,5,5,48000,1,8};uint8_t staging[16];
    unsigned cancel,step,mode,writes;
    pt_cache_init(&c,&d,allocate,release,16);d.max_chunk=1;
    for(cancel=0;cancel<6;++cancel) {
        d.next_offset=0;
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_INVALID);
        assert(pt_playback_pcm_upload_chunks(&c,&p,1,1,&f,staging,1,&d,write_chunk,&busy)==PT_CACHE_BUSY);
        for(step=0;step<cancel;++step)assert(pt_playback_upload_step(&job,staging,1,&out)==PT_CACHE_PENDING);
        pt_playback_upload_cancel(&job);pt_playback_upload_cancel(&job);
        assert(!c.bytes && !job.cache && out.slot==99 && out.serial==123);
        assert(pt_playback_upload_step(&job,staging,1,&out)==PT_CACHE_INVALID);
    }
    for(mode=1;mode<=6;++mode) {
        enum pt_cache_result result;
        d.next_offset=0;d.fail_chunk=mode;
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
        do{result=pt_playback_upload_step(&job,staging,1,&out);}while(result==PT_CACHE_PENDING);
        assert(result==PT_CACHE_TRANSFER && d.next_offset==mode && !job.cache && !c.bytes && out.serial==123);
    }
    for(mode=0;mode<5;++mode) {
        d.next_offset=0;
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
        assert(pt_playback_upload_step(&job,staging,1,&out)==PT_CACHE_PENDING);writes=d.uploads;
        if(mode==0)--p.capacity;
        if(mode==1)assert(!pt_cache_clear(&c));
        if(mode==2)d.fail_chunk=1;
        assert(pt_playback_upload_step(&job,mode==3?(uint8_t *)data:staging,mode==4?0:1,&out)==
            (mode==2?PT_CACHE_TRANSFER:mode==4?PT_CACHE_CAPACITY:PT_CACHE_INVALID));
        assert(d.uploads==writes+(mode==2) && !job.cache && !c.bytes && out.serial==123);
        p.capacity=5;
    }
    /* Cancelling a replacement cannot free the old voice's pinned bytes. */
    d.next_offset=0;d.max_chunk=16;
    assert(pt_playback_pcm_upload_chunks(&c,&p,1,1,&f,staging,16,&d,write_chunk,&old)==PT_CACHE_LOAD);
    assert(pt_playback_upload_begin(&job,&c,&p,1,2,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
    pt_playback_upload_cancel(&job);assert(pt_cache_data(&c,old) && c.bytes==6);
    assert(pt_cache_unpin(&c,old) && pt_cache_clear(&c));
    data[0]=-129;
    assert(pt_playback_upload_begin(&job,&c,&p,1,3,&f,&d,write_chunk,&out)==PT_CACHE_INVALID && !job.cache);
}
static void large_job(void)
{
    struct device d={0};struct pt_sample_cache c;struct pt_playback_upload_job job={0};
    struct pt_cache_lease out={99,123};struct pt_playback_format f={16,0,0,0};
    int32_t data[300];struct pt_pcm pcm={data,300,300,48000,1,24};uint8_t staging[1024],expected[600];unsigned i,steps=0;
    enum pt_cache_result result;
    for(i=0;i<300;++i)data[i]=(int32_t)i*257-40000;
    assert(pt_playback_pcm_pack(&pcm,&f,expected,sizeof(expected))==PT_PCM_OK);
    pt_cache_init(&c,&d,allocate,release,1024);d.max_chunk=256;
    result=pt_playback_upload_begin(&job,&c,&pcm,1,1,&f,&d,write_chunk,&out);
    assert(result==PT_CACHE_PENDING && !d.uploads);
    do{result=pt_playback_upload_step(&job,staging,sizeof(staging),&out);++steps;}while(result==PT_CACHE_PENDING);
    assert(result==PT_CACHE_LOAD && steps==3 && d.uploads==3 && d.next_offset==600);
    assert(!memcmp(d.ram[index_of(&d,pt_cache_data(&c,out))],expected,600));
    assert(pt_cache_unpin(&c,out) && pt_cache_clear(&c));
}
static void storage_guard_jobs(void)
{
    union master {int32_t values[64];struct {int32_t prefix[16];struct pt_cache_lease output;} padding;struct pt_playback_upload_job job;};
    unsigned bits,channels,i,mode;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        union master master,before;struct device d={0};struct pt_sample_cache c;
        struct pt_pcm p,oldp;struct pt_playback_format f={8,0,0,1},oldf=f;
        struct pt_cache_lease out={99,123},oldout=out;struct pt_playback_upload_job job={0};uint8_t staging[16];
        for(i=0;i<64;++i)master.values[i]=i&1?INT32_MIN:INT32_MAX;
        for(i=0;i<3*channels;++i)master.values[i]=i&1?-1:1;
        p=(struct pt_pcm){master.values,64,3,48000,(uint8_t)channels,(uint8_t)bits};memcpy(&oldp,&p,sizeof(p));memcpy(&before,&master,sizeof(master));
        memset(&out,0,sizeof(out));out.slot=99;out.serial=123;memcpy(&oldout,&out,sizeof(out));
        pt_cache_init(&c,&d,allocate,release,16);d.max_chunk=16;
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&master.padding.output)==PT_CACHE_INVALID);
        assert(!job.cache && !c.bytes && !d.uploads && !memcmp(&master,&before,sizeof(master)));
        for(mode=0;mode<6;++mode) {
            uint8_t *bad;
            d.next_offset=0;out=oldout;
            assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
            if(mode==0)bad=(uint8_t *)(master.values+3*channels);
            else if(mode==1)bad=(uint8_t *)master.values+sizeof(master.values)-1;
            else if(mode==2)bad=(uint8_t *)&p;
            else if(mode==3)bad=(uint8_t *)&job;
            else if(mode==4)bad=(uint8_t *)&out;
            else bad=(uint8_t *)(uintptr_t)(UINTPTR_MAX-2);
            assert(pt_playback_upload_step(&job,bad,16,&out)==PT_CACHE_INVALID);
            assert(!job.cache && !c.bytes && !d.uploads && !memcmp(&out,&oldout,sizeof(out)));
            assert(!memcmp(&master,&before,sizeof(master)) && !memcmp(&p,&oldp,sizeof(p)) && !memcmp(&f,&oldf,sizeof(f)));
            for(i=0;i<4;++i)assert(!d.live[i]);
        }
        d.next_offset=0;
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
        assert(pt_playback_upload_step(&job,staging,sizeof(staging),&master.padding.output)==PT_CACHE_INVALID);
        assert(!job.cache && !c.bytes && !d.uploads && !memcmp(&master,&before,sizeof(master)));
        d.next_offset=0;
        assert(pt_playback_upload_begin(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
        {
            struct pt_playback_upload_job oldjob;struct pt_sample_cache oldc;
            memcpy(&oldjob,&job,sizeof(job));memcpy(&oldc,&c,sizeof(c));
            assert(pt_playback_upload_step(&job,staging,sizeof(staging),&job.lease)==PT_CACHE_INVALID);
            assert(!memcmp(&job,&oldjob,sizeof(job)) && !memcmp(&c,&oldc,sizeof(c)) && !d.uploads);
            assert(!memcmp(&master,&before,sizeof(master)) && !memcmp(&out,&oldout,sizeof(out)));
            pt_playback_upload_cancel(&job);assert(!job.cache && !c.bytes);
        }
        d.next_offset=0;
        assert(pt_playback_pcm_upload(&c,&p,1,1,&f,(uint8_t *)(master.values+3*channels),16,&d,upload,&out)==PT_CACHE_INVALID);
        assert(!c.bytes && !d.uploads && !memcmp(&master,&before,sizeof(master)) && !memcmp(&out,&oldout,sizeof(out)));
        assert(pt_playback_pcm_upload(&c,&p,1,1,&f,(uint8_t *)&out,16,&d,upload,&out)==PT_CACHE_INVALID);
        assert(!c.bytes && !d.uploads && !memcmp(&out,&oldout,sizeof(out)));
        d.next_offset=0;
        assert(pt_playback_pcm_upload_chunks(&c,&p,1,1,&f,staging,sizeof(staging),&d,write_chunk,&out)==PT_CACHE_LOAD);
        assert(d.uploads==1 && !memcmp(&master,&before,sizeof(master)));
        assert(pt_cache_unpin(&c,out) && pt_cache_clear(&c));
        for(i=0;i<4;++i)assert(!d.live[i]);
    }
    {
        union master m,old;struct device d={0};struct pt_sample_cache c;struct pt_cache_lease out={99,123};
        struct pt_pcm p={m.values,64,1,48000,1,8};struct pt_playback_format f={8,0,0,0};
        memset(&m,0,sizeof(m));old=m;pt_cache_init(&c,&d,allocate,release,16);
        assert(pt_playback_upload_begin(&m.job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_INVALID);
        assert(!memcmp(&m,&old,sizeof(m)) && !c.bytes && !d.uploads && out.serial==123);
    }
}

static void prepared_storage_guards(void)
{
    union {int32_t values[64];struct {int32_t prefix[16];struct pt_cache_lease out;} padding;} m,before;
    struct pt_pcm p={m.values,64,3,48000,2,24},oldp;struct pt_playback_format f={16,1,1,0};
    struct pt_sample_cache c,oldc;struct device d={0};struct pt_playback_upload_job job={0},oldjob;
    struct pt_cache_lease out={99,123};uint8_t staging[16];unsigned i;
    for(i=0;i<64;++i)m.values[i]=INT32_MIN;for(i=0;i<6;++i)m.values[i]=65537;
    memcpy(&before,&m,sizeof(m));memcpy(&oldp,&p,sizeof(p));pt_cache_init(&c,&d,allocate,release,16);d.max_chunk=16;
    memcpy(&oldc,&c,sizeof(c));
    assert(pt_playback_upload_begin_prepared(&job,&c,&p,1,1,&f,&d,write_chunk,&m.padding.out)==PT_CACHE_INVALID);
    assert(!memcmp(&c,&oldc,sizeof(c)) && !job.cache && !d.uploads);
    assert(pt_playback_upload_begin_prepared(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
    assert(pt_playback_upload_step(&job,(uint8_t *)(m.values+6),16,&out)==PT_CACHE_INVALID);
    assert(!job.cache && !c.bytes && !d.uploads && out.serial==123);
    assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)));
    assert(pt_playback_upload_begin_prepared(&job,&c,&p,1,1,&f,&d,write_chunk,&out)==PT_CACHE_PENDING);
    memcpy(&oldjob,&job,sizeof(job));memcpy(&oldc,&c,sizeof(c));
    assert(pt_playback_upload_step(&job,staging,sizeof(staging),&job.lease)==PT_CACHE_INVALID);
    assert(!memcmp(&job,&oldjob,sizeof(job)) && !memcmp(&c,&oldc,sizeof(c)) && !d.uploads);
    pt_playback_upload_cancel(&job);assert(!job.cache && !c.bytes);
    assert(pt_playback_pcm_upload_chunks(&c,&p,1,1,&f,staging,sizeof(staging),&d,write_chunk,&out)==PT_CACHE_LOAD);
    memcpy(&oldc,&c,sizeof(c));
    assert(pt_playback_upload_begin_prepared(&job,&c,&p,1,1,&f,&d,write_chunk,&m.padding.out)==PT_CACHE_INVALID);
    assert(!memcmp(&c,&oldc,sizeof(c)) && !job.cache && d.uploads==1);
    assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)));
    assert(pt_cache_unpin(&c,out) && pt_cache_clear(&c));for(i=0;i<4;++i)assert(!d.live[i]);
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
    for(incremental=0;incremental<3;++incremental)for(i=8;i<=24;i+=8){chunk_tests(0,i);chunk_tests(1,i);}
    job_refusal();large_job();storage_guard_jobs();prepared_storage_guards();puts("PLAYBACK UPLOAD PASS: bounded chunks and transactional device resources");return 0;
}
