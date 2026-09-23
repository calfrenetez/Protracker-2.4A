#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sample_cache.h"
struct heap {size_t used,limit;unsigned allocations,frees;};
static void *alloc(void *ctx,size_t n)
{struct heap *h=ctx;void *p;if(n>h->limit-h->used)return NULL;p=malloc(n);if(p){h->used+=n;++h->allocations;}return p;}
static void drop(void *ctx,void *p,size_t n)
{struct heap *h=ctx;h->used-=n;++h->frees;free(p);}
int main(void)
{
    struct heap heap={0,100,0,0};struct pt_sample_cache c;
    struct pt_cache_lease a,b,d,old,out={99,123};void *pointer;unsigned allocations,i;
    pt_cache_init(&c,&heap,alloc,drop,100);
    assert(pt_cache_take(&c,1,1,40,&a)==PT_CACHE_LOAD);
    memset(pt_cache_data(&c,a),17,40);
    assert(pt_cache_take(&c,1,1,40,&b)==PT_CACHE_BUSY); /* Unpublished. */
    assert(pt_cache_publish(&c,a));assert(pt_cache_take(&c,1,1,40,&b)==PT_CACHE_HIT);
    assert(pt_cache_data(&c,a)==pt_cache_data(&c,b));
    assert(pt_cache_trim(&c,100)==0);assert(pt_cache_unpin(&c,b));
    pt_cache_invalidate(&c,1);assert(pt_cache_data(&c,a));assert(!pt_cache_publish(&c,a));
    assert(pt_cache_take(&c,1,2,40,&b)==PT_CACHE_LOAD); /* Retired old version stays pinned. */
    assert(pt_cache_data(&c,a)!=pt_cache_data(&c,b));
    assert(*(unsigned char *)pt_cache_data(&c,a)==17);assert(pt_cache_unpin(&c,a));
    assert(heap.used==40);assert(pt_cache_publish(&c,b));pointer=pt_cache_data(&c,b);
    assert(pt_cache_unpin(&c,b));old=b;allocations=heap.allocations;
    assert(pt_cache_take(&c,1,3,40,&b)==PT_CACHE_LOAD);
    assert(pt_cache_data(&c,b)==pointer && heap.allocations==allocations);
    assert(!pt_cache_data(&c,old) && !pt_cache_unpin(&c,old));
    assert(pt_cache_unpin(&c,b) && heap.used==0); /* Aborted fill. */
    assert(pt_cache_take(&c,1,4,40,&a)==PT_CACHE_LOAD);assert(pt_cache_publish(&c,a));assert(pt_cache_unpin(&c,a));
    assert(pt_cache_take(&c,2,1,40,&b)==PT_CACHE_LOAD);assert(pt_cache_publish(&c,b));assert(pt_cache_unpin(&c,b));
    assert(pt_cache_take(&c,1,4,40,&a)==PT_CACHE_HIT);assert(pt_cache_unpin(&c,a)); /* 2 oldest. */
    assert(pt_cache_take(&c,3,1,40,&d)==PT_CACHE_LOAD);assert(!pt_cache_data(&c,b));
    assert(pt_cache_take(&c,1,4,40,&a)==PT_CACHE_HIT); /* 1 survived LRU pressure. */
    assert(pt_cache_take(&c,4,1,40,&out)==PT_CACHE_CAPACITY && out.slot==99);
    assert(!pt_cache_clear(&c));assert(!pt_cache_publish(&c,a));
    assert(pt_cache_unpin(&c,a));assert(pt_cache_unpin(&c,d));assert(heap.used==0);
    /* Backend pressure below cache budget evicts unused entries before retry. */
    heap.limit=40;
    assert(pt_cache_take(&c,1,5,40,&a)==PT_CACHE_LOAD);assert(pt_cache_publish(&c,a));assert(pt_cache_unpin(&c,a));
    assert(pt_cache_take(&c,2,2,40,&b)==PT_CACHE_LOAD);assert(heap.used==40);assert(pt_cache_unpin(&c,b));
    heap.limit=0;assert(pt_cache_take(&c,3,2,1,&b)==PT_CACHE_CAPACITY && !heap.used);
    heap.limit=100;assert(pt_cache_take(&c,0,0,101,&b)==PT_CACHE_CAPACITY);
    assert(pt_cache_take(&c,0,0,0,&b)==PT_CACHE_INVALID);
    /* Every slot pinned: no replacement or invalid pointer can steal ownership. */
    {struct pt_cache_lease leases[PT_CACHE_SLOTS];
     for(i=0;i<PT_CACHE_SLOTS;++i)assert(pt_cache_take(&c,i,10,1,leases+i)==PT_CACHE_LOAD);
     assert(pt_cache_take(&c,100,10,1,&b)==PT_CACHE_BUSY);
     assert(!pt_cache_clear(&c));
     for(i=0;i<PT_CACHE_SLOTS;++i)assert(pt_cache_unpin(&c,leases[i]));}
    assert(pt_cache_clear(&c) && !c.bytes && !heap.used && heap.allocations==heap.frees);
    c.clock=UINT64_MAX;assert(pt_cache_take(&c,1,1,1,&a)==PT_CACHE_CAPACITY);
    puts("SAMPLE CACHE PASS: publication, pinned retirement, revisions, refill, LRU pressure, backend failure, stale handles and complete release");
    return 0;
}
