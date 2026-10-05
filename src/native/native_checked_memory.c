/* Source-only checked ordinary allocator. No assertion or abort path. */
#include "native_checked_memory.h"
#include <limits.h>
#include <string.h>

#define PT_PRIVATE_MEMORY_INITIALIZED UINT32_C(0x4d454d31)
struct memory_control_alignment { char byte; struct pt_private_native_memory value; };
union memory_payload_alignment { long double real; uint64_t integer; void *pointer; };
struct memory_payload_alignment_offset { char byte; union memory_payload_alignment value; };

static int memory_span(const void *p, size_t bytes)
{
    uintptr_t first = (uintptr_t)p;
    return !bytes || (p && bytes <= UINTPTR_MAX-first);
}
static int memory_apart(const void *a, size_t an, const void *b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    if (!memory_span(a,an) || !memory_span(b,bn)) return 0;
    if (!an || !bn) return 1;
    return x >= y ? x-y >= bn : y-x >= an;
}
static int memory_context(const struct pt_private_native_memory *m)
{
    return m && memory_span(m,sizeof(*m))
        && (uintptr_t)m % offsetof(struct memory_control_alignment,value) == 0
        && m->initialized == PT_PRIVATE_MEMORY_INITIALIZED
        && m->guard_count <= PT_PRIVATE_MEMORY_GUARDS;
}
static int memory_fault(struct pt_private_native_memory *m, int reason)
{
    if (!m->failed) m->last_result = (unsigned)reason;
    m->failed = 1;
    m->phase = PT_PRIVATE_MEMORY_RETAIN_ALL;
    return reason;
}
static int memory_enter(struct pt_private_native_memory *m)
{
    if (!memory_context(m)) return PT_PRIVATE_MEMORY_INVALID;
    if (m->busy) return memory_fault(m,PT_PRIVATE_MEMORY_BUSY);
    if (m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL) return PT_PRIVATE_MEMORY_RETAINED;
    if (m->phase != PT_PRIVATE_MEMORY_PREPARE && m->phase != PT_PRIVATE_MEMORY_SEALED_ALLOCATIONS)
        return PT_PRIVATE_MEMORY_INVALID;
    return PT_PRIVATE_MEMORY_OK;
}
static int memory_guarded(const struct pt_private_native_memory *m, const void *p, size_t bytes)
{
    unsigned i;
    if (!memory_apart(p,bytes,m,sizeof(*m))) return 0;
    for (i=0;i<m->guard_count;++i)
        if (!memory_apart(p,bytes,m->guard[i].data,m->guard[i].bytes)) return 0;
    for (i=0;i<PT_PRIVATE_MEMORY_ALLOCATIONS;++i)
        if (m->allocation[i].pointer
            && !memory_apart(p,bytes,m->allocation[i].pointer,m->allocation[i].bytes)) return 0;
    return 1;
}
static int memory_accounted(const struct pt_private_native_memory *m)
{
    size_t sum = 0;
    unsigned i, count = 0;
    if (m->pool.flags != MEMF_FAST || m->pool.used > m->pool.limit) return 0;
    for (i=0;i<PT_PRIVATE_MEMORY_ALLOCATIONS;++i) {
        const struct pt_private_memory_allocation *a = &m->allocation[i];
        if (!a->pointer) { if (a->bytes) return 0; continue; }
        if (!a->bytes || a->bytes > UINT32_MAX || !memory_span(a->pointer,a->bytes)
            || a->bytes > SIZE_MAX-sum) return 0;
        sum += a->bytes;
        ++count;
    }
    return sum == m->pool.used && count == m->live;
}
static int memory_fast(struct pt_private_native_memory *m, const void *p, size_t bytes)
{
    const void *last = (const void *)((uintptr_t)p+bytes-1);
    ULONG flags=TypeOfMem((APTR)p);
    if (m->failed || m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL
        || (flags&(MEMF_FAST|MEMF_CHIP)) != MEMF_FAST) return 0;
    flags=TypeOfMem((APTR)last);
    return !m->failed && m->phase != PT_PRIVATE_MEMORY_RETAIN_ALL
        && (flags&(MEMF_FAST|MEMF_CHIP)) == MEMF_FAST;
}
static void memory_ambiguous(struct pt_private_native_memory *m, void *p, size_t bytes, int reason)
{
    m->ambiguous.pointer = p;
    m->ambiguous.bytes = bytes;
    memory_fault(m,reason);
}

int pt_private_native_memory_init(struct pt_private_native_memory *m, size_t limit,
    const struct pt_private_memory_span *guards, unsigned count)
{
    struct pt_private_native_memory next = {0};
    unsigned i;
    if (!m || !memory_span(m,sizeof(*m))
        || (uintptr_t)m % offsetof(struct memory_control_alignment,value) != 0
        || count > PT_PRIVATE_MEMORY_GUARDS
        || limit > UINT32_MAX || !memory_span(guards,count*sizeof(*guards))
        || !memory_apart(m,sizeof(*m),guards,count*sizeof(*guards))) return PT_PRIVATE_MEMORY_INVALID;
    if (!memory_apart(m,sizeof(*m),&next,sizeof(next))
        || !memory_apart(guards,count*sizeof(*guards),&next,sizeof(next))) return PT_PRIVATE_MEMORY_ALIAS;
    if (m->initialized || m->busy || m->phase != PT_PRIVATE_MEMORY_UNINITIALIZED)
        return PT_PRIVATE_MEMORY_INVALID;
    memset(&next,0,sizeof(next));
    if (count) memcpy(next.guard,guards,count*sizeof(*guards));
    for (i=0;i<count;++i) {
        if (!memory_span(next.guard[i].data,next.guard[i].bytes)) return PT_PRIVATE_MEMORY_INVALID;
        if (!memory_apart(m,sizeof(*m),next.guard[i].data,next.guard[i].bytes)) return PT_PRIVATE_MEMORY_ALIAS;
    }
    /* Availability queries only; do not publish a fallback Chip policy. */
    pt_master_memory_init(&next.pool);
    if (next.pool.flags != MEMF_FAST) return PT_PRIVATE_MEMORY_FAST_UNAVAILABLE;
    if (next.pool.limit > limit) next.pool.limit = limit;
    if (count && memcmp(next.guard,guards,count*sizeof(*guards))) return PT_PRIVATE_MEMORY_INVALID;
    if (m->initialized || m->busy || m->phase != PT_PRIVATE_MEMORY_UNINITIALIZED)
        return PT_PRIVATE_MEMORY_INVALID;
    next.guard_count = count;
    next.initialized = PT_PRIVATE_MEMORY_INITIALIZED;
    next.phase = PT_PRIVATE_MEMORY_PREPARE;
    next.last_result = PT_PRIVATE_MEMORY_OK;
    memcpy(m,&next,sizeof(next));
    return PT_PRIVATE_MEMORY_OK;
}

void *pt_private_native_allocate(void *context, size_t bytes)
{
    struct pt_private_native_memory *m = context;
    unsigned slot;
    size_t available;
    void *p;
    int result = memory_enter(m);
    if (result != PT_PRIVATE_MEMORY_OK) return NULL;
    if (m->phase != PT_PRIVATE_MEMORY_PREPARE) {
        memory_fault(m,PT_PRIVATE_MEMORY_SEALED);
        return NULL;
    }
    if (!bytes || bytes > UINT32_MAX) { m->last_result=PT_PRIVATE_MEMORY_INVALID; return NULL; }
    if (!memory_accounted(m)) { memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING); return NULL; }
    if (m->allocations == UINT_MAX) { memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING); return NULL; }
    for (slot=0;slot<PT_PRIVATE_MEMORY_ALLOCATIONS;++slot)
        if (!m->allocation[slot].pointer) break;
    if (slot == PT_PRIVATE_MEMORY_ALLOCATIONS) { m->last_result=PT_PRIVATE_MEMORY_CAPACITY; return NULL; }
    if (bytes > m->pool.limit-m->pool.used) {
        m->last_result=PT_PRIVATE_MEMORY_BUDGET;
        return NULL;
    }
    m->busy = 1;
    available=pt_master_memory_available(&m->pool);
    if (m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL || m->failed || !memory_accounted(m)) {
        m->busy=0;
        memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING);
        return NULL;
    }
    if (bytes > available) {
        m->last_result=PT_PRIVATE_MEMORY_BUDGET;
        m->busy=0;
        return NULL;
    }
    p = AllocMem((ULONG)bytes,MEMF_FAST|MEMF_PUBLIC);
    if (!p) {
        m->busy=0;
        if (m->phase != PT_PRIVATE_MEMORY_RETAIN_ALL) m->last_result=PT_PRIVATE_MEMORY_BUDGET;
        return NULL;
    }
    if (m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL || m->failed) {
        memory_ambiguous(m,p,bytes,PT_PRIVATE_MEMORY_BUSY);
        m->busy=0;
        return NULL;
    }
    /* No payload/header write or automatic FreeMem occurs on ambiguity. */
    if (!memory_guarded(m,p,bytes)) {
        memory_ambiguous(m,p,bytes,PT_PRIVATE_MEMORY_ALIAS);
        m->busy=0;
        return NULL;
    }
    if ((uintptr_t)p % offsetof(struct memory_payload_alignment_offset,value) != 0) {
        memory_ambiguous(m,p,bytes,PT_PRIVATE_MEMORY_INVALID);
        m->busy=0;
        return NULL;
    }
    if (!memory_fast(m,p,bytes)) {
        memory_ambiguous(m,p,bytes,PT_PRIVATE_MEMORY_TYPE);
        m->busy=0;
        return NULL;
    }
    if (m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL || m->failed || !memory_accounted(m)) {
        memory_ambiguous(m,p,bytes,PT_PRIVATE_MEMORY_ACCOUNTING);
        m->busy=0;
        return NULL;
    }
    m->allocation[slot].pointer=p;
    m->allocation[slot].bytes=bytes;
    m->pool.used+=bytes;
    ++m->live;
    if (m->live > m->peak_live) m->peak_live=m->live;
    ++m->allocations;
    m->last_result=PT_PRIVATE_MEMORY_OK;
    m->busy=0;
    return p;
}

int pt_private_native_release_checked(struct pt_private_native_memory *m, void *p)
{
    unsigned slot;
    size_t bytes;
    int result = memory_enter(m);
    if (result != PT_PRIVATE_MEMORY_OK) return result;
    if (!p) return PT_PRIVATE_MEMORY_OK;
    if (!memory_accounted(m) || m->releases == UINT_MAX)
        return memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING);
    for (slot=0;slot<PT_PRIVATE_MEMORY_ALLOCATIONS;++slot)
        if (m->allocation[slot].pointer == p) break;
    if (slot == PT_PRIVATE_MEMORY_ALLOCATIONS) return memory_fault(m,PT_PRIVATE_MEMORY_INVALID);
    bytes=m->allocation[slot].bytes;
    if (!bytes || bytes > m->pool.used || !m->live)
        return memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING);
    m->busy=1;
    if (!memory_fast(m,p,bytes) || m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL || m->failed) {
        m->busy=0;
        return memory_fault(m,PT_PRIVATE_MEMORY_TYPE);
    }
    /* Known exact pointer+size only. No size header is read from p. Source
     * shutdown/independent domains are the original caller's prerequisite. */
    FreeMem(p,(ULONG)bytes);
    m->allocation[slot].pointer=NULL;
    m->allocation[slot].bytes=0;
    m->pool.used-=bytes;
    --m->live;
    ++m->releases;
    m->busy=0;
    if (m->phase == PT_PRIVATE_MEMORY_RETAIN_ALL || m->failed) return PT_PRIVATE_MEMORY_RETAINED;
    m->last_result=PT_PRIVATE_MEMORY_OK;
    return PT_PRIVATE_MEMORY_OK;
}
void pt_private_native_release(void *context, void *pointer)
{
    (void)pt_private_native_release_checked(context,pointer);
}
int pt_private_native_memory_seal(struct pt_private_native_memory *m)
{
    int result=memory_enter(m);
    if (result != PT_PRIVATE_MEMORY_OK) return result;
    if (!memory_accounted(m)) return memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING);
    m->phase=PT_PRIVATE_MEMORY_SEALED_ALLOCATIONS;
    m->last_result=PT_PRIVATE_MEMORY_OK;
    return PT_PRIVATE_MEMORY_OK;
}
int pt_private_native_memory_retain(struct pt_private_native_memory *m)
{
    if (!memory_context(m)) return PT_PRIVATE_MEMORY_INVALID;
    return memory_fault(m,PT_PRIVATE_MEMORY_RETAINED);
}
int pt_private_native_memory_finish(struct pt_private_native_memory *m)
{
    int result=memory_enter(m);
    if (result != PT_PRIVATE_MEMORY_OK) return result;
    if (!memory_accounted(m)) return memory_fault(m,PT_PRIVATE_MEMORY_ACCOUNTING);
    if (m->live || m->pool.used) return PT_PRIVATE_MEMORY_OWNED;
    if (m->failed || m->ambiguous.pointer) return PT_PRIVATE_MEMORY_FAULT;
    m->phase=PT_PRIVATE_MEMORY_FINISHED;
    m->last_result=PT_PRIVATE_MEMORY_OK;
    return PT_PRIVATE_MEMORY_OK;
}
int pt_private_native_memory_stats(const struct pt_private_native_memory *m,
    struct pt_private_memory_stats *out)
{
    struct pt_private_memory_stats value;
    if (!memory_context(m) || !out || !memory_guarded(m,out,sizeof(*out))) return PT_PRIVATE_MEMORY_ALIAS;
    if (m->busy) return PT_PRIVATE_MEMORY_BUSY;
    if (m->ambiguous.pointer && !memory_apart(out,sizeof(*out),m->ambiguous.pointer,m->ambiguous.bytes))
        return PT_PRIVATE_MEMORY_ALIAS;
    memset(&value,0,sizeof(value));
    value.limit=m->pool.limit; value.used=m->pool.used; value.reserve=m->pool.reserve;
    value.live=m->live; value.peak_live=m->peak_live; value.allocations=m->allocations;
    value.releases=m->releases; value.phase=m->phase; value.failed=m->failed; value.last_result=m->last_result;
    memcpy(out,&value,sizeof(value));
    return PT_PRIVATE_MEMORY_OK;
}
