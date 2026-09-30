#ifndef PT_MIXED_OWNER_H
#define PT_MIXED_OWNER_H
#include "mixed_preflight.h"
#include "paula_voices.h"
struct pt_mixed_owner;
enum pt_mixed_owner_result {PT_MIXED_OWNER_OK,PT_MIXED_OWNER_PREPARING,
    PT_MIXED_OWNER_INVALID,PT_MIXED_OWNER_MEMORY,PT_MIXED_OWNER_CAPABILITY,PT_MIXED_OWNER_STALE,PT_MIXED_OWNER_DEVICE};
/* Combined master owner; live sequence scheduling remains unfinished. Atomically claim both
 * idle bound engines of the SAME sampler/project. Copies options/capabilities;
 * no source pins, cache allocation or output at begin. Ownership predicates
 * may run during guards. Public direct operations
 * refuse while owned. Arrays/PCM and engine/API/context identities immutable;
 * only channels.selected may change. Serialize, no callback edits/reentry.
 * Project, sampler, bridges, engines, reservation and allocators outlive close.
 * Stop/close before edit/undo/import/disposal; in-place writes are forbidden,
 * not detected. Failure preserves *out. No auto device/library reservation. */
enum pt_mixed_owner_result pt_mixed_owner_begin(struct pt_paula_voices *,struct pt_wavetable_voices *,
    const struct pt_render_options *,const struct pt_paula_render_caps *,const struct pt_playback_format *,
    const struct pt_allocator *,struct pt_mixed_owner **);
/* First call fully gates and retains ONE rewound sequence. Subsequent calls
 * reserve one master or copy <=4096 bytes. Pins union of both used-source masks
 * exactly once per slot; unused slots remain unpromoted. No derived copies or
 * output callbacks; live backend ownership predicates remain checked. Setup/traversal/allocation synchronous, outside playback deadline.
 * Failure poisons handle; close still required. Completed unchanged promotions
 * may remain sampler-owned after cancel. Ready is NOT capacity/output evidence. */
enum pt_mixed_owner_result pt_mixed_owner_prepare(struct pt_mixed_owner *,struct pt_mixed_report *);
/* Detect generation/header/API/map/backend changes without bulk PCM scans. */
enum pt_mixed_owner_result pt_mixed_owner_current(struct pt_mixed_owner *);
/* Cancel partial promotion and sequence, block both engines, attempt each reader
 * stop and barrier once. Retain ALL master pins and both ownership tokens until
 * BOTH drains confirm, including interrupt quiescence. Retry pending/error;
 * never force release. Failure leaves handle. Success frees handle, leaves both
 * engines bound but closing: caller closes/rebinds them separately. API/context
 * identities must remain/restored to captured values for safe cleanup. No wait,
 * cache detach, library close, native DMA or hardware acceptance here. */
int pt_mixed_owner_close(struct pt_mixed_owner **);
#endif
