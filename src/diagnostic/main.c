/* AmiGUSTest 0.9: discovery/ownership, read-only wavetable status and bounded FIFO qualification.
 * Never enables playback or installs interrupts. */
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "amigus_calls.h"
#include "ownership.h"
#include "native_driver_window.h"
#include "register_probe.h"
#include "wavetable_probe.h"

struct Library *AmiGUS_Base;
typedef char abi_card_size[(sizeof(struct AmiGUS) == 40) ? 1 : -1];
typedef char abi_type_offset[(offsetof(struct AmiGUS, agus_TypeId) == 32) ? 1 : -1];

static unsigned long reserve(void *card, unsigned long flag, void *owner)
{ return PT_Reserve((struct AmiGUS *)card, flag, owner); }
static void release(void *card, unsigned long flag, void *owner)
{ PT_Free((struct AmiGUS *)card, flag, owner); }
static int cancelled(void *unused)
{ (void)unused; return (SetSignal(0, 0) & SIGBREAKF_CTRL_C) != 0; }

static int diagnostic(int argc, char **argv)
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
    printf("AMIGUSTEST schema=1 version=0.9 mode=%s\n",
           ownership ? "ownership" : "discover");
    puts("SCOPE audio=NOT_TESTED interrupts=NOT_TESTED firmware_write=NO");
    AmiGUS_Base = OpenLibrary("amigus.library", 1);
    if (!AmiGUS_Base) {
        reason = "library-unavailable";
        goto summary;
    }
    printf("LIBRARY version=%u revision=%u\n", (unsigned)AmiGUS_Base->lib_Version,
           (unsigned)AmiGUS_Base->lib_Revision);
    if (ownership && (AmiGUS_Base->lib_Version != 1 || AmiGUS_Base->lib_Revision != 1)) {
        reason = "unsupported-ownership-contract";
        CloseLibrary(AmiGUS_Base);
        AmiGUS_Base = NULL;
        goto summary;
    }
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
               count - 1, (unsigned)card->agus_TypeId, (unsigned long)card->agus_HardwareRev,
               (unsigned long)card->agus_FirmwareRev, card->agus_PcmBase != NULL,
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
            printf("CHECK card=%u block=%u result=%s stage=%s driver=0x%08lx release=0x%08lx confirmed=%u retained=%u\n",
                   count - 1, i, check.result == PT_PASS ? "PASS" :
                   check.result == PT_SKIP ? "SKIP" : "FAIL", check.stage, check.driver_code,
                   check.release_code, (unsigned)check.release_confirmed, (unsigned)check.retained);
            if (check.retained) {
                puts("OWNERSHIP HOLD: release not confirmed; library/card/Task/owner addresses retained; no retry or exit");
                fflush(stdout);
                for (;;) Delay(50);
            }
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


int main(int argc, char **argv)
{
    struct Library *pin;
    struct pt_driver_window window;
    struct pt_native_driver driver = {"AmiGUS.audio", "DEVS:AHI/AmiGUS.audio",
        "$VER: AmiGUS.audio 4.023 (30.8.26) 020 SAS/C cross\r\n", 4, 23};
    struct pt_driver_api api = pt_native_driver_api(&driver);
    int observe = argc==2 && !strcmp(argv[1], "--idle-capacity-observe");
    int result, capacity = argc==2 && !strcmp(argv[1], "--idle-capacity");
    int fifo = argc==2 && !strcmp(argv[1], "--idle-fifo");
    int reset = observe ? 4 : capacity ? 3 : fifo ? 2 : argc==2 && !strcmp(argv[1], "--idle-reset");
    int registers = reset || (argc==2 && !strcmp(argv[1], "--idle-registers"));
    const char *label = observe ? "IDLE-CAPACITY-OBSERVE" : capacity ? "IDLE-CAPACITY" : fifo ? "IDLE-FIFO" : reset ? "IDLE-RESET" : registers ? "IDLE-REGISTERS" : "IDLE-OWNERSHIP";
    char *ownership_args[] = {argv[0], "--ownership"};
    if (argc==2 && !strcmp(argv[1],"--wavetable-status")) return pt_wavetable_probe();
    if (!registers && (argc != 2 || strcmp(argv[1], "--idle-ownership")))
        return diagnostic(argc, argv);
    printf("%s scope=unused-RC6-020-AHI-only audio=NOT_TESTED\n",label);
    if (cancelled(NULL)) return PT_FAIL;
    /* Keep the existing base library resident through driver restoration. */
    pin = OpenLibrary("amigus.library", 1);
    if (!pin) { printf("%s result=SKIP reason=library-unavailable rc=5\n",label); return PT_SKIP; }
    if (pin->lib_Version != 1 || pin->lib_Revision != 1) {
        CloseLibrary(pin);
        printf("%s result=SKIP reason=unsupported-ownership-contract rc=5\n",label);
        return PT_SKIP;
    }
    window = pt_driver_begin(&api);
    printf("DRIVER-WINDOW result=%d stage=%s unloaded=%u restore_needed=%u\n",
           window.result, window.stage, window.unloaded, window.restore_needed);
    fflush(stdout);
    result = window.result;
    if (window.unloaded) result = registers ? pt_register_probe(reset) : diagnostic(2, ownership_args);
    /* diagnostic deliberately never returns with uncertain card ownership.
     * Such a HOLD also keeps this driver restoration obligation and base pin. */
    if (!pt_driver_end(&api, &window)) {
        puts("DRIVER HOLD: restoration unconfirmed; base library/Task retained; no retry or exit");
        fflush(stdout);
        for (;;) Delay(50);
    }
    CloseLibrary(pin);
    printf("%s result=%s unloaded=%u restored=%u restore_needed=%u rc=%d\n",label,
           result == PT_PASS ? "PASS" : result == PT_SKIP ? "SKIP" : "FAIL",
           window.unloaded, window.restored, window.restore_needed, result);
    return result;
}
