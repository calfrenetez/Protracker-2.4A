#ifndef PT_NATIVE_RECOVERY_PREFERENCES_H
#define PT_NATIVE_RECOVERY_PREFERENCES_H
#include "recovery.h"

/* Task-context UI seam only. Serialize every call with document transitions and
 * recovery poll/bind/finish. The idle callback is read-only and returns exactly1
 * only while playback, capture, export and other transactions are idle. It does
 * not stop them. No requester, key dispatch, modal loop or ENV writer is here. */
typedef int (*pt_native_recovery_preferences_idle)(void *);

enum pt_native_recovery_source_kind {
    PT_NATIVE_RECOVERY_SOURCE_UNKNOWN,
    PT_NATIVE_RECOVERY_SOURCE_UNTITLED,
    PT_NATIVE_RECOVERY_SOURCE_NAMED
};
struct pt_native_recovery_source {
    char name[PT_RECOVERY_SOURCE_MAX+1];
    uint8_t kind;
};
enum pt_native_recovery_source_transition {
    PT_NATIVE_RECOVERY_SOURCE_LOAD,
    PT_NATIVE_RECOVERY_SOURCE_SAVE,
    PT_NATIVE_RECOVERY_SOURCE_SAVE_AS,
    PT_NATIVE_RECOVERY_SOURCE_NEW,
    PT_NATIVE_RECOVERY_SOURCE_RESTORE
};
/* Zero-initialize the tracker to UNKNOWN. Call only with the actual result and
 * committed source of a document operation. Successful LOAD/SAVE/SAVE_AS require
 * bounded nonempty text; NEW requires NULL/0 and records explicit Untitled.
 * RESTORE preserves the known original identity, never the recovery-file path.
 * A failed operation, unknown transition, alias, control character or overflow
 * leaves every tracker byte unchanged. capacity is the readable input extent;
 * at most PT_RECOVERY_SOURCE_MAX+1 bytes are inspected. No path lookup occurs. */
int pt_native_recovery_source_commit(struct pt_native_recovery_source *,
    enum pt_native_recovery_source_transition,int succeeded,const char *,size_t capacity);
/* Copy a known identity into disjoint caller storage. Untitled copies an empty
 * string. Unknown/malformed/undersized/aliased output refuses unchanged. */
int pt_native_recovery_source_get(const struct pt_native_recovery_source *,
    char *,size_t capacity);

struct pt_native_recovery_preferences {
    struct pt_native_recovery_configuration draft;
    uint8_t open;
};
enum pt_native_recovery_preferences_result {
    PT_NATIVE_RECOVERY_PREFERENCES_REFUSED,
    PT_NATIVE_RECOVERY_PREFERENCES_NOOP,
    PT_NATIVE_RECOVERY_PREFERENCES_APPLIED
};
/* Zero-initialize the draft. Open/get/edit/cancel never mutate the controller.
 * Open refuses an existing draft or non-idle/opened/owned/busy controller.
 * Edit copies the entire typed draft, including invalid policy values so the
 * caller can correct them after a refused Apply. Controller validation remains
 * the sole authority. Inputs/outputs must be disjoint from their owner. */
int pt_native_recovery_preferences_open(struct pt_native_recovery_preferences *,
    const struct pt_native_recovery *,pt_native_recovery_preferences_idle,void *);
int pt_native_recovery_preferences_get(const struct pt_native_recovery_preferences *,
    struct pt_native_recovery_configuration *);
int pt_native_recovery_preferences_edit(struct pt_native_recovery_preferences *,
    const struct pt_native_recovery_configuration *);
int pt_native_recovery_preferences_cancel(struct pt_native_recovery_preferences *);
/* Apply stages the real typed controller operation, then binds ONLY a previously
 * unbound candidate whose configuration actually changed. A known source is
 * required for that initial bind; canonicalization/stat failure discards the
 * candidate. An already-bound owner is never rebound. Semantic no-op never
 * binds, even when the owner is unconfigured/unbound. Final idle is checked
 * before publication. Refusal preserves ALL controller/draft/source bytes;
 * success closes the draft. No snapshot/discard/find/finish/prompt or ENV write.
 * The controller, draft and any supplied tracker must be mutually disjoint. */
enum pt_native_recovery_preferences_result pt_native_recovery_preferences_apply(
    struct pt_native_recovery_preferences *,struct pt_native_recovery *,
    const struct pt_native_recovery_source *,pt_native_recovery_preferences_idle,void *);
#endif
