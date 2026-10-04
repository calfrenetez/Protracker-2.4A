#ifndef PT_SAMPLE_USAGE_H
#define PT_SAMPLE_USAGE_H
#include "project.h"
#define PT_SAMPLE_USAGE_CHUNK 256
/* This is stored-reference information, never an execution/note-count promise. */
enum pt_sample_usage_result {PT_USAGE_OK,PT_USAGE_INVALID,PT_USAGE_ALIAS,PT_USAGE_STALE};
enum pt_sample_usage_flag {
    PT_USAGE_REFERENCED=1,PT_USAGE_RESERVED=2,PT_USAGE_PROTECTED=4,
    PT_USAGE_UNKNOWN=8,PT_USAGE_CONFIGURED=16,PT_USAGE_EMPTY=32,
    PT_USAGE_ELIGIBLE=64,PT_USAGE_FREE=128
};
struct pt_sample_usage_options {
    uint32_t revision;unsigned generation;
    uint8_t protected_slots[PT_PROJECT_SAMPLES],reserved_slots[PT_PROJECT_SAMPLES];
};
struct pt_sample_usage_row {
    struct pt_sample identity;
    uint32_t references;uint16_t patterns,flags;
    uint64_t active_bytes,resident_bytes;
};
struct pt_sample_usage_preview {
    const struct pt_project *project;struct pt_project snapshot;
    uint32_t revision;unsigned generation;
    uint16_t count,free_count,eligible_count,protected_count;
    struct pt_sample_usage_row rows[PT_PROJECT_SAMPLES];
    uint8_t selected[PT_PROJECT_SAMPLES]; /* Initially all zero; explicit UI choice. */
};
/* Caller heap/workspace; no allocation. Source is a validated immutable project,
 * serialized with its editor. Begin checks metadata, never PCM values. Step
 * visits <=256 stored events. Every pattern/track counts regardless of orders,
 * mute, pitch or route. Nonzero instrument-only fields count. The current model
 * has no independent instrument maps/defaults. Unknown optional extensions and
 * caller-reported unresolved persistent dependencies protect all named slots.
 * Protected/reserved masks are copied. Published tags/rows/classification flags
 * are immutable trusted scan results; only selected[] is an explicit UI choice.
 * All outputs remain unchanged on refusal;
 * preview is published only when ready=1. Caller updates revision/generation for
 * ALL mutations (including external edits); source arrays/PCM stay alive.
 * Scan/job/output spans must be disjoint from borrowed project/owner storage.
 */
struct pt_sample_usage_scan {
    struct pt_sample_usage_preview value;
    uint32_t next_event,total_events;
    uint16_t last_pattern[PT_PROJECT_SAMPLES];
    unsigned ready;enum pt_sample_usage_result failure;
};
enum pt_sample_usage_result pt_sample_usage_begin(struct pt_sample_usage_scan *,
    const struct pt_project *,const struct pt_sample_usage_options *);
enum pt_sample_usage_result pt_sample_usage_step(struct pt_sample_usage_scan *,
    const struct pt_project *,uint32_t revision,unsigned generation,
    struct pt_sample_usage_preview *,unsigned *ready);
int pt_sample_usage_current(const struct pt_sample_usage_preview *,const struct pt_project *,uint32_t,unsigned);
/* Shared strict free-slot predicate: canonical unnamed, unconfigured, no storage,
 * no stored/unknown/protected/reserved dependency. No sample renumbering. */
int pt_sample_usage_free(const struct pt_sample_usage_preview *,unsigned slot);
/* Metadata-only full-capacity/markers/project-table output guard. */
int pt_sample_usage_output_disjoint(const struct pt_project *,const void *,size_t);
#endif
