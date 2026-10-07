#ifndef PT_MIXED_READERS_CAUSAL_QUEUE_INTERNAL_H
#define PT_MIXED_READERS_CAUSAL_QUEUE_INTERNAL_H
#include "mixed_scheduled_readers.h"

/* Private task-only validation of the actual genuine first command and its
 * retained domains. Values originate in the causal owner's accepted packet,
 * never in a caller prediction. This invokes genuine holder-current checks;
 * OK is neither an ACTIVE key, adoption receipt nor lasting authorization.
 * The copied event/reference/binding addresses are compared only with still
 * retained original queue entries. Do not call after first command disposal,
 * from fire/IRQ, or with a second queue constructed around a borrowed backend.
 */
struct pt_mixed_causal_queue_basis {
    const void *backend_owner;
    const struct pt_mixed_readers_event *event;
    struct pt_mixed_readers_binding command_binding;
    uint64_t session,generation,ticket,command_owner,frame,first,last;
    unsigned count,expected_mask;
    struct pt_mixed_readers_key expected[20],key[16];
    struct pt_mixed_readers_action action[16];
    const struct pt_mixed_readers_domain *reference[16];
    struct pt_mixed_readers_binding binding[16];
};
/* successor=0 checks the first accepted publication before genuine successor
 * admission. Nonzero additionally checks the actual same-queue unpublished
 * second pure-TRIGGER command, all its real domains and exact frame ordering.
 * No command/reference transfer, prediction publication or state promotion.
 */
enum pt_mixed_readers_result pt_mixed_readers_causal_validate(
    struct pt_mixed_readers_output *,const struct pt_mixed_causal_queue_basis *,
    uint64_t successor);
#endif
