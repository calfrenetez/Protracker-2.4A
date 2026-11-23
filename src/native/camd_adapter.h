#ifndef PT_NATIVE_CAMD_ADAPTER_H
#define PT_NATIVE_CAMD_ADAPTER_H

#include "../platform/camd_control.h"

struct Library;

struct pt_native_camd {
    struct pt_native_camd *self;
    struct Library *library;
    void *lock;
};

/* Binds the portable control owner to the classic 68k CAMD ABI. Both original
 * objects start zeroed and live until the portable owner finishes closing.
 * Explicit control activation can load drivers. Discovery and sender-link
 * creation do not grant delivery or exact-timing capability. */
int pt_native_camd_control_init(struct pt_native_camd *, struct pt_camd_control *);

#endif
