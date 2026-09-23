#ifndef PT_PAULA_CACHE_H
#define PT_PAULA_CACHE_H
#include <string.h>
#include "mod_inspect.h"
/* Conservative working set: every stored pattern remains selectable/editable.
 * Instrument-only events also load a sample; note presence is irrelevant. */
struct pt_paula_cache_plan { size_t bytes; uint32_t instruments; struct pt_mod_info mod; };
static inline int pt_paula_cache_plan(const uint8_t *source,size_t bytes,struct pt_paula_cache_plan *out)
{
    struct pt_paula_cache_plan p;size_t i;
    if(!out || pt_mod_inspect(source,bytes,&p.mod)!=PT_MOD_OK || p.mod.warnings)return 0;
    p.bytes=p.mod.sample_offset;p.instruments=0;
    for(i=1084;i<p.mod.sample_offset;i+=4) {
        unsigned sample=(source[i]&0xf0)|(source[i+2]>>4);
        if(sample)p.instruments|=UINT32_C(1)<<(sample-1);
    }
    for(i=0;i<31;++i)if(p.instruments&(UINT32_C(1)<<i))
        p.bytes+=2*((size_t)source[42+i*30]*256+source[43+i*30]);
    *out=p;return 1;
}
/* Source/destination must be disjoint; source is an immutable private export,
 * never the master. The caller owns two further bytes for the silent DMA word. */
static inline int pt_paula_cache_copy(const uint8_t *source,size_t bytes,uint8_t *out,size_t capacity)
{
    struct pt_paula_cache_plan p;size_t i,from,to;
    if(!out || !pt_paula_cache_plan(source,bytes,&p) || capacity<p.bytes)return 0;
    if((uintptr_t)out<=(uintptr_t)source ? (uintptr_t)source-(uintptr_t)out<capacity : (uintptr_t)out-(uintptr_t)source<bytes)return 0;
    memcpy(out,source,p.mod.sample_offset);from=to=p.mod.sample_offset;
    for(i=0;i<31;++i) {
        size_t length=2*((size_t)source[42+i*30]*256+source[43+i*30]);
        if(p.instruments&(UINT32_C(1)<<i)) {memcpy(out+to,source+from,length);to+=length;}
        else {uint8_t *h=out+20+i*30;h[22]=h[23]=h[26]=h[27]=h[28]=0;h[29]=1;}
        from+=length;
    }
    return 1;
}
/* A new instrument set or changed sample/header requires stopped cache rebuild.
 * Row edits among already cached instruments may be published without touching
 * the private DMA bytes (including deliberate EFx mutations). */
static inline int pt_paula_cache_compatible(const uint8_t *before,const uint8_t *after,size_t bytes)
{
    struct pt_paula_cache_plan a,b;
    if(!pt_paula_cache_plan(before,bytes,&a) || !pt_paula_cache_plan(after,bytes,&b) ||
       a.instruments!=b.instruments || a.mod.sample_offset!=b.mod.sample_offset)return 0;
    return !memcmp(before+20,after+20,930) &&
        !memcmp(before+a.mod.sample_offset,after+b.mod.sample_offset,bytes-a.mod.sample_offset);
}
#endif
