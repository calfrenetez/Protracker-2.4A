#ifndef PT_PAULA_PREVIEW_H
#define PT_PAULA_PREVIEW_H
#include "pcm.h"
/* Mono Paula preview only. Storage is caller-owned Fast workspace; padding
 * belongs to the derived copy. No stereo downmix. Loop mapping affects derived coordinates only. */
static inline enum pt_pcm_result pt_paula_preview_frames(const struct pt_pcm *source,
                                                         uint32_t rate,uint32_t *frames)
{
    uint32_t n;enum pt_pcm_result r=pt_pcm_validate(source);
    if(r!=PT_PCM_OK)return r;
    if(source->channels!=1 || !frames)return PT_PCM_INVALID;
    r=pt_pcm_resampled_frames(source,rate,&n);if(r!=PT_PCM_OK)return r;
    if(n>131070 || (uint64_t)rate*128<source->rate)return PT_PCM_CAPACITY;
    *frames=n+(n&1);return PT_PCM_OK;
}
/* Map a forward loop to nearest DMA word boundaries (ties advance). Keep the
 * end within real converted audio, never the silent pad. Refuse collapsed or
 * one-word loops instead of disabling looping. Output is atomic on failure. */
static inline enum pt_pcm_result pt_paula_preview_loop(const struct pt_pcm *source,
    uint32_t rate,uint32_t start,uint32_t end,uint32_t *mapped_start,uint32_t *mapped_end)
{
    uint32_t padded,frames,a,b;uint64_t divisor;enum pt_pcm_result r;
    if(!source || !mapped_start || !mapped_end || mapped_start==mapped_end || start>=end || end>source->frames)
        return PT_PCM_INVALID;
    r=pt_paula_preview_frames(source,rate,&padded);if(r!=PT_PCM_OK)return r;
    r=pt_pcm_resampled_frames(source,rate,&frames);if(r!=PT_PCM_OK)return r;
    divisor=(uint64_t)source->rate*2;
    a=(uint32_t)(((uint64_t)start*rate+source->rate)/divisor)*2;
    b=(uint32_t)(((uint64_t)end*rate+source->rate)/divisor)*2;
    if(b>(frames&~1U))b=frames&~1U;
    if(a>=b || b-a<=2)return PT_PCM_CAPACITY;
    *mapped_start=a;*mapped_end=b;return PT_PCM_OK;
}
static inline enum pt_pcm_result pt_paula_preview_prepare(const struct pt_pcm *source,
                                                         struct pt_pcm *out)
{
    struct pt_pcm converted,from;uint32_t padded,n;enum pt_pcm_result r;
    uintptr_t a,b;size_t src_bytes,dst_bytes;
    if(!out)return PT_PCM_INVALID;
    r=pt_paula_preview_frames(source,out->rate,&padded);if(r!=PT_PCM_OK)return r;
    if(out->capacity<padded)return PT_PCM_CAPACITY;
    if(out->channels!=1 || out->bits!=8 || out->frames!=padded)return PT_PCM_INVALID;
    src_bytes=(size_t)source->frames*sizeof(int32_t);dst_bytes=(size_t)padded*sizeof(int32_t);
    a=(uintptr_t)source->data;b=(uintptr_t)out->data;
    if(dst_bytes && (!out->data || (a<=b?b-a<src_bytes:a-b<dst_bytes)))return PT_PCM_ALIAS;
    r=pt_pcm_resampled_frames(source,out->rate,&n);if(r!=PT_PCM_OK)return r;
    converted=*out;converted.frames=n;
    if(source->rate==out->rate)r=pt_pcm_convert(source,&converted);
    else {
        converted.bits=source->bits;
        r=pt_pcm_resample_filtered(source,&converted);
        if(r==PT_PCM_OK) {from=converted;converted.bits=8;r=pt_pcm_convert(&from,&converted);}
    }
    if(r!=PT_PCM_OK)return r;
    if(padded>n)out->data[n]=0;
    return PT_PCM_OK;
}
#endif
