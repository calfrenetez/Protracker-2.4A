#include "raw.h"
static int valid(const struct pt_raw_format *f)
{return f && f->rate && f->rate<=192000 && (f->bits==8 || f->bits==16 || f->bits==24) && (f->channels==1 || f->channels==2) && f->little_endian<=1 && f->unsigned8<=1 && (!f->unsigned8 || f->bits==8);}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an && bn && (an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an));
}
enum pt_raw_result pt_raw_frames(size_t n,const struct pt_raw_format *f,uint32_t *out)
{
    unsigned align;
    if(!valid(f) || !out)return PT_RAW_INVALID;
    align=f->channels*(f->bits/8);
    if(n%align)return PT_RAW_INVALID;
    if(n/align>UINT32_MAX)return PT_RAW_CAPACITY;
    *out=(uint32_t)(n/align);return PT_RAW_OK;
}
enum pt_raw_result pt_raw_decode(const uint8_t *bytes,size_t n,const struct pt_raw_format *f,struct pt_pcm *pcm)
{
    uint32_t frames;size_t i,count;unsigned width,j;enum pt_raw_result result=pt_raw_frames(n,f,&frames);
    if(result!=PT_RAW_OK)return result;
    if(!pcm || (n && !bytes) || (frames && !pcm->data) || pcm->frames!=frames || pcm->bits!=f->bits || pcm->channels!=f->channels || pcm->rate!=f->rate)return PT_RAW_INVALID;
    if(frames>SIZE_MAX/sizeof(int32_t)/f->channels || frames>pcm->capacity/f->channels)return PT_RAW_CAPACITY;
    count=(size_t)frames*f->channels;width=f->bits/8;
    if(overlap(bytes,n,pcm->data,count*sizeof(int32_t)))return PT_RAW_ALIAS;
    for(i=0;i<count;++i) {
        uint32_t value=0;
        for(j=0;j<width;++j)value|=(uint32_t)bytes[i*width+j]<<(8*(f->little_endian?j:width-1-j));
        pcm->data[i]=f->unsigned8?(int32_t)value-128:(int32_t)value-((value&((uint32_t)1<<(f->bits-1)))?((int32_t)1<<f->bits):0);
    }
    return PT_RAW_OK;
}
enum pt_raw_result pt_raw_size(const struct pt_pcm *pcm,const struct pt_raw_format *f,size_t *out)
{
    uint64_t n;
    if(!valid(f) || !out || pt_pcm_validate(pcm)!=PT_PCM_OK || pcm->bits!=f->bits || pcm->channels!=f->channels || pcm->rate!=f->rate)return PT_RAW_INVALID;
    n=(uint64_t)pcm->frames*f->channels*(f->bits/8);
    if(n>SIZE_MAX)return PT_RAW_CAPACITY;
    *out=(size_t)n;return PT_RAW_OK;
}
enum pt_raw_result pt_raw_encode(const struct pt_pcm *pcm,const struct pt_raw_format *f,uint8_t *bytes,size_t capacity,size_t *written)
{
    size_t n,i,count;unsigned width,j;enum pt_raw_result result=pt_raw_size(pcm,f,&n);
    if(result!=PT_RAW_OK)return result;
    if((n && !bytes) || !written)return PT_RAW_INVALID;
    if(capacity<n)return PT_RAW_CAPACITY;
    count=(size_t)pcm->frames*pcm->channels;width=f->bits/8;
    if(overlap(bytes,n,pcm->data,count*sizeof(int32_t)) || overlap(bytes,n,f,sizeof(*f)))return PT_RAW_ALIAS;
    for(i=0;i<count;++i) {
        uint32_t value=(uint32_t)(pcm->data[i]+(f->unsigned8?128:0));
        for(j=0;j<width;++j)bytes[i*width+j]=(uint8_t)(value>>(8*(f->little_endian?j:width-1-j)));
    }
    *written=n;return PT_RAW_OK;
}
