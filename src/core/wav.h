#ifndef PT_WAV_H
#define PT_WAV_H
#include "pcm.h"
enum pt_wav_result { PT_WAV_OK, PT_WAV_TRUNCATED, PT_WAV_UNSUPPORTED,
                     PT_WAV_INVALID, PT_WAV_CAPACITY, PT_WAV_ALIAS };
struct pt_wav_info {
    uint32_t frames, rate, data_offset, data_bytes;
    uint8_t channels, bits;
};
/* Synchronous exact positional reads; returns1 only after filling all bytes.
 * Reads header metadata only, with fixed workspace and identical format rules. */
typedef int (*pt_wav_read)(void *,size_t,uint8_t *,size_t);
enum pt_wav_result pt_wav_inspect_reader(pt_wav_read,void *,size_t,struct pt_wav_info *);
enum pt_wav_result pt_wav_inspect(const uint8_t *, size_t, struct pt_wav_info *);
enum pt_wav_result pt_wav_decode(const uint8_t *, size_t, struct pt_pcm *);
enum pt_wav_result pt_wav_size(const struct pt_pcm *, size_t *);
enum pt_wav_result pt_wav_encode(const struct pt_pcm *, uint8_t *, size_t, size_t *);
#endif
