#ifndef PT_PITCH_H
#define PT_PITCH_H
#include "flow.h"
/* Reference PCM pitch: raw note periods, no finetune/table quantization.
 * Retain the original replay's 16-bit stored word separately from its latest
 * period-register write (slides mask that write to 12 bits; PerNop does not).
 * Only 000, 1xx, 2xx, A/B/C/D, E1/E2/E6/EA/EB/EC/EE and F are supported by
 * the renderer. Call once after each completed flow tick, including delays. */
struct pt_pitch_channel {uint16_t period,output;uint8_t instrument,sounding,empty;};
struct pt_pitch {struct pt_pitch_channel channel[PT_CHANNEL_LIMIT];};
void pt_pitch_init(struct pt_pitch *);
void pt_pitch_tick(struct pt_pitch *,const struct pt_flow *,uint16_t tracks);
#endif
