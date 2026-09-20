#ifndef PT_PITCH_H
#define PT_PITCH_H
#include "flow.h"
/* Reference PCM pitch: native ordinary-note quantization and all 16 finetune tables.
 * Retain the original replay's 16-bit stored word separately from its latest
 * period-register write (slides mask that write to 12 bits; PerNop does not).
 * Renderer preflight defines supported effects and sample restrictions.
 * Call once after each completed flow tick, including delays. */
struct pt_pitch_channel {uint16_t period,output,target;uint8_t instrument,sounding,empty,speed,up,unsupported,vib_command,vib_phase,vib_control,gliss,finetune;};
struct pt_pitch {struct pt_pitch_channel channel[PT_CHANNEL_LIMIT];};
void pt_pitch_init(struct pt_pitch *);
void pt_pitch_tick(struct pt_pitch *,const struct pt_flow *,uint16_t tracks);
#endif
