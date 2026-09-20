#ifndef PT_RAW_H
#define PT_RAW_H
#include "pcm.h"
/* Headerless PCM must always use explicit settings. No content autodetection. */
struct pt_raw_format {uint32_t rate;uint8_t bits,channels,little_endian,unsigned8;};
enum pt_raw_result {PT_RAW_OK,PT_RAW_INVALID,PT_RAW_CAPACITY,PT_RAW_ALIAS};
enum pt_raw_result pt_raw_frames(size_t,const struct pt_raw_format *,uint32_t *);
enum pt_raw_result pt_raw_decode(const uint8_t *,size_t,const struct pt_raw_format *,struct pt_pcm *);
enum pt_raw_result pt_raw_size(const struct pt_pcm *,const struct pt_raw_format *,size_t *);
enum pt_raw_result pt_raw_encode(const struct pt_pcm *,const struct pt_raw_format *,uint8_t *,size_t,size_t *);
#endif
