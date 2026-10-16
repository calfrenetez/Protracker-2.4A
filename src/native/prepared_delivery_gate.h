#ifndef PT_NATIVE_PREPARED_DELIVERY_GATE_H
#define PT_NATIVE_PREPARED_DELIVERY_GATE_H
#include <stddef.h>
#include <stdint.h>

#define PT_NATIVE_DELIVERY_GATE_SLOTS 4U
struct pt_native_delivery_gate;
enum pt_native_delivery_gate_result {
    PT_NATIVE_DELIVERY_GATE_FAULT=-1,
    PT_NATIVE_DELIVERY_GATE_REFUSED=0,
    PT_NATIVE_DELIVERY_GATE_OK=1
};
/* An entry-order token, not a source pin, activation or quiet certificate.
 * Only the original output object is accepted. Do not copy, alter, reuse or
 * release it until its successful leave has returned. It remains live through
 * a failed/uncertain restore, together with the gate and every caller context. */
struct pt_native_delivery_gate_token {
    const struct pt_native_delivery_gate *gate;
    uint64_t generation,serial;
    uint32_t kind;
};
/* Numeric observation only. No value denotes actual clock acquisition, output,
 * adoption, command detachment, reader retirement or source quiet. */
struct pt_native_delivery_gate_record {
    uint64_t ticket,observed,issued;
    int32_t result;
    uint32_t detail;
};

/* Restricted original normal-TASK/local ordinary-C API. NEVER call from a real
 * IRQ, Exec Cause/software interrupt, exception, supervisor or signal path.
 * Task identity is only a cross-task check, not proof of execution context.
 * No caller-imposed arbitrary IPL/SR state is supported. Existing balanced Exec
 * Disable nesting is permitted. These restrictions require a genuine static
 * call path; no token, FindTask identity or counter observation proves them.
 *
 * The gate owns one matched Disable/Enable obligation per admitted entry, not
 * an arbitrary saved-SR/IPL restoration promise. No Wait, allocation, traversal,
 * retention/release callback, timer, output, SR write or counter write is here.
 * Caller code between enter/leave must likewise be fixed, bounded metadata work
 * only: never preparation/conversion, blocking calls or external callbacks.
 * All storage is ordinary, complete, stable and noncopyable; caller supplies
 * accessible objects, not arbitrary pointers. Unknown external extents remain
 * caller-disjoint. Known full gate capacity, ExecBase, original current Task and
 * active/restoring token spans exclude all caller outputs.
 *
 * Before Enable, active entry ends but a distinct restoring anchor remains.
 * A deferred LOCAL ordinary-C model delivery may record a staged slot during
 * TASK restore. TASK reentry and nested delivery refuse. Real Enable admitting
 * a hardware interrupt does NOT authorize that interrupt to call this API.
 * Fake-Enable interleaving qualifies state ordering only. No post-restore
 * snapshot rollback erases delivery writes. A foreign Task/Exec identity seen
 * before admission or an operation REFUSES unchanged: it cannot poison another
 * owner's gate or release that owner's pair. A nesting fault, or an identity
 * anomaly across THIS call's Disable/Enable, latches FAULT and retains anchors;
 * no compensating Enable, restore retry, reset, disposal or release is authorized.
 * A matching successful leave means only that this owned Exec nesting pair
 * completed and returned.
 */
size_t pt_native_delivery_gate_size(void);
size_t pt_native_delivery_gate_alignment(void);
/* Complete aligned storage of at least size(); only the owned size() prefix
 * must be ZERO and is checked. The capacity tail is never read or written, but
 * the full supplied capacity protects later outputs. generation is nonzero.
 * Constructor refusal writes nothing, calls no Disable/Enable and leaves
 * storage unchanged. */
enum pt_native_delivery_gate_result pt_native_delivery_gate_init(
    struct pt_native_delivery_gate *,size_t capacity,uint64_t generation);
enum pt_native_delivery_gate_result pt_native_delivery_gate_task_enter(
    struct pt_native_delivery_gate *,struct pt_native_delivery_gate_token *);
/* Calls below require the original currently active TASK token. Four distinct
 * nonzero tickets are staged once, with no reuse even after observation. */
enum pt_native_delivery_gate_result pt_native_delivery_gate_stage(
    struct pt_native_delivery_gate *,const struct pt_native_delivery_gate_token *,
    unsigned slot,uint64_t original_ticket);
/* Copy a completely recorded slot ONCE. Later observation refuses without
 * changing output; the retained payload is not erased and the slot never
 * reopens. Observation grants no release authority. */
enum pt_native_delivery_gate_result pt_native_delivery_gate_observe(
    struct pt_native_delivery_gate *,const struct pt_native_delivery_gate_token *,
    unsigned slot,struct pt_native_delivery_gate_record *);
/* Idempotently blocks NEW staging, never pending local delivery/observation.
 * Does not cancel a source, erase a slot or prove any form of quiet. */
enum pt_native_delivery_gate_result pt_native_delivery_gate_stop_admission(
    struct pt_native_delivery_gate *,const struct pt_native_delivery_gate_token *);
enum pt_native_delivery_gate_result pt_native_delivery_gate_task_leave(
    struct pt_native_delivery_gate *,const struct pt_native_delivery_gate_token *);

/* Local ordinary-C delivery only, in the same original normal-task call path.
 * Exactly ONE successful enter is allowed per original staged slot/ticket;
 * success consumes the attempt even if leave occurs without a record. No timer/
 * source caller is authenticated here. No nested delivery or delivery during
 * active TASK. Refused entry consumes no attempt. */
enum pt_native_delivery_gate_result pt_native_delivery_gate_local_enter(
    struct pt_native_delivery_gate *,unsigned slot,uint64_t original_ticket,
    struct pt_native_delivery_gate_token *);
/* At most one complete numeric record per slot. record.ticket must match the
 * staged original. No caller pointer is retained and no callback is invoked. */
enum pt_native_delivery_gate_result pt_native_delivery_gate_local_record(
    struct pt_native_delivery_gate *,const struct pt_native_delivery_gate_token *,
    const struct pt_native_delivery_gate_record *);
enum pt_native_delivery_gate_result pt_native_delivery_gate_local_leave(
    struct pt_native_delivery_gate *,const struct pt_native_delivery_gate_token *);

/* Deliberately no reset, dispose, source release or native backend adapter API.
 * The caller retains the gate/code/context until all actual external lifetimes
 * end independently. This gate neither owns nor retires the three source pins,
 * cache leases, command/reader domains or a timer/interrupt source. */
#endif
