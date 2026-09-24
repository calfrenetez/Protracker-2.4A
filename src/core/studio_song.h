#ifndef PT_STUDIO_SONG_H
#define PT_STUDIO_SONG_H
#include "studio_plan.h"
struct pt_studio_song;
/* Owner-thread, immutable-project song session, 48k stereo24 only. Bindings are
 * copied, exactly one per project sample in slot order, matching its descriptor.
 * Source/allocator contexts and project/PCM must outlive close; stop before edits
 * or document replacement. Provider must pin the exact bound master generation.
 * Three bounded allocations (controller, sequencer, mixer), no output on failure.
 * Open preflights/measures the song. Failure preserves *out and releases all state. */
enum pt_render_result pt_studio_song_open(const struct pt_project *,const struct pt_render_options *,
    const struct pt_allocator *,const struct pt_studio_source *,const struct pt_studio_binding *,unsigned,
    struct pt_studio_song **out);
/* Pull at most max_frames (1..256). Performs at most one interval transition OR
 * one audio block, including discarded pre-roll. OK + *pcm=NULL + *done=0 means
 * progress without output: yield/control-check then pull again. Non-null PCM is
 * session-owned, valid only until next pull/stop/close; caller must copy to queues.
 * OK + done=1 marks end/stopped, with no output. Natural end and internal failure
 * release mixer pins and sequence allocations. Error poisons playback; no retry.
 * Invalid public arguments are refused without advancing. No transport, interrupt
 * safety or real-time guarantee. Provider callbacks may allocate while dispatching. */
enum pt_render_result pt_studio_song_pull(struct pt_studio_song *,unsigned max_frames,
    const struct pt_pcm **pcm,unsigned *done);
void pt_studio_song_stop(struct pt_studio_song *);
void pt_studio_song_close(struct pt_studio_song *);
#endif
