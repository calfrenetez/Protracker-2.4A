#ifndef PT_STEMS_H
#define PT_STEMS_H
#include "channels.h"
enum pt_stem_result {PT_STEM_OK,PT_STEM_INVALID,PT_STEM_MIDI};
struct pt_stem {uint16_t tracks;uint8_t group,channel;};
struct pt_stem_plan {struct pt_stem item[PT_CHANNEL_LIMIT];uint8_t count;};
/* Stable order by first selected channel. Group0 stays individual; grouped mode
 * combines selected members of groups1..15. Masks partition selection exactly.
 * Keep global project flow, mute/solo and gain unchanged during every render.
 * Selected MIDI is refused even when muted. Output changes only on success. */
enum pt_stem_result pt_stems_plan(const struct pt_channels *,uint16_t selected,
                                unsigned grouped,struct pt_stem_plan *);
#endif
