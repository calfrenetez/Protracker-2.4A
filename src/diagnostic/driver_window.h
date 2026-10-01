#ifndef PT_DRIVER_WINDOW_H
#define PT_DRIVER_WINDOW_H
#include "ownership.h"

/* Snapshots are copied while Exec's task scheduling is forbidden. Never keep
 * or dereference the library address after requesting its normal expunge. */
struct pt_driver_snapshot {
    int present, supported, delayed_expunge;
    unsigned int driver_users, ahi_users;
};
struct pt_driver_api {
    void *context;
    void (*enter)(void *);
    void (*leave)(void *);
    struct pt_driver_snapshot (*snapshot)(void *);
    void (*remove_idle)(void *);
    int (*restore)(void *);
};
struct pt_driver_window {
    enum pt_result result;
    const char *stage;
    int restore_needed, unloaded, restored;
};
struct pt_driver_window pt_driver_begin(const struct pt_driver_api *);
int pt_driver_end(const struct pt_driver_api *, struct pt_driver_window *);
#endif
