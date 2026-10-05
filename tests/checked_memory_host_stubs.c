#include "checked_memory_host_stubs.h"
#include "native_checked_memory.h"
#include <assert.h>
#include <string.h>

struct cm_model cm_model;
union cm_arena cm_arena[CM_MODEL_BLOCKS];
static void cm_record(enum cm_event event,const void *p,size_t bytes,ULONG flags)
{
    struct cm_observation *o;
    assert(cm_model.trace_count < CM_MODEL_TRACE);
    o=&cm_model.trace[cm_model.trace_count++];
    o->event=event; o->pointer=p; o->bytes=bytes; o->flags=flags;
}
static void cm_reenter(enum cm_event event,unsigned call)
{
    if (cm_model.hook_event != event || cm_model.hook_call != call) return;
    cm_model.hook_event=0;
    ++cm_model.hook_fired;
    /* NULL release has no external effects, but genuine initialized busy
     * entry must still latch retention before it can accept the no-op. */
    cm_model.nested_result=pt_private_native_release_checked(cm_model.hook_context,NULL);
}
void cm_reset(void)
{
    /* Each caller first ends every model client. Reset of a static host arena
     * is test disposal, not native force-free/recovery of retained owners. */
    memset(&cm_model,0,sizeof(cm_model));
    memset(cm_arena,0xa5,sizeof(cm_arena));
    cm_model.fast_total=1024U*1024U;
    cm_model.fast_free=1024U*1024U;
    cm_model.chip_free=1024U*1024U;
}
void cm_trace_reset(void)
{
    cm_model.trace_count=0; cm_model.avail_calls=0; cm_model.alloc_calls=0;
    cm_model.type_calls=0; cm_model.free_calls=0; cm_model.released_bytes=0;
    memset(cm_model.trace,0,sizeof(cm_model.trace));
}
unsigned cm_live(void)
{
    unsigned i,n=0;
    for (i=0;i<CM_MODEL_BLOCKS;++i) n+=cm_model.owned[i].live;
    return n;
}
size_t cm_live_bytes(void)
{
    unsigned i; size_t n=0;
    for (i=0;i<CM_MODEL_BLOCKS;++i)
        if (cm_model.owned[i].live) n+=cm_model.owned[i].bytes;
    return n;
}
void cm_end(unsigned remaining)
{
    assert(cm_live()==remaining);
    assert(cm_model.trace_count<=CM_MODEL_TRACE);
    /* No allocator cleanup is performed here. Retained instances are never
     * resumed. Subsequent reset occurs only after this test's clients end. */
}
void cm_hook(enum cm_event event,unsigned call,struct pt_private_native_memory *m)
{
    cm_model.hook_event=event; cm_model.hook_call=call;
    cm_model.hook_context=m; cm_model.hook_fired=0; cm_model.nested_result=-1;
}
void cm_override(void *p)
{
    cm_model.override_next=1; cm_model.override_pointer=p;
}
ULONG AvailMem(ULONG flags)
{
    size_t result;
    cm_record(CM_AVAIL,NULL,0,flags);
    ++cm_model.avail_calls;
    result=(flags&MEMF_TOTAL)?cm_model.fast_total:
        (flags&MEMF_FAST)?cm_model.fast_free:cm_model.chip_free;
    assert(result<=UINT32_MAX);
    cm_reenter(CM_AVAIL,cm_model.avail_calls);
    return (ULONG)result;
}
APTR AllocMem(ULONG bytes,ULONG flags)
{
    unsigned i; void *p=NULL;
    ++cm_model.alloc_calls;
    assert(bytes && bytes<=CM_MODEL_BLOCK_BYTES);
    assert(flags==(MEMF_FAST|MEMF_PUBLIC));
    if (cm_model.null_next) cm_model.null_next=0;
    else if (cm_model.override_next) {
        p=cm_model.override_pointer; cm_model.override_next=0;
        /* An alias supplied by the stub is NOT a fresh model allocation. */
    } else {
        for (i=0;i<CM_MODEL_BLOCKS;++i) if (!cm_model.owned[i].live) break;
        assert(i<CM_MODEL_BLOCKS && bytes<=cm_model.fast_free);
        p=cm_arena[i].bytes+32U;
        cm_model.owned[i].pointer=p; cm_model.owned[i].bytes=bytes;
        cm_model.owned[i].live=1; cm_model.fast_free-=bytes;
    }
    cm_record(CM_ALLOC,p,bytes,flags);
    cm_reenter(CM_ALLOC,cm_model.alloc_calls);
    return p;
}
ULONG TypeOfMem(APTR p)
{
    unsigned i; ULONG result=MEMF_FAST;
    ++cm_model.type_calls;
    for (i=0;i<CM_MODEL_BLOCKS;++i) {
        const struct cm_owned *a=&cm_model.owned[i];
        uintptr_t x=(uintptr_t)p,y=(uintptr_t)a->pointer;
        if (a->live && x>=y && x-y<a->bytes) break;
    }
    assert(i<CM_MODEL_BLOCKS); /* No unknown-pointer placement probe. */
    if (cm_model.type_calls==cm_model.wrong_type_call) result=cm_model.wrong_type_flags;
    cm_record(CM_TYPE,p,0,result);
    cm_reenter(CM_TYPE,cm_model.type_calls);
    return result;
}
void FreeMem(APTR p,ULONG bytes)
{
    unsigned i;
    ++cm_model.free_calls;
    for (i=0;i<CM_MODEL_BLOCKS;++i)
        if (cm_model.owned[i].live && cm_model.owned[i].pointer==p) break;
    assert(i<CM_MODEL_BLOCKS && cm_model.owned[i].bytes==bytes);
    cm_record(CM_FREE,p,bytes,0);
    cm_model.owned[i].live=0;
    cm_model.fast_free+=bytes; cm_model.released_bytes+=bytes;
    /* This free has completed before the bounded reentry observation. */
    cm_reenter(CM_FREE,cm_model.free_calls);
}
