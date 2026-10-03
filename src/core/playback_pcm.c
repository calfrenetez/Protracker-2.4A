#include "playback_internal.h"
#include "pcm_internal.h"
#include <string.h>
/* Metadata only: capacity is the complete owned int32_t extent, not just the
 * active frames. Padding is protected without reading its values. */
static int overlap(const void *a,size_t n,const void *b,size_t m)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(!n || !m)return 0;
    if(!a || !b || n>UINTPTR_MAX-x || m>UINTPTR_MAX-y)return 1;
    return x<y+m && y<x+n;
}
static int source_alias(const struct pt_pcm *p,const struct pt_playback_format *f,const void *out,size_t bytes)
{
    size_t source_bytes;
    if(!bytes)return 0;
    if(!out || bytes>UINTPTR_MAX-(uintptr_t)out || p->capacity>SIZE_MAX/sizeof(*p->data))return 1;
    source_bytes=p->capacity*sizeof(*p->data);
    return overlap(out,bytes,p,sizeof(*p)) || overlap(out,bytes,f,sizeof(*f)) ||
        overlap(out,bytes,p->data,source_bytes);
}
static enum pt_pcm_result size(const struct pt_pcm *p,const struct pt_playback_format *f,size_t *out,unsigned prepared)
{
    size_t bytes;enum pt_pcm_result r=prepared?pt_pcm_shape(p):pt_pcm_validate(p);
    if(r!=PT_PCM_OK)return r;
    if(!f || !out || (f->bits!=8 && f->bits!=16) || f->channel>=p->channels ||
       f->little_endian>1 || f->word_pad>1)return PT_PCM_INVALID;
    if(p->frames>SIZE_MAX/(f->bits/8))return PT_PCM_CAPACITY;
    bytes=(size_t)p->frames*(f->bits/8);
    if(f->word_pad && (bytes&1)) {if(bytes==SIZE_MAX)return PT_PCM_CAPACITY;++bytes;}
    *out=bytes;return PT_PCM_OK;
}
enum pt_pcm_result pt_playback_pcm_size(const struct pt_pcm *p,const struct pt_playback_format *f,size_t *out)
{
    size_t bytes;enum pt_pcm_result r=size(p,f,out?&bytes:NULL,0);
    if(r!=PT_PCM_OK)return r;
    if(source_alias(p,f,out,sizeof(*out)))return PT_PCM_ALIAS;
    *out=bytes;return PT_PCM_OK;
}
/* Caller validates the complete immutable source once before chunking. */
static void pack_frames(const struct pt_pcm *p,const struct pt_playback_format *f,
                        uint32_t start,uint32_t count,uint8_t *out)
{
    uint32_t frame;
    for(frame=0;frame<count;++frame) {
        int32_t value=p->data[((size_t)start+frame)*p->channels+f->channel];
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
}
enum pt_pcm_result pt_playback_pcm_pack(const struct pt_pcm *p,const struct pt_playback_format *f,uint8_t *out,size_t capacity)
{
    size_t bytes;enum pt_pcm_result r=size(p,f,&bytes,0);
    if(r!=PT_PCM_OK)return r;
    if(capacity<bytes)return PT_PCM_CAPACITY;
    if(bytes && !out)return PT_PCM_INVALID;
    if(source_alias(p,f,out,bytes))return PT_PCM_ALIAS;
    pack_frames(p,f,0,p->frames,out);
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
    if(source_alias(p,f,out,sizeof(*out)))return PT_CACHE_INVALID;
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

enum pt_cache_result pt_playback_pcm_upload(struct pt_sample_cache *c,const struct pt_pcm *p,
    uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    uint8_t *staging,size_t capacity,void *context,
    int (*upload)(void *,void *,const uint8_t *,size_t),struct pt_cache_lease *out)
{
    size_t bytes;struct pt_cache_lease lease;enum pt_cache_result result;enum pt_pcm_result r;
    if(!c || !out || !upload)return PT_CACHE_INVALID;
    r=pt_playback_pcm_size(p,f,&bytes);
    if(r!=PT_PCM_OK || !bytes)return r==PT_PCM_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
    if(source_alias(p,f,out,sizeof(*out)))return PT_CACHE_INVALID;
    result=pt_cache_take(c,key(identity,f),version,bytes,&lease);
    if(result==PT_CACHE_LOAD) {
        if(capacity>=bytes && overlap(staging,bytes,out,sizeof(*out))) {
            pt_cache_unpin(c,lease);return PT_CACHE_INVALID;
        }
        r=pt_playback_pcm_pack(p,f,staging,capacity);
        if(r!=PT_PCM_OK) {
            pt_cache_unpin(c,lease);return r==PT_PCM_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
        }
        if(!upload(context,pt_cache_data(c,lease),staging,bytes) || !pt_cache_publish(c,lease)) {
            pt_cache_unpin(c,lease);return PT_CACHE_TRANSFER;
        }
    }
    if(result==PT_CACHE_LOAD || result==PT_CACHE_HIT)*out=lease;
    return result;
}

void pt_playback_upload_cancel(struct pt_playback_upload_job *j)
{
    if(!j)return;
    if(j->cache)pt_cache_unpin(j->cache,j->lease);
    memset(j,0,sizeof(*j));
}
static enum pt_cache_result upload_begin(struct pt_playback_upload_job *j,struct pt_sample_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    void *context,int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out,unsigned prepared)
{
    size_t bytes;struct pt_cache_lease lease;enum pt_cache_result result;enum pt_pcm_result r;
    if(!j || j->cache || !c || !out || !write)return PT_CACHE_INVALID;
    r=size(p,f,&bytes,prepared);
    if(r!=PT_PCM_OK || !bytes)return r==PT_PCM_CAPACITY?PT_CACHE_CAPACITY:PT_CACHE_INVALID;
    if(source_alias(p,f,j,sizeof(*j)) || source_alias(p,f,out,sizeof(*out)) ||
       overlap(j,sizeof(*j),out,sizeof(*out)))return PT_CACHE_INVALID;
    result=pt_cache_take(c,key(identity,f),version,bytes,&lease);
    if(result==PT_CACHE_HIT){*out=lease;return result;}
    if(result!=PT_CACHE_LOAD)return result;
    memset(j,0,sizeof(*j));j->cache=c;j->source=p;j->pcm=*p;j->format=*f;j->lease=lease;
    j->bytes=bytes;j->context=context;j->write=write;return PT_CACHE_PENDING;
}
enum pt_cache_result pt_playback_upload_begin(struct pt_playback_upload_job *j,struct pt_sample_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    void *context,int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out)
{return upload_begin(j,c,p,identity,version,f,context,write,out,0);}
enum pt_cache_result pt_playback_upload_begin_prepared(struct pt_playback_upload_job *j,struct pt_sample_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    void *context,int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out)
{return upload_begin(j,c,p,identity,version,f,context,write,out,1);}
static int upload_current(const struct pt_playback_upload_job *j)
{
    const struct pt_pcm *p=j->source,*q=&j->pcm;
    return p && p->data==q->data && p->capacity==q->capacity && p->frames==q->frames &&
        p->rate==q->rate && p->channels==q->channels && p->bits==q->bits &&
        pt_cache_data(j->cache,j->lease) && j->cache->entry[j->lease.slot].valid==0;
}
enum pt_cache_result pt_playback_upload_step(struct pt_playback_upload_job *j,uint8_t *staging,
    size_t capacity,struct pt_cache_lease *out)
{
    size_t width,chunk,n,start,frames;enum pt_cache_result result=PT_CACHE_INVALID;
    if(!j || !j->cache)return result;
    /* Cancelling would itself overwrite an output located inside the job.
     * Leave this valid unpublished job intact for explicit retry/cancel. */
    if(out && overlap(out,sizeof(*out),j,sizeof(*j)))return result;
    if(!out || !upload_current(j))goto fail;
    if(source_alias(j->source,&j->format,out,sizeof(*out)))goto fail;
    if(capacity>PT_PLAYBACK_UPLOAD_CHUNK)capacity=PT_PLAYBACK_UPLOAD_CHUNK;
    width=j->format.bits/8;chunk=capacity-capacity%width;
    if(!chunk){result=PT_CACHE_CAPACITY;goto fail;}
    n=j->bytes-j->offset;if(n>chunk)n=chunk;
    if(source_alias(j->source,&j->format,staging,n) ||
       overlap(staging,n,j,sizeof(*j)) || overlap(staging,n,out,sizeof(*out)))goto fail;
    start=j->offset/width;frames=n/width;
    if(frames>j->pcm.frames-start)frames=j->pcm.frames-start;
    pack_frames(&j->pcm,&j->format,(uint32_t)start,(uint32_t)frames,staging);
    if(n>frames*width)staging[n-1]=0;
    result=PT_CACHE_TRANSFER;
    if(!j->write(j->context,pt_cache_data(j->cache,j->lease),j->offset,staging,n) || !upload_current(j))goto fail;
    j->offset+=n;if(j->offset<j->bytes)return PT_CACHE_PENDING;
    if(!pt_cache_publish(j->cache,j->lease))goto fail;
    *out=j->lease;memset(j,0,sizeof(*j));return PT_CACHE_LOAD;
fail:
    pt_playback_upload_cancel(j);return result;
}
static enum pt_cache_result upload_chunks(struct pt_sample_cache *c,const struct pt_pcm *p,
    uint32_t identity,uint64_t version,const struct pt_playback_format *f,uint8_t *staging,size_t capacity,
    void *context,int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out,unsigned prepared)
{
    struct pt_playback_upload_job job={0};
    enum pt_cache_result result=upload_begin(&job,c,p,identity,version,f,context,write,out,prepared);
    while(result==PT_CACHE_PENDING)result=pt_playback_upload_step(&job,staging,capacity,out);
    return result;
}

enum pt_cache_result pt_playback_pcm_upload_chunks(struct pt_sample_cache *c,const struct pt_pcm *p,
    uint32_t identity,uint64_t version,const struct pt_playback_format *f,uint8_t *staging,size_t capacity,
    void *context,int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out)
{return upload_chunks(c,p,identity,version,f,staging,capacity,context,write,out,0);}
enum pt_cache_result pt_playback_pcm_upload_prepared(struct pt_sample_cache *c,const struct pt_pcm *p,
    uint32_t identity,uint64_t version,const struct pt_playback_format *f,uint8_t *staging,size_t capacity,
    void *context,int (*write)(void *,void *,size_t,const uint8_t *,size_t),struct pt_cache_lease *out)
{return upload_chunks(c,p,identity,version,f,staging,capacity,context,write,out,1);}
