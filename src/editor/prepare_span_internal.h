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
#endif
