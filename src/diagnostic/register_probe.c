#include "register_probe.h"
#include "ownership.h"
#include "amigus_calls.h"
#include "../native/amigus_pcm_read.h"
#include "../native/amigus_reservation.h"
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

static void end_access(struct pt_amigus_reservation *reservation)
{
    if (!pt_amigus_reservation_end(reservation)) {
        puts("REGISTER HOLD: access end unconfirmed; Task/library/card retained");
        fflush(stdout); for (;;) Delay(50);
    }
}

static void release_registers(struct pt_native_amigus_library *native,
                              struct pt_amigus_reservation *reservation, ULONG resource)
{
    struct Library *AmiGUS_Base = native->base;
    PT_Free(reservation->card, resource, reservation);
    const unsigned long release_code = PT_Reserve(reservation->card, resource, NULL);
    printf("REGISTER RELEASE confirmed=%u retained=%u driver=0x%08lx\n",
           release_code==0,release_code!=0,release_code);
    if (release_code) {
        puts("REGISTER HOLD: release unconfirmed; Task/library/card/owner retained");
        fflush(stdout); for (;;) Delay(50);
    }
    /* Suppress the core's unacknowledged release callback after verified release. */
    reservation->reserved=0;
    if (!pt_amigus_reservation_close(reservation)) {
        puts("REGISTER HOLD: library close unconfirmed; Task retained");
        fflush(stdout); for (;;) Delay(50);
    }
}

int pt_record_status_probe(int with_wavetable)
{
    puts("RECORD-PROBE scope=Mini-7ea663e7 writes=NO fifo_data_reads=NO audio=NOT_TESTED");
    struct Library *pin = OpenLibrary((CONST_STRPTR)"amigus.library", 1);
    if (!pin) {
        puts("RECORD result=SKIP reason=library-unavailable rc=5");
        return PT_SKIP;
    }
    if (pin->lib_Version != 1 || pin->lib_Revision != 1) {
        CloseLibrary(pin);
        puts("RECORD result=SKIP reason=unsupported-ownership-contract rc=5");
        return PT_SKIP;
    }
    struct pt_native_amigus_library native = {0};
    struct pt_amigus_reservation reservation = {0};
    struct pt_amigus_reservation_api api = pt_native_amigus_reservation_api(&native);
    struct pt_native_amigus_library wave_native = {0};
    struct pt_amigus_reservation wave = {0};
    if (with_wavetable) {
        struct pt_amigus_reservation_api wave_api = pt_native_amigus_reservation_api(&wave_native);
        const enum pt_amigus_reservation_result opened =
            pt_amigus_reservation_open_resource(&wave, &wave_api, 0, PT_AMIGUS_WAVETABLE);
        if (opened != PT_AMIGUS_RESERVED) {
            CloseLibrary(pin);
            printf("RECORD result=SKIP reason=wavetable-reservation-unavailable status=%u rc=5\n", opened);
            return PT_SKIP;
        }
    }
    const enum pt_amigus_reservation_result opened = pt_amigus_reservation_open(&reservation, &api, 0);
    if (opened != PT_AMIGUS_RESERVED) {
        if (with_wavetable)
            release_registers(&wave_native, &wave, AMIGUS_FLAG_WAVETABLE);
        CloseLibrary(pin);
        printf("RECORD result=SKIP reason=reservation-unavailable status=%u rc=5\n", opened);
        return PT_SKIP;
    }
    const char *reason = "register-contract-unavailable";
    int result = PT_SKIP;
    if (with_wavetable && wave.card != reservation.card) {
        reason = "card-mismatch";
    } else if (pt_amigus_reservation_begin(&reservation)) {
        if (with_wavetable)
            puts("RECORD simultaneous ownership: PCM and WAVETABLE held on the same card");
        struct pt_native_amigus_record_status status;
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) {
            reason = "cancelled";
            result = PT_FAIL;
        } else if (pt_native_amigus_pcm_record_status(&reservation, &status)) {
            const int idle = !(status.control & 0x8000) && !(status.irq_mask & 0x70) && !status.pending_words;
            printf("RECORD STATUS mask=0x%04x pending_irq=0x%04x format=0x%04x control=0x%04x left=0x%04x right=0x%04x watermark=%u pending_words=%u\n",
                   (unsigned)status.irq_mask, (unsigned)status.pending_irq,
                   (unsigned)status.format, (unsigned)status.control,
                   (unsigned)status.volume_left, (unsigned)status.volume_right,
                   (unsigned)status.watermark, (unsigned)status.pending_words);
            reason = idle ? "idle" : "not-idle";
            result = idle ? PT_PASS : PT_SKIP;
        }
        end_access(&reservation);
    }
    release_registers(&native, &reservation, AMIGUS_FLAG_PCM);
    if (with_wavetable) {
        release_registers(&wave_native, &wave, AMIGUS_FLAG_WAVETABLE);
        puts("RECORD release confirmed: PCM and WAVETABLE");
    }
    CloseLibrary(pin);
    printf("RECORD result=%s reason=%s rc=%d\n",
           result == PT_PASS ? "PASS" : result == PT_SKIP ? "SKIP" : "FAIL", reason, result);
    return result;
}

int pt_register_probe(int reset)
{
    struct pt_native_amigus_library native = {0};
    struct pt_amigus_reservation r = {0};
    struct pt_amigus_reservation_api api = pt_native_amigus_reservation_api(&native);
    enum pt_amigus_reservation_result opened;
    uint16_t values[5] = {0,0,0,0,0};
    unsigned i;
    struct pt_native_amigus_quiesce quiesce={0};
    int result = PT_SKIP;
    const char *reason = "register-contract-unavailable";
    printf("REGISTER-PROBE scope=Mini-7ea663e7-%s interrupts=NO audio=NOT_TESTED\n",
           reset==4 ? "disabled-capacity-final-observation" : reset==3 ? "disabled-bounded-FIFO-capacity" : reset==2 ? "disabled-six-word-FIFO" : reset ? "silent-disable-reset" : "read-only writes=NO");
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
            if (reset==3 || reset==4) {
                struct pt_native_amigus_capacity_probe capacity={0};
                uint16_t used=0;
                int full=-1;
                if (!pt_native_amigus_capacity_begin(&capacity,&r)) {
                    result=PT_FAIL; reason="capacity-begin-refused";
                } else {
                    for(i=0;i<2048;++i) {
                        if ((SetSignal(0,0)&SIGBREAKF_CTRL_C) ||
                            !pt_native_amigus_capacity_step(&capacity) ||
                            !pt_native_amigus_pcm_read16(&r,0x10,&used) ||
                            (used!=(i+1)*2 && !(reset==4 && i==2047 && used==4094))) {
                            result=PT_FAIL; reason="capacity-count-or-guard-failed"; break;
                        }
                        if (used==512 || used==1024 || used==2048 || used==4096)
                            printf("PCM CAPACITY progress_words=%u stores=%u\n",used,capacity.issued);
                    }
                    if (reset==4 && result==PT_PASS) {
                        uint16_t flags=0,rate=0,mask=0;
                        for(i=0;i<8;++i) {
                            if (!pt_native_amigus_pcm_read16(&r,0x10,&used) ||
                                !pt_native_amigus_pcm_read16(&r,0x00,&flags) ||
                                !pt_native_amigus_pcm_read16(&r,0x06,&rate) ||
                                !pt_native_amigus_pcm_read16(&r,0x02,&mask) ||
                                used>4096 || (used&1) || (rate&0x8000) || (mask&7)) {
                                result=PT_FAIL;reason="capacity-observation-refused";break;
                            }
                            printf("PCM CAPACITY OBSERVE index=%u pending_words=%u flags=0x%04x rate=0x%04x mask=0x%04x\n",
                                   i+1,used,flags,rate,mask);
                            if(i<7)Delay(1);
                        }
                        printf("PCM CAPACITY OBSERVATION complete=%u polls=%u stores=%u writes_after_poll_start=0 capacity=NOT_QUALIFIED\n",
                               i==8,i,capacity.issued);
                    }
                    if (reset==3 && result==PT_PASS) {
                        for(i=0;i<3;++i) {
                            full=pt_native_amigus_capacity_poll(&capacity);
                            if (full!=0) break;
                            if(i<2) Delay(1);
                        }
                        if(full!=1) {result=PT_FAIL;reason="capacity-full-unconfirmed";}
                    }
                }
                if(reset==3)printf("PCM CAPACITY result=%s stores=%u pending_words=%u full=%u overflow_writes=0 ordering=NOT_TESTED\n",
                       result==PT_PASS ? "PASS" : "FAIL",capacity.issued,used,capacity.confirmed);
            }
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
        end_access(&r);
    }
    release_registers(&native, &r, AMIGUS_FLAG_PCM);
    printf("REGISTERS result=%s reason=%s rc=%d\n",
           result==PT_PASS ? "PASS" : result==PT_SKIP ? "SKIP" : "FAIL",reason,result);
    return result;
}
