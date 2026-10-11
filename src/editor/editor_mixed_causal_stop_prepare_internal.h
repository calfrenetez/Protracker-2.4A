#ifndef PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_STOP_PREPARE_INTERNAL_H
#include "editor_mixed_causal_prepare.h"
#include "sampler_mixed_causal_stop_internal.h"
#include "../core/mixed_readers_causal_stop_internal.h"
/* PRIVATE finite after-first TRIGGER -> STOP alternative controller.
 * Whole noncopyable zero wrapper, original complete immutable input and ordinary
 * contexts survive until positive terminal close. Contexts cover the WHOLE
 * wrapper, including its genuine factory binding; public fields confer no STOP
 * capability. The genuinely opened empty owner installs a distinct once-only
 * STOP port before original registration/factory acquisition. Source promotion,
 * producer/controller orchestration and default two-TRIGGER semantics stay
 * separate. Task exclusion may call the original owned/exclusion callback.
 *
 * Typed STOP targets only original controller-issued R references. It derives
 * genuine handles internally; the factory/core recheck actual completed-first
 * all20 registry and genuinely ACTIVE selected queue readers. One new C holder,
 * zero R/master/PCM/cache/Chip/persistent-pin acquisition or upload. The STOP
 * command's controller count is zero: old R records/tickets remain unchanged.
 * Actual enqueue OK alone transfers the new C even after an outer fault.
 *
 * Literal requested absolute frame/grid never rebase. Early/original windows,
 * fail-closed unknown/partial/late/mutation/exclusion outcomes and independent
 * C/R/source quiet retention apply. Closing admission or validated STOP commit
 * is not hardware stop/reader/cache quiet. Local cancel remains shutdown only.
 * No third lineage, CONTROL-to-STOP, native prepublication aperture, live exact
 * schedule, IRQ/timer/device/audio/listening or whole application acceptance.
 */
struct pt_editor_mixed_causal_stop_inputs {
    struct pt_editor_mixed_causal_prepare_inputs original;
    struct pt_mixed_causal_stop_port stop;
};
struct pt_editor_mixed_causal_stop_prepare {
    struct pt_editor_mixed_causal_prepare original;
    struct pt_editor_mixed_causal_stop_prepare *self;
    const struct pt_editor_mixed_causal_stop_inputs *inputs;
    struct pt_editor_mixed_causal_stop_inputs saved;
    struct pt_sampler_mixed_causal_stop_binding factory;
};
struct pt_editor_mixed_causal_stop_batch {
    uint64_t frame;
    unsigned count;
    struct pt_editor_mixed_reader_ref reader[PT_SAMPLER_MIXED_ACTIONS];
};
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_stop_prepare_begin(
    struct pt_editor_mixed_causal_stop_prepare *,const struct pt_editor_mixed_causal_stop_inputs *);
/* Guard complete original request/output/local scratch BEFORE writes, reentry,
 * getters or allocation. Unique genuine original references, zero unused tail.
 * No supplied READY/key/sample/geometry and no new reader_reference result.
 * Advance/enqueue/publish/service/cancel/close use the original controller API.
 */
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_stop_prepare_batch_begin(
    struct pt_editor_mixed_causal_stop_prepare *,const struct pt_editor_mixed_causal_stop_batch *,
    struct pt_editor_mixed_command_ref *);
#endif
