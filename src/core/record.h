#ifndef PT_RECORD_H
#define PT_RECORD_H
#include "project.h"
enum pt_record_result {PT_RECORD_OK,PT_RECORD_IGNORED,PT_RECORD_INVALID,
    PT_RECORD_TIME,PT_RECORD_CAPACITY,PT_RECORD_COLLISION,PT_RECORD_STORE_FAILED};
struct pt_record_voice {uint32_t on_row,last_row;uint8_t active,note,written;};
struct pt_record_settings {
    uint32_t first_row,row_limit; /* absolute logical rows, exclusive limit */
    uint8_t grid_rows,hold,velocity,instrument;
};
struct pt_recorder {
    struct pt_record_settings settings;
    struct pt_record_voice voices[16];
    void *context;
    /* Commit one event atomically through editor undo. Return zero without
       mutation on collision/allocation failure. Must not call back into us. */
    int (*store)(void *,uint32_t,unsigned,const struct pt_event *);
    uint64_t origin,last_position;
    uint8_t armed,started;
};
/* Positions are monotonic sequencer row positions in Q16, not wall-clock time.
   The sequencer must account for tempo changes/latency before this boundary.
   Grid is 1..64 whole rows. Hold starts only on an accepted playable Note On. */
int pt_record_arm(struct pt_recorder *,const struct pt_record_settings *,uint64_t,
    void *,int (*)(void *,uint32_t,unsigned,const struct pt_event *));
enum pt_record_result pt_record_input(struct pt_recorder *,uint64_t,unsigned,unsigned,unsigned,int);
/* Record OFF for held keys before disarming. Failed/overflow releases remain
   armed for recovery. Cancel is explicit and does not stop external MIDI audio. */
enum pt_record_result pt_record_stop(struct pt_recorder *,uint64_t);
void pt_record_cancel(struct pt_recorder *);
#endif
