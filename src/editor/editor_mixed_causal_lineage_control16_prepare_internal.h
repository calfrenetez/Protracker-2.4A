#ifndef PT_EDITOR_MIXED_CAUSAL_LINEAGE_CONTROL16_PREPARE_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_LINEAGE_CONTROL16_PREPARE_INTERNAL_H
#include "editor_mixed_causal_lineage_prepare_internal.h"
#include "sampler_mixed_causal_lineage_control16_internal.h"
/* PRIVATE additive uint16 final AmiGUS register levels. The original lineage
 * wrapper/layout/binding remain unchanged; genuine original R references derive
 * lower handles, master/cache/route/slot/keys. No source/key/READY certificate.
 * Complete original wide caller input/output/local transformed aggregates are
 * guarded through actual predecessor/getter/allocation/callback operations.
 * One later C, zero new R/pins/cache/conversion/upload and consumed lifetime
 * stages retain exact original windows/positive transfer/independent quiet.
 * Old uint8 entry and STOP/default paths are unchanged. No new public mode.
 * SOURCE prototype only; no host/native/physical acceptance is asserted. */
struct pt_editor_mixed_causal_lineage_control16_action {
    struct pt_editor_mixed_reader_ref reader;
    uint16_t period;uint8_t volume;
    uint32_t rate;uint16_t left,right;
};
struct pt_editor_mixed_causal_lineage_control16_batch {
    uint64_t frame;unsigned count;
    struct pt_editor_mixed_causal_lineage_control16_action action[PT_SAMPLER_MIXED_ACTIONS];
};
enum pt_editor_mixed_readers_result pt_editor_mixed_causal_lineage_control16_prepare_batch_begin(
    struct pt_editor_mixed_causal_lineage_prepare *,const struct pt_editor_mixed_causal_lineage_control16_batch *,
    struct pt_editor_mixed_command_ref *);
#endif
