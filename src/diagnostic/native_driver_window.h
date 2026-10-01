#ifndef PT_NATIVE_DRIVER_WINDOW_H
#define PT_NATIVE_DRIVER_WINDOW_H
#include "driver_window.h"
struct pt_native_driver {
    const char *name, *path, *id;
    unsigned int version, revision;
};
struct pt_driver_api pt_native_driver_api(struct pt_native_driver *);
#endif
