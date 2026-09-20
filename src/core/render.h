#ifndef PT_RENDER_H
#define PT_RENDER_H
#include "timeline.h"
#include "voice.h"
enum pt_render_result { PT_RENDER_OK, PT_RENDER_INVALID, PT_RENDER_ROUTE,
                        PT_RENDER_EFFECT, PT_RENDER_SAMPLE, PT_RENDER_TICK_LIMIT,
                        PT_RENDER_FRAME_LIMIT, PT_RENDER_CANCELLED, PT_RENDER_SINK };
enum pt_render_end { PT_RENDER_F00, PT_RENDER_POSITION_RETURN };
enum pt_render_phase { PT_RENDER_ANALYSE, PT_RENDER_MIX, PT_RENDER_VERIFY };
struct pt_render_options {
    uint64_t frame_limit;
    uint32_t tick_limit,rate,gain_q16;
    uint16_t tracks,start_order,pattern;
    uint8_t bits,pattern_only,include_lead_in;
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
 * Preflight rejects selected MIDI routes, MIDI pitches, nonzero finetune,
 * crossfade metadata, instrument-only events, sample changes/slices on tone-portamento notes,
 * zero playback periods and unsupported
 * effects. Supported
 * effects: 0xy, 1xx, 2xx, 3xx, 4xy, 5xx, 6xy, Axx, Bxx, Cxx, Dxx, E1x, E2x, E4x, E6x, EAx, EBx, ECx, EEx, Fxx.
 * Pitch slides retain native stored-word wrap and register-write semantics.
 * Ordinary notes retain raw periods; tone targets use the zero-finetune table.
 * 3xx/5xx retain voice phase; explicit velocity may change without retriggering.
 * Finetune remains unsupported.
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
#endif
