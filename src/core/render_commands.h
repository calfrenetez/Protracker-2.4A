#ifndef PT_RENDER_COMMANDS_H
#define PT_RENDER_COMMANDS_H
#include "render.h"
#include "pitch.h"
/* Shared audited renderer command state for future Studio dispatch. This is
 * low-level: project/options/flow/pitch/ranges MUST have passed renderer preflight
 * and remain immutable/consistent for the whole tick. PCM lifetime is borrowed.
 * Neither these functions nor this state pin master versions. Studio must use
 * its ownership provider before translating operations; not yet wired here. */
struct pt_render_tremolo {uint8_t command,phase,control;};
struct pt_render_range {uint32_t start,length,trigger_start,trigger_length,trigger_frames;uint8_t offset,loaded,retrigger;};
struct pt_render_command_state {
    struct pt_voice voice[16];uint32_t gain[16][2];
    uint8_t instrument[16],volume[16],velocity[16],output_volume[16];
    struct pt_render_tremolo trem[16];
};
void pt_render_commands_init(struct pt_render_command_state *);
/* Recompute per-side gains from already-applied state. */
void pt_render_commands_gains(const struct pt_project *,const struct pt_render_options *,struct pt_render_command_state *);
/* Apply completed flow/pitch tick AFTER rendering the preceding interval.
 * Retains existing effect/range/handoff behavior. Failure may partially advance
 * command state; abort the session rather than continuing or retrying the tick.
 * No allocation, hardware, callbacks or master writes. */
enum pt_render_result pt_render_commands_tick(const struct pt_project *,const struct pt_render_options *,
    const struct pt_flow *,const struct pt_pitch *,const struct pt_render_range *,uint16_t offsets,
    struct pt_render_command_state *);
enum pt_render_action_kind {PT_RENDER_TRIGGER,PT_RENDER_SEGMENT,PT_RENDER_REPEAT,PT_RENDER_STOP,PT_RENDER_CONTROL};
struct pt_render_action {
    enum pt_render_action_kind kind;unsigned channel;
    struct pt_voice voice;uint32_t gain[2];
};
#define PT_RENDER_ACTIONS 64
struct pt_render_plan {unsigned count;struct pt_render_action action[PT_RENDER_ACTIONS];};
/* Same audited tick with explicit ordered operations; caller-owned bounded plan.
 * Trigger/segment captures initialized voice; repeat captures pending source and
 * bounds; control captures final step/gains. PCM pointers still borrowed.
 * At most four operations per channel. On failure count=0; state may be partial
 * and must be discarded. Do not dispatch partial plans. Plan/state/inputs disjoint.
 * Success also recomputes state's gains. This is not master pinning or transport. */
enum pt_render_result pt_render_commands_plan(const struct pt_project *,const struct pt_render_options *,
    const struct pt_flow *,const struct pt_pitch *,const struct pt_render_range *,uint16_t offsets,
    struct pt_render_command_state *,struct pt_render_plan *);
/* Allocator-owned incremental audited sequence. Open measures/preflights the
 * complete immutable project before publishing a session; exactly one allocation.
 * Options are copied; project and all source storage must outlive close.
 * Protocol: next -> consume successful audio blocks (1..256, exactly span.frames)
 * -> complete (apply returned plan before next). Even zero-frame/end spans require
 * complete. emit=0 means pre-roll: mix/discard but still consume. end!=0 marks the
 * final preceding interval. Open/sequence failure never authorizes partial output.
 * Invalid protocol is refused without advancement; internal tick/command failure
 * poisons the sequence. Close releases state only, not external Studio pins.
 * No hardware/transport, source pinning, callbacks during steps, or PCM reads in
 * consume. Caller must stop Studio on error. */
struct pt_render_sequence;
struct pt_render_interval {uint32_t frames;unsigned emit,end;};
enum pt_render_result pt_render_sequence_open(const struct pt_project *,const struct pt_render_options *,
    const struct pt_allocator *,struct pt_render_sequence **);
enum pt_render_result pt_render_sequence_next(struct pt_render_sequence *,struct pt_render_interval *);
enum pt_render_result pt_render_sequence_consume(struct pt_render_sequence *,uint32_t frames);
enum pt_render_result pt_render_sequence_complete(struct pt_render_sequence *,struct pt_render_plan *);
void pt_render_sequence_close(struct pt_render_sequence *);
/* Internal offline staging hook: caller prevalidates private sample descriptors.
 * Requires classic whole mono8 playback descriptors; one-shots retain a two-frame
 * repeat independently of PCM contents. Flow observes the immutable project;
 * voices read playback. Tick runs only
 * after the preceding audio interval, before subsequent PCM is produced. */
struct pt_render_mutation {
    const struct pt_project *playback;
    void *context;
    int (*tick)(void *,const struct pt_flow *);
};
enum pt_render_result pt_render_mutating_allocated(const struct pt_project *,const struct pt_render_options *,
    pt_render_sink,void *,pt_render_progress,void *,struct pt_render_report *,
    const struct pt_allocator *,const struct pt_render_mutation *);
#endif
