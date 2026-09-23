#ifndef PT_MASTER_MEMORY_H
#define PT_MASTER_MEMORY_H
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdint.h>
#include <stddef.h>
/* One shared ceiling for document, sampler versions/undo and song storage.
 * Fast-equipped machines never spill masters into Chip when Fast is exhausted.
 * Chip-only machines may use the remainder above a display/Paula reserve. */
struct pt_master_memory { size_t limit,used,reserve; ULONG flags; };
static inline void pt_master_memory_init(struct pt_master_memory *m)
{
    size_t free_bytes;
    m->flags=AvailMem(MEMF_FAST|MEMF_TOTAL)?MEMF_FAST:MEMF_CHIP;
    free_bytes=AvailMem(m->flags);
    m->reserve=m->flags==MEMF_FAST?256UL*1024:512UL*1024;
    m->limit=free_bytes>m->reserve?free_bytes-m->reserve:0;m->used=0;
}
static inline size_t pt_master_memory_available(const struct pt_master_memory *m)
{
    size_t available=AvailMem(m->flags),budget=m->used<m->limit?m->limit-m->used:0;
    available=available>m->reserve?available-m->reserve:0;
    return available<budget?available:budget;
}
static inline void *pt_master_allocate(void *context,size_t bytes)
{
    struct pt_master_memory *m=context;size_t total;size_t *p;
    if(!m || !bytes || bytes>SIZE_MAX-sizeof(size_t))return NULL;
    total=bytes+sizeof(size_t);
    if(total>UINT32_MAX || total>pt_master_memory_available(m))return NULL;
    p=AllocMem((ULONG)total,m->flags|MEMF_PUBLIC);if(!p)return NULL;
    *p=total;m->used+=total;return p+1;
}
static inline void pt_master_release(void *context,void *pointer)
{
    struct pt_master_memory *m=context;size_t *p,total;
    if(!pointer)return;
    p=(size_t *)pointer-1;total=*p;m->used-=total;FreeMem(p,(ULONG)total);
}
#endif
