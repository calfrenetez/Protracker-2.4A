#ifndef PT_PCM_INTERNAL_H
#define PT_PCM_INTERNAL_H
#include "pcm.h"
/* Bounded descriptor validation only; no sample-value reads. Internal use. */
static enum pt_pcm_result pt_pcm_shape(const struct pt_pcm *pcm)
{
    if (!pcm || (pcm->bits != 8 && pcm->bits != 16 && pcm->bits != 24) ||
        (pcm->channels != 1 && pcm->channels != 2) || !pcm->rate || pcm->rate > 192000 ||
        (pcm->frames && !pcm->data)) return PT_PCM_INVALID;
    if (pcm->frames > pcm->capacity / pcm->channels ||
        pcm->frames > SIZE_MAX / sizeof(int32_t) / pcm->channels) return PT_PCM_CAPACITY;
    return PT_PCM_OK;
}
#endif
