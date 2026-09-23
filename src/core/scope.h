#ifndef PT_SCOPE_H
#define PT_SCOPE_H
#include <stddef.h>
#include <stdint.h>
/* A bounded software phase estimate driven by replay ticks. Paula's running
   DMA pointer is not readable; this visualisation is not an audio capture.
   A new hardware trigger resets the phase, including same-sample retriggers. */
struct pt_scope_phase {uint32_t generation,tick;uint64_t position;};
static inline void pt_scope_wave(struct pt_scope_phase *phase,int8_t wave[81],
    const int8_t *data,size_t bytes,uint32_t start,uint32_t length,
    uint32_t loop,uint32_t repeat,uint32_t generation,uint32_t trigger_tick,
    uint32_t tick,unsigned period,unsigned bpm,uint32_t clock)
{
    uint64_t step,loop_step,position,first=(uint64_t)length<<16,cycle=(uint64_t)repeat<<16;
    unsigned j;
    for(j=0;j<81;++j)wave[j]=0;
    if(!generation || !length || !repeat || start>bytes || length>bytes-start ||
       loop>bytes || repeat>bytes-loop || period<1 || bpm<32)return;
    if(phase->generation!=generation) {
        phase->generation=generation;phase->tick=trigger_tick;phase->position=0;
    }
    position=phase->position+((uint64_t)(tick-phase->tick)*clock*5*65536)/(2UL*bpm*period);
    if(position>=first)position=first+(position-first)%cycle;
    phase->position=position;phase->tick=tick;
    /* Fixed time window: pitch changes alter the spacing between wave peaks. */
    step=((uint64_t)clock<<16)/((uint32_t)period*16000);
    loop_step=step%cycle;
    for(j=0;j<81;++j) {
        uint32_t offset=position<first?start+(uint32_t)(position>>16):
            loop+(uint32_t)((position-first)>>16);
        wave[j]=data[offset];
        /* Divide once per voice, not once per scope pixel on a 68000.
           Preserve the initial attack and arbitrary (non-power-of-two) loops. */
        if(position<first) {
            position+=step;
            if(position>=first)position=first+(position-first)%cycle;
        } else {
            position+=loop_step;
            if(position>=first+cycle)position-=cycle;
        }
    }
}
#endif
