#include <stdint.h>
#include <string.h>
#include "pcm.h"
#include "sinc_kernel.h"
/* Symmetric 16-zero-crossing Blackman-windowed sinc, integer Q24 table.
   Each output is normalized for exact DC gain; source endpoints are extended.
   No floating point or allocation is used by the sample loop. */
enum pt_pcm_result pt_pcm_resample_filtered_progress(const struct pt_pcm *source,struct pt_pcm *dest,pt_pcm_progress progress,void *context)
{
    uint32_t needed,radius,denominator,frame,chunk,next=0,left=0,fraction=0,whole,remainder_step,cached=UINT32_MAX;
    int32_t weights[4098];int64_t weight_sum=0;unsigned channel;
    uintptr_t a,b;size_t an,bn;
    enum pt_pcm_result result=pt_pcm_validate(source);
    if(result!=PT_PCM_OK)return result;
    if(!dest || !dest->rate || dest->rate>192000 || dest->bits!=source->bits || dest->channels!=source->channels)return PT_PCM_INVALID;
    result=pt_pcm_resampled_frames(source,dest->rate,&needed);if(result!=PT_PCM_OK)return result;
    if(dest->frames!=needed || (needed && !dest->data))return PT_PCM_INVALID;
    if(needed>SIZE_MAX/sizeof(int32_t)/dest->channels || dest->capacity<(size_t)needed*dest->channels)return PT_PCM_CAPACITY;
    denominator=source->rate>dest->rate?source->rate:dest->rate;
    radius=(16*denominator+dest->rate-1)/dest->rate;
    if(radius>2048)return PT_PCM_CAPACITY;
    a=(uintptr_t)source->data;b=(uintptr_t)dest->data;
    an=(size_t)source->frames*source->channels*sizeof(int32_t);bn=(size_t)needed*dest->channels*sizeof(int32_t);
    if(an && bn && (an>UINTPTR_MAX-a || bn>UINTPTR_MAX-b || (a<b+bn && b<a+an)))return PT_PCM_ALIAS;
    if(progress && !progress(context,0,needed))return PT_PCM_CANCELLED;
    if(source->rate==dest->rate) {
        if(bn)memcpy(dest->data,source->data,bn);
        return progress && !progress(context,needed,needed)?PT_PCM_CANCELLED:PT_PCM_OK;
    }
    chunk=4096/(2*radius+2);if(!chunk)chunk=1;if(chunk>32)chunk=32;
    whole=source->rate/dest->rate;remainder_step=source->rate%dest->rate;
    for(frame=0;frame<needed;++frame) {
        int64_t sum[2]={0,0};int offset;
        if(progress && frame==next) {if(frame && !progress(context,frame,needed))return PT_PCM_CANCELLED;next=frame>UINT32_MAX-chunk?UINT32_MAX:frame+chunk;}
        if(cached!=fraction) {
            weight_sum=0;
            for(offset=-(int)radius;offset<=(int)radius+1;++offset) {
                int32_t distance=offset*(int32_t)dest->rate-(int32_t)fraction,weight=0;
                uint32_t scaled,index,remainder,phase;
                if(distance<0)distance=-distance;
                /* Rate <=192000 and bounded radius keep scaled below 2^32.
                   Q14 phase and maximum kernel delta 89763 fit signed 32 bits.
                   Avoid software 64-bit division for every filter tap on 68k. */
                scaled=(uint32_t)distance*256;index=scaled/denominator;remainder=scaled-index*denominator;
                if(index<4096) {
                    phase=(remainder*16384)/denominator;
                    weight=pt_filter_kernel[index]+(pt_filter_kernel[index+1]-pt_filter_kernel[index])*(int32_t)phase/16384;
                }
                weights[offset+(int)radius]=weight;weight_sum+=weight;
            }
            cached=fraction;
        }
        for(offset=-(int)radius;offset<=(int)radius+1;++offset) {
            int32_t weight=weights[offset+(int)radius];uint32_t input;
            if(!weight)continue;
            if(offset<0 && left<(unsigned)(-offset))input=0;
            else if(offset>0 && (unsigned)offset>=source->frames-left)input=source->frames-1;
            else input=left+offset;
            for(channel=0;channel<source->channels;++channel)sum[channel]+=(int64_t)source->data[(size_t)input*source->channels+channel]*weight;
        }
        for(channel=0;channel<source->channels;++channel) {
            int64_t value=sum[channel],peak=(int32_t)1<<(source->bits-1);
            value=value<0?-((-value+weight_sum/2)/weight_sum):(value+weight_sum/2)/weight_sum;
            if(value< -peak)value= -peak;else if(value>=peak)value=peak-1;
            dest->data[(size_t)frame*dest->channels+channel]=(int32_t)value;
        }
        if(frame+1<needed) {
            left+=whole;fraction+=remainder_step;
            if(fraction>=dest->rate) {fraction-=dest->rate;++left;}
        }
    }
    if(progress && !progress(context,needed,needed))return PT_PCM_CANCELLED;
    return PT_PCM_OK;
}
enum pt_pcm_result pt_pcm_resample_filtered(const struct pt_pcm *source,struct pt_pcm *dest)
{return pt_pcm_resample_filtered_progress(source,dest,NULL,NULL);}
