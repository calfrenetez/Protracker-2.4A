#ifndef PT_NATIVE_AMIGUS_INTERRUPT_CALLS_H
#define PT_NATIVE_AMIGUS_INTERRUPT_CALLS_H
#include "../core/amigus_interrupt_owner.h"
/* Low-level install/remove callbacks for pt_amigus_interrupt_api. Context is a
 * pt_native_amigus_library used by this reservation; binding is a fully prepared
 * pt_native_amigus_interrupt. A device adapter MUST supply an independent quiesce
 * callback: these calls neither disable the source nor prove callback removal.
 * Not wired into the editor, discovery probe or a hardware playback endpoint. */
unsigned long pt_native_amigus_interrupt_install(void *,struct pt_amigus_reservation *,void *);
void pt_native_amigus_interrupt_remove(void *,struct pt_amigus_reservation *);
#endif
