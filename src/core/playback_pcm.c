#include "playback_pcm.h"
enum pt_pcm_result pt_playback_pcm_size(const struct pt_pcm *p,const struct pt_playback_format *f,size_t *out)
{
    size_t bytes;enum pt_pcm_result r=pt_pcm_validate(p);
    if(r!=PT_PCM_OK)return r;
    if(!f || !out || (f->bits!=8 && f->bits!=16) || f->channel>=p->channels ||
       f->little_endian>1 || f->word_pad>1)return PT_PCM_INVALID;
    if(p->frames>SIZE_MAX/(f->bits/8))return PT_PCM_CAPACITY;
    bytes=(size_t)p->frames*(f->bits/8);
    if(f->word_pad && (bytes&1)) {if(bytes==SIZE_MAX)return PT_PCM_CAPACITY;++bytes;}
    *out=bytes;return PT_PCM_OK;
}
enum pt_pcm_result pt_playback_pcm_pack(const struct pt_pcm *p,const struct pt_playback_format *f,uint8_t *out,size_t capacity)
{
    size_t bytes,source_bytes;uint32_t frame;uintptr_t a,b;enum pt_pcm_result r=pt_playback_pcm_size(p,f,&bytes);
    if(r!=PT_PCM_OK)return r;
    if(capacity<bytes)return PT_PCM_CAPACITY;
    if(bytes && !out)return PT_PCM_INVALID;
    source_bytes=(size_t)p->frames*p->channels*sizeof(int32_t);a=(uintptr_t)p->data;b=(uintptr_t)out;
    if(bytes && (a<=b?b-a<source_bytes:a-b<bytes))return PT_PCM_ALIAS;
    for(frame=0;frame<p->frames;++frame) {
        int32_t value=p->data[(size_t)frame*p->channels+f->channel];
        int32_t high=((int32_t)1<<(f->bits-1))-1,low=-high-1;
        if(p->bits<f->bits)value*=((int32_t)1<<(f->bits-p->bits));
        else if(p->bits>f->bits) {
            int32_t divisor=(int32_t)1<<(p->bits-f->bits);
            value=value<0?-((-value+divisor/2)/divisor):(value+divisor/2)/divisor;
        }
        if(value>high)value=high;
        if(value<low)value=low;
        if(f->bits==8)out[frame]=(uint8_t)value;
        else {
            uint16_t word=(uint16_t)value;size_t offset=(size_t)frame*2;
            out[offset+f->little_endian]=(uint8_t)(word>>8);
            out[offset+1-f->little_endian]=(uint8_t)word;
        }
    }
    if(f->word_pad && f->bits==8 && (p->frames&1))out[p->frames]=0;
    return PT_PCM_OK;
}
static uint64_t key(uint32_t identity,const struct pt_playback_format *f)
{return ((uint64_t)identity<<8)|(f->bits==16?1:0)|(f->channel<<1)|(f->little_endian<<2)|(f->word_pad<<3);}
enum pt_cache_result pt_playback_pcm_acquire(struct pt_sample_cache *c,const struct pt_pcm *p,
    uint32_t identity,uint64_t version,const struct pt_playback_format *f,struct pt_cache_lease *out)
{
    size_t bytes;struct pt_cache_lease lease;enum pt_cache_result result;enum pt_pcm_result r;
    if(!out)return PT_CACHE_INVALID;
    r=pt_playback_pcm_size(p,f,&bytes);
    if(r!=PT_PCM_OK || !bytes)return r==PT_PCM_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
    result=pt_cache_take(c,key(identity,f),version,bytes,&lease);
    if(result==PT_CACHE_LOAD) {
        if(pt_playback_pcm_pack(p,f,pt_cache_data(c,lease),bytes)!=PT_PCM_OK || !pt_cache_publish(c,lease)) {
            pt_cache_unpin(c,lease);return PT_CACHE_INVALID;
        }
    }
    if(result==PT_CACHE_LOAD || result==PT_CACHE_HIT)*out=lease;
    return result;
}
void pt_playback_pcm_invalidate(struct pt_sample_cache *c,uint32_t identity)
{
    unsigned i;for(i=0;i<PT_CACHE_SLOTS;++i)if(c->entry[i].data && (c->entry[i].key>>8)==identity)
        pt_cache_invalidate(c,c->entry[i].key);
}
