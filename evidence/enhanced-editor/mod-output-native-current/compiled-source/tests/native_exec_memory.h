#ifndef PT_NATIVE_EXEC_MEMORY_TEST_H
#define PT_NATIVE_EXEC_MEMORY_TEST_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/native/master_memory.h"
static struct pt_master_memory native_pool;
static unsigned native_allocations,native_started;
static void native_memory_start(void)
{
    size_t limit;
    pt_master_memory_init(&native_pool);
    assert(native_pool.flags==MEMF_FAST && native_pool.limit>65536);
    /* Refuse through the real pool budget without exhausting the shared guest. */
    limit=native_pool.limit;native_pool.limit=0;
    assert(!pt_master_allocate(&native_pool,1) && !native_pool.used);
    assert(native_pool.flags==MEMF_FAST);
    native_pool.limit=limit;native_started=1;
    printf("EXEC MEMORY pool_limit=%lu reserve=%lu flags=%lu\n",
        (unsigned long)limit,(unsigned long)native_pool.reserve,(unsigned long)native_pool.flags);
}
static void *native_allocate(size_t bytes)
{
    void *p;assert(native_started);
    p=pt_master_allocate(&native_pool,bytes);assert(p);
    assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
    ++native_allocations;return p;
}
static void native_release(void *p)
{
    if(p)assert((TypeOfMem(p)&(MEMF_FAST|MEMF_CHIP))==MEMF_FAST);
    pt_master_release(&native_pool,p);
}
static void native_memory_finish(void)
{
    assert(native_allocations && !native_pool.used);
    printf("EXEC MEMORY PASS: %u Fast allocations, zero owned bytes, budget refusal without Chip fallback\n",native_allocations);
}
#endif
