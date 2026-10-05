/* Explicit host software model. Force-include in all three TUs. */
#ifndef PT_CHECKED_MEMORY_HOST_STUBS_H
#define PT_CHECKED_MEMORY_HOST_STUBS_H
#include <stddef.h>
#include <stdint.h>
typedef uint32_t ULONG;
typedef void *APTR;
#define MEMF_PUBLIC UINT32_C(1)
#define MEMF_CHIP UINT32_C(2)
#define MEMF_FAST UINT32_C(4)
#define MEMF_TOTAL UINT32_C(0x80000)
ULONG AvailMem(ULONG);
APTR AllocMem(ULONG,ULONG);
void FreeMem(APTR,ULONG);
ULONG TypeOfMem(APTR);

#define CM_MODEL_BLOCKS 40U
#define CM_MODEL_BLOCK_BYTES 512U
#define CM_MODEL_TRACE 512U
enum cm_event { CM_AVAIL=1, CM_ALLOC, CM_TYPE, CM_FREE };
union cm_arena {
    long double alignment;
    uint64_t integer;
    void *pointer;
    unsigned char bytes[CM_MODEL_BLOCK_BYTES+64U];
};
struct cm_owned { void *pointer; size_t bytes; unsigned live; };
struct cm_observation { enum cm_event event; const void *pointer; size_t bytes; ULONG flags; };
struct pt_private_native_memory;
/* Dedicated bookkeeping context; never used as allocator/source/output data. */
struct cm_model {
    struct cm_owned owned[CM_MODEL_BLOCKS];
    struct cm_observation trace[CM_MODEL_TRACE];
    unsigned trace_count, avail_calls, alloc_calls, type_calls, free_calls;
    size_t fast_total, fast_free, chip_free, released_bytes;
    unsigned null_next, override_next;
    void *override_pointer;
    unsigned wrong_type_call;
    ULONG wrong_type_flags;
    enum cm_event hook_event;
    unsigned hook_call, hook_fired;
    struct pt_private_native_memory *hook_context;
    int nested_result;
};
extern struct cm_model cm_model;
extern union cm_arena cm_arena[CM_MODEL_BLOCKS];
void cm_reset(void);
void cm_trace_reset(void);
unsigned cm_live(void);
size_t cm_live_bytes(void);
void cm_end(unsigned);
void cm_hook(enum cm_event,unsigned,struct pt_private_native_memory *);
void cm_override(void *);
#endif
