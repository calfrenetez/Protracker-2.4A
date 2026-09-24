#ifndef PT_AMIGUS_RESERVATION_H
#define PT_AMIGUS_RESERVATION_H
/* Serial owner-thread lifecycle only. No allocation, registers or interrupts.
 * Initialize storage to zero, never copy/move an open owner. Callbacks and their
 * context outlive close; all callbacks are synchronous and non-reentrant. */
struct pt_amigus_reservation_api {
    void *context;
    int (*open)(void *); /* 1 library opened, 0 unavailable; no partial ownership */
    void (*close)(void *);
    void *(*find)(void *, void *previous);
    int (*supported)(void *, void *card); /* known type with PCM block */
    unsigned long (*reserve)(void *, void *card, void *owner);
    void (*release)(void *, void *card, void *owner);
};
enum pt_amigus_reservation_result {
    PT_AMIGUS_RESERVED, PT_AMIGUS_NO_LIBRARY, PT_AMIGUS_NO_CARD,
    PT_AMIGUS_UNSUPPORTED, PT_AMIGUS_BUSY, PT_AMIGUS_DRIVER_ERROR,
    PT_AMIGUS_BAD_ENUMERATION, PT_AMIGUS_INVALID
};
struct pt_amigus_reservation {
    struct pt_amigus_reservation_api api;
    void *card;
    unsigned long driver_code;
    unsigned opened, reserved, access;
};
/* Select explicit zero-based library enumeration index 0..15. Never silently
 * fall back to another card. Repeated/cyclic enumeration is rejected. */
enum pt_amigus_reservation_result pt_amigus_reservation_open(
    struct pt_amigus_reservation *, const struct pt_amigus_reservation_api *, unsigned index);
/* A single access lease prevents library/card release during downstream use.
 * Acquire BEFORE creating any port/session. End ONLY after confirmed downstream
 * reset/detach and removal of any installed interrupt. These are caller proofs,
 * not assertions this lifecycle can verify. Reservation alone permits no MMIO. */
int pt_amigus_reservation_begin(struct pt_amigus_reservation *);
int pt_amigus_reservation_end(struct pt_amigus_reservation *);
/* 0 while leased; otherwise release own PCM reservation then close library.
 * FreeCard has no acknowledgement: success means calls issued, not hardware proof.
 * Idempotent. Failed open already unwinds its library reference. */
int pt_amigus_reservation_close(struct pt_amigus_reservation *);
/* Discovery only: never calls reserve/release, exports no card pointers. Context
 * must not be in concurrent use. Available means library opened, NOT playable.
 * Status 1 completed (including absent library), 0 invalid API, -1 bad/bounded
 * enumeration. Counts on error are partial. Every successful open is closed. */
struct pt_amigus_discovery {unsigned available, cards, pcm_cards;};
int pt_amigus_discover(const struct pt_amigus_reservation_api *, struct pt_amigus_discovery *);
#endif
