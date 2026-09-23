#include <assert.h>
#include <stdlib.h>
#include "paula_cache.h"
int main(void)
{
    enum { DATA=1084+2048, SIZE=DATA+12 };
    uint8_t source[SIZE]={0},copy[SIZE],cache[SIZE],changed[SIZE];
    struct pt_paula_cache_plan p;struct pt_mod_info info;unsigned i;
    source[950]=1;source[1079]=1;memcpy(source+1080,"M.K.",4);
    for(i=0;i<31;++i)source[49+i*30]=1;
    source[43]=2;source[73]=2;source[103]=2;
    /* Instrument 3 only in an otherwise inactive stored pattern, no note. */
    source[1086]=0x10;source[1084+1024+2]=0x30;
    for(i=0;i<12;++i)source[DATA+i]=(uint8_t)(i+1);
    memcpy(copy,source,SIZE);
    assert(pt_paula_cache_plan(source,SIZE,&p) && p.instruments==5 && p.bytes==DATA+8);
    memset(cache,0xa5,SIZE);
    assert(!pt_paula_cache_copy(source,SIZE,cache,p.bytes-1));assert(cache[0]==0xa5);
    assert(!pt_paula_cache_copy(source,SIZE,source,SIZE));
    assert(pt_paula_cache_copy(source,SIZE,cache,SIZE));
    assert(!memcmp(source,copy,SIZE));assert(cache[73]==0 && cache[79]==1);
    assert(!memcmp(cache+DATA,source+DATA,4));assert(!memcmp(cache+DATA+4,source+DATA+8,4));
    assert(cache[DATA+8]==0xa5); /* caller's guard untouched */
    assert(pt_mod_inspect(cache,p.bytes,&info)==PT_MOD_OK && !info.warnings);
    memcpy(changed,source,SIZE);changed[1085]=0x71;
    assert(pt_paula_cache_compatible(source,changed,SIZE));
    changed[1086]=0x20;assert(!pt_paula_cache_compatible(source,changed,SIZE));
    memcpy(changed,source,SIZE);changed[DATA]++;assert(!pt_paula_cache_compatible(source,changed,SIZE));
    memcpy(changed,source,SIZE);changed[45]=32;assert(!pt_paula_cache_compatible(source,changed,SIZE));
    /* Clearing/changing Chip playback bytes must not change compatibility. */
    cache[DATA]^=0xff;assert(pt_paula_cache_compatible(source,copy,SIZE));
    source[1086]=source[1084+1024+2]=0;
    assert(pt_paula_cache_plan(source,SIZE,&p) && !p.instruments && p.bytes==DATA);
    assert(pt_paula_cache_copy(source,SIZE,cache,SIZE));
    assert(pt_mod_inspect(cache,DATA,&info)==PT_MOD_OK && !info.sample_bytes);
    source[1086]=0xf0;source[1084]=0x10; /* instrument 31, empty */
    assert(pt_paula_cache_plan(source,SIZE,&p) && p.instruments==UINT32_C(0x40000000));
    source[1084]=0x20;assert(!pt_paula_cache_plan(source,SIZE,&p));
    assert(!pt_paula_cache_plan(source,10,&p));
    return 0;
}
