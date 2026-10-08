/* PRIVATE HOST MODEL ONLY. No native acquisition, IRQ, clock or QUIET path. */
#ifndef PT_PRIVATE_HOST_TWO_LEASE_H
#define PT_PRIVATE_HOST_TWO_LEASE_H
#include <stddef.h>
#include <stdint.h>
#ifndef PT_PRIVATE_TWO_LEASE_HOST_MODEL
#error "Explicit HOST two-lease model opt-in required"
#endif
#if defined(__m68k__) || defined(__mc68000__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "Two-lease transaction model is HOST-only; native admission remains closed"
#endif

/* Logical HOST projection of the exact four registration identity fields.
 * These pointers are opaque: no owner/queue/Task/Device/native traversal. */
struct pt_host_pair_registration {
    const void *owner, *queue;
    uint64_t session, generation;
};
typedef int (*pt_host_pair_entry)(void *);
struct pt_host_pair_binding {
    struct pt_host_pair_registration registration;
    uint64_t ticket, first;
    uint32_t frequency;
    const void *resource, *server, *server_data;
    pt_host_pair_entry server_code;
    unsigned chip, bit;
};
struct pt_host_pair_image {
    uint16_t programmed_latch, mask, pending;
    uint8_t control;
    const void *vector_data;
    pt_host_pair_entry vector_code;
};
struct pt_host_pair_input {
    const void *source, *original_task, *timer, *port;
    struct pt_host_pair_registration registration;
    struct pt_host_pair_binding binding[2];
    struct pt_host_pair_image before[2];
};
/* Assertions supplied only by a synchronous HOST mock. They neither capture
 * an original native latch nor establish native exclusion or full return. */
struct pt_host_pair_snapshot {
    struct pt_host_pair_image image;
    const void *resource_owner, *server_owner, *installed_server;
    unsigned available, resource_held, server_installed;
    unsigned image_complete, whole_exclusion, full_return;
    unsigned callbacks_inflight, delivery_queued;
};
enum pt_host_pair_effect {
    PT_HOST_PAIR_NO_EFFECTS = 0,
    PT_HOST_PAIR_EFFECT_DONE = 1,
    PT_HOST_PAIR_EFFECT_UNKNOWN = -1
};
struct pt_host_pair_ops {
    void *context;
    size_t context_bytes;
    /* inspect is read-only. All four calls are bounded, synchronous, non-
     * reentrant and retain neither borrowed binding nor before-image pointers.
     * acquire addresses ONE distinct resource/chip/bit/server binding.
     * remove_server restores that exact complete image but retains the lease.
     * return_resource is a separate operation, after proven server restoration.
     * 0 means no NEW effects;1 completed;anything else means unknown. Even1
     * requires the actual mock snapshot to match, including unchanged sibling.
     * No callback may mutate this transaction or its copied identity storage. */
    int (*inspect)(void *, const struct pt_host_pair_binding *, struct pt_host_pair_snapshot *);
    int (*acquire)(void *, const struct pt_host_pair_binding *, const void *, const struct pt_host_pair_image *);
    int (*remove_server)(void *, const struct pt_host_pair_binding *, const void *, const struct pt_host_pair_image *);
    int (*return_resource)(void *, const struct pt_host_pair_binding *, const void *);
};
enum pt_host_pair_recorded_effect {
    PT_HOST_PAIR_NOT_CALLED = 0,
    PT_HOST_PAIR_RECORDED_NO_EFFECTS = 1,
    PT_HOST_PAIR_RECORDED_DONE = 2,
    PT_HOST_PAIR_RECORDED_UNKNOWN = 3
};
enum pt_host_pair_result {
    PT_HOST_PAIR_INVALID = -1,
    PT_HOST_PAIR_RETAINED = -2,
    PT_HOST_PAIR_REFUSED = 0,
    PT_HOST_PAIR_ACQUIRED = 1,
    PT_HOST_PAIR_RESTORED = 2,
    PT_HOST_PAIR_OBSERVED = 3
};
struct pt_host_pair_record {
    struct pt_host_pair_binding binding;
    unsigned restore_intent;
    enum pt_host_pair_recorded_effect acquire, remove_server, return_resource;
};
struct pt_host_pair_transaction {
    struct pt_host_pair_transaction *self;
    struct pt_host_pair_input input, original;
    struct pt_host_pair_ops ops, original_ops;
    struct pt_host_pair_record record[2];
    struct pt_host_pair_snapshot initial[2], last[2];
    unsigned initialized, attempted, snapshots_known, close_intent, fault;
    unsigned call_busy;
};
/* Fresh full-zero, disjoint caller storage only. No reset/refill. */
int pt_host_pair_init(struct pt_host_pair_transaction *, const struct pt_host_pair_input *, const struct pt_host_pair_ops *);
/* At most one acquisition call per lease. A second no-effect refusal invokes
 * once-only first-lease removal and separate resource return. Unknown outcomes
 * stop effects and retain BOTH records; they never trigger an automatic retry. */
int pt_host_pair_acquire(struct pt_host_pair_transaction *);
/* Exact selector validates both copied bindings before any intent/effect.
 * Original ticket/absolute first/frequency are never rebased or recomputed. */
const struct pt_host_pair_record *pt_host_pair_lookup(const struct pt_host_pair_transaction *, const struct pt_host_pair_registration *, uint64_t);
int pt_host_pair_restore_once(struct pt_host_pair_transaction *, const struct pt_host_pair_registration *, uint64_t);
/* First close requests cleanup in reverse acquisition order. Repeats return
 * cached results without invoking inspect, removal or resource return again. */
int pt_host_pair_close_once(struct pt_host_pair_transaction *);
/* <=one read-only inspect. No intent/outcome/fault writes and no cleanup retry.
 * Returned mock state cannot clear a retained unknown outcome. */
int pt_host_pair_probe(const struct pt_host_pair_transaction *, const struct pt_host_pair_registration *, uint64_t, struct pt_host_pair_snapshot *);
/* RESTORED describes only the HOST transaction effects. Every record remains
 * in caller storage. It is NOT a native QUIET/full-outer-leave/disposal proof. */
#endif
