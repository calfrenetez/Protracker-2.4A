#ifndef PT_AMIGUS_PCM_PACK_H
#define PT_AMIGUS_PCM_PACK_H
#include "pcm.h"
/* Pure packing for pinned AmiGUS stereo24 FIFO format, not a device driver.
 * Zero initialize. Two stereo frames become three numeric 32-bit FIFO writes,
 * MSB first, L/R interleaved. One odd frame is retained across input blocks.
 * Each call accepts1..256 frames at48k/stereo24, writes <=384 words. Output and
 * state must not overlap input. No allocation or master mutation. Outputs are
 * numeric register words, NOT host-endian serialized bytes. */
struct pt_amigus_pcm_pack {int32_t tail[2];unsigned held,finished;};
int pt_amigus_pcm_pack_block(struct pt_amigus_pcm_pack *,const struct pt_pcm *,uint32_t *words,size_t capacity,size_t *count);
/* Explicit end pads an odd last frame with ONE silent stereo frame, reported
 * in padded_frames. No padding occurs between blocks. Refusal changes nothing.
 * Repeated finish succeeds with zero words/padding. Reset only after transport
 * shutdown; clearing software state does not reset hardware FIFO alignment. */
int pt_amigus_pcm_pack_finish(struct pt_amigus_pcm_pack *,uint32_t *words,size_t capacity,size_t *count,unsigned *padded_frames);
#endif
