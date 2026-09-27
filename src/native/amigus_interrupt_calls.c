#include "amigus_interrupt_calls.h"
#include "amigus_interrupt.h"
#include "amigus_reservation.h"
#include "../diagnostic/amigus_calls.h"
static int valid(struct pt_native_amigus_library *n,struct pt_amigus_reservation *r)
{
    return n && n->base && r && r->api.context==n && r->card && r->opened &&
        r->reserved && r->access && r->interrupt &&
        (r->resource==PT_AMIGUS_PCM || r->resource==PT_AMIGUS_WAVETABLE);
}
unsigned long pt_native_amigus_interrupt_install(void *context,struct pt_amigus_reservation *r,void *binding)
{
    struct pt_native_amigus_library *n=context;
    struct pt_native_amigus_interrupt *i=binding;
    struct Library *AmiGUS_Base;
    if(!valid(n,r) || !i || !i->handler || i->armed!=1)return AmiGUS_InterruptInstallFailed;
    AmiGUS_Base=n->base;
    return PT_InstallInterrupt((struct AmiGUS *)r->card,(LONG)r->resource,r,
        (AmiGUS_Interrupt)pt_native_amigus_interrupt_entry,i);
}
void pt_native_amigus_interrupt_remove(void *context,struct pt_amigus_reservation *r)
{
    struct pt_native_amigus_library *n=context;struct Library *AmiGUS_Base;
    if(!valid(n,r))return;
    AmiGUS_Base=n->base;
    PT_RemoveInterrupt((struct AmiGUS *)r->card,(LONG)r->resource,r);
}
