#ifndef PT_PRIVATE_NATIVE_AUDIO_RESERVATION_H
#define PT_PRIVATE_NATIVE_AUDIO_RESERVATION_H
#if !defined(PT_NATIVE_AUDIO_RESERVATION_PRIVATE_SOURCE) || PT_NATIVE_AUDIO_RESERVATION_PRIVATE_SOURCE != 1
#error "Select the separate private audio reservation source explicitly"
#endif
#include <stddef.h>
#include <stdint.h>

/* One private, one-shot, ordinary-Process owner. No public populated-struct
 * adoption, caller completion flag, reset, retry, publication or replay API.
 * RESERVED and RETIRED concern only resources acquired by this module. They
 * convey no authority over legacy Paula/CIA/cache/document owners. */
enum pt_private_audio_result {
    PT_PRIVATE_AUDIO_NEW=0, PT_PRIVATE_AUDIO_INVALID,
    PT_PRIVATE_AUDIO_ENROLLING, PT_PRIVATE_AUDIO_REFUSED_CLOSED,
    PT_PRIVATE_AUDIO_RESERVED, PT_PRIVATE_AUDIO_RETIRING,
    PT_PRIVATE_AUDIO_RETIRED, PT_PRIVATE_AUDIO_HELD
};
enum pt_private_audio_stage {
    PT_PRIVATE_AUDIO_UNISSUED=0, PT_PRIVATE_AUDIO_ALLOCATING,
    PT_PRIVATE_AUDIO_OPENING, PT_PRIVATE_AUDIO_PRECEDENCE,
    PT_PRIVATE_AUDIO_LOCKING, PT_PRIVATE_AUDIO_FREEING,
    PT_PRIVATE_AUDIO_CONSUMING_LOCK, PT_PRIVATE_AUDIO_CLOSING,
    PT_PRIVATE_AUDIO_CLOSED, PT_PRIVATE_AUDIO_QUARANTINED
};
struct pt_private_audio_observation {
    enum pt_private_audio_result result;
    enum pt_private_audio_stage stage;
    int8_t open_return,open_error,precedence_return,precedence_error;
    int8_t free_return,free_error,lock_return,lock_error;
    unsigned open_completed,precedence_completed,free_completed;
    unsigned lock_issued,lock_completed;
    unsigned allocated_mask,freed_mask,lock_returned_mask;
};
/* Enrolls only its own genuine Create/Open/SETPREC/SendIO transitions. Fixed
 * four-channel selection and -128 initial precedence cannot steal channels.
 * No sample WRITE, direct register access, replay or CIA call is made. */
enum pt_private_audio_result pt_private_audio_enroll(void);
enum pt_private_audio_result pt_private_audio_retire(void);
/* Output is a diagnostic copy, never a certificate accepted by another API.
 * The output must be writable for capacity bytes and disjoint from this owner. */
int pt_private_audio_observe(struct pt_private_audio_observation *,size_t capacity);
/* Any uncertainty after enrollment enters a nonreturning sticky HELD path.
 * The original Process/HUNK/stack and every surviving resource remain owned.
 * Qualification must use a separately owned disposable VM. Not a default API. */
#endif
