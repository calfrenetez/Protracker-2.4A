#ifndef PT_READERS_ACTIVATION_H
#define PT_READERS_ACTIVATION_H
#include "scheduled_readers.h"
#define PT_READERS_ACTIVATION_COMMANDS 2U
#define PT_READERS_ACTIVATION_READERS 8U
#define PT_READERS_ACTIVATION_PORT_VERSION 1U
#define PT_READERS_ACTIVATION_PORT_REQUIRED 7U
struct pt_readers_activation;
/* Complete copied activation input: no descriptor, span, holder or callback.
 * Queue addresses in keys are opaque identities, never dereferenced. All four
 * original expected slots are compared before any effect. Inactive slots have
 * a zero expected key. Data points only to independently retained signed8 RAM.
 */
struct pt_readers_activation_packet {
    uint64_t session,generation,ticket,frame,first,last;
    unsigned count,expected_mask;
    struct pt_readers_key expected[PT_READERS_ACTIONS],key[PT_READERS_ACTIONS];
    struct pt_scheduled_action action[PT_READERS_ACTIONS];
};
/* adopted_mask names positive adoption of the CURRENT ACTIVE slot sources,
 * so it equals active_mask on complete success. It does not summarize draining
 * or historical reader references; those need independent reader_quiet proof. */
struct pt_readers_activation_actual {
    unsigned active_mask,adopted_mask;
    struct pt_readers_key slot[PT_READERS_ACTIONS];
};
/* Explicitly injected, separately qualified port. These flags are declarations
 * only, never qualification. No existing immediate audio.device/WAITECLOCK port
 * satisfies this interface. All callbacks/context live through ledger close.
 *
 * read_clock is used in task work AND both actual activation pre/post reads.
 * Each read must be bounded and report the honest actual counter/frequency,
 * with no allocation/conversion/owner/core/editor traversal or reentry. Its
 * activation serialization, context lifetime and residency/timing must be
 * separately qualified alongside commit. Flags alone qualify neither.
 *
 * Caller/port serialize EVERY ledger entry and actual activation under their
 * own task/activation exclusion. No concurrent calls, input edits or callback
 * reentry. A busy refusal latches failure; it is never a spin lock or retry.
 * Do not infer IRQ/residency/WCET/aggregate-stack safety from finite loops.
 *
 * publish: task-only, independently arranges activation of this exact ticket
 * inside its original window. Atomically copy/prepare/arm all values or return0
 * with NO references/effects; -1 is uncertain and retains possible domains.
 * It may retain the ledger/ticket callback identity until command_quiet, but
 * must COPY values, never retain the packet address or original owner storage.
 * Check actual current keys, generation, frequency and clock before acceptance.
 * No early start, task-wakeup activation, catch-up or deadline rebasing.
 *
 * commit: actual activation only, one bounded whole-batch operation. Independently
 * repeat all actual expected keys/generation before the first effect. Return1
 * ONLY for actual persistent reference adoption and successful physical/model
 * activation/control/stop; report the actual resulting slot keys/masks. This
 * is not submission/BeginIO/command-completion acceptance. Return0 proves no
 * effects; -1/partial effects remain unknown and retain possible references.
 * No allocation/conversion/current/terminal/release/editor/owner traversal.
 * Never retain packet/actual output addresses. The ledger samples actual clock
 * before and after commit; late completion fails without erasing any effects.
 * A native port must independently prove worst-case whole-batch window fit.
 *
 * quiet callbacks: task-only, bounded one probe/cancel operation. 1 independently
 * proves no future port activation/callback reference to the named command OR
 * reader/sample capacities. 0 pending, -1 uncertain. cancel_reader must cancel
 * every pending WHOLE batch targeting exactly that key, without touching a
 * replacement. Already copied receipt identities do not permit dereferencing
 * or reviving retired storage. No owner callback is ever invoked by this port.
 */
struct pt_readers_activation_port {
    void *context;size_t context_bytes;unsigned version,flags;
    int (*read_clock)(void *,uint64_t *,uint32_t *);
    int (*publish)(void *,struct pt_readers_activation *,const struct pt_readers_activation_packet *);
    int (*commit)(void *,const struct pt_readers_activation_packet *,struct pt_readers_activation_actual *);
    int (*command_quiet)(void *,uint64_t,unsigned cancel);
    int (*reader_quiet)(void *,const struct pt_readers_key *,unsigned cancel);
};
enum pt_readers_activation_result {PT_READERS_ACTIVATION_INVALID=-2,
    PT_READERS_ACTIVATION_FAILED=-1,PT_READERS_ACTIVATION_EARLY=0,
    PT_READERS_ACTIVATION_COMMITTED=1};
/* One fixed ordinary allocation; exactly2/8/4. No source values or owners are
 * acquired here. Config/allocator originals live through this call only; copied
 * callbacks/contexts survive close. Opaque allocator contexts stay caller-disjoint.
 * Returned known aliases are refused without treating them as fresh ownership.
 * Metadata constructor callback reentry is serialized by caller before a handle
 * exists. Session is fresh/nonzero, never reused after queue-address recycling.
 * One ledger serves ONE publishing queue: first accepted/uncertain publication
 * permanently binds its queue identity. Other queues may hold copied APIs but
 * cannot publish through this ledger; all API holders still close before it.
 */
enum pt_scheduled_result pt_readers_activation_open(const struct pt_allocator *,
    const struct pt_scheduled_grid *,uint64_t session,
    const struct pt_readers_activation_port *,struct pt_readers_activation **);
enum pt_scheduled_result pt_readers_activation_api(struct pt_readers_activation *,
    struct pt_readers_backend *);
/* Qualified autonomous port calls this exact ticket independently of task wakeup.
 * It never calls a core queue or traverses event/domain/spans/holder objects.
 * Early preserves the original event for its actual scheduled invocation; late,
 * uncertainty, changed frequency/keys or reentry latches failure without retry.
 */
enum pt_readers_activation_result pt_readers_activation_fire(struct pt_readers_activation *,uint64_t);
/* Before close, caller must close EVERY core queue holding a copied backend API
 * and terminate all future autonomous port callback sources. Empty ledger counts
 * alone cannot enforce this order: ABI2 has no queue-close notification. Contexts
 * survive that complete shutdown. No implicit cancellation, musical STOP, reader
 * retirement or forced disposal.
 * Typed handle slot must contain this live owner and be disjoint from all known
 * source/control extents. Close refuses unchanged while either domain remains.
 * A successful final release consumes *owner. Observable release reentry reports
 * failure after consumption, never retries or reads the freed ledger.
 */
int pt_readers_activation_close(struct pt_readers_activation **owner);
#endif
