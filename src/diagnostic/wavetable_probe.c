#include "wavetable_probe.h"
#include "ownership.h"
#include "amigus_calls.h"
#include "../native/amigus_wavetable_read.h"
#include "../native/amigus_reservation.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

int pt_wavetable_probe(void)
{
    struct Library *pin;
    struct pt_native_amigus_library native={0};
    struct pt_amigus_reservation r={0};
    struct pt_amigus_reservation_api api=pt_native_amigus_reservation_api(&native);
    struct pt_native_amigus_wavetable_read n={0};
    enum pt_amigus_reservation_result opened;
    unsigned i=0;
    unsigned long release_code;
    int result=PT_SKIP;
    const char *reason="register-contract-unavailable";
    puts("WAVETABLE-PROBE version=0.9 scope=Mini-7ea663e7-global-status writes=NO bank_select=NO audio=NOT_TESTED capacity=NOT_QUALIFIED voices_stopped=NOT_QUALIFIED");
    pin=OpenLibrary("amigus.library",1);
    if (!pin) { puts("WAVETABLE result=SKIP reason=library-unavailable rc=5"); return PT_SKIP; }
    if (pin->lib_Version!=1 || pin->lib_Revision!=1) {
        CloseLibrary(pin);
        puts("WAVETABLE result=SKIP reason=unsupported-ownership-contract rc=5");
        return PT_SKIP;
    }
    opened=pt_amigus_reservation_open_resource(&r,&api,0,PT_AMIGUS_WAVETABLE);
    if (opened!=PT_AMIGUS_RESERVED) {
        printf("WAVETABLE result=SKIP reason=reservation-unavailable status=%u rc=5\n",opened);
        CloseLibrary(pin);
        return PT_SKIP;
    }
    if (pt_amigus_reservation_begin(&r)) {
        if (pt_native_amigus_wavetable_read_bind(&n,&r)) {
            result=PT_FAIL; reason="read-refused";
            for (i=0;i<8;++i) {
                uint16_t value=0;
                if (SetSignal(0,0)&SIGBREAKF_CTRL_C) { reason="cancelled"; break; }
                if (!pt_native_amigus_wavetable_read16(&n,i*2,&value)) break;
                printf("WAVETABLE STATUS offset=0x%02x value=0x%04x\n",i*2,value);
            }
            if (i==8) { result=PT_PASS; reason="observed"; }
        }
        if (!pt_amigus_reservation_end(&r)) {
            puts("WAVETABLE HOLD: access end unconfirmed; Task/library/card retained");
            fflush(stdout); for (;;) Delay(50);
        }
    }
    /* No device writes occurred, so there is no stop/reset obligation. Keep
     * Task/library/owner alive unless our own library lease release is proven. */
    AmiGUS_Base=native.base;
    PT_Free(r.card,AMIGUS_FLAG_WAVETABLE,&r);
    release_code=PT_Reserve(r.card,AMIGUS_FLAG_WAVETABLE,NULL);
    printf("WAVETABLE RELEASE confirmed=%u retained=%u driver=0x%08lx\n",
           release_code==0,release_code!=0,release_code);
    if (release_code) {
        puts("WAVETABLE HOLD: release unconfirmed; Task/library/card/owner retained");
        fflush(stdout); for (;;) Delay(50);
    }
    r.reserved=0;
    if (!pt_amigus_reservation_close(&r)) {
        puts("WAVETABLE HOLD: library close unconfirmed; Task retained");
        fflush(stdout); for (;;) Delay(50);
    }
    AmiGUS_Base=NULL;
    CloseLibrary(pin);
    printf("WAVETABLE result=%s reason=%s reads=%u rc=%d\n",
           result==PT_PASS ? "PASS" : result==PT_SKIP ? "SKIP" : "FAIL",reason,i,result);
    return result;
}
