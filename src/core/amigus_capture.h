#ifndef PT_AMIGUS_CAPTURE_H
#define PT_AMIGUS_CAPTURE_H
#include "capture_session.h"
#include "amigus_reservation.h"
/* Injected recording under one PCM access lease. The pinned library reserves
 * the whole PCM block (playback, recording and mixer), not an independent input
 * block. This owner therefore excludes a concurrent Studio owner. No duplex
 * sharing, hardware capability inference, interrupt install or native I/O.
 * Caller holds reservation and stable input context until close succeeds.
 * Input stop must disable capture and remove/quiesce its interrupt owner;
 * even a claimed stop acknowledgment cannot bypass reservation.interrupt.
 * Do not manipulate the internal session or end this lease independently. */
struct pt_amigus_capture {
    struct pt_capture_session session;
    struct pt_amigus_reservation *reservation;
    struct pt_capture_input input;
};
/* Returns0 without acquiring input when busy, wrong block, malformed or out of
 * memory. No callbacks run in open. Caller owns library/reservation release. */
int pt_amigus_capture_open(struct pt_amigus_capture *,struct pt_amigus_reservation *,const struct pt_capture_input *,const struct pt_allocator *,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget);
/* Same bounded callback/error/cleanup contract as capture_session. Ends access
 * only after confirmed stop and no interrupt owner, including fault/abort paths. */
enum pt_capture_poll pt_amigus_capture_step(struct pt_amigus_capture *);
void pt_amigus_capture_finish(struct pt_amigus_capture *);
void pt_amigus_capture_abort(struct pt_amigus_capture *);
int pt_amigus_capture_take(struct pt_amigus_capture *,struct pt_capture *);
int pt_amigus_capture_close(struct pt_amigus_capture *);
#endif
