#include <string.h>
#include "amigus_sample_ram.h"
#include "amigus_voice_plan.h"
/* Pinned public wavetable map: byte address port 0x14, data port 0x10.
 * Independent implementation; no utility upload code is incorporated. */
#define RAM_ADDRESS 0x14
#define RAM_DATA 0x10
int pt_amigus_sample_ram_init(struct pt_amigus_sample_ram *a,uint32_t base,
    uint32_t capacity,void *ctx,int (*owned)(void *),int (*write32)(void *,unsigned,uint32_t))
{
    if(!a || !capacity || (base&3) || (capacity&3) ||
       base>=PT_AMIGUS_RAM_ADDRESS_SPACE || capacity>PT_AMIGUS_RAM_ADDRESS_SPACE-base || !owned || !write32)return 0;
    memset(a,0,sizeof(*a));a->base=base;a->capacity=capacity;
    a->context=ctx;a->owned=owned;a->write32=write32;return 1;
}
static struct pt_amigus_ram_block *lookup(struct pt_amigus_sample_ram *a,void *resource)
{
    unsigned i;if(!a)return NULL;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(resource==a->block+i && a->block[i].reserved)return a->block+i;
    return NULL;
}
void *pt_amigus_sample_ram_allocate(void *ctx,size_t bytes)
{
    struct pt_amigus_sample_ram *a=ctx;unsigned i,pass,slot=PT_CACHE_SLOTS;
    uint32_t needed,start;
    if(!a || !a->owned || !a->owned(a->context) || !bytes ||
       bytes>UINT32_MAX-3)return NULL;
    needed=((uint32_t)bytes+3)&~(uint32_t)3;
    if(needed>a->capacity)return NULL;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(!a->block[i].reserved) {slot=i;break;}
    if(slot==PT_CACHE_SLOTS)return NULL;
    start=a->base;
    /* At most one forward move per occupied slot, followed by a free scan. */
    for(pass=0;pass<PT_CACHE_SLOTS+1;++pass) {
        if(start-a->base>a->capacity-needed)return NULL;
        for(i=0;i<PT_CACHE_SLOTS;++i) {
            const struct pt_amigus_ram_block *b=a->block+i;
            if(b->reserved && start<b->address+b->reserved && b->address<start+needed) {
                start=b->address+b->reserved;break;
            }
        }
        if(i==PT_CACHE_SLOTS) {
            struct pt_amigus_ram_block *b=a->block+slot;
            memset(b,0,sizeof(*b));b->address=start;b->bytes=(uint32_t)bytes;b->reserved=needed;
            return b;
        }
    }
    return NULL;
}
void pt_amigus_sample_ram_release(void *ctx,void *resource,size_t bytes)
{
    struct pt_amigus_ram_block *b=lookup(ctx,resource);
    if(b && b->bytes==bytes)memset(b,0,sizeof(*b));
}
static int store(struct pt_amigus_sample_ram *a,struct pt_amigus_ram_block *b,uint32_t offset)
{
    if(!a->owned(a->context) ||
       !a->write32(a->context,RAM_ADDRESS,b->address+offset) ||
       !a->write32(a->context,RAM_DATA,b->word)) {b->failed=1;return 0;}
    b->word=0;b->fill=0;return 1;
}
int pt_amigus_sample_ram_write(void *ctx,void *resource,size_t offset,const uint8_t *data,size_t bytes)
{
    struct pt_amigus_sample_ram *a=ctx;struct pt_amigus_ram_block *b=lookup(a,resource);size_t i;
    if(!b)return 0;
    if(offset==0) {b->written=0;b->word=0;b->fill=0;b->failed=0;}
    if(b->failed || !data || !bytes || bytes>PT_AMIGUS_RAM_WRITE_MAX ||
       offset!=b->written || bytes>b->bytes-b->written || !a->owned(a->context)) {
        b->failed=1;return 0;
    }
    for(i=0;i<bytes;++i) {
        b->word=(b->word<<8)|data[i];++b->fill;++b->written;
        if(b->fill==4 && !store(a,b,b->written-4))return 0;
    }
    if(b->written==b->bytes && b->fill) {
        uint32_t start=b->written-b->fill;
        b->word<<=(4-b->fill)*8;
        if(!store(a,b,start))return 0;
    }
    return 1;
}
int pt_amigus_sample_ram_location(const struct pt_amigus_sample_ram *a,const void *resource,uint32_t *address,uint32_t *bytes)
{
    unsigned i;if(!a || !address || !bytes || !a->owned || !a->owned(a->context))return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(resource==a->block+i) {
        const struct pt_amigus_ram_block *b=a->block+i;
        if(!b->reserved || b->written!=b->bytes || b->fill || b->failed)return 0;
        *address=b->address;*bytes=b->bytes;return 1;
    }
    return 0;
}
