#ifndef PT_SVX_H
#define PT_SVX_H
#include "pcm.h"
enum pt_svx_result {PT_SVX_OK,PT_SVX_TRUNCATED,PT_SVX_UNSUPPORTED,PT_SVX_INVALID,PT_SVX_CAPACITY,PT_SVX_ALIAS};
struct pt_svx_info {
 uint32_t frames,rate,loop_start,loop_end,volume,cycles,offset,bytes;
 uint8_t compression;
 char name[32];
};
/* Exact synchronous positional reads. Reader decode may write a prefix on I/O
 * failure; callers must use unpublished staging. Source must remain stable. */
typedef int (*pt_svx_read)(void *,size_t,uint8_t *,size_t);
enum pt_svx_result pt_svx_inspect_reader(pt_svx_read,void *,size_t,struct pt_svx_info *);
enum pt_svx_result pt_svx_decode_reader(pt_svx_read,void *,size_t,struct pt_pcm *);
enum pt_svx_result pt_svx_inspect(const uint8_t *,size_t,struct pt_svx_info *);
enum pt_svx_result pt_svx_decode(const uint8_t *,size_t,struct pt_pcm *);
enum pt_svx_result pt_svx_size(const struct pt_pcm *,const struct pt_svx_info *,size_t *);
enum pt_svx_result pt_svx_encode(const struct pt_pcm *,const struct pt_svx_info *,uint8_t *,size_t,size_t *);
#endif
