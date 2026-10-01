#include "driver_window.h"

struct pt_driver_window pt_driver_begin(const struct pt_driver_api *api)
{
    struct pt_driver_window r = {PT_SKIP, "driver-unavailable", 0, 0, 0};
    struct pt_driver_snapshot s;
    api->enter(api->context);
    s = api->snapshot(api->context);
    if (!s.present) goto done;
    r.stage = "driver-unsupported";
    if (!s.supported) goto done;
    r.stage = "driver-in-use";
    if (s.driver_users || s.ahi_users) goto done;
    r.stage = "driver-delayed-expunge";
    if (s.delayed_expunge) goto done;
    /* Guards and normal removal are one serialized operation. Refusing a
     * busy library avoids setting its delayed-expunge flag. */
    r.restore_needed = 1;
    api->remove_idle(api->context);
    s = api->snapshot(api->context);
    r.stage = "driver-removal-unconfirmed";
    r.result = PT_FAIL;
    if (!s.present) {
        r.unloaded = 1;
        r.result = PT_PASS;
        r.stage = "driver-unloaded";
    }
done:
    api->leave(api->context);
    return r;
}

int pt_driver_end(const struct pt_driver_api *api, struct pt_driver_window *r)
{
    if (!r->restore_needed) return 1;
    /* Even a refused expunge needs normal Open/Close to clear a possible
     * delayed expunge. Never retry removal or write library flags ourselves. */
    if (!api->restore(api->context)) {
        r->result = PT_FAIL;
        r->stage = "driver-restoration-unconfirmed";
        return 0;
    }
    r->restore_needed = 0;
    r->restored = 1;
    return 1;
}
