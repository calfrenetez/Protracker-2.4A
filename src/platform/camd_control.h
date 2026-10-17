#ifndef PT_CAMD_CONTROL_H
#define PT_CAMD_CONTROL_H
#include "midi_endpoints.h"

/* Portable control-only owner. No CAMD ABI, MIDI delivery or timing authority.
 * Zero complete original storage before init; never copy/reinitialize a live
 * owner, mutate internals or cast away the registry observation's constness.
 * Context/owner storage remains alive until CLOSED; HELD retains all storage.
 * Callbacks may operate their actual model resources and designated outputs
 * only, not alter the control or retain temporary output pointers. Reentry
 * refuses unchanged; a rejected reentrant call does not roll back outer work.
 */
enum pt_camd_done {PT_CAMD_DONE,PT_CAMD_NO_EFFECT,PT_CAMD_UNKNOWN};
enum pt_camd_next {PT_CAMD_ITEM,PT_CAMD_END,PT_CAMD_READ_FAILED};
enum pt_camd_state {PT_CAMD_EMPTY,PT_CAMD_READY,PT_CAMD_PARTIAL,
    PT_CAMD_CLOSING,PT_CAMD_HELD,PT_CAMD_CLOSED};
enum pt_camd_result {PT_CAMD_OK,PT_CAMD_REFUSED,PT_CAMD_INVALID,
    PT_CAMD_BUSY,PT_CAMD_CAPACITY,PT_CAMD_IN_USE,PT_CAMD_HOLD,PT_CAMD_MORE,PT_CAMD_FINISHED};
struct pt_camd_view {const void *cursor;char name[PT_MIDI_ENDPOINT];unsigned receivers;};
struct pt_camd_ops {
    /* Synchronous original pointer/NULL: NULL grants no caller-owned child.
     * Open is explicit library/driver activation, never passive discovery. */
    void *(*open)(void *);
    void *(*create_node)(void *,void *library,const char *stable_name);
    void *(*add_sender)(void *,void *library,void *node,const char *stable_name);
    void *(*lock)(void *,void *library);
    /* Under that original lock: exact copied name and actual receiver-list
     * presence, including driver receivers. Never count our sender as receiver.
     * Borrowed cursor is valid only while locked; no callbacks/allocations for
     * UI or delivery. Snapshot bound is16 TOTAL clusters before filtering. */
    enum pt_camd_next (*next_copy)(void *,void *library,const void *last,struct pt_camd_view *);
    /* DONE consumes the exact original handle. NO_EFFECT refused BEFORE any
     * consumption. UNKNOWN permits no retry, including if consumption occurred.
     * These classify software resource calls, never wire/device quiet. */
    enum pt_camd_done (*unlock)(void *,void *library,void *lock);
    enum pt_camd_done (*remove_link)(void *,void *library,void *link);
    enum pt_camd_done (*delete_node)(void *,void *library,void *node);
    enum pt_camd_done (*close)(void *,void *library);
};
struct pt_camd_control {
    struct pt_camd_control *self;
    struct pt_midi_endpoints endpoints;
    struct pt_camd_ops ops;
    void *context;size_t context_bytes;
    void *library,*node,*lock,*links[PT_MIDI_PORTS];
    char node_name[PT_MIDI_ENDPOINT];
    struct pt_midi_endpoint_name link_names[PT_MIDI_PORTS];
    enum pt_camd_state state;
    unsigned busy;
};
int pt_camd_control_init(struct pt_camd_control *,const struct pt_camd_ops *,void *,size_t);
enum pt_camd_result pt_camd_control_activate(struct pt_camd_control *);
/* Full copied-name validation and known successful unlock precede the sole
 * genuine registry observe. Invalid/capacity/failed scans leave it unchanged. */
enum pt_camd_result pt_camd_control_refresh(struct pt_camd_control *);
enum pt_camd_result pt_camd_control_link(struct pt_camd_control *,unsigned);
/* Retire original sender before genuine forget. Unknown teardown leaves all
 * names/contexts reachable in HELD; an original consumed handle is never replayed. */
enum pt_camd_result pt_camd_control_retire(struct pt_camd_control *,unsigned);
/* <=one child callback per close_step; all links then node then library, followed
 * by callback-free CLOSED. No new work after closing/partial activation. */
enum pt_camd_result pt_camd_control_close_step(struct pt_camd_control *);
int pt_camd_control_state(const struct pt_camd_control *);
const struct pt_midi_endpoints *pt_camd_control_registry(const struct pt_camd_control *);
#endif
