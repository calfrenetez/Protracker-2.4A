#ifndef PT_WAVE_SUMMARY_H
#define PT_WAVE_SUMMARY_H
#include "sample_range.h"
#define PT_WAVE_SUMMARY_COLUMNS 620
#define PT_WAVE_SUMMARY_VALUES_PER_STEP 4096
#define PT_WAVE_SUMMARY_BINS (PT_WAVE_SUMMARY_COLUMNS * 2)
struct pt_wave_summary_bin { int32_t minimum,maximum; };
/* Caller owns bins (normally editor-owned Fast memory). Column-major,
 * bins[column*channels+channel], actual signed8/16/24 values. Drawing may include
 * zero in its vertical extent. This descriptor is published only after take.
 * It owns no source/cache pins; current() must precede drawing. */
struct pt_wave_summary {
    const struct pt_project *project;
    const struct pt_sample *table,*sample;
    const uint16_t *orders;
    const struct pt_event *events;
    const struct pt_extension *extensions;
    struct pt_pcm pcm;
    const struct pt_wave_summary_bin *bins;
    size_t capacity;
    uint64_t generation;
    struct pt_sample_range view;
    uint16_t sample_count,slot,columns,order_count,pattern_count,extension_count;
    uint8_t tracks;
    uint8_t channels;
};
/* Unique, noncopyable, owner-thread job. Staging bins must be separate from any
 * displayed summary and source/control storage; never draw partial bins. */
struct pt_wave_summary_job {
    struct pt_wave_summary summary;
    struct pt_wave_summary_bin *bins;
    uint32_t frame,last;
    int32_t minimum,maximum;
    unsigned column,channel,have_value,state,last_values;
};
enum pt_wave_summary_result { PT_WAVE_SUMMARY_OK, PT_WAVE_SUMMARY_PENDING,
    PT_WAVE_SUMMARY_INVALID, PT_WAVE_SUMMARY_CAPACITY, PT_WAVE_SUMMARY_ALIAS,
    PT_WAVE_SUMMARY_STALE };
/* Metadata-only begin, caller-owned workspace; no allocator/cache/backend calls.
 * Nonempty view in [0,frames). Generation is caller's sample/document generation.
 * Project is an identity/table adapter, not a full validated playback project:
 * a persistent source-pane adapter may have one sample and zero track/event/order
 * counts. Nonzero tables respect project limits (extensions<=4090). Caller
 * supplies validated PCM values; only descriptor/spans are checked here.
 * Source remains immutable/alive until take/cancel; metadata changes must advance
 * generation before the old table can be freed. No full PCM validation scan. */
enum pt_wave_summary_result pt_wave_summary_begin(struct pt_wave_summary_job *,
    const struct pt_project *,unsigned slot,uint64_t generation,
    const struct pt_sample_range *view,struct pt_wave_summary_bin *,size_t capacity);
/* <=4096 actual PCM-value reads per call, including repeated bins for short
 * views. last_values records that count. Valid pending call publishes ready0;
 * completion ready1. Refused ready aliases preserve job/source/output. Stale
 * calls preserve ready and latch failed without scanning changed tables. */
enum pt_wave_summary_result pt_wave_summary_step(struct pt_wave_summary_job *,
    uint64_t generation,unsigned *ready);
/* Rechecks current source, publishes one complete descriptor then closes job.
 * Failure leaves descriptor unchanged. Partial staging is discarded on cancel;
 * caller owns/reuses that memory. Cancel never dereferences borrowed source. */
enum pt_wave_summary_result pt_wave_summary_take(struct pt_wave_summary_job *,
    uint64_t generation,struct pt_wave_summary *);
void pt_wave_summary_cancel(struct pt_wave_summary_job *);
/* Metadata-only tag check. Selection/status-only changes can reuse bins. A new
 * viewport requires a new tagged build; no approximate/coarse summary is implied.
 * Check before drawing while caller guarantees source and bins lifetime. */
int pt_wave_summary_current(const struct pt_wave_summary *,const struct pt_project *,
    uint64_t generation);
#endif
