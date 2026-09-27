#include "native_exec_memory.h"
#include "../src/native/eclock.h"
#include "../src/native/eclock_alarm.h"
#include <proto/dos.h>
#define malloc native_allocate
#define free native_release
#define PT_EDITOR_WAVETABLE_NATIVE
#include "editor_wavetable_test.c"
static void native_eclock_fixture(void)
{
    struct pt_native_eclock clock={0};struct pt_elapsed_clock elapsed;
    pt_wavetable_clock_read read=pt_native_eclock_read;
    uint64_t ticks=99,previous,first,frames,deadline;uint32_t frequency=77,initial;unsigned i,epoch;
    assert(!read(&clock,&ticks,&frequency) && ticks==99 && frequency==77);
    for(epoch=0;epoch<3;++epoch) {
        assert(pt_native_eclock_open(&clock));assert(!pt_native_eclock_open(&clock));
        assert(read(&clock,&first,&initial) && initial);
        assert(pt_elapsed_clock_init(&elapsed,initial,48000,first,0)==PT_ELAPSED_OK);previous=first;
        for(i=0;i<100;++i) {
            assert(read(&clock,&ticks,&frequency) && frequency==initial && ticks>=previous);previous=ticks;
            assert(pt_elapsed_clock_advance(&elapsed,frequency,ticks,&frames)==PT_ELAPSED_OK);
            assert(pt_elapsed_clock_deadline(&elapsed,frames+48000,&deadline)==PT_ELAPSED_OK && deadline>ticks);
        }
        assert(ticks>first);
        printf("NATIVE ECLOCK observed frequency=%lu reads=101 epoch=%u\n",(unsigned long)initial,epoch);
        pt_native_eclock_close(&clock);pt_native_eclock_close(&clock);ticks=99;frequency=77;
        assert(!clock.port && !clock.request && !clock.opened);
        assert(!read(&clock,&ticks,&frequency) && ticks==99 && frequency==77);
    }
    puts("NATIVE ECLOCK PASS: real timer.device reads, stable frequency, monotonic64-bit counter, checked frame/deadline conversion, repeated close/reopen; no timer requests or audio output");
}
static void native_alarm_fixture(void)
{
    struct pt_native_eclock clock={0};struct pt_native_alarm alarm={0};
    uint64_t now,deadline;uint32_t frequency,initial;unsigned i,guard;enum pt_alarm_result r;
    assert(pt_native_eclock_open(&clock) && pt_native_alarm_open(&alarm));
    assert(pt_native_eclock_read(&clock,&now,&initial) && initial>=500);
    for(i=0;i<4;++i) {
        assert(pt_native_eclock_read(&clock,&now,&frequency) && frequency==initial);
        deadline=now+frequency/500;
        assert(pt_native_alarm_arm(&alarm,deadline)==PT_ALARM_WAITING && pt_native_alarm_signal(&alarm));
        /* Coarse diagnostic polling deliberately includes caller latency.
           Never interpret these observations as precise wakeup performance. */
        guard=0;
        do {Delay(1);r=pt_native_alarm_poll(&alarm);assert(++guard<=8);}while(r==PT_ALARM_WAITING);
        assert(r==PT_ALARM_READY && !alarm.pending && !pt_native_alarm_signal(&alarm));
        assert(pt_native_eclock_read(&clock,&now,&frequency) && frequency==initial && now>=deadline);
        printf("NATIVE ALARM observed late_ticks=%lu frequency=%lu poll=Delay(1) case=%u\n",(unsigned long)(now-deadline),(unsigned long)frequency,i);
    }
    assert(pt_native_eclock_read(&clock,&now,&frequency));
    assert(pt_native_alarm_arm(&alarm,now+(uint64_t)frequency*10)==PT_ALARM_WAITING);
    guard=0;while(!pt_native_alarm_close(&alarm)){assert(++guard<=8);Delay(1);}
    assert(!alarm.port && !alarm.request && !alarm.opened && !alarm.pending);
    assert(pt_native_alarm_close(&alarm) && pt_native_alarm_open(&alarm) && pt_native_alarm_close(&alarm));
    pt_native_eclock_close(&clock);
    puts("NATIVE ALARM PASS: absolute EClock alarm completion, reply collection/reuse, future-alarm cancel/close and reopen; coarse diagnostic polling, no audio or timing qualification");
}
static void native_alarm_signal_fixture(void)
{
    struct pt_native_eclock clock={0};struct pt_native_alarm alarm={0},watchdog={0};
    uint64_t now,deadline,late[16],minimum=UINT64_MAX,maximum=0;
    uint32_t frequency,initial;unsigned i,guard,over_frame=0;enum pt_alarm_result r;
    assert(pt_native_eclock_open(&clock) && pt_native_alarm_open(&alarm) && pt_native_alarm_open(&watchdog));
    assert(pt_native_eclock_read(&clock,&now,&initial) && initial>=500);
    assert(pt_native_alarm_arm(&watchdog,now+(uint64_t)initial*2)==PT_ALARM_WAITING);
    for(i=0;i<16;++i) {
        ULONG mask;
        assert(pt_native_eclock_read(&clock,&now,&frequency) && frequency==initial);
        deadline=now+frequency/(i&1?100:500);
        assert(pt_native_alarm_arm(&alarm,deadline)==PT_ALARM_WAITING);
        mask=pt_native_alarm_signal(&alarm)|pt_native_alarm_signal(&watchdog);
        assert(pt_native_alarm_signal(&alarm) && pt_native_alarm_signal(&watchdog) &&
            pt_native_alarm_signal(&alarm)!=pt_native_alarm_signal(&watchdog));
        guard=0;
        for(;;) {
            assert(pt_native_alarm_poll(&watchdog)==PT_ALARM_WAITING);
            r=pt_native_alarm_poll(&alarm);if(r!=PT_ALARM_WAITING)break;
            assert(++guard<=64);Wait(mask);
        }
        assert(r==PT_ALARM_READY);
        assert(pt_native_eclock_read(&clock,&now,&frequency) && frequency==initial && now>=deadline);
        late[i]=now-deadline;if(late[i]<minimum)minimum=late[i];if(late[i]>maximum)maximum=late[i];
        if(late[i]*48000>=frequency)++over_frame;
    }
    guard=0;while(!pt_native_alarm_close(&watchdog)){assert(++guard<=8);Delay(1);}
    assert(pt_native_alarm_close(&alarm));pt_native_eclock_close(&clock);
    for(i=0;i<16;++i)printf("NATIVE SIGNAL observed late_ticks=%lu frequency=%lu delay_ms=%u case=%u\n",
        (unsigned long)late[i],(unsigned long)initial,i&1?10:2,i);
    printf("NATIVE SIGNAL SUMMARY cases=16 min_ticks=%lu max_ticks=%lu over_48k_frame=%u frequency=%lu\n",
        (unsigned long)minimum,(unsigned long)maximum,over_frame,(unsigned long)initial);
    puts("NATIVE SIGNAL PASS: task signal wake, independent watchdog, actual-time resampling and all requests/resources closed; diagnostic observation only, no playback dispatch");
}
int main(void){int result;native_memory_start();native_eclock_fixture();native_alarm_fixture();native_alarm_signal_fixture();result=editor_wavetable_fixture();native_memory_finish();return result;}
