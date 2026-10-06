#ifndef PT_RENDER_H
#define PT_RENDER_H
#include "timeline.h"
#include "voice.h"
enum pt_render_result { PT_RENDER_OK, PT_RENDER_INVALID, PT_RENDER_ROUTE,
                        PT_RENDER_EFFECT, PT_RENDER_SAMPLE, PT_RENDER_TICK_LIMIT,
                        PT_RENDER_FRAME_LIMIT, PT_RENDER_CANCELLED, PT_RENDER_SINK, PT_RENDER_EMPTY_RANGE, PT_RENDER_MEMORY };
enum pt_render_end { PT_RENDER_F00, PT_RENDER_POSITION_RETURN, PT_RENDER_ROW_EXIT };
enum pt_render_phase { PT_RENDER_ANALYSE, PT_RENDER_MIX, PT_RENDER_VERIFY };
struct pt_render_options {
    uint64_t frame_limit;
    uint32_t tick_limit,rate,gain_q16;
    uint16_t tracks,start_order,pattern;
    uint8_t bits,pattern_only,include_lead_in,row_range,row_first,row_end;
};
struct pt_render_report {uint64_t frames,clipped;uint32_t ticks;enum pt_render_end end;};
typedef int (*pt_render_progress)(void *,enum pt_render_phase,uint32_t ticks,uint64_t frames);
typedef int (*pt_render_sink)(void *,const struct pt_pcm *,uint64_t offset);
/* Deterministic offline reference: IDEAL_BPM_Q32 and PCM sample rate at period
 * 428 (C-2), step ratio 428/period. This is not a CIA/Paula analogue model.
 * Explicit selected tracks, gain, output rate 44100/48000, bits16/24 and budgets.
 * End at F00 or the first backwards/same-order position transition, after the
 * outgoing row duration. E6 row loops are retained. Pattern mode uses one order.
 * Default (include_lead_in=0) omits the silent initial speed-count lead-in.
 * Optional row_range requires pattern_only, no lead-in, and
 * 0 <= row_first < row_end <= 64 (half-open). Pre-roll from row0 advances
 * voices/effects silently; capture begins at first fetched row in range and
 * stops before the first subsequent fresh row outside it. Retained/delayed
 * rows and loops wholly within range remain. Unreached range is refused.
 * Tick/frame budgets include pre-roll; report frames/clips count only output.
 * Project, source PCM and options remain immutable throughout each call,
 * including progress/sink callbacks. Full initial validation permits private
 * voice setup to reuse validated sample values while retaining bounded checks.
 * Preflight still validates the whole selected pattern.
 * Preflight rejects selected MIDI routes, MIDI pitches,
 * crossfade metadata, unsupported active cross-sample/slice instrument-only handoffs,
 * sample changes/slices on tone-portamento notes,
 * zero playback periods and unsupported
 * effects. Supported
 * effects: 0xy..Dxx, E1x..EEx and Fxx. 8xx restores stored pitch
 * without panning; E8x does nothing, matching the pinned2.3F replay.
 * Instrument-only rows can preload silently or reload the same whole sample
 * without restarting phase. Different active samples may hand off their next
 * repeat when both are mono8 forward loops, even bounded classic ranges,
 * matching rates and no interpolation. Supported handoff effects are 0xx-7xx,
 * 8xx-Fxx except E0x/EFx; sliced handoffs are refused.
 * 9xx retains offset memory and stored range updates across sample changes.
 * E90 preserves phase; nonzero E9x retriggers. Old trigger bounds are retained.
 * 3xx/5xy target notes and actual EDx delayed notes use the same guarded handoff.
 * EDx queues repeats immediately and triggers only on its tick.
 * Either source may also be mono8 non-looping PCM whose first word is
 * already zero. Whole-sample playback retains that silent two-frame repeat
 * so later instrument handoffs work after the initial fetch has ended.
 * Pitch slides retain native stored-word wrap and register-write semantics.
 * Ordinary notes and tone targets follow pinned tables for all16 tunings.
 * 3xx/5xx retain voice phase; explicit velocity may change without retriggering.
 * E5x overrides tuning; E3x quantizes tone-portamento output.
 * Selected slice ranges loop only if the complete loop lies within the slice.
 * Mono pan is linear L/R; stereo pan is balance with unity at centre128.
 * Muting/solo remain global project settings. Voices advance at zero output gain.
 * Immutable project/PCM lifetime and non-aliasing report are caller obligations.
 */
enum pt_render_result pt_render_measure(const struct pt_project *,const struct pt_render_options *,
                                       pt_render_progress,void *,struct pt_render_report *);
/* Runs full preflight and measurement before first sink call. Sink receives
 * temporary blocks of at most 256 frames. Caller stages output and publishes it
 * only after OK; cancellation/sink failure may have delivered a partial stream.
 * Report is replaced only on OK. Progress zero cancels; sink zero refuses.
 * No project writes, allocation, filesystem access or hardware use. */
enum pt_render_result pt_render_stream(const struct pt_project *,const struct pt_render_options *,
                                      pt_render_sink,void *,pt_render_progress,void *,struct pt_render_report *);
/* Allocated variants move timeline/voice/PCM working state off the stack.
 * Exactly one caller allocation per call, released on every return. Allocator
 * must provide ordinary C object alignment and remain valid through callbacks.
 * Failure returns MEMORY without sink calls or changing report/master data.
 * No retained allocation or pointer; callbacks must not free the workspace.
 * Legacy variants above retain their stack-based, allocation-free behavior. */
struct pt_allocator;
enum pt_render_result pt_render_measure_allocated(const struct pt_project *,const struct pt_render_options *,
    pt_render_progress,void *,struct pt_render_report *,const struct pt_allocator *);
enum pt_render_result pt_render_stream_allocated(const struct pt_project *,const struct pt_render_options *,
    pt_render_sink,void *,pt_render_progress,void *,struct pt_render_report *,const struct pt_allocator *);
/* Optional task-side initial setup. The opaque owner contains the ONLY genuine
 * project/flow validation workspace; no caller certificate or validation flag is
 * consumed. Begin scans bounded metadata/full source extents and owns one fixed
 * control allocation, but reads no PCM/order/event/slice values. Borrow p and all
 * source storage plus the original o/a argument structs immutable/alive through
 * cancel/take, including allocator callbacks. Callback contexts must be disjoint
 * from known storage; arbitrary opaque context extents cannot be guessed here.
 * Revision/generation are source tags, not a timestamp backend generation.
 */
enum pt_render_setup_result { PT_RENDER_SETUP_PENDING, PT_RENDER_SETUP_READY,
    PT_RENDER_SETUP_INVALID, PT_RENDER_SETUP_STALE, PT_RENDER_SETUP_ALIAS,
    PT_RENDER_SETUP_CAPACITY, PT_RENDER_SETUP_BUSY, PT_RENDER_SETUP_FAILED };
enum pt_render_setup_phase { PT_RENDER_SETUP_VALIDATE, PT_RENDER_SETUP_USED,
    PT_RENDER_SETUP_OFFSETS, PT_RENDER_SETUP_EFFECTS, PT_RENDER_SETUP_COMPLETE };
struct pt_render_setup_report {
    enum pt_render_result render_result;
    enum pt_render_setup_phase phase;
    unsigned last_work;
};
#define PT_RENDER_SETUP_GUARDS 8U
struct pt_render_setup_guard {const void *data;size_t bytes;};
struct pt_render_sequence_setup;
/* Descriptive actual opaque control extents only. No validity/progress proof. */
size_t pt_render_sequence_setup_control_size(void);
size_t pt_render_sequence_setup_control_alignment(void);
size_t pt_render_sequence_control_size(void);
size_t pt_render_sequence_control_alignment(void);
struct pt_render_sequence;
enum pt_render_setup_result pt_render_sequence_setup_begin(const struct pt_project *,
    const struct pt_render_options *, const struct pt_allocator *, uint32_t revision,
    uint32_t generation, const struct pt_render_setup_guard *, unsigned guard_count,
    struct pt_render_sequence_setup **);
/* One phase/call, <=work charged validation or metadata items (1..4096).
 * Fixed header/currentness comparisons and finite metadata output-span checks
 * are outside charged items; item counts are not a wall-clock/IRQ guarantee.
 * READY means only initial setup; the returned sequence still needs its complete
 * bounded timeline measurement before next(). Invalid work/stale/alias refusal
 * never advances. Result/report is not authorization to construct another owner.
 */
enum pt_render_setup_result pt_render_sequence_setup_step(struct pt_render_sequence_setup *,
    uint32_t revision, uint32_t generation, unsigned work);
/* NULL report is a checked current/status query. No partial PCM/source mask.
 * Non-NULL report must be disjoint; stale/alias output remains untouched. */
enum pt_render_setup_result pt_render_sequence_setup_get(struct pt_render_sequence_setup *,
    uint32_t revision, uint32_t generation, struct pt_render_setup_report *);
/* Consumes genuine completed startup; allocates the actual opaque sequence,
 * initially awaiting measurement. No semantic rescan. CAPACITY retains completed
 * startup for explicit retry; success releases startup, never retains its address.
 * Output/handle slots are distinct/disjoint from startup, original options and
 * allocator, and project/source storage; the guard descriptor array is immutable
 * and also disjoint. Reserved parent spans may contain legitimate parent-owned
 * slots: those spans protect new allocation initialization, not slot publication.
 * Parent wrappers separately guard their actual external caller output/handle.
 * Ordinary pending/alias/capacity refusal preserves both. A release callback
 * failure after consuming startup clears that handle, publishes no sequence and
 * releases the private candidate; explicit cancel/restart is required. Callback-
 * mutated slots are not overwritten; only an unchanged genuine owner is cleared.
 */
/* Optional parent guards protect actual full caller controls/input extents before
 * allocation initialization. Their descriptor array is also protected; count<=8.
 * Array entries may name parent private publisher slots; the descriptor array
 * itself must not overlap either publishing slot. No guard/report bypasses genuine
 * completed-current progress. */
enum pt_render_setup_result pt_render_sequence_setup_take(struct pt_render_sequence_setup **,
    uint32_t revision, uint32_t generation, const struct pt_render_setup_guard *,
    unsigned guard_count, struct pt_render_sequence **);
/* Read-only checked span query for a parent control allocation. This is never a
 * semantic certificate: constructor/take independently checks actual progress. */
int pt_render_sequence_setup_output_disjoint(struct pt_render_sequence_setup *,
    uint32_t revision, uint32_t generation, const void *, size_t);
/* Fixed original control/header/table/input extents only, usable after STALE
 * without reading former descriptors. This classifies known ambiguous allocator
 * aliases before cleanup, not validation/currentness or semantic permission. */
int pt_render_sequence_setup_control_output_disjoint(const struct pt_render_sequence_setup *,
    const void *, size_t);
/* Genuine owner handle only. Cancel reads no former source arrays; caller handle
 * storage must outlive the owner and be disjoint from borrowed source storage.
 * Allocators must return fresh disjoint storage. A returned known live/source
 * alias is refused WITHOUT release: ownership of overlapping memory was never
 * acquired. Concurrent source destruction or a freeing invalid-allocator callback
 * cannot be made safe by span checks and violates the borrowing contract.
 * Begin callbacks must remain serialized (no owner exists yet); same-slot handle
 * publication or observable input changes are refused, but recursive begin-and-
 * cancel with no observable change cannot be detected. Subsequent take/release
 * callbacks keep the existing owner BUSY; observed reentry latches failure.
 */
enum pt_render_setup_result pt_render_sequence_setup_cancel(struct pt_render_sequence_setup **);
/* Parent owner close guard: fixed control/captured-table spans when header is
 * stale, current full capacities otherwise. No former descriptor reads. Genuine
 * external handle storage must separately remain disjoint from borrowed storage. */
int pt_render_sequence_control_output_disjoint(const struct pt_render_sequence *,const void *,size_t);
#endif
