#ifndef PT_CAPTURE_H
#define PT_CAPTURE_H
#include "document.h"
enum pt_capture_result {PT_CAPTURE_OK,PT_CAPTURE_INVALID,PT_CAPTURE_CAPACITY,PT_CAPTURE_OVERRUN};
/* Serialized software collector only, not device capture. The caller negotiates
 * the exact format with its backend; accepting a format here proves no hardware
 * capability. Allocate before starting the device. Append synchronously copies
 * at most256 frames and retains no input pointer. No device may write directly
 * into this allocation or retain it; a device owner must separately stop and
 * quiesce callbacks before destroying its contexts. Zero-init once. */
struct pt_capture {
    struct pt_allocator allocator;
    struct pt_pcm pcm;
    uint32_t limit;
    size_t bytes;
    unsigned active,finished,failed;
};
enum pt_capture_result pt_capture_open(struct pt_capture *,const struct pt_allocator *,unsigned bits,unsigned channels,uint32_t rate,uint32_t max_frames,size_t budget);
/* Format/value/size errors poison publication; no partial chunk is copied.
 * Capacity exhaustion is an explicit overrun, never silent truncation. */
enum pt_capture_result pt_capture_append(struct pt_capture *,const struct pt_pcm *);
void pt_capture_overrun(struct pt_capture *);
enum pt_capture_result pt_capture_finish(struct pt_capture *);
/* Borrowed immutable finished PCM, NULL when empty, active or failed. */
const struct pt_pcm *pt_capture_pcm(const struct pt_capture *);
/* Discard software staging. Does not stop or release any device resource. */
void pt_capture_close(struct pt_capture *);
#endif
