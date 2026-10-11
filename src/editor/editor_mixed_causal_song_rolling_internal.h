#ifndef PT_EDITOR_MIXED_CAUSAL_SONG_ROLLING_INTERNAL_H
#define PT_EDITOR_MIXED_CAUSAL_SONG_ROLLING_INTERNAL_H
#include "editor_mixed_causal_song_lineage_internal.h"
#include "editor_mixed_causal_rolling_source_internal.h"
/* First rolling profile: exactly T16/C16/C16/S16. Shared genuine preflight,
 * audit/take/replay/lookahead and ownership engine; not arbitrary songs/PLAY.
 * Two live C rows, four numeric history identities for this fixed diagnostic;
 * actual core/port completion history remains bounded root plus latest.
 */
struct pt_editor_mixed_causal_song_rolling_outputs {
    struct pt_editor_mixed_causal_song_lineage_outputs original;
    struct pt_mixed_plan_quantized_batch fourth;
};
struct pt_editor_mixed_causal_song_rolling {
    struct pt_editor_mixed_causal_song_lineage original;
    /* Caller initial configuration, admitted complete before owned writes. */
    struct pt_editor_mixed_causal_rolling_source_prepare *joint;
    struct pt_mixed_causal_rolling_port port;
    struct pt_editor_mixed_causal_rolling_inputs preparation;
    /* Constructor-issued noncopyable state; all initially zero. */
    struct pt_editor_mixed_causal_song_rolling *self;
    const struct pt_editor_mixed_causal_rolling_source_inputs *source;
    struct pt_editor_mixed_causal_song_rolling_outputs *outputs;
    struct pt_mixed_causal_rolling_port saved_port;
    struct pt_mixed_plan_quantized_batch fourth;
    struct pt_editor_mixed_causal_song_lineage_history fourth_history;
};
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_rolling_begin(
    struct pt_editor_mixed_causal_song_rolling *,
    const struct pt_editor_mixed_causal_rolling_source_inputs *,struct pt_editor_mixed_causal_source_borrow *);
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_rolling_step(
    struct pt_editor_mixed_causal_song_rolling *,unsigned);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_rolling_publish(struct pt_editor_mixed_causal_song_rolling *);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_rolling_service_command(
    struct pt_editor_mixed_causal_song_rolling *,unsigned,unsigned);
enum pt_mixed_readers_result pt_editor_mixed_causal_song_rolling_service_reader(
    struct pt_editor_mixed_causal_song_rolling *,unsigned,unsigned);
enum pt_editor_mixed_causal_song_lineage_result pt_editor_mixed_causal_song_rolling_cancel(
    struct pt_editor_mixed_causal_song_rolling *);
int pt_editor_mixed_causal_song_rolling_close(struct pt_editor_mixed_causal_song_rolling *);
#endif
