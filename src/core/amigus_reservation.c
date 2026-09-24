#include "amigus_reservation.h"
#include <string.h>
int pt_amigus_reservation_close(struct pt_amigus_reservation *r)
{
    if (!r || r->access) return 0;
    if (r->reserved) r->api.release(r->api.context, r->card, r);
    if (r->opened) r->api.close(r->api.context);
    memset(r, 0, sizeof(*r));
    return 1;
}
enum pt_amigus_reservation_result pt_amigus_reservation_open(
    struct pt_amigus_reservation *r, const struct pt_amigus_reservation_api *api, unsigned index)
{
    void *seen[16], *card = 0;
    unsigned i, j;
    unsigned long code;
    enum pt_amigus_reservation_result result = PT_AMIGUS_NO_CARD;
    if (!r || r->opened || r->reserved || r->access || !api || !api->open ||
        !api->close || !api->find || !api->supported || !api->reserve ||
        !api->release || index >= 16) return PT_AMIGUS_INVALID;
    r->api = *api;
    r->driver_code = 0;
    if (api->open(api->context) != 1) {
        memset(r, 0, sizeof(*r));
        return PT_AMIGUS_NO_LIBRARY;
    }
    r->opened = 1;
    for (i = 0; i <= index; ++i) {
        card = api->find(api->context, card);
        if (!card) goto failed;
        for (j = 0; j < i; ++j) if (seen[j] == card) {
            result = PT_AMIGUS_BAD_ENUMERATION;
            goto failed;
        }
        seen[i] = card;
    }
    if (api->supported(api->context, card) != 1) {
        result = PT_AMIGUS_UNSUPPORTED;
        goto failed;
    }
    code = api->reserve(api->context, card, r);
    if (code) {
        result = code == 0x101UL ? PT_AMIGUS_BUSY : PT_AMIGUS_DRIVER_ERROR;
        pt_amigus_reservation_close(r);
        r->driver_code = code;
        return result;
    }
    r->card = card;
    r->reserved = 1;
    return PT_AMIGUS_RESERVED;
failed:
    pt_amigus_reservation_close(r);
    return result;
}
int pt_amigus_reservation_begin(struct pt_amigus_reservation *r)
{
    if (!r || !r->reserved || r->access) return 0;
    r->access = 1;
    return 1;
}
int pt_amigus_reservation_end(struct pt_amigus_reservation *r)
{
    if (!r || !r->access) return 0;
    r->access = 0;
    return 1;
}

int pt_amigus_discover(const struct pt_amigus_reservation_api *api,
                       struct pt_amigus_discovery *result)
{
    void *seen[16], *card = 0;
    unsigned i;
    int status = 1;
    if (!result) return 0;
    memset(result, 0, sizeof(*result));
    if (!api || !api->open || !api->close || !api->find || !api->supported) return 0;
    if (api->open(api->context) != 1) return 1;
    result->available = 1;
    for (;;) {
        card = api->find(api->context, card);
        if (!card) break;
        if (result->cards == 16) {status = -1;break;}
        for (i = 0; i < result->cards; ++i) if (seen[i] == card) break;
        if (i != result->cards) {status = -1;break;}
        seen[result->cards++] = card;
        if (api->supported(api->context, card) == 1) ++result->pcm_cards;
    }
    api->close(api->context);
    return status;
}
