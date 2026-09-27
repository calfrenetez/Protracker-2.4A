#include <string.h>
#include "amigus_wavetable_cache.h"
#include "amigus_voice_plan.h"
static int owns(void *context)
{
    struct pt_amigus_wavetable_cache *c=context;
    struct pt_amigus_reservation *r=c->reservation;
    if(!c->faulted && r && r->opened && r->reserved && r->access &&
       r->resource==PT_AMIGUS_WAVETABLE && c->owned(c->context))return 1;
    c->faulted=1;return 0;
}
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
enum pt_cache_result pt_amigus_wavetable_cache_acquire(struct pt_amigus_wavetable_cache *c,
    const struct pt_pcm *p,uint32_t identity,uint64_t version,const struct pt_playback_format *format,
    uint8_t *staging,size_t capacity,struct pt_cache_lease *lease)
{
    if(!c || !c->reservation || c->closing || !owns(c))return PT_CACHE_INVALID;
    /* Cap work per bus callback while allowing a larger supplied staging buffer. */
    if(capacity>PT_AMIGUS_RAM_WRITE_MAX)capacity=PT_AMIGUS_RAM_WRITE_MAX;
    return pt_playback_pcm_upload_chunks(&c->cache,p,identity,version,format,staging,capacity,
        &c->arena,pt_amigus_sample_ram_write,lease);
}
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
