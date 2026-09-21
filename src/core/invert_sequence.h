#ifndef PT_INVERT_SEQUENCE_H
#define PT_INVERT_SEQUENCE_H
#include "flow.h"
#include "invert_pcm.h"
/* One zero-initialized instance per render. Workspace entries are indexed by
 * instrument-1, shared across channels, and prepared before playback starts.
 * This strict classic path requires whole mono8 samples and forward/no loops.
 * Call after each successful flow tick, before generating its following PCM.
 * Error means discard this staging run; earlier channels may already mutate. */
struct pt_invert_sequence { struct pt_invert_loop channel[16]; uint8_t instrument[16]; };
enum pt_pcm_result pt_invert_sequence_tick(struct pt_invert_sequence *,const struct pt_flow *,uint16_t tracks,struct pt_invert_pcm *,size_t count);
#endif
