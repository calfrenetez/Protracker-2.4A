#ifndef PT_AMIGUS_INTERRUPT_OWNER_H
#define PT_AMIGUS_INTERRUPT_OWNER_H
#include "amigus_reservation.h"
/* Control-thread only, synchronous/nonreentrant callbacks. No allocation or IRQ
 * dispatch here. Zero-initialize and keep owner, reservation, API context and
 * binding stable until stop confirms quiescence. Binding is adapter-owned stable
 * callback storage, fully prepared before begin; installation may call it at once.
 * The already-held reservation access lease belongs to the surrounding output
 * owner. This guard prevents ending it while a callback might still be retained. */
struct pt_amigus_interrupt_api {
    void *context;
    unsigned long (*install)(void *,struct pt_amigus_reservation *,void *binding);
    void (*remove)(void *,struct pt_amigus_reservation *);
    /* Exactly1 proves the device/source is disabled, removal complete, no callback
     * executing and none possible later. 0 pending; every other value uncertain.
     * A VOID library removal return alone cannot establish this proof. */
    int (*quiesce)(void *,struct pt_amigus_reservation *,void *binding);
};
struct pt_amigus_interrupt_owner {
    struct pt_amigus_interrupt_api api;
    struct pt_amigus_reservation *reservation;
    void *binding;
    unsigned long install_code;
    unsigned remove_requested,done;
};
enum pt_amigus_interrupt_result {
    PT_AMIGUS_INTERRUPT_INVALID, PT_AMIGUS_INTERRUPT_READY,
    PT_AMIGUS_INTERRUPT_FAILED
};
/* Nonzero install status can follow partial installation: FAILED still owns the
 * guard and MUST be stopped. Repeated begin never replaces an attached owner. */
enum pt_amigus_interrupt_result pt_amigus_interrupt_owner_begin(
    struct pt_amigus_interrupt_owner *,struct pt_amigus_reservation *,
    const struct pt_amigus_interrupt_api *,void *binding);
/* Request removal once, then make one bounded quiesce call per invocation.
 * Returns1 only on confirmed quiescence (also idempotently when done).
 * Stop producer/device traffic separately. Retain storage on any other result. */
int pt_amigus_interrupt_owner_stop(struct pt_amigus_interrupt_owner *);
/* Refuses before confirmed stop; releases only this software owner. The caller
 * still owns the access lease and must complete all other downstream teardown. */
int pt_amigus_interrupt_owner_detach(struct pt_amigus_interrupt_owner *);
#endif
