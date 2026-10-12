#ifndef PT_PAULA_SEGMENT_PROVIDER_H
#define PT_PAULA_SEGMENT_PROVIDER_H
#include "paula_segment_packet32.h"
#if defined(__m68k__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)
#error "The typed geometry provider is an inhibited serialized HOST model"
#endif
#define PT_PAULA_PROVIDER_RECORDS 4U
#define PT_PAULA_PROVIDER_LABELS 3U
struct pt_paula_segment_provider;
enum pt_paula_provider_packet_action {
    PT_PAULA_PROVIDER_ARM=1,PT_PAULA_PROVIDER_REARM,PT_PAULA_PROVIDER_APPLY
};
enum pt_paula_provider_ticket_action {
    PT_PAULA_PROVIDER_ACK=1,PT_PAULA_PROVIDER_CANCEL,PT_PAULA_PROVIDER_TICKET_QUIET
};
/* Owned, immutable through actual ticket quiet and the following positive outer
 * restore. No original descriptor, owner, cache or HOST pointer is exposed.
 * The big-endian model record does not designate real native addresses. */
struct pt_paula_provider_record {
    union pt_paula_packet32_record packet;
    uint64_t ticket,first,last,frame;
    unsigned publication;
};
struct pt_paula_provider_label {
    uint16_t sample;
    uint32_t descriptor,master,version,sampler,project,cache_owner,cache_base32;
};
struct pt_paula_provider_config {
    uint32_t frequency;
    uint64_t frame[PT_PAULA_PROVIDER_RECORDS];
    struct pt_paula_provider_label label[PT_PAULA_PROVIDER_LABELS];
};
/* These injected operations are a resource contract, not an implementation of
 * a timer, IRQ mask, SR restore, DMA or native ABI. enter1 exclusively owns the
 * entire TASK/delivery entry; restore1 positively restores its exact token.
 * Unknown enter/restore holds storage and admission. Every operation is bounded
 * and forbids nested adapter/owner calls and mutable holder/record writes.
 * arm0 positively owns no packet/callback/effect; all unknown returns retain.
 * apply1 is the provider's complete modeled activation; it does not grant real
 * hardware or genuine core adoption. ack/cancel are separate from ticket quiet.
 * ticket quiet1 positively ends all resource use of that exact owned record;
 * reader quiet1 independently ends all current/pending/superseded output use.
 * source close is attempted once, source quiet is an independent proof.
 * Callback context, code and complete storage remain live through final restore.
 * This HOST contract refuses callbacks during restore. Native deferred delivery
 * during real SR restoration requires a separate ABI/serialization implementation.
 */
struct pt_paula_provider_resources {
    void *context;size_t context_bytes;
    int (*enter)(void *,enum pt_paula_future_entry,uint64_t *);
    int (*restore)(void *,enum pt_paula_future_entry,uint64_t);
    int (*clock)(void *,uint64_t *,uint32_t *);
    int (*packet)(void *,enum pt_paula_provider_packet_action,
                  const struct pt_paula_provider_record *);
    int (*ticket)(void *,enum pt_paula_provider_ticket_action,uint64_t);
    int (*reader_quiet)(void *);
    int (*source_close)(void *);
    int (*source_quiet)(void *);
};
struct pt_paula_provider_status {
    unsigned published,retained,quiet,entered,restore_hold,failed,
        reader_quiet,source_attempted,source_quiet;
    int enter_raw,restore_raw,source_close_raw;
    unsigned entry_kind;uint64_t entry_token; /* Observation only, no restoration grant. */
};
struct pt_paula_provider_row_status {
    uint64_t ticket,first,last,frame;
    unsigned present,attempted,quiet,original_released,applied,cancel_attempted;
    int arm_raw,rearm_raw,apply_raw,ack_raw,cancel_raw,quiet_raw;
};
size_t pt_paula_segment_provider_size(void);
size_t pt_paula_segment_provider_alignment(void);
/* No allocation or callback on constructor refusal; outputs remain untouched.
 * Complete aligned ZERO noncopyable storage and immutable config are required.
 * Labels are explicit model numbers, never HOST pointer conversions. Arm binds
 * their sample keys to supplied prepared metadata; equality is not authentication.
 * Source/version/cache ownership and currentness are upstream obligations.
 * Numeric known master-capacity/cache/descriptor/guard extents protect all owned
 * writes, with no payload reads. Unknown opaque extents remain caller-disjoint.
 * Four once-only publications, no slot reuse, retiming or automatic retry.
 * Original packet storage stays live until ticket quiet plus positive restore.
 * Full-holder restoration repairs only this module's owned metadata, never the
 * supplied packet, PCM, core history, reference counts or source/cache objects.
 * Callback faults latch failure but genuine independent quiet can still retire
 * resource obligations. Unknown outer restore cannot be retried or disposed.
 */
int pt_paula_segment_provider_init(void *,size_t,
    const struct pt_paula_provider_resources *,const struct pt_paula_provider_config *,
    struct pt_paula_future_ops *);
int pt_paula_segment_provider_get(const struct pt_paula_segment_provider *,
    struct pt_paula_provider_status *);
int pt_paula_segment_provider_row(const struct pt_paula_segment_provider *,unsigned,
    struct pt_paula_provider_row_status *);
int pt_paula_segment_provider_dispose(struct pt_paula_segment_provider *);
#endif
