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
struct native_gate_clock {struct pt_native_eclock clock;uint64_t observed;uint32_t frequency;};
static int native_gate_read(void *p,uint64_t *ticks,uint32_t *frequency)
{
    struct native_gate_clock *c=p;
    if(!pt_native_eclock_read(&c->clock,ticks,frequency))return 0;
    c->observed=*ticks;c->frequency=*frequency;return 1;
}
static void native_song_gate_fixture(void)
{
    struct fixture *f=malloc(sizeof(*f));struct dispatch_bus *bus=malloc(sizeof(*bus));
    struct pt_allocator a={NULL,allocate_master,release_master};struct pt_document d;
    struct pt_sampler sampler;struct pt_sampler_wavetable bridge={0};struct pt_wavetable_voices owner={0};
    struct pt_wavetable_song *song=NULL;struct pt_wavetable_preflight_report report;
    struct pt_render_options o={0};struct pt_playback_format format={16,0,0,0};
    struct native_gate_clock clock={0};struct pt_native_alarm alarm={0},watchdog={0};
    uint64_t origin,deadline,tick,frame;unsigned mode,guard;enum pt_wavetable_song_result r;enum pt_alarm_result ar;
    int32_t data[8]={257,-513,1025,-2049,17,31,47,63};
    assert(f && bus && pt_native_eclock_open(&clock.clock));
    pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
    d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};d.project.samples[0].volume=64;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_end=8;
    d.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};d.project.events[4].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=100000;
    for(mode=0;mode<2;++mode) {
        pt_sampler_init(&sampler,&a,1024*1024);song_bind(f,&bridge,&owner,bus,&sampler,&d.project);
        assert(pt_wavetable_song_open(&owner,&o,&format,&a,&report,&song)==PT_WAVETABLE_SONG_OK);
        assert(pt_native_alarm_open(&alarm) && pt_native_alarm_open(&watchdog));
        assert(pt_wavetable_song_clocked_begin(song,9600,native_gate_read,&clock)==PT_WAVETABLE_SONG_OK);origin=clock.observed;
        guard=0;do{r=pt_wavetable_song_clocked_service(song,&deadline);assert(++guard<1000);}while(r==PT_WAVETABLE_SONG_WAITING);
        assert(r==PT_WAVETABLE_SONG_OK && pins(f) && !bus->starts && !bus->controls);
        assert(pt_wavetable_song_clocked_deadline(song,&tick)==PT_WAVETABLE_SONG_OK);
        assert(pt_native_alarm_arm(&watchdog,tick+clock.frequency)==PT_ALARM_WAITING);
        assert(pt_native_alarm_arm(&alarm,tick)==PT_ALARM_WAITING);guard=0;
        for(;;) {
            assert(pt_native_alarm_poll(&watchdog)==PT_ALARM_WAITING);
            ar=pt_native_alarm_poll(&alarm);if(ar!=PT_ALARM_WAITING)break;
            assert(++guard<=64);Wait(pt_native_alarm_signal(&alarm)|pt_native_alarm_signal(&watchdog));
        }
        assert(ar==PT_ALARM_READY);
        if(mode)Delay(1); /* Deterministic late-service case, explicitly labelled. */
        deadline=77;r=pt_wavetable_song_clocked_service(song,&deadline);
        frame=(clock.observed-origin)*48000/clock.frequency;assert(frame>=9600);
        if(mode)assert(frame>9600);
        if(frame>9600) {
            assert(r==PT_WAVETABLE_SONG_DEADLINE && deadline==77 && !pins(f));
            assert(!bus->starts && !bus->controls && !bus->restores);
        }else assert(r==PT_WAVETABLE_SONG_WAITING && bus->starts);
        printf("NATIVE SONG GATE observed frame=%lu start=9600 result=%u starts=%u injected_delay=%u\n",
            (unsigned long)frame,(unsigned)r,bus->starts,mode);
        assert(pt_wavetable_song_close(&song) && !pins(f));
        assert(pt_amigus_reservation_close(&f->reservation));pt_sampler_release(&sampler);
        d.project.samples[0].pcm=(struct pt_pcm){data,8,8,48000,1,24};
        guard=0;while(!pt_native_alarm_close(&watchdog)){assert(++guard<=8);Delay(1);}
        assert(pt_native_alarm_close(&alarm));
    }
    pt_native_eclock_close(&clock.clock);pt_document_release(&d);free(bus);free(f);
    puts("NATIVE SONG GATE PASS: real clock/alarms, primed sample leases, observed late service refuses all voice callbacks and releases leases; injected voice bus, no audio output");
}
int main(void){int result;native_memory_start();native_eclock_fixture();native_alarm_fixture();native_alarm_signal_fixture();native_song_gate_fixture();result=editor_wavetable_fixture();native_memory_finish();return result;}
