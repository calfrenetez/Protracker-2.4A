#include "ownership.h"

struct pt_ownership_result pt_check_ownership(
    const struct pt_ownership_api *api, unsigned long flag, void *a, void *b)
{
    struct pt_ownership_result r = { PT_FAIL, "invalid-input", 0 };
    if (!api || !api->reserve || !api->release || !api->cancelled ||
        !a || !b || a == b || (flag != 1 && flag != 2))
        return r;
    r.stage = "cancelled";
    if (api->cancelled(api->context)) return r;

    r.stage = "reserve-a";
    r.driver_code = api->reserve(api->context, flag, a);
    if (r.driver_code == (0x100UL | flag)) {
        r.result = PT_SKIP; /* Another application owns this block. */
        return r;
    }
    if (r.driver_code) return r;

    r.stage = "cancelled";
    if (api->cancelled(api->context)) goto cleanup;
    r.stage = "exclude-b";
    r.driver_code = api->reserve(api->context, flag, b);
    if (r.driver_code != (0x100UL | flag)) goto cleanup;

    /* Releasing with the wrong owner must leave A's reservation intact. */
    api->release(api->context, flag, b);
    r.stage = "wrong-owner-release";
    r.driver_code = api->reserve(api->context, flag, b);
    if (r.driver_code != (0x100UL | flag)) goto cleanup;
    r.stage = "cancelled";
    if (api->cancelled(api->context)) goto cleanup;

    api->release(api->context, flag, a);
    r.stage = "reacquire-b";
    r.driver_code = api->reserve(api->context, flag, b);
    if (r.driver_code) goto cleanup;
    api->release(api->context, flag, b);
    r.stage = "reacquire-a";
    r.driver_code = api->reserve(api->context, flag, a);
    if (r.driver_code) goto cleanup;
    r.result = PT_PASS;
    r.stage = "complete";
cleanup:
    /* FreeCard has no return value. Use both identities even on a broken
     * exclusivity result; a conforming driver ignores non-owner release. */
    api->release(api->context, flag, b);
    api->release(api->context, flag, a);
    return r;
}
