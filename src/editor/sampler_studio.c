#include "sampler_studio.h"
#include <limits.h>
static int acquire(void *ctx,uint64_t key,uint64_t generation,struct pt_pcm *pcm,void **token)
{
    struct pt_sampler_studio *source=ctx;struct pt_sample_version *v;
    if(!source || !key || key>PT_PROJECT_SAMPLES || generation>UINT_MAX)return 0;
    if(pt_sampler_pin(source->sampler,source->project,(unsigned)key-1,(unsigned)generation,pcm,&v)!=PT_EDIT_OK)return 0;
    *token=v;return 1;
}
static void release(void *ctx,void *token) {(void)ctx;pt_sampler_unpin(token);}
struct pt_studio_source pt_sampler_studio_source(struct pt_sampler_studio *ctx)
{
    struct pt_studio_source source={ctx,acquire,release};return source;
}
