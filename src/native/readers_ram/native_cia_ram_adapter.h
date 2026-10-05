/* Opt-in test-only CIA adapter; no application binding or native launch
 * qualification.
 * Every task-side operation below is called from the original normal task,
 * never an IRQ, exception, signal handler, NMI or owner callback. The caller
 * serializes the ENTIRE ledger/core task entry against the CIA path. These
 * functions nest matched Disable/Enable and never Wait, allocate or print.
 *
 * On the documented single-CPU Exec path, normal task execution resumes only
 * after the interrupt/server/epilogue returns. Entering Disable from that task
 * excludes new maskable callbacks/preemption until the owned timer is masked,
 * stopped and pending-clear sampled. This is a source-design inference with
 * explicit call-graph assumptions, not an armed0/FindTask/flags certificate.
 * IRQ ABI, actual library/helper behavior, system-stack bounds, residency and
 * whole-batch WCET remain unqualified. No native run eligibility follows here.
 */
#ifndef PT_PRIVATE_CIA_RAM_ADAPTER_H
#define PT_PRIVATE_CIA_RAM_ADAPTER_H
#include "native_ram_irq_layout.h"
#ifndef PT_PRIVATE_CIA_ADAPTER_STUB
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <devices/timer.h>
#include <hardware/cia.h>
#include <proto/exec.h>
#include <proto/timer.h>
#define CIA_BASE_NAME cia_resource
#include <proto/cia.h>
#endif
#ifndef PT_PRIVATE_CIA_HARDWARE
#define PT_PRIVATE_CIA_HARDWARE(chip) ((volatile struct CIA *)(chip?0xbfe001UL:0xbfd000UL))
#endif
#ifndef PT_PRIVATE_CIA_CONTROL_WRITE
#define PT_PRIVATE_CIA_CONTROL_WRITE(pointer,value) (*(pointer)=(value))
#endif
enum pt_private_cia_phase {PT_PRIVATE_CIA_EMPTY,PT_PRIVATE_CIA_STOPPED,
    PT_PRIVATE_CIA_ARMED,PT_PRIVATE_CIA_UNCERTAIN,PT_PRIVATE_CIA_CLOSED};
struct pt_private_cia_ram_adapter {
    struct pt_private_cia_ram_adapter *self;
    struct Task *task;struct Library *resource;struct Device *timer;
    struct Interrupt server;struct pt_private_ram_irq *irq;
    volatile UBYTE *control,*low,*high;
    UBYTE saved_control;unsigned chip,bit;
    unsigned vector_owned,close_attempted,removal_attempted;
    enum pt_private_cia_phase phase;
    uint64_t active_ticket,arm_observed;
    uint32_t frequency,arm_frequency;
    unsigned arm_count,arms,disarms,quiet_probes;
    unsigned exec_version,resource_version,timer_version;
    WORD last_mask,last_pending;
};
/* Fresh zero-init/noncopyable separately owned context. IRQ/task/port/timer and
 * complete control/output extents are already independently retained/disjoint.
 * 1 owns a STOPPED, masked, pending-clear free vector. 0 never acquired one.
 * -1 acquired then became uncertain: all code/process/HUNK/library/contexts
 * remain live; do not free/Exit or automatically repeat an operation.
 * Actual Exec/timer/resource common Library versions>=36 precede AddICRVector.
 * Actual ReadEClock frequency is checked before acquisition; that eligibility
 * sample is not an epoch, deadline correction or permission to shift a window.
 */
int pt_private_cia_ram_acquire(struct pt_private_cia_ram_adapter *,struct Device *,
    struct pt_private_ram_irq *,uint32_t);
/* Direct actual ReadEClock through a copied immutable Device base; no original
 * request/owner traversal or FindTask in this IRQ-capable callback. Raw SDK
 * ReadEClock returns its frequency; this wrapper returns EXACTLY1 only after
 * validating the actual returned frequency, and publishes outputs only then.
 */
int pt_private_cia_ram_clock(void *,uint64_t *,uint32_t *);
/* Task-only tri-state publication:1 armed original deadline;0 positively
 * stopped/masked/pending-clear with dispatch disabled and no new ticket;
 * -1 uncertain, retaining prepared callback identity/code/library/contexts.
 * No shifted deadline, latency subtraction, task-wakeup activation or IRQ rearm.
 */
int pt_private_cia_ram_arm_at(void *,uint64_t,uint32_t);
/* Explicit exact-owned task-side disarm retaining vector. The source's current
 * ticket must match; a mismatched successor is never cancelled. An uncertain
 * arm may be recovered only by ONE guarded source-close attempt, not retries.
 */
int pt_private_cia_ram_disarm(struct pt_private_cia_ram_adapter *,uint64_t);
int pt_private_cia_ram_ticket_quiet(void *,uint64_t,unsigned);
/* One guarded cleanup attempt;1 follows stop/mask/pending queries, exact vector
 * removal under the same normal-task exclusion and post-removal queries.
 * Resource register/library query failures retain opaque owner metadata and
 * all lifetimes. Subsequent calls after failed cleanup return-1 without retry.
 * The future entry/launcher MUST implement a non-killing hold; no timeout exit.
 * Free timer count latches are not restored; original stopped control is.
 */
int pt_private_cia_ram_source_close(void *);
struct pt_private_ram_adapter pt_private_cia_ram_api(struct pt_private_cia_ram_adapter *);
#endif
