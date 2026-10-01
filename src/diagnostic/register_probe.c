#include "register_probe.h"
#include "ownership.h"
#include "amigus_calls.h"
#include "../native/amigus_pcm_read.h"
#include "../native/amigus_reservation.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

int pt_register_probe(int reset)
{
    struct pt_native_amigus_library native = {0};
    struct pt_amigus_reservation r = {0};
    struct pt_amigus_reservation_api api = pt_native_amigus_reservation_api(&native);
    enum pt_amigus_reservation_result opened;
    uint16_t values[5] = {0,0,0,0,0};
    unsigned i;
    struct pt_native_amigus_quiesce quiesce={0};
    unsigned long release_code;
    int result = PT_SKIP;
    const char *reason = "register-contract-unavailable";
    printf("REGISTER-PROBE scope=Mini-7ea663e7-%s interrupts=NO audio=NOT_TESTED\n",
           reset==2 ? "disabled-six-word-FIFO" : reset ? "silent-disable-reset" : "read-only writes=NO");
    opened = pt_amigus_reservation_open(&r, &api, 0);
    if (opened != PT_AMIGUS_RESERVED) {
        printf("REGISTERS result=SKIP reason=reservation-unavailable status=%u rc=5\n", opened);
        return PT_SKIP;
    }
    if (pt_amigus_reservation_begin(&r)) {
        for (i=0; i<5; ++i) {
            if (SetSignal(0,0)&SIGBREAKF_CTRL_C) { result=PT_FAIL; reason="cancelled"; break; }
            if (!pt_native_amigus_pcm_read16(&r, i==4 ? 0x10 : i*2, &values[i])) break;
        }
        if (i==5) {
            reason="not-idle";
            if (!(values[3]&0x8000) && !(values[1]&7) && !values[4]) {
                result=PT_PASS; reason="idle";
            }
            printf("PCM STATUS flags=0x%04x mask=0x%04x format=0x%04x rate=0x%04x pending_words=%u\n",
                   values[0],values[1],values[2],values[3],values[4]);
        }
        if (reset && result==PT_PASS) {
            int status=-1;
            if (reset==2) {
                uint16_t used=0;
                for(i=0;i<3;++i) {
                    if (!pt_native_amigus_pcm_fifo_probe32(&r,0) ||
                        !pt_native_amigus_pcm_read16(&r,0x10,&used) || used!=(i+1)*2) {
                        result=PT_FAIL; reason="fifo-count-mismatch"; break;
                    }
                    printf("PCM FIFO long=%u pending_words=%u\n",i+1,used);
                }
                printf("PCM FIFO result=%s pending_words=%u capacity=NOT_TESTED ordering=NOT_TESTED\n",
                       i==3 ? "PASS" : "FAIL",used);
            }
            /* FIFO failure still requires confirmed reset before release. */
            if (!pt_native_amigus_quiesce_begin(&quiesce,&r)) {
                puts("QUIESCE HOLD: reset refused; possible FIFO data/access/Task/library/card/owner retained; no retry");
                fflush(stdout); for (;;) Delay(50);
            } else {
                /* Three bounded read-only observations. Never repeat writes,
                 * release the access lease or reload AHI without confirmation. */
                for (i=0;i<3;++i) {
                    status=pt_native_amigus_quiesce_poll(&quiesce);
                    if (status!=0) break;
                    if (i<2) Delay(1);
                }
                printf("PCM QUIESCE requested=%u confirmed=%u status=%d polls=%u\n",
                       quiesce.requested,quiesce.confirmed,status,i<3 ? i+1 : 3);
                if (status!=1) {
                    puts("QUIESCE HOLD: disable/reset unconfirmed; access/Task/library/card/owner retained; no retry");
                    fflush(stdout); for (;;) Delay(50);
                }
            }
        }
        if (!pt_amigus_reservation_end(&r)) {
            puts("REGISTER HOLD: access end unconfirmed; Task/library/card retained");
            fflush(stdout); for (;;) Delay(50);
        }
    }
    {
        struct Library *AmiGUS_Base = native.base;
        PT_Free(r.card, AMIGUS_FLAG_PCM, &r);
        release_code = PT_Reserve(r.card, AMIGUS_FLAG_PCM, NULL);
    }
    printf("REGISTER RELEASE confirmed=%u retained=%u driver=0x%08lx\n",
           release_code==0,release_code!=0,release_code);
    if (release_code) {
        puts("REGISTER HOLD: release unconfirmed; Task/library/card/owner retained");
        fflush(stdout); for (;;) Delay(50);
    }
    /* Own release was verified before ending the library lifetime. Suppress
     * the core's unacknowledged release callback on this diagnostic path. */
    r.reserved=0;
    if (!pt_amigus_reservation_close(&r)) {
        puts("REGISTER HOLD: library close unconfirmed; Task retained");
        fflush(stdout); for (;;) Delay(50);
    }
    printf("REGISTERS result=%s reason=%s rc=%d\n",
           result==PT_PASS ? "PASS" : result==PT_SKIP ? "SKIP" : "FAIL",reason,result);
    return result;
}
