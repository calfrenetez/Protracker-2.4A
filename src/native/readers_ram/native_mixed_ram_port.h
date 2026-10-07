/* Private ordinary-RAM diagnostic port, independent of the four-Paula port.
 * No CIA, MMIO, audio, source acquisition or native entry is wired here.
 * Whole task entries, manual dispatch and borrowed input edits are externally
 * excluded. Fixed context and the COMPLETE adapter context extent stay live,
 * immutable and disjoint through positive source quiet and owner disposal.
 */
#ifndef PT_PRIVATE_NATIVE_MIXED_RAM_PORT_H
#define PT_PRIVATE_NATIVE_MIXED_RAM_PORT_H
#include "mixed_readers_activation.h"
#define PT_PRIVATE_MIXED_RAM_COMMANDS 2U
#define PT_PRIVATE_MIXED_RAM_READERS 32U
#define PT_PRIVATE_MIXED_RAM_SLOTS 20U
#define PT_PRIVATE_MIXED_RAM_ACTIONS 16U
#define PT_PRIVATE_MIXED_RAM_TRACE 64U

struct pt_private_mixed_ram_adapter {
    void *context;size_t context_bytes;
    /* Exactly1 publishes honest actual counter/frequency into local scratch.
     * No shifted clock, estimated ticks, fallback or hidden source arm. */
    int (*clock)(void *,uint64_t *,uint32_t *);
    /* Original copied registration/ticket only; never dereference its pointers.
     * 1 armed,0 positive absence of NEW callback references/effects/pending
     * ticket,-1 unknown. Raw0 after callback reentry is NOT absence. */
    int (*arm_at)(void *,const struct pt_mixed_activation_registration *,uint64_t,uint64_t,uint32_t);
    /* 1 proves no present/future callback names this exact original ticket,
     * under whole-entry exclusion, without cancelling a successor ticket.
     * 0 pending,-1 unknown. A flag or completed body alone is not proof. */
    int (*ticket_quiet)(void *,const struct pt_mixed_activation_registration *,uint64_t,unsigned);
    /* One bounded actual shutdown attempt, then separate read-only probes.
     * 1 proves no callback can name the exact owner/registration. Queue may be
     * freed: both pointers are opaque identities, NEVER traversed. */
    int (*source_close)(void *,const struct pt_mixed_activation_registration *);
    int (*source_quiet)(void *,const struct pt_mixed_activation_registration *);
};
struct pt_private_mixed_ram_command {
    struct pt_mixed_activation_packet packet;
    struct pt_mixed_readers_activation *owner;
    uint64_t arm_tick;
    unsigned live,armed,finished,disabled,uncertain;
    int arm_outcome,ticket_outcome;
};
struct pt_private_mixed_ram_reader {
    struct pt_mixed_readers_key key;
    struct pt_mixed_readers_action action;
    struct pt_mixed_readers_card card;
    unsigned live,active;
};
struct pt_private_mixed_ram_trace {uint64_t ticks;uint32_t frequency;};
struct pt_private_mixed_ram_port {
    struct pt_private_mixed_ram_port *self;
    struct pt_private_mixed_ram_adapter adapter;
    struct pt_mixed_activation_registration registration;
    struct pt_private_mixed_ram_command command[PT_PRIVATE_MIXED_RAM_COMMANDS];
    struct pt_private_mixed_ram_reader reader[PT_PRIVATE_MIXED_RAM_READERS];
    struct pt_mixed_readers_key slot[PT_PRIVATE_MIXED_RAM_SLOTS];
    struct pt_private_mixed_ram_trace trace[PT_PRIVATE_MIXED_RAM_TRACE];
    uint64_t session,generation,last_clock;
    uint32_t frequency;
    unsigned initialized,bound,mask,clock_seen,early_ticks,residency_ticks,maximum_reads;
    unsigned entry_busy,adapter_busy,dispatching,fire_running,failed,faults;
    unsigned trace_count,publishes,commits,effects,entries,fires;
    unsigned source_attempted,source_closed,source_probes,quiet_probes,unbound_terminal;
    int armed_index,clock_outcome,source_outcome,quiet_outcome,dispatch_result,ledger_result;
};
/* Fresh ZERO context only. Noncopyable and never reset/reused after init,
 * including after close; every later lifetime needs fresh storage/session.
 * Complete valid readable/writable capacities are caller-owned. Init checks
 * full zero bytes, nonwrapping extents and known port/descriptor/context aliases
 * before any write; no initialized or dirty context can be reset. */
int pt_private_mixed_ram_init(struct pt_private_mixed_ram_port *,uint64_t,uint64_t,
    uint32_t,const struct pt_private_mixed_ram_adapter *,unsigned,unsigned,unsigned);
/* Private caller binds ONCE immediately after successful genuine activation
 * open + borrow_queue, before validation/factory/enqueue/publication or exposing
 * any callback source. Includes empty/cancel-before-publication lifetimes.
 * These are original borrowed identities, not a readiness certificate/getter;
 * no first packet may learn the registration. Failed constructor recovery with
 * no genuine borrowed queue uses only the terminal software-empty constructor
 * recovery branch: never-bound, no references/source exposure ever occurred,
 * matching immutable session/generation. No incoming owner/queue is learned;
 * no shutdown is called. The branch forbids future bind/publication/dispatch.
 * This supplies no physical source proof; source acquisition before bind is
 * prohibited. Actual failed-constructor recovery still needs separate testing.
 */
int pt_private_mixed_ram_bind(struct pt_private_mixed_ram_port *,
    struct pt_mixed_readers_activation *,struct pt_mixed_readers_output *,uint64_t,uint64_t);
struct pt_mixed_activation_port pt_private_mixed_ram_api(struct pt_private_mixed_ram_port *);
/* Manual bounded entry calls genuine fire once inside the original window.
 * Only copied scalar geometry is traversed. Native trampoline, helpers, total
 * IRQ stack, residency/WCET/timing and device acceptance are separate. */
int pt_private_mixed_ram_dispatch(struct pt_private_mixed_ram_port *);
#endif
