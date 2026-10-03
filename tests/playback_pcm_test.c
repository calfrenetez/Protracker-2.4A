#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "playback_pcm.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p,size_t n) {(void)ctx;(void)n;free(p);}
static void storage_guard_tests(void)
{
    union master {int32_t values[32];size_t scalar;struct {int32_t prefix[8];size_t scalar;} padding;};
    union descriptor {struct pt_pcm pcm;size_t scalar;};
    union settings {struct pt_playback_format format;size_t scalar;};
    static const int32_t active[3][3]={{-128,-1,127},{-32768,-257,32767},{-8388608,-65537,8388607}};
    static const uint8_t wire16[3][6]={{128,0,255,0,127,0},{128,0,254,255,127,255},{128,0,255,0,127,255}};
    const uint8_t wire8[4]={128,255,127,0};
    unsigned bits,channels,i,k;size_t n;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        union master m,before;union descriptor p,oldp;union settings f,oldf;
        uint8_t out[16],oldout[16];void *targets[7];
        for(i=0;i<32;++i)m.values[i]=i&1?INT32_MIN:INT32_MAX;
        for(i=0;i<3*channels;++i)m.values[i]=active[bits/8-1][i/channels];
        p.pcm=(struct pt_pcm){m.values,32,3,48000,(uint8_t)channels,(uint8_t)bits};
        f.format=(struct pt_playback_format){8,0,0,1};memcpy(&before,&m,sizeof(m));memcpy(&oldp,&p,sizeof(p));memcpy(&oldf,&f,sizeof(f));
        n=123;assert(pt_playback_pcm_size(&p.pcm,&f.format,&n)==PT_PCM_OK && n==4);
        assert(pt_playback_pcm_size(&p.pcm,&f.format,&m.scalar)==PT_PCM_ALIAS);
        assert(pt_playback_pcm_size(&p.pcm,&f.format,&m.padding.scalar)==PT_PCM_ALIAS);
        assert(pt_playback_pcm_size(&p.pcm,&f.format,&p.scalar)==PT_PCM_ALIAS);
        assert(pt_playback_pcm_size(&p.pcm,&f.format,&f.scalar)==PT_PCM_ALIAS);
        assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)) && !memcmp(&f,&oldf,sizeof(f)));
        targets[0]=m.values;targets[1]=m.values+3*channels;targets[2]=(uint8_t *)m.values+sizeof(m.values)-1;
        targets[3]=&p.pcm;targets[4]=(uint8_t *)&p.pcm+sizeof(p.pcm)-1;
        targets[5]=&f.format;targets[6]=(uint8_t *)&f.format+sizeof(f.format)-1;
        for(k=0;k<7;++k) {
            assert(pt_playback_pcm_pack(&p.pcm,&f.format,targets[k],16)==PT_PCM_ALIAS);
            assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)) && !memcmp(&f,&oldf,sizeof(f)));
        }
        memset(out,0xa5,sizeof(out));assert(pt_playback_pcm_pack(&p.pcm,&f.format,out,sizeof(out))==PT_PCM_OK);
        assert(!memcmp(out,wire8,4) && out[4]==0xa5);
        f.format.bits=16;f.format.word_pad=0;memcpy(&oldf,&f,sizeof(f));
        assert(pt_playback_pcm_pack(&p.pcm,&f.format,out,sizeof(out))==PT_PCM_OK);
        assert(!memcmp(out,wire16[bits/8-1],6) && out[6]==0xa5);
        assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)) && !memcmp(&f,&oldf,sizeof(f)));
        memset(out,0xa5,sizeof(out));memcpy(oldout,out,sizeof(out));
        assert(pt_playback_pcm_pack(&p.pcm,&f.format,(uint8_t *)(uintptr_t)(UINTPTR_MAX-2),16)==PT_PCM_ALIAS);
        assert(pt_playback_pcm_pack(&p.pcm,&f.format,(uint8_t *)m.values,1)==PT_PCM_CAPACITY);
        assert(pt_playback_pcm_pack(&p.pcm,&f.format,NULL,16)==PT_PCM_INVALID);
        assert(!memcmp(out,oldout,sizeof(out)) && !memcmp(&m,&before,sizeof(m)));
    }
    {
        int32_t value=0;struct pt_pcm p={NULL,0,0,48000,1,8};struct pt_playback_format f={8,0,0,1};
        size_t n=123;uint8_t out[4]={1,2,3,4},before[4];memcpy(before,out,sizeof(out));
        assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_OK && !n);
        assert(pt_playback_pcm_pack(&p,&f,NULL,0)==PT_PCM_OK);
        assert(pt_playback_pcm_pack(&p,&f,(uint8_t *)&p,sizeof(p))==PT_PCM_OK);
        n=123;p.capacity=1;assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_ALIAS && n==123);
        assert(pt_playback_pcm_pack(&p,&f,NULL,0)==PT_PCM_OK);
        p.data=&value;p.capacity=SIZE_MAX/sizeof(value)+1;
        assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_ALIAS && n==123);
        p.capacity=1;p.data=(int32_t *)(uintptr_t)(UINTPTR_MAX-2);
        assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_ALIAS && n==123);
        p.data=&value;p.frames=2;
        assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_CAPACITY && n==123);
        p.frames=1;f.bits=24;
        assert(pt_playback_pcm_pack(&p,&f,out,sizeof(out))==PT_PCM_INVALID && !memcmp(out,before,sizeof(out)));
    }
}

struct guard_allocations {unsigned allocations,releases;};
static void *guard_allocate(void *ctx,size_t n)
{struct guard_allocations *a=ctx;void *p=malloc(n);if(p)++a->allocations;return p;}
static void guard_release(void *ctx,void *p,size_t n)
{struct guard_allocations *a=ctx;(void)n;++a->releases;free(p);}
static void acquire_output_guards(void)
{
    union {int32_t values[32];struct {int32_t prefix[8];struct pt_cache_lease out;} padding;} m,before;
    union {struct pt_pcm pcm;struct pt_cache_lease out;} p,oldp;
    struct pt_playback_format f={8,0,0,1};struct pt_sample_cache c,oldc;
    struct pt_cache_lease a,b;struct guard_allocations alloc={0};unsigned i;
    for(i=0;i<32;++i)m.values[i]=INT32_MAX;m.values[0]=65537;
    p.pcm=(struct pt_pcm){m.values,32,1,48000,1,24};
    memcpy(&before,&m,sizeof(m));memcpy(&oldp,&p,sizeof(p));pt_cache_init(&c,&alloc,guard_allocate,guard_release,16);
    memcpy(&oldc,&c,sizeof(c));
    assert(pt_playback_pcm_acquire(&c,&p.pcm,1,1,&f,&m.padding.out)==PT_CACHE_INVALID);
    assert(pt_playback_pcm_acquire(&c,&p.pcm,1,1,&f,&p.out)==PT_CACHE_INVALID);
    assert(!memcmp(&c,&oldc,sizeof(c)) && !alloc.allocations && !alloc.releases);
    assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)));
    assert(pt_playback_pcm_acquire(&c,&p.pcm,1,1,&f,&a)==PT_CACHE_LOAD);
    memcpy(&oldc,&c,sizeof(c));
    assert(pt_playback_pcm_acquire(&c,&p.pcm,1,1,&f,&m.padding.out)==PT_CACHE_INVALID);
    assert(pt_playback_pcm_acquire(&c,&p.pcm,1,1,&f,&p.out)==PT_CACHE_INVALID);
    assert(!memcmp(&c,&oldc,sizeof(c)) && alloc.allocations==1 && !alloc.releases);
    assert(!memcmp(&m,&before,sizeof(m)) && !memcmp(&p,&oldp,sizeof(p)));
    assert(pt_playback_pcm_acquire(&c,&p.pcm,1,1,&f,&b)==PT_CACHE_HIT);
    assert(pt_cache_unpin(&c,b) && pt_cache_unpin(&c,a) && pt_cache_clear(&c));
    assert(!c.bytes && alloc.allocations==alloc.releases);
}

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
    /* Preserve the old no-byte pack even with missing retained capacity;
     * publishing a scalar now refuses that malformed full-storage span. */
    n=123;assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_ALIAS && n==123);
    assert(pt_playback_pcm_pack(&p,&f,NULL,0)==PT_PCM_OK);
    p.capacity=0;assert(pt_playback_pcm_size(&p,&f,&n)==PT_PCM_OK && !n);
    storage_guard_tests();acquire_output_guards();
    puts("PLAYBACK PCM PASS: true24 master preservation, signed rounding/clipping, 8/16 bit endian/channel/padding, coexistence, revision invalidation and refusal");
    return 0;
}
