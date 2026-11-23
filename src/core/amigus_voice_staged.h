#ifndef PT_AMIGUS_VOICE_STAGED_H
#define PT_AMIGUS_VOICE_STAGED_H

#include "amigus_voice_plan.h"

struct pt_amigus_voice_staged_start {
    struct pt_amigus_voice_register_program preload;
    struct pt_amigus_voice_register_write frame[2];
};

/* Split an encoded trigger for an independently verified inactive voice.
 * The preload selects the voice and writes only its parameters. The final
 * two writes reselect the voice and start it. The future adapter still owns
 * the original aperture and reader, and must account for its staged writes
 * even when activation is cancelled. No bus access. */
int pt_amigus_voice_split_start_registers(const struct pt_amigus_voice_register_program *,
    struct pt_amigus_voice_staged_start *);

#endif
