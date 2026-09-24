#ifndef PT_SAMPLE_FILE_H
#define PT_SAMPLE_FILE_H
#include "file_save.h"
#include "pcm.h"
#include "raw.h"
#include "svx.h"
/* Lossless WAV from the immutable master using fixed caller-budgeted workspace.
 * Source remains alive/unchanged until return; no event processing or conversion.
 * Supports mono/stereo8/16/24, including correct unsigned8 and RIFF pad. */
enum pt_save_result pt_sample_wav_save(const char *,const struct pt_pcm *,const struct pt_allocator *);
/* Exact matching RAW format only; signed8/unsigned8 and explicit byte order.
 * No resampling, precision conversion or metadata change. Same source lifetime. */
enum pt_save_result pt_sample_raw_save(const char *,const struct pt_pcm *,const struct pt_raw_format *,const struct pt_allocator *);
/* Uncompressed mono8 IFF/8SVX, exact core encoder metadata and padding.
 * Caller applies project-specific eligibility (slices, finetune, loop modes). */
enum pt_save_result pt_sample_svx_save(const char *,const struct pt_pcm *,const struct pt_svx_info *,const struct pt_allocator *);
#endif
