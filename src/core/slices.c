#include <string.h>
#include "slices.h"
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!an || !bn)return 0;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return 1;
    return x<y+bn && y<x+an;
}
int pt_slices_valid(uint32_t frames,const uint32_t *markers,size_t count)
{
    size_t i;if(count>4096 || (count && !markers))return 0;
    for(i=0;i<count;++i)if(markers[i]>=frames || (i && markers[i]<=markers[i-1]))return 0;
    return 1;
}
enum pt_slice_result pt_slice_insert(uint32_t frames,uint32_t *markers,size_t *count,size_t capacity,uint32_t frame)
{
    size_t i;
    if(!count || !markers || frame>=frames || !pt_slices_valid(frames,markers,*count))return PT_SLICE_INVALID;
    for(i=0;i<*count && markers[i]<frame;++i) {}
    if(i<*count && markers[i]==frame)return PT_SLICE_OK;
    if(*count>=capacity || *count==4096)return PT_SLICE_CAPACITY;
    if(overlap(markers,(*count+1)*sizeof(*markers),count,sizeof(*count)))return PT_SLICE_ALIAS;
    memmove(markers+i+1,markers+i,(*count-i)*sizeof(*markers));markers[i]=frame;++*count;return PT_SLICE_OK;
}
enum pt_slice_result pt_slice_remove(uint32_t frames,uint32_t *markers,size_t *count,size_t index)
{
    if(!count || !pt_slices_valid(frames,markers,*count) || index>=*count)return PT_SLICE_INVALID;
    if(overlap(markers,*count*sizeof(*markers),count,sizeof(*count)))return PT_SLICE_ALIAS;
    memmove(markers+index,markers+index+1,(*count-index-1)*sizeof(*markers));--*count;return PT_SLICE_OK;
}
static uint32_t amplitude(const struct pt_pcm *pcm,uint32_t frame)
{
    unsigned c;uint32_t peak=0;
    for(c=0;c<pcm->channels;++c) {
        int32_t v=pcm->data[(size_t)frame*pcm->channels+c];uint32_t a=(uint32_t)(v<0?-v:v);
        if(a>peak)peak=a;
    }
    return peak;
}
static int crossing(const struct pt_pcm *pcm,uint32_t frame)
{
    unsigned c;if(!frame)return amplitude(pcm,0)==0;
    for(c=0;c<pcm->channels;++c) {
        int32_t a=pcm->data[(size_t)(frame-1)*pcm->channels+c],b=pcm->data[(size_t)frame*pcm->channels+c];
        if(a && b && ((a<0)==(b<0)))return 0;
    }
    return 1;
}
static uint32_t refine(const struct pt_pcm *pcm,uint32_t frame,uint32_t lower,uint32_t radius)
{
    uint32_t d;
    for(d=0;d<=radius;++d) {
        if(d<=frame && frame-d>=lower && crossing(pcm,frame-d))return frame-d;
        if(d && d<pcm->frames-frame && crossing(pcm,frame+d))return frame+d;
    }
    return frame;
}
static size_t propose(const struct pt_pcm *pcm,const struct pt_slice_options *o,uint32_t threshold,uint32_t *out)
{
    uint32_t frame,last=0,envelope=0;size_t count=pcm->frames?1:0;int armed=1;
    if(count && out)out[0]=0;
    for(frame=0;frame<pcm->frames;++frame) {
        uint32_t a=amplitude(pcm,frame),target=a*256,baseline=envelope/256;
        uint32_t attack=a>baseline?a-baseline:0;
        if(attack>=threshold && armed) {
            if(frame>=last && frame-last>=o->minimum_spacing) {
                uint32_t marker=refine(pcm,frame,last+o->minimum_spacing,o->zero_radius);
                if(out)out[count]=marker;
                last=marker;if(++count>4096)return count;
            }
            armed=0;
        } else if(attack<threshold/2+1)armed=1;
        if(target>=envelope)envelope+=(target-envelope)>>o->envelope_shift;
        else envelope-=(envelope-target)>>o->envelope_shift;
    }
    return count;
}
enum pt_slice_result pt_auto_slice(const struct pt_pcm *pcm,const struct pt_slice_options *o,uint32_t *out,size_t capacity,size_t *written)
{
    uint32_t frame,peak=0,threshold;size_t needed,pcm_bytes;
    if(pt_pcm_validate(pcm)!=PT_PCM_OK || !o || !written || !o->minimum_spacing ||
       !o->threshold_per_mille || o->threshold_per_mille>1000 || !o->envelope_shift ||
       o->envelope_shift>12 || o->zero_radius>4096)return PT_SLICE_INVALID;
    for(frame=0;frame<pcm->frames;++frame) {uint32_t a=amplitude(pcm,frame);if(a>peak)peak=a;}
    threshold=(uint32_t)(((uint64_t)peak*o->threshold_per_mille+999)/1000);if(!threshold)threshold=1;
    needed=propose(pcm,o,threshold,NULL);
    if(needed>4096 || needed>capacity)return PT_SLICE_CAPACITY;
    if(needed && !out)return PT_SLICE_INVALID;
    pcm_bytes=(size_t)pcm->frames*pcm->channels*sizeof(*pcm->data);
    if(overlap(out,needed*sizeof(*out),pcm->data,pcm_bytes) || overlap(out,needed*sizeof(*out),pcm,sizeof(*pcm)) ||
       overlap(out,needed*sizeof(*out),o,sizeof(*o)) || overlap(out,needed*sizeof(*out),written,sizeof(*written)) ||
       overlap(written,sizeof(*written),pcm->data,pcm_bytes) || overlap(written,sizeof(*written),pcm,sizeof(*pcm)) ||
       overlap(written,sizeof(*written),o,sizeof(*o)))return PT_SLICE_ALIAS;
    propose(pcm,o,threshold,out);*written=needed;return PT_SLICE_OK;
}
enum pt_pcm_result pt_pcm_crossfade_loop(struct pt_pcm *pcm,uint32_t start,uint32_t end,uint32_t fade,uint32_t *next_start)
{
    enum pt_pcm_result result=pt_pcm_validate(pcm);uint32_t i;unsigned c;
    if(result!=PT_PCM_OK)return result;
    if(!next_start || start>=end || end>pcm->frames || !fade || fade>(end-start)/2)return PT_PCM_INVALID;
    if(overlap(next_start,sizeof(*next_start),pcm,sizeof(*pcm)) ||
       overlap(next_start,sizeof(*next_start),pcm->data,(size_t)pcm->frames*pcm->channels*sizeof(*pcm->data)))return PT_PCM_ALIAS;
    for(i=0;i<fade;++i)for(c=0;c<pcm->channels;++c) {
        size_t head=((size_t)start+i)*pcm->channels+c,tail=((size_t)end-fade+i)*pcm->channels+c;
        int64_t blended=(int64_t)pcm->data[tail]*(fade-1-i)+(int64_t)pcm->data[head]*(i+1);
        pcm->data[tail]=(int32_t)(blended/fade);
    }
    *next_start=start+fade;return PT_PCM_OK;
}
