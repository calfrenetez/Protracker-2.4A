/* Separate private ordinary-RAM causal pair diagnostic. No timer/IRQ/CIA,
 * MMIO, native entry, source acquisition, audio or whole-song PLAY wiring. */
#ifndef PT_PRIVATE_NATIVE_MIXED_CAUSAL_RAM_PORT_H
#define PT_PRIVATE_NATIVE_MIXED_CAUSAL_RAM_PORT_H
#include "mixed_readers_causal.h"
#include "mixed_readers_causal_control_internal.h"
#include "mixed_readers_causal_lineage_internal.h"
#include "mixed_readers_causal_rolling_internal.h"
#define PT_PRIVATE_CAUSAL_RAM_COMMANDS 2U
#define PT_PRIVATE_CAUSAL_RAM_READERS 32U
#define PT_PRIVATE_CAUSAL_RAM_SLOTS 20U
#define PT_PRIVATE_CAUSAL_RAM_ACTIONS 16U
#define PT_PRIVATE_CAUSAL_RAM_TRACE 64U
struct pt_private_mixed_causal_ram_adapter {
    void *context;size_t context_bytes;
    int (*clock)(void *,uint64_t *,uint32_t *);
    /* Original registration/ticket/first tick. 1 independently armed, 0
     * positive absence of NEW callbacks/effects, -1 unknown. Raw0 after
     * callback reentry is uncertain. Pointers are never dereferenced. */
    int (*arm_at)(void *,const struct pt_mixed_causal_registration *,uint64_t,uint64_t,uint32_t);
    /* Exact independent ticket proof; may not silently cancel its successor.
     * 1 no future callback,0 pending,-1 unknown; malformed also uncertain. */
    int (*ticket_quiet)(void *,const struct pt_mixed_causal_registration *,uint64_t,unsigned);
    int (*source_close)(void *,const struct pt_mixed_causal_registration *);
    int (*source_quiet)(void *,const struct pt_mixed_causal_registration *);
};
/* Rolling-only recorded control flow, never inferred from !armed/raw0.
 * NOT_ENTERED authenticates the pre-clock owned identity without a packet;
 * ENTERED is sticky around every actual arm/quiet callback. Old modes use0. */
#define PT_PRIVATE_CAUSAL_RAM_ARM_NOT_ENTERED 1U
#define PT_PRIVATE_CAUSAL_RAM_ARM_ENTERED 2U
struct pt_private_mixed_causal_ram_command {
    struct pt_mixed_causal_packet packet;
    struct pt_mixed_causal_command_identity identity,predecessor;
    struct pt_mixed_causal_owner *owner;
    uint64_t arm_tick,serial,predecessor_observed,predecessor_issued;
    unsigned known,live,armed,finished,disabled,uncertain,quiet_confirmed;
    unsigned successor,predecessor_completed;
    unsigned rolling_arm_state;
    unsigned lineage_phase; /* 0 existing modes; 1 root,2 CONTROL,3 STOP. */
    int arm_outcome,ticket_outcome,fire_result;
    int rolling_ticket_raw; /* Actual adapter return; ticket_outcome is guarded. */
};
struct pt_private_mixed_causal_ram_reader {
    struct pt_mixed_readers_key key;
    struct pt_mixed_readers_action action;
    struct pt_mixed_readers_card card;
    unsigned live,active;
};
struct pt_private_mixed_causal_ram_trace {uint64_t ticks;uint32_t frequency;};
/* Geometry-free ORIGINAL first completion. Event/holder addresses are opaque
 * copied identities, including after actual command disposal; never followed. */
struct pt_private_mixed_causal_ram_completed {
    struct pt_mixed_causal_command_identity identity;
    uint64_t frame,first_tick,last_tick,observed,issued;
    struct pt_mixed_causal_actual post;
    unsigned valid;
};
struct pt_private_mixed_causal_ram_port {
    struct pt_private_mixed_causal_ram_port *self;
    struct pt_private_mixed_causal_ram_adapter adapter;
    struct pt_mixed_causal_registration registration;
    struct pt_private_mixed_causal_ram_command command[2];
    struct pt_private_mixed_causal_ram_reader reader[32];
    struct pt_mixed_readers_key slot[20];
    struct pt_private_mixed_causal_ram_trace trace[64];
    uint64_t session,generation,last_clock;
    uint32_t frequency;
    unsigned initialized,bound,mask,clock_seen,maximum_reads,residency_ticks;
    unsigned entry_busy,adapter_busy,dispatching,fire_running,failed,faults;
    unsigned trace_count,publishes,commits,effects,entries,fires,pair_used,suppressed;
    unsigned source_attempted,source_closed,source_probes,quiet_probes;
    int clock_outcome,source_outcome,quiet_outcome,dispatch_result,ledger_result;
    /* Additive opt-in after-first CONTROL mode. Default init leaves these ZERO.
     * This full port parent must be admitted with its enlarged sizeof. */
    unsigned control_mode,controls;
    struct pt_private_mixed_causal_ram_completed completed;
    struct pt_mixed_causal_control_publication control_before;
    /* Separate finite capability. Existing constructors keep this full tail ZERO.
     * Histories contain scalar original identities only, never live C ownership. */
    unsigned lineage_mode,stops,root_command_quiet;
    struct pt_private_mixed_causal_ram_completed control_completed;
    struct pt_mixed_causal_control_stop_publication stop_before;
    /* Constructor-issued disjoint rolling capability. Two rows are recycled
     * only after independent ticket quiet; root/latest completion copies are
     * scalar histories, never ownership or dereferenced disposed addresses. */
    unsigned rolling_mode;
    uint64_t rolling_serial;
    struct pt_mixed_causal_rolling_publication rolling_before;
};
/* Fresh full ZERO, noncopyable, immutable complete extents, no reset/reuse.
 * Whole-entry task/fire/input-edit exclusion is caller-owned. The entire port,
 * adapter callback/context extent and original owner controls remain live until
 * independent source quiet and actual owner disposal. Unknown retains all. */
int pt_private_mixed_causal_ram_init(struct pt_private_mixed_causal_ram_port *,
    uint64_t,uint64_t,uint32_t,const struct pt_private_mixed_causal_ram_adapter *,unsigned,unsigned);
/* Explicit fresh opt-in constructor, otherwise the same original contracts.
 * Caller must separately bind control_api to the genuine EMPTY core owner before
 * factory/first admission. No STOP, third command, source or timer authority. */
int pt_private_mixed_causal_ram_init_control(struct pt_private_mixed_causal_ram_port *,
    uint64_t,uint64_t,uint32_t,const struct pt_private_mixed_causal_ram_adapter *,unsigned,unsigned);
/* Distinct fresh finite TRIGGER16 -> CONTROL16 -> STOP16 constructor. Two
 * command rows only; actual independent C1 quiet plus the genuine core typed
 * C1-detachment admission permit exactly one reuse of row0 for C3. Bind the
 * distinct lineage_api to the genuine EMPTY owner through its typed controller.
 * Never bind the existing CONTROL-only capability for this mode. Histories
 * remain copied values after actual C1 storage disposal. No fourth command. */
int pt_private_mixed_causal_ram_init_lineage(struct pt_private_mixed_causal_ram_port *,
    uint64_t,uint64_t,uint32_t,const struct pt_private_mixed_causal_ram_adapter *,unsigned,unsigned);
struct pt_mixed_causal_control_stop_port pt_private_mixed_causal_ram_lineage_api(
    struct pt_private_mixed_causal_ram_port *);
/* Distinct rolling constructor; existing CONTROL/finite API getters refuse
 * this mode. Actual core/factory disposal is checked independently by their
 * typed rolling admission, never inferred from this port's quiet result. */
int pt_private_mixed_causal_ram_init_rolling(struct pt_private_mixed_causal_ram_port *,
    uint64_t,uint64_t,uint32_t,const struct pt_private_mixed_causal_ram_adapter *,unsigned,unsigned);
struct pt_mixed_causal_rolling_port pt_private_mixed_causal_ram_rolling_api(
    struct pt_private_mixed_causal_ram_port *);
/* Once immediately after genuine open+queue borrow, before factory, packets
 * or source exposure. Includes successful empty/cancel-before-publish owners.
 * Unbound constructor-recovery close is deliberately refused; no software
 * absence or shutdown is inferred for an owner the port never received. */
int pt_private_mixed_causal_ram_bind(struct pt_private_mixed_causal_ram_port *,
    struct pt_mixed_causal_owner *,struct pt_mixed_readers_output *,uint64_t,uint64_t);
struct pt_mixed_causal_port pt_private_mixed_causal_ram_api(struct pt_private_mixed_causal_ram_port *);
/* Returns an all-ZERO refused capability unless explicit constructor mode was
 * selected. Copied by genuine control_bind; no implicit bind or callback. */
struct pt_mixed_causal_control_port pt_private_mixed_causal_ram_control_api(struct pt_private_mixed_causal_ram_port *);
/* Genuine fire once for this exact original ticket. EARLY retains ticket,
 * callbacks and geometry. No polling, rebasing or predicted ACTIVE. COMMITTED
 * preserves raw adoption separately from a later completion-check failure.
 * A first completion tombstone is accepted only after actual core post-clock
 * result, complete resulting registry and scalar copied-value match against the
 * genuine tombstone. The current clock trace must contain exactly entry/pre/post
 * reads; a changed call graph refuses rather than guessing clock phases. */
int pt_private_mixed_causal_ram_dispatch(struct pt_private_mixed_causal_ram_port *,uint64_t);
#endif
