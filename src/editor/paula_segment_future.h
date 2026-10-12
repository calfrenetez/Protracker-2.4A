#ifndef PT_PAULA_SEGMENT_FUTURE_H
#define PT_PAULA_SEGMENT_FUTURE_H
#include "sampler_paula_segment_readers.h"
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The fixed geometry future backend is a serialized HOST model only"
#endif
#define PT_PAULA_FUTURE_COMMANDS 4U
struct pt_paula_segment_future;
/* Prepared by TASK publication, never by activation. Original pointers are not
 * given to the resource provider. The packet is immutable while borrowed and
 * names genuinely retained source/cache identities, not ownership certificates.
 * REPROGRAM preserves initial fetch/phase; its repeat becomes effective at the
 * existing fetch boundary, not at an inferred frame/sample-count deadline. */
struct pt_paula_future_packet {
    uint64_t ticket,first,last;struct pt_paula_segment_plan geometry;
};
enum pt_paula_future_entry {PT_PAULA_FUTURE_TASK=1,PT_PAULA_FUTURE_DELIVERY=2};
/* Exact finite resource-provider contract. Every callback is bounded, forbids
 * reentry/mutation and runs inside enter/leave's WHOLE entry exclusion. enter1
 * gives exclusive TASK+delivery ownership; leave1 restores that original token.
 * Clock is actual original-frequency time. arm1 retains packet and schedules
 * exactly its [first,last); arm0 owns no reference/callback/effect; arm-1 is
 * unknown. rearm never rebases. apply1 is a real complete original activation
 * with actual persistent adoption; apply0/-1 may have effects and stay unknown.
 * apply sees only preconverted geometry. ack is not quiet. cancel requests
 * cancellation, never proves absence. ticket_quiet1 independently proves no
 * future resource delivery/callback can reference that exact packet/ticket.
 * reader_quiet1 independently drops ALL current/pending/superseded sample/cache
 * output references. source_close is attempted once; source_quiet1 separately
 * confirms no future delivery/callback and restoration of the source. Providers
 * live until dispose after positive quiet AND the final outer leave. Unknown
 * outer restoration retains all original recovery anchors; no retry or forced
 * release. Borrowed descriptors are checked at original saved extents before
 * reuse through independent reader retirement even after command detachment.
 * Source PCM mutation latches a conservative reader hold, never repaired or
 * forwarded as retirement proof; this model never repairs genuine core or
 * source objects. No MMIO,
 * native timer ABI or physical qualification is implemented by this API. */
struct pt_paula_future_ops {
    void *context;size_t context_bytes;
    int (*enter)(void *,enum pt_paula_future_entry,uint64_t *);
    int (*leave)(void *,enum pt_paula_future_entry,uint64_t);
    int (*clock)(void *,uint64_t *,uint32_t *);
    int (*arm)(void *,const struct pt_paula_future_packet *);
    int (*rearm)(void *,const struct pt_paula_future_packet *);
    int (*apply)(void *,const struct pt_paula_future_packet *);
    int (*ack)(void *,uint64_t);
    int (*cancel)(void *,uint64_t);
    int (*ticket_quiet)(void *,uint64_t);
    int (*reader_quiet)(void *);
    int (*source_close)(void *);
    int (*source_quiet)(void *);
};
enum pt_paula_future_result {PT_PAULA_FUTURE_OK,PT_PAULA_FUTURE_PENDING,
    PT_PAULA_FUTURE_EARLY,PT_PAULA_FUTURE_STALE,PT_PAULA_FUTURE_LATE,
    PT_PAULA_FUTURE_UNKNOWN,PT_PAULA_FUTURE_INVALID};
struct pt_paula_future_status {
    unsigned accepted,detached,reader_retired,adopted,closing,source_attempted,
        source_quiet,failed;uint64_t trigger_observed,trigger_issued;
};
size_t pt_paula_segment_future_size(void);
size_t pt_paula_segment_future_alignment(void);
/* Caller-owned aligned, complete, zero storage is noncopyable. Numeric extent
 * and disjointness checks precede typed inputs. No callback/allocation on refusal.
 * Only exact increasing four boundaries and one persistent reader are admitted.
 * Output is the opt-in geometry backend; ordinary sampler/song refusals stay. */
int pt_paula_segment_future_init(void *,size_t,const struct pt_paula_future_ops *,
    const struct pt_scheduled_grid *,const uint64_t [4],struct pt_paula_segment_backend *);
/* HOST delivery seam, not a native entry. A stale old ticket cannot act on a
 * newer record. EARLY reuses exactly the same window. Crossed/late/unknown paths
 * retain the real command/reader/resource obligations; no immediate fallback. */
enum pt_paula_future_result pt_paula_segment_future_deliver(struct pt_paula_segment_future *,uint64_t);
/* Closes admission and requests each actual outstanding cancellation once.
 * Source shutdown is attempted only after distinct command and reader proofs;
 * subsequent calls are read-only source quiet. No forced disposal. */
enum pt_paula_future_result pt_paula_segment_future_stop(struct pt_paula_segment_future *);
int pt_paula_segment_future_get(const struct pt_paula_segment_future *,struct pt_paula_future_status *);
int pt_paula_segment_future_dispose(struct pt_paula_segment_future *);
#endif
