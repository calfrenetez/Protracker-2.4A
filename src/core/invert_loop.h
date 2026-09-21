#ifndef PT_INVERT_LOOP_H
#define PT_INVERT_LOOP_H
#include <stdint.h>
/* EFx address/clock state only. Zero-initialize once per replay channel.
 * Caller owns private mutable PCM shared by channels using the same sample.
 * Binding a sample resets its cursor, retaining speed and accumulator.
 * Offsets are byte/frame positions in mono8 PCM, never native pointers. */
struct pt_invert_loop { uint32_t start,end,cursor; uint8_t speed,accumulator,bound; };
int pt_invert_loop_bind(struct pt_invert_loop *,uint32_t frames,uint32_t start,uint32_t end);
int pt_invert_loop_speed(struct pt_invert_loop *,unsigned speed);
/* Call at each reference UpdateFunk point, including the immediate update
 * for nonzero EFx on tick zero. EF0 has no immediate update.
 * Returns 1 and writes index when a byte must become -1-value, else 0.
 * Does not access PCM, allocate, or alter output index when no event occurs. */
int pt_invert_loop_update(struct pt_invert_loop *,uint32_t *index);
#endif
