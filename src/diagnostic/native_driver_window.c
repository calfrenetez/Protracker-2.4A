#include "native_driver_window.h"
#include <exec/execbase.h>
#include <exec/devices.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>

static void enter(void *unused) { (void)unused; Forbid(); }
static void leave(void *unused) { (void)unused; Permit(); }
static struct pt_driver_snapshot snapshot(void *context)
{
    struct pt_native_driver *d = context;
    struct pt_driver_snapshot s = {0,0,0,0,0};
    struct Library *lib = (struct Library *)FindName(&SysBase->LibList, (STRPTR)d->name);
    struct Device *ahi = (struct Device *)FindName(&SysBase->DeviceList, "ahi.device");
    if (ahi) s.ahi_users = ahi->dd_Library.lib_OpenCnt;
    if (lib) {
        s.present = 1;
        s.supported = lib->lib_Version == d->version && lib->lib_Revision == d->revision &&
            lib->lib_IdString && !strcmp((const char *)lib->lib_IdString, d->id);
        s.driver_users = lib->lib_OpenCnt;
        s.delayed_expunge = (lib->lib_Flags & LIBF_DELEXP) != 0;
    }
    return s;
}
static void remove_idle(void *context)
{
    struct pt_native_driver *d = context;
    struct Library *lib = (struct Library *)FindName(&SysBase->LibList, (STRPTR)d->name);
    /* Caller holds Forbid across the copied guards and this normal request.
     * RemLibrary owns expunge/segment teardown; no foreign FreeCard or flag edits. */
    if (lib) RemLibrary(lib);
}
static int restore(void *context)
{
    struct pt_native_driver *d = context;
    struct Library *lib = OpenLibrary((STRPTR)d->path, d->version);
    struct pt_driver_snapshot s;
    if (!lib) return 0;
    CloseLibrary(lib);
    Forbid();
    s = snapshot(context);
    Permit();
    printf("DRIVER resident=%u supported=%u delayed=%u users=%u ahi_users=%u\n",
           s.present, s.supported, s.delayed_expunge, s.driver_users, s.ahi_users);
    return s.present && s.supported && !s.delayed_expunge;
}
struct pt_driver_api pt_native_driver_api(struct pt_native_driver *driver)
{
    struct pt_driver_api api = {driver,enter,leave,snapshot,remove_idle,restore};
    return api;
}
