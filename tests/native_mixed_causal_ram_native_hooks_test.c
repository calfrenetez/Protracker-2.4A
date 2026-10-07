/* Separate HOST hook composition: genuine34 entry once. Memory classes below
 * are explicit heap models, never Exec/Fast/Chip placement qualification. */
#ifdef NDEBUG
#error The genuine native hook composition requires assertions enabled.
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
union nh_alignment {long double real;void *pointer;uint64_t integer;};
struct nh_region {
    struct nh_region *next;size_t bytes,total;unsigned kind;
    union nh_alignment alignment;
};
static struct nh_region *nh_regions;
static size_t nh_used[2],nh_limit[2];
static unsigned nh_calls[2],nh_releases[2],nh_checks[2],nh_started;
static void *nch_platform_allocate(size_t bytes,unsigned kind)
{
    struct nh_region *region;size_t total;unsigned i;
    assert(nh_started && (kind==1U || kind==2U));i=kind-1U;
    if(!bytes || bytes>SIZE_MAX-sizeof(*region))return NULL;
    total=sizeof(*region)+bytes;
    if(nh_used[i]>nh_limit[i] || total>nh_limit[i]-nh_used[i])return NULL;
    assert(nh_calls[i]<UINT_MAX);region=malloc(total);if(!region)return NULL;
    region->bytes=bytes;region->total=total;region->kind=kind;
    region->next=nh_regions;nh_regions=region;nh_used[i]+=total;++nh_calls[i];
    return region+1;
}
static int nch_platform_kind(void *p,size_t bytes,unsigned kind)
{
    struct nh_region *r;uintptr_t address=(uintptr_t)p;
    assert(nh_started && (kind==1U || kind==2U));
    if(!address || !bytes || bytes-1U>UINTPTR_MAX-address)return 0;
    for(r=nh_regions;r;r=r->next){uintptr_t start=(uintptr_t)(r+1);
        if(address>=start && address-start<r->bytes && bytes<=r->bytes-(address-start)){
            if(r->kind!=kind)return 0;
            assert(nh_checks[kind-1U]<UINT_MAX);++nh_checks[kind-1U];return 1;
        }}
    return 0;
}
static void nch_platform_release(void *p,size_t bytes,unsigned kind)
{
    struct nh_region **link=&nh_regions,*r;unsigned i;
    assert(nh_started && p && bytes && (kind==1U || kind==2U));i=kind-1U;
    while(*link && (void *)(*link+1)!=p)link=&(*link)->next;
    assert(*link);r=*link;
    assert(r->bytes==bytes && r->kind==kind && r->total<=nh_used[i] && nh_releases[i]<UINT_MAX);
    *link=r->next;nh_used[i]-=r->total;++nh_releases[i];free(r);
}
static size_t nch_platform_chip_available(void)
{
    /* Model admission includes its bookkeeping overhead; no physical capacity
     * or reserve is learned from this fixed HOST-only budget. */
    size_t free_bytes=nh_used[1]<nh_limit[1]?nh_limit[1]-nh_used[1]:0;
    return free_bytes>sizeof(struct nh_region)?free_bytes-sizeof(struct nh_region):0;
}
static int nch_platform_empty(void)
{return !nh_regions && !nh_used[0] && !nh_used[1];}
struct cp_trial;
static void *nch_allocate(size_t);
static void *nch_callocate(size_t,size_t);
static void nch_release(void *);
static void nch_configure_chip(struct cp_trial *);
static void nch_case_begin(unsigned,unsigned,unsigned,unsigned,unsigned);
static void nch_case_end(unsigned);
#define NCH_TRACE_PREFIX "HOST MIXED CAUSAL NATIVE HOOK"
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN nh_fixture_once
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_CALLOC nch_callocate
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CONFIGURE_CHIP nch_configure_chip
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_BEGIN nch_case_begin
#define PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_END nch_case_end
#define malloc nch_allocate
#define free nch_release
#include "native_mixed_causal_ram_port_test.c"
#undef free
#undef malloc
#undef calloc
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_END
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CASE_BEGIN
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_CONFIGURE_CHIP
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_CALLOC
#undef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_MAIN
#ifndef PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_VERSION
#error Missing reviewed genuine34 fixture hook version.
#elif PT_NATIVE_MIXED_CAUSAL_RAM_PORT_TEST_VERSION != 1U
#error Unreviewed genuine34 fixture hook version.
#endif
#include "native_mixed_causal_ram_hooks_internal.h"
int main(void)
{
    size_t limit;unsigned calls;int result;
    assert(!nh_started && nch_platform_empty());nh_started=1;
    nh_limit[0]=16UL*1024UL*1024UL;nh_limit[1]=4096;
    /* One real refusal through the explicit model allocator, before malloc,
     * without exhaustion, a second class fallback or a hidden allocation. */
    limit=nh_limit[0];nh_limit[0]=0;calls=nh_calls[0];
    assert(!nch_platform_allocate(1,1U) && nh_calls[0]==calls && nch_platform_empty());
    nh_limit[0]=limit;
    assert(!nch_allocate(0) && !nch_callocate(0,1) && !nch_callocate(1,0) &&
        !nch_callocate(SIZE_MAX,2) && nch_platform_empty() && !nch_fast_calls);
    result=nh_fixture_once();assert(result==0);nch_final();
    assert(nh_calls[0]==nch_fast_calls && nh_releases[0]==nh_calls[0] &&
        nh_calls[0]<=UINT_MAX/4U && nh_checks[0]==4U*nh_calls[0] &&
        nh_calls[1]==52U && nh_releases[1]==52U && nh_checks[1]==104U && nch_platform_empty());
    puts("HOST MIXED CAUSAL NATIVE HOOKS PASS:34 genuine cases;all malloc/calloc/free intercepted;34 exact pre-begin Chip bindings,52 selective allocations and per-case zero ledgers;full pointer/context/size release proofs;HEAP_CLASS_MODELS_ONLY;native placement/target NOT_RUN");
    assert(!fflush(stdout));return 0;
}
