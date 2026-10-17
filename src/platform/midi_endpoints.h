#ifndef PT_MIDI_ENDPOINTS_H
#define PT_MIDI_ENDPOINTS_H
#include "../core/midi.h"
#include "../core/project.h"

/* Portable serialized control-side registry, not a CAMD/link/clock backend.
 * Zero the complete owner before init; never copy/reinitialize a live owner or
 * operate its genuine embedded MIDI core independently. Identity strings are
 * exact/case-sensitive, nonempty and NUL-terminated within PT_MIDI_ENDPOINT.
 * They are configured endpoint identities, not hardware quiet certificates.
 * The sink/context and owner live until explicit retirement/owner release; the
 * context's full declared span must be disjoint from the owner. Callback code
 * may only record/accept the complete message; it must not modify the owner,
 * retain message/name pointers or destroy contexts. Reentrant registry calls
 * refuse unchanged. An accepted outer send is not rolled back by that refusal.
 */
struct pt_midi_endpoint_name {char identity[PT_MIDI_ENDPOINT];};
struct pt_midi_endpoint_sink {
    void *context;
    size_t context_bytes;
    /* Atomic complete 1..3-byte acceptance, otherwise zero/none accepted. */
    int (*send)(void *,const char *identity,const uint8_t *,size_t);
};
enum pt_midi_endpoint_result {
    PT_ENDPOINT_OK,PT_ENDPOINT_INVALID,PT_ENDPOINT_BUSY,PT_ENDPOINT_CAPACITY,
    PT_ENDPOINT_IN_USE,PT_ENDPOINT_UNAVAILABLE,PT_ENDPOINT_SEND_FAILED
};
struct pt_midi_endpoints {
    struct pt_midi_endpoints *self;
    struct pt_midi midi;
    struct pt_midi_endpoint_sink sink;
    struct pt_midi_endpoint_name names[PT_MIDI_PORTS];
    uint16_t available,link_active;
    unsigned busy;
};
int pt_midi_endpoints_init(struct pt_midi_endpoints *,const struct pt_midi_endpoint_sink *);
/* Validate the entire bounded snapshot, including duplicates/capacity/alias,
 * before mutation. Observation emits no messages. Existing missing endpoints
 * are genuinely disconnected (sendless, retaining queued core releases); newly
 * discovered/reappearing names are NEVER connected or flushed implicitly.
 * Absent identities occupy their original slots until explicit safe forget.
 */
enum pt_midi_endpoint_result pt_midi_endpoints_observe(struct pt_midi_endpoints *,
    const struct pt_midi_endpoint_name *,size_t count);
/* Scalar/borrowed read-only observations refuse while busy. find's bytes include
 * exactly one final NUL, 2..64. name pointer lives only until registry mutation;
 * it grants no note/link/quiet authority and must not be retained by callbacks. */
int pt_midi_endpoints_find(const struct pt_midi_endpoints *,const char *,size_t bytes);
int pt_midi_endpoints_available(const struct pt_midi_endpoints *,unsigned);
const struct pt_midi_endpoint_name *pt_midi_endpoints_name(const struct pt_midi_endpoints *,unsigned);
/* Explicit connect1 may flush genuine old Note Off/sustain obligations. Failure
 * preserves the exact identity and core obligations; only caller-requested retry.
 * Disconnect0 is sendless. Discovery/selection do not invoke connect1. */
enum pt_midi_endpoint_result pt_midi_endpoints_connection(struct pt_midi_endpoints *,unsigned,int);
/* Refuse while ANY bound/active voice, pending_off, sustain or needs_flush uses
 * this index. A clean core connected bit1 is allowed: forget invalidates adapter
 * link_active+mapping, not a physical link. All sends require adapter availability
 * AND explicit link_active. New identity must separately connect1 even if the
 * core's old clean connected bit remains1. No core ledger is cleared/copied. */
enum pt_midi_endpoint_result pt_midi_endpoints_forget(struct pt_midi_endpoints *,unsigned);
/* Selection binds only an inactive track and emits no messages; explicit release
 * or stop is required before changing an active selection. Unbind may release.
 * Unavailable registered identities may be selected without emitting output. */
enum pt_midi_endpoint_result pt_midi_endpoints_bind(struct pt_midi_endpoints *,unsigned track,unsigned endpoint,unsigned channel);
enum pt_midi_endpoint_result pt_midi_endpoints_unbind(struct pt_midi_endpoints *,unsigned track);
enum pt_midi_endpoint_result pt_midi_endpoints_note_on(struct pt_midi_endpoints *,unsigned,unsigned,unsigned);
enum pt_midi_endpoint_result pt_midi_endpoints_note_off(struct pt_midi_endpoints *,unsigned);
enum pt_midi_endpoint_result pt_midi_endpoints_controller(struct pt_midi_endpoints *,unsigned,unsigned,unsigned);
enum pt_midi_endpoint_result pt_midi_endpoints_program(struct pt_midi_endpoints *,unsigned,unsigned);
enum pt_midi_endpoint_result pt_midi_endpoints_bend(struct pt_midi_endpoints *,unsigned,unsigned);
enum pt_midi_endpoint_result pt_midi_endpoints_realtime(struct pt_midi_endpoints *,unsigned,unsigned);
enum pt_midi_endpoint_result pt_midi_endpoints_stop(struct pt_midi_endpoints *);
#endif
