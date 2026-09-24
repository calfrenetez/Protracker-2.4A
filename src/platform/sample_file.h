#ifndef PT_SAMPLE_FILE_H
#define PT_SAMPLE_FILE_H
#include "file_save.h"
#include "pcm.h"
/* Lossless WAV from the immutable master using fixed caller-budgeted workspace.
 * Source remains alive/unchanged until return; no event processing or conversion.
 * Supports mono/stereo8/16/24, including correct unsigned8 and RIFF pad. */
enum pt_save_result pt_sample_wav_save(const char *,const struct pt_pcm *,const struct pt_allocator *);
#endif
