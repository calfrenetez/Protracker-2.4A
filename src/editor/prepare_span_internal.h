#ifndef PT_PREPARE_SPAN_INTERNAL_H
#define PT_PREPARE_SPAN_INTERNAL_H
#include <stddef.h>
#include <stdint.h>

/* Numeric containment only, never allocation provenance or reader authority.
 * A valid parent and bounded offset imply child+bytes cannot overflow. Keep
 * both non-NULL checks and the original nonempty-child convention. No pointee
 * is followed, even for refused numeric boundary probes. */
static inline int pt_prepare_span_contains(const void *parent,size_t parent_bytes,
    const void *child,size_t child_bytes)
{
    uintptr_t start=(uintptr_t)parent,address=(uintptr_t)child;
    if(!child_bytes||!parent||!child||parent_bytes>UINTPTR_MAX-start||
       child_bytes>parent_bytes||address<start)return 0;
    return address-start<=parent_bytes-child_bytes;
}

/* Caller has just proved first != NULL, first_bytes != 0 and no first-end
 * overflow. Reuse only inside that callback-free numeric query; never across
 * calls or ownership checks. The complete other span is still checked here.
 * No pointee is followed and no allocation/reader authority is granted. */
static inline int pt_prepare_valid_span_apart(const void *first,size_t first_bytes,
    const void *other,size_t other_bytes)
{
    uintptr_t x=(uintptr_t)first,y=(uintptr_t)other;
    return (!other_bytes||(other&&other_bytes<=UINTPTR_MAX-y))&&
        (!other_bytes||(x<=y?first_bytes<=y-x:other_bytes<=x-y));
}
#endif
