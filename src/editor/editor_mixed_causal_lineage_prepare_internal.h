#ifndef PT_EDITOR_MIXED_CAUSAL_LINEAGE_PREPARE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_LINEAGE_PREPARE_INTERNAL_H
#include "editor_mixed_causal_prepare.h"
#include "sampler_mixed_causal_lineage_internal.h"
#include "../core/mixed_readers_causal_lineage_internal.h"
/* PRIVATE opt-in finite TRIGGER -> CONTROL -> STOP. Entire zero/noncopyable
 * wrapper and complete immutable inputs survive until positive terminal close;
 * original ordinary contexts must cover the WHOLE wrapper. Install the distinct
 * composite port before original registration and factory acquisition. Default
 * two-TRIGGER, STOP-only and SOURCE/producer modes keep their original paths.
 * Typed later commands name only genuine original controller R references.
 * Derive lower handles, route/slot/keys internally; one C, zero new R/cache/
 * persistent pin/lease/conversion/upload. Whole input/output/transformed local
 * stay guarded across actual getters, allocator and port callbacks.
 *
 * Actual allocation consumes each lifetime stage before the external callback.
 * A consumed NULL/alias/fault cannot reconstruct it. Two live C/32 R remain;
 * C3 needs actual completed CONTROL and independent genuine C1 service(cancel0)
 * plus actual original-handle NULL consumption. A reused ledger slot gets a
 * fresh serial; its old numeric C1 reference cannot identify C3. No fourth.
 * Reuse original advance/enqueue/publish/service/cancel/close; lower enqueue OK
 * remains transfer authority even amid outer faults. Later C records have no
 * new reader-reference children. Exact original absolute frames/windows never
 * rebase; independent C/R/SOURCE quiet governs resources and context release.
 * No native aperture/timer/IRQ/WCET/device stop/audio/listening qualification.
 */
struct pt_editor_mixed_causal_lineage_inputs {
    struct pt_editor_mixed_causal_prepare_inputs original;
    struct pt_mixed_causal_control_stop_port lineage;
};
struct pt_editor_mixed_causal_lineage_prepare {
    struct pt_editor_mixed_causal_prepare original;
    struct pt_editor_mixed_causal_lineage_prepare *self;
    const struct pt_editor_mixed_causal_lineage_inputs *inputs;
    struct pt_editor_mixed_causal_lineage_inputs saved;
    struct pt_sampler_mixed_causal_lineage_binding factory;
    struct pt_editor_mixed_command_ref root,control;
    uint64_t root_ticket,control_ticket,control_frame;
    unsigned stage,constructing,root_detached,root_closed;
};
struct pt_editor_mixed_causal_lineage_control_action {
    struct pt_editor_mixed_reader_ref reader;
    uint16_t period;uint8_t volume;
    uint32_t rate;uint8_t left,right;
};
struct pt_editor_mixed_causal_lineage_control_batch {
    uint64_t frame;unsigned count;
    struct pt_editor_mixed_causal_lineage_control_action action[PT_SAMPLER_MIXED_ACTIONS];
};
struct pt_editor_mixed_causal_lineage_stop_batch {
    uint64_t frame;unsigned count;
    struct pt_editor_mixed_reader_ref reader[PT_SAMPLER_MIXED_ACTIONS];
};
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_lineage_prepare_begin(
    struct pt_editor_mixed_causal_lineage_prepare *,const struct pt_editor_mixed_causal_lineage_inputs *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_lineage_control_prepare_batch_begin(
    struct pt_editor_mixed_causal_lineage_prepare *,const struct pt_editor_mixed_causal_lineage_control_batch *,
    struct pt_editor_mixed_command_ref *);
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_lineage_stop_prepare_batch_begin(
    struct pt_editor_mixed_causal_lineage_prepare *,const struct pt_editor_mixed_causal_lineage_stop_batch *,
    struct pt_editor_mixed_command_ref *);
#endif
