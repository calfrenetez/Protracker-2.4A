#ifndef PT_PLAYBACK_H
#define PT_PLAYBACK_H
#include <stdint.h>
struct pt_playback {
    unsigned active,order,pattern,row,speed,bpm,mode;
    uint32_t ticks;
    uint16_t period[4];
    uint8_t volume[4];
    int8_t wave[4][81]; /* Time-domain voice preview, not captured analogue audio. */
};
#endif
