/* RAM-only diagnostic protocol. No application backend is enabled here.
 * All adapter callbacks below remain separately unqualified native obligations.
 * Every task/IRQ entry and all borrowed input edits require external exclusion.
 */
#ifndef PT_PRIVATE_NATIVE_RAM_PORT_H
#define PT_PRIVATE_NATIVE_RAM_PORT_H
#include "readers_activation.h"
#define PT_PRIVATE_RAM_COMMANDS 2U
#define PT_PRIVATE_RAM_READERS 8U
#define PT_PRIVATE_RAM_SLOTS 4U
#define PT_PRIVATE_RAM_TRACE 64U

struct pt_private_ram_adapter {
    void *context;
    /* Return1 ONLY for actual ReadEClock success: honest counter/frequency,
     * never shifted/estimated. Any other return refuses without publishing
     * a trace entry or caller clock outputs. Adapter outputs are local scratch.
     */
    int (*clock)(void *,uint64_t *,uint32_t *);
    /* Task-side independently owned stopped CIA vector only. 1 = copied
     * dispatch state armed. 0 = no callback references and positively stopped,
     * no pending owned interrupt. -1 = uncertain; never erase prepared state.
     * Existing pt_diagnostic_cia_arm_at's boolean is NOT this classification.
     */
    int (*arm_at)(void *,uint64_t,uint32_t);
    /* Task-only:1 positively proves no future dispatch names THIS ticket.
     * Quiet for the current owned ticket requires positive owned mask/stop/
     * pending-clear and dispatch-disable observations. A superseded ticket may
     * leave the exact successor armed ONLY with actual normal-task callback-
     * return provenance, whole-entry exclusion and immutable future dispatch
     * naming that successor. Wrong-task or exact-ticket mismatch refuses;
     * immutable port/source contexts and serialized entry are REQUIRED, not
     * arbitrary mutation detection promised by this adapter. Neither a
     * completed flag nor FindTask alone proves callback return.
     * Never remove/rearm a foreign timer or invoke a task owner from IRQ.
     */
    int (*ticket_quiet)(void *,uint64_t,unsigned);
    /* Task-only:1 positively stops/removes the exact owned callback source;
     * uncertain close leaves this adapter and all contexts live for recovery.
     */
    int (*source_close)(void *);
};
struct pt_private_ram_command {
    struct pt_readers_activation_packet packet;
    struct pt_readers_activation *ledger;
    uint64_t arm_tick;
    unsigned live,armed,finished,uncertain;
};
struct pt_private_ram_reader {
    struct pt_readers_key key;
    const uint8_t *data;size_t bytes;
    uint16_t period;uint8_t volume;
    unsigned live,active;
};
struct pt_private_ram_trace {uint64_t ticks;uint32_t frequency;};
struct pt_private_ram_port {
    struct pt_private_ram_adapter adapter;
    struct pt_private_ram_command command[PT_PRIVATE_RAM_COMMANDS];
    struct pt_private_ram_reader reader[PT_PRIVATE_RAM_READERS];
    struct pt_readers_key slot[PT_PRIVATE_RAM_SLOTS];
    struct pt_private_ram_trace trace[PT_PRIVATE_RAM_TRACE];
    uint64_t session,generation,last_clock;
    uint32_t frequency;
    unsigned mask,early_ticks,residency_ticks,maximum_reads;
    unsigned dispatching,trace_count,source_closed,clock_seen;
    unsigned publishes,commits,effects,entries,fires;
    int armed_index,dispatch_result,ledger_result;
};
/* Entire context is fixed separately owned RAM. Initialize in task context
 * before exposing any CIA source. Config/contexts/clock request remain live
 * through positive callback shutdown, queue close and ledger close.
 */
int pt_private_ram_init(struct pt_private_ram_port *,uint64_t,uint64_t,uint32_t,
    const struct pt_private_ram_adapter *,unsigned,unsigned,unsigned);
struct pt_readers_activation_port pt_private_ram_api(struct pt_private_ram_port *);
/* Called only by the independently owned CIA library-style trampoline. It
 * invokes the genuine ledger exactly once after an actual bounded aperture.
 * Never calls any core queue, owner, allocator, editor, or conversion API.
 */
int pt_private_ram_dispatch(struct pt_private_ram_port *);
int pt_private_ram_source_close(struct pt_private_ram_port *);
#endif
