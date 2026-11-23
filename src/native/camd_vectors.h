#ifndef PT_NATIVE_CAMD_VECTORS_H
#define PT_NATIVE_CAMD_VECTORS_H

#include <stdint.h>

struct Library;

/* Classic 68k public ABI. See the original table and argument declarations in
 * kmatheussen/camd at 55a2f589 and the classic CAMD .fd table. The synchronous
 * PutMidi/GetMidi vectors serve the isolated loopback diagnostic only. */
enum pt_native_camd_vector_offset {
    PT_CAMD_LOCK = -30, PT_CAMD_UNLOCK = -36, PT_CAMD_CREATE_NODE = -42, PT_CAMD_DELETE_NODE = -48,
    PT_CAMD_ADD_LINK = -84, PT_CAMD_REMOVE_LINK = -90, PT_CAMD_NEXT_CLUSTER = -126,
    PT_CAMD_FIND_CLUSTER = -132, PT_CAMD_PUT = -138, PT_CAMD_GET = -144, PT_CAMD_GET_ERROR = -180
};

enum pt_native_camd_type { PT_CAMD_RECEIVER = 0, PT_CAMD_SENDER = 1 };

#define PT_CAMD_NODE_NAME_TAG UINT32_C(0x80000041)
#define PT_CAMD_LINK_LOCATION_TAG UINT32_C(0x80000041)
#define PT_CAMD_RECEIVE_QUEUE_TAG UINT32_C(0x80000047)
#define PT_CAMD_RECEIVE_STAMP_TAG UINT32_C(0x80000049)

uint32_t pt_native_camd_vector(struct Library *, int32_t offset, uint32_t d0,
    const void *a0, const void *a1);

#endif
