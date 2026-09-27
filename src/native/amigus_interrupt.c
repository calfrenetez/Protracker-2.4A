#include "amigus_interrupt.h"
LONG pt_native_amigus_interrupt_dispatch(struct pt_native_amigus_interrupt *owner)
{
    if(!owner || !owner->armed || !owner->handler)return 0;
    return owner->handler(owner->context)==1?1:0;
}
