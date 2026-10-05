/* Prepared, unexecuted software model. Production allocator is a separate TU. */
#include "native_checked_memory.h"
#include "checked_memory_host_stubs.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#if !defined(PT_CHECKED_MEMORY_HOST_ASAN) || PT_CHECKED_MEMORY_HOST_ASAN != 1
#error This fixture requires the explicit ASan host recipe
#endif
#include <sanitizer/asan_interface.h>

struct cm_guarded_source {
    unsigned char unused[64];
    struct pt_private_memory_stats scalar;
    unsigned char tail[64];
};
union cm_control_and_stats {
    struct pt_private_native_memory control;
    struct pt_private_memory_stats stats;
};
union cm_control_bytes {
    struct pt_private_native_memory aligned;
    unsigned char bytes[sizeof(struct pt_private_native_memory)+32U];
};
static void cm_init(struct pt_private_native_memory *m,size_t ceiling)
{
    memset(m,0,sizeof(*m));
    assert(pt_private_native_memory_init(m,ceiling,NULL,0)==PT_PRIVATE_MEMORY_OK);
}
static void cm_retained(const struct pt_private_native_memory *m,int first)
{
    assert(m->failed && m->phase==PT_PRIVATE_MEMORY_RETAIN_ALL);
    assert(m->last_result==(unsigned)first && !m->busy);
}
static void cm_expect(unsigned index,enum cm_event event,const void *p,size_t bytes,ULONG flags)
{
    const struct cm_observation *o;
    assert(index<cm_model.trace_count); o=&cm_model.trace[index];
    assert(o->event==event && o->pointer==p && o->bytes==bytes && o->flags==flags);
}
static void fast_ceiling_reserve_payload_charge(void)
{
    struct pt_private_native_memory m={0};
    struct pt_private_memory_stats s={0};
    struct cm_guarded_source source,before;
    struct pt_private_memory_span guard;
    void *a,*b,*c; unsigned i;
    cm_reset(); memset(&source,0x37,sizeof(source)); memcpy(&before,&source,sizeof(before));
    guard.data=&source; guard.bytes=sizeof(source);
    assert(pt_private_native_memory_init(&m,1024U,&guard,1)==PT_PRIVATE_MEMORY_OK);
    assert(m.pool.flags==MEMF_FAST && m.pool.reserve==256U*1024U && m.pool.limit==1024U);
    a=pt_private_native_allocate(&m,17); b=pt_private_native_allocate(&m,33);
    c=pt_private_native_allocate(&m,65); assert(a && b && c && a!=b && a!=c && b!=c);
    assert(cm_live_bytes()==115 && m.pool.used==115 && m.live==3 && m.peak_live==3);
    assert(pt_private_native_memory_stats(&m,&s)==PT_PRIVATE_MEMORY_OK && s.used==115);
    assert(pt_private_native_release_checked(&m,b)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_release_checked(&m,a)==PT_PRIVATE_MEMORY_OK);
    pt_private_native_release(&m,c);
    assert(m.live==0 && m.pool.used==0 && m.releases==3 && cm_model.released_bytes==115);
    assert(!memcmp(&source,&before,sizeof(source)));
    cm_expect(0,CM_AVAIL,NULL,0,MEMF_FAST|MEMF_TOTAL);
    cm_expect(1,CM_AVAIL,NULL,0,MEMF_FAST);
    {
        void *allocated[3]={a,b,c},*released[3]={b,a,c};
        const size_t sizes[3]={17,33,65},released_sizes[3]={33,17,65};
        for (i=0;i<3;++i) {
            unsigned at=2+4*i;
            cm_expect(at,CM_AVAIL,NULL,0,MEMF_FAST);
            cm_expect(at+1,CM_ALLOC,allocated[i],sizes[i],MEMF_FAST|MEMF_PUBLIC);
            cm_expect(at+2,CM_TYPE,allocated[i],0,MEMF_FAST);
            cm_expect(at+3,CM_TYPE,(unsigned char *)allocated[i]+sizes[i]-1,0,MEMF_FAST);
            at=14+3*i;
            cm_expect(at,CM_TYPE,released[i],0,MEMF_FAST);
            cm_expect(at+1,CM_TYPE,(unsigned char *)released[i]+released_sizes[i]-1,0,MEMF_FAST);
            cm_expect(at+2,CM_FREE,released[i],released_sizes[i],0);
        }
        assert(cm_model.trace_count==23);
    }
    for (i=0;i<cm_model.trace_count;++i) {
        const struct cm_observation *o=&cm_model.trace[i];
        if (o->event==CM_ALLOC) assert(o->flags==(MEMF_FAST|MEMF_PUBLIC));
        if (o->event==CM_FREE) assert(o->bytes==17 || o->bytes==33 || o->bytes==65);
    }
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
}
static void no_fast_and_zero_limit(void)
{
    struct pt_private_native_memory m={0},before={0};
    memset(&m,0,sizeof(m));
    memcpy(&before,&m,sizeof(m));
    cm_reset(); cm_model.fast_total=0;
    assert(pt_private_native_memory_init(&m,64,NULL,0)==PT_PRIVATE_MEMORY_FAST_UNAVAILABLE);
    assert(!memcmp(&m,&before,sizeof(m)) && cm_model.alloc_calls==0);
    assert(cm_model.trace_count==2 && cm_model.trace[1].flags==MEMF_CHIP); cm_end(0);
    cm_reset(); cm_init(&m,0); cm_trace_reset();
    assert(!pt_private_native_allocate(&m,1));
    assert(m.last_result==PT_PRIVATE_MEMORY_BUDGET && cm_model.alloc_calls==0);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
    cm_reset(); cm_model.fast_free=256U*1024U;
    cm_init(&m,64); assert(m.pool.limit==0);
    assert(!pt_private_native_allocate(&m,1) && cm_model.alloc_calls==0);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
    cm_reset(); cm_init(&m,2U*1024U*1024U);
    assert(m.pool.limit==768U*1024U);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
}
static void invalid_shapes_and_wrap(void)
{
    struct pt_private_native_memory m={0},before={0};
    union cm_control_bytes storage,snapshot;
    struct pt_private_memory_span guards[17];
    unsigned char source[64]; unsigned i;
    memset(&m,0,sizeof(m));
    memcpy(&before,&m,sizeof(m));
    cm_reset(); memset(&storage,0,sizeof(storage)); memcpy(&snapshot,&storage,sizeof(storage));
    memset(guards,0,sizeof(guards)); memset(source,0x61,sizeof(source));
    assert(pt_private_native_memory_init(NULL,64,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
    assert(pt_private_native_memory_init((struct pt_private_native_memory *)(UINTPTR_MAX-7U),64,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
    assert(pt_private_native_memory_init((struct pt_private_native_memory *)(storage.bytes+1),64,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
    assert(!memcmp(&storage,&snapshot,sizeof(storage)));
    assert(pt_private_native_memory_init(&m,64,NULL,1)==PT_PRIVATE_MEMORY_INVALID);
    assert(pt_private_native_memory_init(&m,64,guards,17)==PT_PRIVATE_MEMORY_INVALID);
    guards[0].data=(const void *)(UINTPTR_MAX-4U); guards[0].bytes=16;
    assert(pt_private_native_memory_init(&m,64,guards,1)==PT_PRIVATE_MEMORY_INVALID);
    guards[0].data=&m; guards[0].bytes=sizeof(m);
    assert(pt_private_native_memory_init(&m,64,guards,1)==PT_PRIVATE_MEMORY_ALIAS);
#if SIZE_MAX > UINT32_MAX
    assert(pt_private_native_memory_init(&m,(size_t)UINT32_MAX+1U,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
#endif
    assert(!memcmp(&m,&before,sizeof(m)) && cm_model.trace_count==0);
    assert(!pt_private_native_allocate(NULL,1));
    assert(pt_private_native_release_checked(NULL,NULL)==PT_PRIVATE_MEMORY_INVALID);
    for (i=0;i<sizeof(source);++i) assert(source[i]==0x61);
    /* An unused zero-count vector is not dereferenced. */
    assert(pt_private_native_memory_init(&m,64,(const struct pt_private_memory_span *)(UINTPTR_MAX-1U),0)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
    cm_reset(); memset(&m,0,sizeof(m));
    for (i=0;i<16;++i) { guards[i].data=source+i; guards[i].bytes=1; }
    assert(pt_private_native_memory_init(&m,64,guards,16)==PT_PRIVATE_MEMORY_OK);
    assert(m.guard_count==16 && !memcmp(m.guard,guards,16*sizeof(*guards)));
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
}
static void budget_size_and_fixed_slot_pressure(void)
{
    struct pt_private_native_memory m;
    void *p[32],*replacement; unsigned i,calls;
    cm_reset(); cm_init(&m,1024); cm_trace_reset();
    assert(!pt_private_native_allocate(&m,0));
#if SIZE_MAX > UINT32_MAX
    assert(!pt_private_native_allocate(&m,(size_t)UINT32_MAX+1U));
#endif
    assert(cm_model.alloc_calls==0 && cm_model.avail_calls==0);
    cm_model.null_next=1;
    assert(!pt_private_native_allocate(&m,16));
    assert(m.live==0 && m.pool.used==0 && m.phase==PT_PRIVATE_MEMORY_PREPARE);
    for (i=0;i<32;++i) { p[i]=pt_private_native_allocate(&m,16); assert(p[i]); }
    calls=cm_model.alloc_calls;
    assert(!pt_private_native_allocate(&m,16) && cm_model.alloc_calls==calls);
    assert(m.last_result==PT_PRIVATE_MEMORY_CAPACITY && m.live==32 && cm_live()==32);
    for (i=0;i<32;++i) {
        unsigned j; for (j=0;j<16;++j) assert(((unsigned char *)p[i])[j]==0xa5);
    }
    assert(pt_private_native_release_checked(&m,p[7])==PT_PRIVATE_MEMORY_OK);
    replacement=pt_private_native_allocate(&m,16); assert(replacement);
    assert(m.live==32 && m.peak_live==32);
    for (i=0;i<32;++i) if (i!=7) assert(pt_private_native_release_checked(&m,p[i])==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_release_checked(&m,replacement)==PT_PRIVATE_MEMORY_OK);
    assert(cm_model.released_bytes==33U*16U && m.pool.used==0);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
    cm_reset(); cm_init(&m,32); assert(pt_private_native_allocate(&m,32));
    calls=cm_model.alloc_calls;
    assert(!pt_private_native_allocate(&m,1) && cm_model.alloc_calls==calls);
    assert(m.last_result==PT_PRIVATE_MEMORY_BUDGET); cm_end(1);
    cm_reset(); cm_init(&m,1024); cm_trace_reset(); cm_model.fast_free=m.pool.reserve;
    assert(!pt_private_native_allocate(&m,1));
    assert(cm_model.avail_calls==1 && cm_model.alloc_calls==0 && m.live==0);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
}
static void returned_control_source_and_live_overlap(void)
{
    unsigned mode;
    for (mode=0;mode<6;++mode) {
        struct pt_private_native_memory m={0};
        struct cm_guarded_source source,before;
        struct pt_private_memory_span guard;
        unsigned char payload[128]; void *old=NULL,*bad; unsigned expected=0;
        cm_reset(); memset(&source,0x4c,sizeof(source)); memcpy(&before,&source,sizeof(before));
        guard.data=&source; guard.bytes=sizeof(source);
        assert(pt_private_native_memory_init(&m,1024,&guard,1)==PT_PRIVATE_MEMORY_OK);
        if (mode==2 || mode==3) {
            old=pt_private_native_allocate(&m,128); assert(old); expected=1;
            memcpy(payload,old,sizeof(payload));
        }
        bad=mode==0?(void *)&m:mode==1?(void *)&source:
            mode==2?old:mode==3?(void *)((unsigned char *)old+16):
            mode==4?(void *)(UINTPTR_MAX-7U):(void *)(cm_arena[0].bytes+1);
        cm_trace_reset(); cm_override(bad);
        assert(!pt_private_native_allocate(&m,64));
        assert(cm_model.alloc_calls==1 && cm_model.type_calls==0 && cm_model.free_calls==0);
        assert(m.ambiguous.pointer==bad && m.ambiguous.bytes==64);
        cm_retained(&m,mode==5?PT_PRIVATE_MEMORY_INVALID:PT_PRIVATE_MEMORY_ALIAS);
        assert(m.live==expected && !memcmp(&source,&before,sizeof(source)));
        if (old) assert(!memcmp(old,payload,sizeof(payload)));
        assert(!pt_private_native_allocate(&m,16));
        assert(pt_private_native_release_checked(&m,old)==PT_PRIVATE_MEMORY_RETAINED);
        assert(cm_model.alloc_calls==1 && cm_model.free_calls==0); cm_end(expected);
    }
}
static void wrong_class_endpoints(void)
{
    static const ULONG flags[4]={0,MEMF_CHIP,MEMF_FAST|MEMF_CHIP,MEMF_CHIP};
    unsigned mode;
    for (mode=0;mode<4;++mode) {
        struct pt_private_native_memory m;
        cm_reset(); cm_init(&m,1024); cm_trace_reset();
        cm_model.wrong_type_call=mode==3?2U:1U; cm_model.wrong_type_flags=flags[mode];
        assert(!pt_private_native_allocate(&m,64)); cm_retained(&m,PT_PRIVATE_MEMORY_TYPE);
        assert(m.live==0 && m.ambiguous.pointer && cm_live()==1 && cm_model.free_calls==0);
        assert(cm_model.type_calls==(mode==3?2U:1U)); cm_end(1);
    }
    /* A changed release endpoint refuses before the exact existing owner is freed. */
    {
        struct pt_private_native_memory m; void *p;
        cm_reset(); cm_init(&m,1024); p=pt_private_native_allocate(&m,64); assert(p);
        cm_trace_reset(); cm_model.wrong_type_call=2; cm_model.wrong_type_flags=0;
        assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_TYPE);
        assert(m.live==1 && cm_model.free_calls==0 && cm_model.type_calls==2); cm_end(1);
    }
}
static void no_pointer_header_release(void)
{
    struct pt_private_native_memory m; unsigned char *p;
    cm_reset(); cm_init(&m,1024); p=pt_private_native_allocate(&m,64); assert(p);
    __asan_poison_memory_region(p-32,32); __asan_poison_memory_region(p,64);
    assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_OK);
    assert(cm_model.free_calls==1 && cm_model.released_bytes==64 && m.live==0);
    __asan_unpoison_memory_region(p-32,96);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
}
static void foreign_interior_duplicate_release(void)
{
    unsigned mode;
    for (mode=0;mode<3;++mode) {
        struct pt_private_native_memory m; unsigned char foreign[64]; void *p,*bad;
        cm_reset(); cm_init(&m,1024); p=pt_private_native_allocate(&m,64); assert(p);
        memset(foreign,0x52,sizeof(foreign));
        if (mode==2) assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_OK);
        bad=mode==0?(void *)foreign:mode==1?(void *)((unsigned char *)p+16):p;
        cm_trace_reset(); __asan_poison_memory_region(foreign,sizeof(foreign));
        assert(pt_private_native_release_checked(&m,bad)==PT_PRIVATE_MEMORY_INVALID);
        cm_retained(&m,PT_PRIVATE_MEMORY_INVALID);
        assert(cm_model.type_calls==0 && cm_model.free_calls==0 && cm_model.trace_count==0);
        assert(m.live==(mode==2?0U:1U));
        pt_private_native_release(&m,bad);
        assert(m.last_result==PT_PRIVATE_MEMORY_INVALID && cm_model.trace_count==0);
        __asan_unpoison_memory_region(foreign,sizeof(foreign)); cm_end(mode==2?0U:1U);
    }
}
static void accounting_underflow_refusal(void)
{
    unsigned mode;
    for (mode=0;mode<6;++mode) {
        struct pt_private_native_memory m; void *p; unsigned char before[64];
        cm_reset(); cm_init(&m,1024); p=pt_private_native_allocate(&m,64); assert(p);
        memcpy(before,p,sizeof(before)); cm_trace_reset();
        /* Visible private allocator test state only, never an opaque core holder. */
        if (mode==0) m.pool.used=0;
        else if (mode==1) m.live=0;
        else if (mode==2) ++m.allocation[0].bytes;
        else if (mode==3) m.pool.flags=MEMF_CHIP;
        else if (mode==4) m.releases=UINT_MAX;
        else m.allocations=UINT_MAX;
        if (mode==5) assert(!pt_private_native_allocate(&m,16));
        else assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_ACCOUNTING);
        cm_retained(&m,PT_PRIVATE_MEMORY_ACCOUNTING);
        assert(cm_model.trace_count==0 && !memcmp(p,before,sizeof(before))); cm_end(1);
    }
}
static void operation_reentry(void)
{
    unsigned mode;
    for (mode=0;mode<7;++mode) {
        struct pt_private_native_memory m; void *p=NULL; int result;
        enum cm_event event=mode==0?CM_AVAIL:mode==1?CM_ALLOC:
            mode==4?CM_FREE:CM_TYPE;
        unsigned call=(mode==3 || mode==6)?2U:1U;
        cm_reset(); cm_init(&m,1024);
        if (mode>=4) { p=pt_private_native_allocate(&m,64); assert(p); }
        cm_trace_reset(); cm_hook(event,call,&m);
        if (mode<4) assert(!pt_private_native_allocate(&m,64));
        else {
            result=pt_private_native_release_checked(&m,p);
            assert(result==(mode==4?PT_PRIVATE_MEMORY_RETAINED:PT_PRIVATE_MEMORY_TYPE));
        }
        assert(cm_model.hook_fired==1 && cm_model.nested_result==PT_PRIVATE_MEMORY_BUSY);
        cm_retained(&m,PT_PRIVATE_MEMORY_BUSY);
        assert(cm_model.alloc_calls==(mode==1 || mode==2 || mode==3?1U:0U));
        if (mode==4) {
            assert(cm_model.free_calls==1 && cm_model.released_bytes==64);
            assert(m.live==0 && m.pool.used==0 && m.releases==1); cm_end(0);
        } else {
            assert(cm_model.free_calls==0);
            if (mode==0) { assert(m.live==0); cm_end(0); }
            else if (mode<4) { assert(m.live==0 && m.ambiguous.pointer); cm_end(1); }
            else { assert(m.live==1); cm_end(1); }
        }
    }
}
static void seal_before_source_exposure(void)
{
    struct pt_private_native_memory m; void *a,*b;
    cm_reset(); cm_init(&m,1024); a=pt_private_native_allocate(&m,32);
    b=pt_private_native_allocate(&m,64); assert(a && b);
    assert(pt_private_native_memory_seal(&m)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OWNED);
    assert(m.phase==PT_PRIVATE_MEMORY_SEALED_ALLOCATIONS);
    /* No source was exposed in this model: exact cleanup is independently allowed. */
    assert(pt_private_native_release_checked(&m,a)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_release_checked(&m,b)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK); cm_end(0);
    cm_reset(); cm_init(&m,1024); a=pt_private_native_allocate(&m,32); assert(a);
    assert(pt_private_native_memory_seal(&m)==PT_PRIVATE_MEMORY_OK); cm_trace_reset();
    assert(!pt_private_native_allocate(&m,1)); cm_retained(&m,PT_PRIVATE_MEMORY_SEALED);
    assert(cm_model.trace_count==0 && m.live==1);
    assert(pt_private_native_release_checked(&m,a)==PT_PRIVATE_MEMORY_RETAINED); cm_end(1);
}
static void explicit_uncertainty_retention(void)
{
    struct pt_private_native_memory m; void *p; unsigned char before[64];
    cm_reset(); cm_init(&m,1024); p=pt_private_native_allocate(&m,64); assert(p);
    memcpy(before,p,sizeof(before)); cm_trace_reset();
    assert(pt_private_native_memory_retain(&m)==PT_PRIVATE_MEMORY_RETAINED);
    assert(!pt_private_native_allocate(&m,1));
    assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_RETAINED);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_RETAINED);
    assert(pt_private_native_memory_seal(&m)==PT_PRIVATE_MEMORY_RETAINED);
    assert(pt_private_native_memory_init(&m,1024,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
    cm_retained(&m,PT_PRIVATE_MEMORY_RETAINED);
    assert(!memcmp(p,before,sizeof(before)) && cm_model.trace_count==0 && m.live==1); cm_end(1);
}
static void stats_aliases_full_capacity(void)
{
    union cm_control_and_stats u;
    struct cm_guarded_source source,before;
    struct pt_private_memory_span guard;
    struct pt_private_memory_stats s;
    unsigned char payload[160]; void *p;
    cm_reset(); memset(&u,0,sizeof(u)); memset(&source,0x6c,sizeof(source));
    memcpy(&before,&source,sizeof(before)); guard.data=&source; guard.bytes=sizeof(source);
    assert(pt_private_native_memory_init(&u.control,1024,&guard,1)==PT_PRIVATE_MEMORY_OK);
    p=pt_private_native_allocate(&u.control,160); assert(p); memcpy(payload,p,sizeof(payload));
    cm_trace_reset();
    {
        struct pt_private_native_memory image;
        memcpy(&image,&u.control,sizeof(image));
        assert(pt_private_native_memory_stats(&u.control,&u.stats)==PT_PRIVATE_MEMORY_ALIAS);
        assert(!memcmp(&image,&u.control,sizeof(image)));
    }
    assert(pt_private_native_memory_stats(&u.control,&source.scalar)==PT_PRIVATE_MEMORY_ALIAS);
    assert(pt_private_native_memory_stats(&u.control,(struct pt_private_memory_stats *)((unsigned char *)p+32))==PT_PRIVATE_MEMORY_ALIAS);
    assert(pt_private_native_memory_stats(&u.control,(struct pt_private_memory_stats *)(UINTPTR_MAX-7U))==PT_PRIVATE_MEMORY_ALIAS);
    assert(!memcmp(&source,&before,sizeof(source)) && !memcmp(payload,p,sizeof(payload)));
    memset(&s,0x28,sizeof(s));
    assert(pt_private_native_memory_stats(&u.control,&s)==PT_PRIVATE_MEMORY_OK);
    assert(s.live==1 && s.used==160 && cm_model.trace_count==0);
    assert(pt_private_native_release_checked(&u.control,p)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_memory_finish(&u.control)==PT_PRIVATE_MEMORY_OK); cm_end(0);
    {
        struct pt_private_native_memory m;
        unsigned char image[sizeof(struct pt_private_memory_stats)];
        struct pt_private_memory_stats *inside;
        cm_reset(); cm_init(&m,1024);
        cm_override(cm_arena[0].bytes+1); assert(!pt_private_native_allocate(&m,160));
        inside=(struct pt_private_memory_stats *)(void *)(cm_arena[0].bytes+32);
        memcpy(image,inside,sizeof(image)); cm_trace_reset();
        assert(pt_private_native_memory_stats(&m,inside)==PT_PRIVATE_MEMORY_ALIAS);
        assert(!memcmp(image,inside,sizeof(image)) && cm_model.trace_count==0); cm_end(0);
    }
}
static void finish_and_reinitialization(void)
{
    struct pt_private_native_memory m,image; void *p;
    cm_reset(); cm_init(&m,1024); p=pt_private_native_allocate(&m,64); assert(p);
    memcpy(&image,&m,sizeof(image)); cm_trace_reset();
    assert(pt_private_native_memory_init(&m,1024,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
    assert(!memcmp(&image,&m,sizeof(m)) && cm_model.trace_count==0);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OWNED);
    assert(m.phase==PT_PRIVATE_MEMORY_PREPARE && cm_model.free_calls==0);
    assert(pt_private_native_release_checked(&m,NULL)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_OK);
    assert(pt_private_native_memory_finish(&m)==PT_PRIVATE_MEMORY_OK);
    memcpy(&image,&m,sizeof(image)); cm_trace_reset();
    assert(pt_private_native_memory_init(&m,1024,NULL,0)==PT_PRIVATE_MEMORY_INVALID);
    assert(!pt_private_native_allocate(&m,16));
    assert(pt_private_native_release_checked(&m,p)==PT_PRIVATE_MEMORY_INVALID);
    assert(!memcmp(&image,&m,sizeof(m)) && cm_model.trace_count==0); cm_end(0);
}
int main(void)
{
    fast_ceiling_reserve_payload_charge();
    no_fast_and_zero_limit();
    invalid_shapes_and_wrap();
    budget_size_and_fixed_slot_pressure();
    returned_control_source_and_live_overlap();
    wrong_class_endpoints();
    no_pointer_header_release();
    foreign_interior_duplicate_release();
    accounting_underflow_refusal();
    operation_reentry();
    seal_before_source_exposure();
    explicit_uncertainty_retention();
    stats_aliases_full_capacity();
    finish_and_reinitialization();
    puts("CHECKED MEMORY HOST MODEL PASS: 14 bounded ownership/policy cases; no native placement or source-quiet proof");
    return 0;
}
