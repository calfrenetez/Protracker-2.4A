#include <string.h>
#include <limits.h>
#include "sample_cache.h"
void pt_cache_init(struct pt_sample_cache *c,void *context,void *(*allocate)(void *,size_t),void (*release)(void *,void *,size_t),size_t budget)
{memset(c,0,sizeof(*c));c->context=context;c->allocate=allocate;c->release=release;c->budget=budget;}
static void drop(struct pt_sample_cache *c,unsigned slot)
{
    struct pt_cache_entry *e=c->entry+slot;
    if(e->data) {c->release(c->context,e->data,e->bytes);c->bytes-=e->bytes;}
    memset(e,0,sizeof(*e));
}
static unsigned oldest(const struct pt_sample_cache *c)
{
    unsigned i,slot=PT_CACHE_SLOTS;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(c->entry[i].data && !c->entry[i].pins &&
       (slot==PT_CACHE_SLOTS || c->entry[i].touched<c->entry[slot].touched))slot=i;
    return slot;
}
size_t pt_cache_trim(struct pt_sample_cache *c,size_t wanted)
{
    size_t before=c->bytes;unsigned slot;
    while(before-c->bytes<wanted && (slot=oldest(c))!=PT_CACHE_SLOTS)drop(c,slot);
    return before-c->bytes;
}
static struct pt_cache_entry *leased(struct pt_sample_cache *c,struct pt_cache_lease l)
{
    struct pt_cache_entry *e;
    if(l.slot>=PT_CACHE_SLOTS)return NULL;
    e=c->entry+l.slot;
    return e->data && e->pins && e->serial==l.serial?e:NULL;
}
void *pt_cache_data(struct pt_sample_cache *c,struct pt_cache_lease l)
{struct pt_cache_entry *e=leased(c,l);return e?e->data:NULL;}
int pt_cache_publish(struct pt_sample_cache *c,struct pt_cache_lease l)
{struct pt_cache_entry *e=leased(c,l);if(!e || e->valid==2)return 0;e->valid=1;return 1;}
int pt_cache_unpin(struct pt_sample_cache *c,struct pt_cache_lease l)
{
    struct pt_cache_entry *e=leased(c,l);if(!e)return 0;
    if(!--e->pins && e->valid!=1)drop(c,l.slot);
    return 1;
}
void pt_cache_invalidate(struct pt_sample_cache *c,uint64_t key)
{
    unsigned i;for(i=0;i<PT_CACHE_SLOTS;++i)if(c->entry[i].data && c->entry[i].key==key) {
        c->entry[i].valid=2;if(!c->entry[i].pins)drop(c,i);
    }
}
int pt_cache_clear(struct pt_sample_cache *c)
{
    unsigned i;int free=1;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(c->entry[i].data) {
        c->entry[i].valid=2;if(c->entry[i].pins)free=0;else drop(c,i);
    }
    return free;
}
enum pt_cache_result pt_cache_take(struct pt_sample_cache *c,uint64_t key,uint64_t version,size_t bytes,struct pt_cache_lease *out)
{
    unsigned i,slot=PT_CACHE_SLOTS,reuse=PT_CACHE_SLOTS;struct pt_cache_entry *e;void *data;
    if(!out || !c->allocate || !c->release || !bytes)return PT_CACHE_INVALID;
    if(bytes>c->budget || c->clock==UINT64_MAX)return PT_CACHE_CAPACITY;
    for(i=0;i<PT_CACHE_SLOTS;++i) {
        e=c->entry+i;
        if(!e->data) {if(slot==PT_CACHE_SLOTS)slot=i;continue;}
        if(e->key==key && e->version==version && e->bytes==bytes) {
            if(e->valid!=1 && e->pins)return PT_CACHE_BUSY;
            if(e->valid==1) {
                if(e->pins==UINT_MAX)return PT_CACHE_BUSY;
                ++e->pins;e->touched=++c->clock;*out=(struct pt_cache_lease){i,e->serial};return PT_CACHE_HIT;
            }
        }
        if(!e->pins && e->key==key && e->bytes==bytes)reuse=i;
    }
    /* Refill an unpinned equal-sized allocation; old content is unpublished. */
    if(reuse!=PT_CACHE_SLOTS) {slot=reuse;data=c->entry[slot].data;}
    else {
        if(slot==PT_CACHE_SLOTS) {slot=oldest(c);if(slot==PT_CACHE_SLOTS)return PT_CACHE_BUSY;drop(c,slot);}
        while(c->bytes>c->budget || bytes>c->budget-c->bytes) {
            unsigned victim=oldest(c);if(victim==PT_CACHE_SLOTS)return PT_CACHE_CAPACITY;drop(c,victim);
        }
        data=c->allocate(c->context,bytes);
        while(!data) {
            unsigned victim=oldest(c);if(victim==PT_CACHE_SLOTS)return PT_CACHE_CAPACITY;
            drop(c,victim);data=c->allocate(c->context,bytes);
        }
        c->bytes+=bytes;
    }
    ++c->clock;e=c->entry+slot;*e=(struct pt_cache_entry){data,bytes,key,version,c->clock,c->clock,1,0};
    *out=(struct pt_cache_lease){slot,e->serial};return PT_CACHE_LOAD;
}
