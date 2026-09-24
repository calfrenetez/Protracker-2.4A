#ifndef PT_PAULA_MEMORY_H
#define PT_PAULA_MEMORY_H
#include <exec/memory.h>
#include <proto/exec.h>
#include <stddef.h>
#include <stdint.h>
/* Match the Chip-only master allocator's display/system reserve. Query on
 * every allocation: a startup budget alone cannot track other applications.
 * This is a conservative admission check, not a reservation against concurrent
 * system allocation. AllocMem remains the final authority for fragmentation. */
#define PT_PAULA_CHIP_RESERVE (512UL*1024)
static inline size_t pt_paula_chip_available(void)
{
    size_t available=AvailMem(MEMF_CHIP);
    return available>PT_PAULA_CHIP_RESERVE?available-PT_PAULA_CHIP_RESERVE:0;
}
static inline void *pt_paula_chip_allocate(void *context,size_t bytes)
{
    (void)context;
    if(!bytes || bytes>UINT32_MAX || bytes>pt_paula_chip_available())return NULL;
    return AllocMem((ULONG)bytes,MEMF_CHIP|MEMF_PUBLIC);
}
static inline void pt_paula_chip_release(void *context,void *data,size_t bytes)
{(void)context;FreeMem(data,(ULONG)bytes);}
#endif
