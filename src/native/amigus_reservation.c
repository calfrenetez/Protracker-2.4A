#include "amigus_reservation.h"
#include <proto/exec.h>
#include "../diagnostic/amigus_calls.h"
static int library_open(void *ctx)
{
    struct pt_native_amigus_library *n = ctx;
    if (n->base) return 0; /* Do not replace another live library reference. */
    n->base = OpenLibrary("amigus.library", 1);
    return n->base != 0;
}
static void library_close(void *ctx)
{
    struct pt_native_amigus_library *n = ctx;
    if (n->base) CloseLibrary(n->base);
    n->base = 0;
}
static void *find_card(void *ctx, void *previous)
{
    struct Library *AmiGUS_Base = ((struct pt_native_amigus_library *)ctx)->base;
    return PT_FindCard((struct AmiGUS *)previous);
}
static int supported(void *ctx, void *value)
{
    const struct AmiGUS *card = value;
    (void)ctx;
    return card->agus_PcmBase &&
        (card->agus_TypeId == AmiGUS_Zorro2 || card->agus_TypeId == AmiGUS_mini);
}
static unsigned long reserve_card(void *ctx, void *card, void *owner)
{
    struct Library *AmiGUS_Base = ((struct pt_native_amigus_library *)ctx)->base;
    return PT_Reserve((struct AmiGUS *)card, AMIGUS_FLAG_PCM, owner);
}
static void release_card(void *ctx, void *card, void *owner)
{
    struct Library *AmiGUS_Base = ((struct pt_native_amigus_library *)ctx)->base;
    PT_Free((struct AmiGUS *)card, AMIGUS_FLAG_PCM, owner);
}
struct pt_amigus_reservation_api pt_native_amigus_reservation_api(
    struct pt_native_amigus_library *n)
{
    struct pt_amigus_reservation_api api = {
        n, library_open, library_close, find_card, supported, reserve_card, release_card
    };
    if (!n) api.open = 0;
    return api;
}
