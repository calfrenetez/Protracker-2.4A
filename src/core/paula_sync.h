#ifndef PT_PAULA_SYNC_H
#define PT_PAULA_SYNC_H
#include "paula_cache.h"
#include "mod_project.h"
/* Prepare only metadata/pattern bytes in unpublished caller workspace. Compare
 * streamed sample bytes against the immutable export, never EF-mutated DMA.
 * Source project/export must remain stable and disjoint from workspace. Failure
 * may change workspace, but the caller must publish no rows until success. */
struct pt_paula_sync_stream {
    const uint8_t *before;
    uint8_t *workspace;
    size_t bytes,prefix,offset;
};
static inline int pt_paula_sync_sink(void *context,const uint8_t *data,size_t bytes)
{
    struct pt_paula_sync_stream *s=context;size_t start=s->offset,end,n;
    if(start>s->bytes || bytes>s->bytes-start)return 0;
    end=start+bytes;
    if(start<950 && end>20) {
        size_t from=start>20?start:20,to=end<950?end:950;
        if(memcmp(s->before+from,data+(from-start),to-from))return 0;
    }
    if(end>s->prefix) {
        size_t from=start>s->prefix?start:s->prefix;
        if(memcmp(s->before+from,data+(from-start),end-from))return 0;
    }
    n=start<s->prefix?s->prefix-start:0;if(n>bytes)n=bytes;
    if(n)memcpy(s->workspace+start,data,n);
    s->offset=end;return 1;
}
static inline int pt_paula_sync_prepare(const struct pt_project *p,
    const uint8_t *before,size_t bytes,uint8_t *workspace,size_t capacity)
{
    struct pt_paula_cache_plan plan;struct pt_paula_sync_stream stream;
    size_t i;uint32_t instruments=0;
    if(!p || !workspace || !pt_paula_cache_plan(before,bytes,&plan) ||
       capacity<plan.mod.sample_offset || 1084+(size_t)p->pattern_count*1024!=plan.mod.sample_offset)return 0;
    if((uintptr_t)workspace<=(uintptr_t)before ?
       (uintptr_t)before-(uintptr_t)workspace<capacity :
       (uintptr_t)workspace-(uintptr_t)before<bytes)return 0;
    stream=(struct pt_paula_sync_stream){before,workspace,bytes,plan.mod.sample_offset,0};
    if(pt_mod_export_stream(p,0,pt_paula_sync_sink,&stream)!=PT_PROJECT_OK || stream.offset!=bytes)return 0;
    for(i=1084;i<plan.mod.sample_offset;i+=4) {
        unsigned sample=(workspace[i]&0xf0)|(workspace[i+2]>>4);
        if(sample>31)return 0;
        if(sample)instruments|=UINT32_C(1)<<(sample-1);
    }
    return instruments==plan.instruments;
}
#endif
