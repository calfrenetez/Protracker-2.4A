/* AmiGUSTest 0.1: discovery and exclusive-ownership checks only.
 * Does not program PCM/wavetable/codec registers or install interrupts. */
#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "amigus_calls.h"
#include "ownership.h"

struct Library *AmiGUS_Base;
typedef char abi_card_size[(sizeof(struct AmiGUS) == 40) ? 1 : -1];
typedef char abi_type_offset[(offsetof(struct AmiGUS, agus_TypeId) == 32) ? 1 : -1];

static unsigned long reserve(void *card, unsigned long flag, void *owner)
{ return PT_Reserve((struct AmiGUS *)card, flag, owner); }
static void release(void *card, unsigned long flag, void *owner)
{ PT_Free((struct AmiGUS *)card, flag, owner); }
static int cancelled(void *unused)
{ (void)unused; return (SetSignal(0, 0) & SIGBREAKF_CTRL_C) != 0; }

int main(int argc, char **argv)
{
    struct AmiGUS *cards[16], *card = NULL;
    unsigned int count = 0, i, passed = 0, skipped = 0, failed = 0;
    int ownership = 0, owner_a, owner_b, result = PT_SKIP;
    const char *reason = "no-card";
    if (argc == 2 && !strcmp(argv[1], "--ownership")) ownership = 1;
    else if (argc > 2 || (argc == 2 && strcmp(argv[1], "--discover"))) {
        puts("usage: AmiGUSTest [--discover|--ownership]");
        return PT_FAIL;
    }
    printf("AMIGUSTEST schema=1 version=0.1 mode=%s\n",
           ownership ? "ownership" : "discover");
    puts("SCOPE audio=NOT_TESTED interrupts=NOT_TESTED firmware_write=NO");
    AmiGUS_Base = OpenLibrary("amigus.library", 1);
    if (!AmiGUS_Base) {
        reason = "library-unavailable";
        goto summary;
    }
    printf("LIBRARY version=%u revision=%u\n", (unsigned)AmiGUS_Base->lib_Version,
           (unsigned)AmiGUS_Base->lib_Revision);
    for (;;) {
        if (cancelled(NULL)) { result = PT_FAIL; reason = "cancelled"; break; }
        card = PT_FindCard(card);
        if (!card) break;
        if (count == 16) { result = PT_FAIL; reason = "card-limit"; break; }
        for (i = 0; i < count; ++i) if (cards[i] == card) break;
        if (i != count) { result = PT_FAIL; reason = "enumeration-cycle"; break; }
        cards[count++] = card;
        printf("CARD index=%u type=0x%04x hardware=0x%08lx firmware=0x%08lx "
               "pcm=%u wavetable=%u codec=%u date=%u-%02u-%02uT%02u:%02u\n",
               count - 1, (unsigned)card->agus_TypeId, card->agus_HardwareRev,
               card->agus_FirmwareRev, card->agus_PcmBase != NULL,
               card->agus_WavetableBase != NULL, card->agus_CodecBase != NULL,
               (unsigned)card->agus_Year, (unsigned)card->agus_Month,
               (unsigned)card->agus_Day, (unsigned)card->agus_Hour,
               (unsigned)card->agus_Minute);
        if (!ownership) { ++passed; continue; }
        if (card->agus_TypeId != AmiGUS_Zorro2 && card->agus_TypeId != AmiGUS_mini) {
            ++skipped;
            printf("CHECK card=%u result=SKIP reason=unknown-card-type\n", count - 1);
            continue;
        }
        for (i = 1; i <= 2; ++i) {
            struct pt_ownership_api api = { card, reserve, release, cancelled };
            struct pt_ownership_result check;
            if ((i == 1 && !card->agus_PcmBase) ||
                (i == 2 && !card->agus_WavetableBase)) {
                ++skipped;
                printf("CHECK card=%u block=%u result=SKIP reason=block-unavailable\n", count - 1, i);
                continue;
            }
            check = pt_check_ownership(&api, i, &owner_a, &owner_b);
            printf("CHECK card=%u block=%u result=%s stage=%s driver=0x%08lx\n",
                   count - 1, i, check.result == PT_PASS ? "PASS" :
                   check.result == PT_SKIP ? "SKIP" : "FAIL", check.stage, check.driver_code);
            if (check.result == PT_PASS) ++passed;
            else if (check.result == PT_SKIP) ++skipped;
            else ++failed;
        }
    }
    CloseLibrary(AmiGUS_Base);
    AmiGUS_Base = NULL;
    if (result != PT_FAIL && count) {
        result = failed ? PT_FAIL : skipped ? PT_SKIP : PT_PASS;
        reason = failed ? "check-failed" : skipped ? "incomplete" : "complete";
    }
summary:
    printf("SUMMARY result=%s reason=%s cards=%u passed=%u skipped=%u failed=%u rc=%d\n",
           result == PT_PASS ? "PASS" : result == PT_SKIP ? "SKIP" : "FAIL",
           reason, count, passed, skipped, failed, result);
    return result;
}
