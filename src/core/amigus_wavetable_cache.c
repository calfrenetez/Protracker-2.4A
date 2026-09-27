#include <string.h>
#include "playback_internal.h"
#include "amigus_voice_plan.h"
static int owns(void *context)
{
    struct pt_amigus_wavetable_cache *c=context;
    struct pt_amigus_reservation *r=c->reservation;
    if(!c->faulted && r && r->opened && r->reserved && r->access &&
       r->resource==PT_AMIGUS_WAVETABLE && c->owned && c->owned(c->context))return 1;
    c->faulted=1;return 0;
}
int pt_amigus_wavetable_cache_current(struct pt_amigus_wavetable_cache *c)
{return c && c->reservation && !c->closing && owns(c);}
static int write_word(void *context,unsigned reg,uint32_t word)
{
    struct pt_amigus_wavetable_cache *c=context;
    return owns(c) && c->write32(c->context,reg,word);
}
int pt_amigus_wavetable_cache_attach(struct pt_amigus_wavetable_cache *c,
    struct pt_amigus_reservation *r,uint32_t base,uint32_t capacity,size_t budget,
    void *context,int (*owned)(void *),int (*write32)(void *,unsigned,uint32_t))
{
    if(!c || c->reservation || !r || !r->opened || !r->reserved || r->access ||
       r->resource!=PT_AMIGUS_WAVETABLE || !owned || !write32 || !budget ||
       !capacity || (base&3) || (capacity&3) || base>=PT_AMIGUS_RAM_ADDRESS_SPACE || capacity>PT_AMIGUS_RAM_ADDRESS_SPACE-base)return 0;
    if(!owned(context) || !pt_amigus_reservation_begin(r))return 0;
    memset(c,0,sizeof(*c));c->reservation=r;c->context=context;c->owned=owned;c->write32=write32;
    pt_amigus_sample_ram_init(&c->arena,base,capacity,c,owns,write_word);
    pt_cache_init(&c->cache,&c->arena,pt_amigus_sample_ram_allocate,pt_amigus_sample_ram_release,budget);
    return 1;
}
void pt_amigus_upload_cancel(struct pt_amigus_upload_job *j)
{
    if(!j)return;
    pt_playback_upload_cancel(&j->upload);memset(j,0,sizeof(*j));
}
static int job_write(void *context,void *resource,size_t offset,const uint8_t *data,size_t bytes)
{
    struct pt_amigus_upload_job *j=context;
    if(j->backend->reservation!=j->reservation || !pt_amigus_wavetable_cache_current(j->backend))return 0;
    return pt_amigus_sample_ram_write(&j->backend->arena,resource,offset,data,bytes) &&
        j->backend->reservation==j->reservation && pt_amigus_wavetable_cache_current(j->backend);
}
static enum pt_cache_result begin(struct pt_amigus_upload_job *j,struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    struct pt_cache_lease *out,unsigned prepared)
{
    enum pt_cache_result result;
    if(!j || j->backend || j->upload.cache || !pt_amigus_wavetable_cache_current(c))return PT_CACHE_INVALID;
    result=prepared?pt_playback_upload_begin_prepared(&j->upload,&c->cache,p,identity,version,f,j,job_write,out):
        pt_playback_upload_begin(&j->upload,&c->cache,p,identity,version,f,j,job_write,out);
    if(result==PT_CACHE_PENDING){j->backend=c;j->reservation=c->reservation;}
    return result;
}
enum pt_cache_result pt_amigus_upload_begin(struct pt_amigus_upload_job *j,struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,struct pt_cache_lease *out)
{return begin(j,c,p,identity,version,f,out,0);}
enum pt_cache_result pt_amigus_upload_begin_prepared(struct pt_amigus_upload_job *j,struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,struct pt_cache_lease *out)
{return begin(j,c,p,identity,version,f,out,1);}
enum pt_cache_result pt_amigus_upload_step(struct pt_amigus_upload_job *j,uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{
    enum pt_cache_result result;
    if(!j || !j->backend)return PT_CACHE_INVALID;
    if(j->backend->reservation!=j->reservation || !pt_amigus_wavetable_cache_current(j->backend)) {
        pt_amigus_upload_cancel(j);return PT_CACHE_INVALID;
    }
    if(capacity>PT_AMIGUS_RAM_WRITE_MAX)capacity=PT_AMIGUS_RAM_WRITE_MAX;
    result=pt_playback_upload_step(&j->upload,staging,capacity,out);
    if(result!=PT_CACHE_PENDING)memset(j,0,sizeof(*j));
    return result;
}
static enum pt_cache_result acquire(struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *format,
    uint8_t *staging,size_t capacity,struct pt_cache_lease *lease,unsigned prepared)
{
    struct pt_amigus_upload_job job={0};enum pt_cache_result result=begin(&job,c,p,identity,version,format,lease,prepared);
    while(result==PT_CACHE_PENDING)result=pt_amigus_upload_step(&job,staging,capacity,lease);
    return result;
}
enum pt_cache_result pt_amigus_wavetable_cache_acquire(struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{return acquire(c,p,identity,version,f,staging,capacity,out,0);}
enum pt_cache_result pt_amigus_wavetable_cache_acquire_prepared(struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *f,
    uint8_t *staging,size_t capacity,struct pt_cache_lease *out)
{return acquire(c,p,identity,version,f,staging,capacity,out,1);}
int pt_amigus_wavetable_cache_location(struct pt_amigus_wavetable_cache *c,struct pt_cache_lease lease,
    uint32_t *address,uint32_t *bytes)
{
    if(!c || !c->reservation || c->closing || !owns(c))return 0;
    return pt_amigus_sample_ram_location(&c->arena,pt_cache_data(&c->cache,lease),address,bytes);
}
int pt_amigus_wavetable_cache_unpin(struct pt_amigus_wavetable_cache *c,struct pt_cache_lease lease)
{return c && c->reservation && pt_cache_unpin(&c->cache,lease);}
void pt_amigus_wavetable_cache_invalidate(struct pt_amigus_wavetable_cache *c,uint32_t identity)
{if(c && c->reservation)pt_playback_pcm_invalidate(&c->cache,identity);}
int pt_amigus_wavetable_cache_detach(struct pt_amigus_wavetable_cache *c)
{
    if(!c)return 0;
    if(!c->reservation)return 1;
    c->closing=1;
    if(!pt_cache_clear(&c->cache))return 0;
    if(!pt_amigus_reservation_end(c->reservation))return 0;
    memset(c,0,sizeof(*c));return 1;
}
