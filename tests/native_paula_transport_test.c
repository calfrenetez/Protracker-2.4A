/* Independent private timer mocks around the actual prepared audio owner. */
#define main audio_fixture_main
#define CreateMsgPort audio_CreateMsgPort
#define DeleteMsgPort audio_DeleteMsgPort
#define CreateIORequest audio_CreateIORequest
#define DeleteIORequest audio_DeleteIORequest
#define OpenDevice audio_OpenDevice
#define CloseDevice audio_CloseDevice
#define CheckIO audio_CheckIO
#define WaitIO audio_WaitIO
#define AbortIO audio_AbortIO
#include "paula_output_test.c"
#undef main
#undef CreateMsgPort
#undef DeleteMsgPort
#undef CreateIORequest
#undef DeleteIORequest
#undef OpenDevice
#undef CloseDevice
#undef CheckIO
#undef WaitIO
#undef AbortIO
#include "../src/native/paula_pump.h"
struct Task {unsigned identity;};
static struct Task own_task={1},foreign_task={2};static struct Task *current_task=&own_task;
static struct Device timer={{36}};
static struct IORequest *alarm_io[2];
static unsigned alarm_ready[2];
static unsigned alarm_slot(struct IORequest *q)
{unsigned i;for(i=0;i<2;++i)if(alarm_io[i]==q)return i;assert(0);return 0;}
static int alarm_live(struct IORequest *q){return alarm_io[0]==q || alarm_io[1]==q;}
static unsigned timer_ports,timer_requests,timer_opens,timer_reads,timer_sends,timer_waits,timer_aborts;
static unsigned timer_hold_abort,timer_fail_open;
static unsigned timer_jump_read;static uint64_t timer_jump_ticks;
static uint64_t timer_now,timer_read_cost;static ULONG timer_rate=48000;
struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));assert(p);p->mp_SigBit=6+timer_ports;p->mp_SigTask=current_task;++timer_ports;return p;}
void DeleteMsgPort(struct MsgPort *p){assert(timer_ports);--timer_ports;free(p);}
struct IORequest *CreateIORequest(struct MsgPort *p,ULONG size){struct IORequest *q=calloc(1,size);assert(q && p);++timer_requests;return q;}
void DeleteIORequest(struct IORequest *q){assert(timer_requests && !alarm_live(q));--timer_requests;free(q);}
LONG OpenDevice(const char *name,ULONG unit,struct IORequest *q,ULONG flags)
{assert(!strcmp(name,TIMERNAME) && (unit==UNIT_ECLOCK || unit==UNIT_WAITECLOCK) && !flags);if(timer_fail_open==unit)return -1;q->io_Device=&timer;++timer_opens;return 0;}
void CloseDevice(struct IORequest *q){assert(q->io_Device==&timer && timer_opens && !alarm_live(q));--timer_opens;}
void SendIO(struct IORequest *q){unsigned i;assert(q->io_Device==&timer);for(i=0;i<2 && alarm_io[i];++i){}assert(i<2);alarm_io[i]=q;alarm_ready[i]=0;++timer_sends;}
struct IORequest *CheckIO(struct IORequest *q){unsigned i=alarm_slot(q);struct timerequest *r=(struct timerequest *)q;uint64_t deadline=((uint64_t)r->tr_time.tv_secs<<32)|r->tr_time.tv_micro;return alarm_ready[i] || (!q->io_Error && timer_now>=deadline)?q:NULL;}
LONG WaitIO(struct IORequest *q){unsigned i=alarm_slot(q);assert(CheckIO(q));alarm_io[i]=NULL;++timer_waits;return q->io_Error;}
LONG AbortIO(struct IORequest *q){unsigned i=alarm_slot(q);assert(!CheckIO(q));++timer_aborts;q->io_Error=IOERR_ABORTED;if(!timer_hold_abort)alarm_ready[i]=1;return 0;}
ULONG fake_read(struct Device *d,struct EClockVal *v){assert(d==&timer && timer_opens);++timer_reads;v->ev_hi=(ULONG)(timer_now>>32);v->ev_lo=(ULONG)timer_now;timer_now+=timer_read_cost;if(timer_reads==timer_jump_read)timer_now+=timer_jump_ticks;return timer_rate;}
static ULONG pending_signals,wait_return,wait_mask;static unsigned wait_calls,wait_change_task;
static uint64_t wait_advance;
struct Task *FindTask(const char *name){assert(!name);return current_task;}
ULONG SetSignal(ULONG signals,ULONG mask){assert(!signals && !mask);return pending_signals;}
ULONG Wait(ULONG mask){assert(mask);wait_mask=mask;++wait_calls;if(wait_advance==UINT64_MAX) {
    uint64_t earliest=UINT64_MAX;unsigned i;
    for(i=0;i<2;++i)if(alarm_io[i]){struct timerequest *q=(struct timerequest *)alarm_io[i];uint64_t target=((uint64_t)q->tr_time.tv_secs<<32)|q->tr_time.tv_micro;if(target<earliest)earliest=target;}
    assert(earliest>=timer_now && earliest!=UINT64_MAX);timer_now=earliest;
}else timer_now+=wait_advance;if(wait_change_task)current_task=&foreign_task;return wait_return;}
static unsigned chip_owned;
ULONG AvailMem(ULONG f){assert(f==MEMF_CHIP);return 512UL*1024+32;}
void *AllocMem(ULONG size,ULONG f){assert(size==32 && f==(MEMF_CHIP|MEMF_PUBLIC) && !chip_owned);chip_owned=1;return chip.bytes;}
void FreeMem(void *p,ULONG size){assert(p==chip.bytes && size==32 && chip_owned && !(dma&15));chip_owned=0;}
static void *fast_alloc(void *c,size_t n){(void)c;return malloc(n);}
static void fast_free(void *c,void *p){(void)c;free(p);}
static void finish(struct pt_native_paula_transport *t,struct pt_editor *ed,struct pt_document *doc)
{unsigned i;for(i=0;i<20 && !pt_editor_paula_stop(&t->native.binding);++i){}assert(i<20);assert(pt_editor_paula_detach(&t->native.binding));assert(pt_editor_dispose(ed));pt_document_release(doc);free(ed);assert(!live && !chip_owned && !timer_opens && !timer_ports && !timer_requests && !alarm_io[0] && !alarm_io[1]);}
static void fixture(unsigned mode)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *ed=malloc(sizeof(*ed));struct pt_native_paula_transport t={0};
    struct pt_render_options o={0};int32_t pcm[32]={0};unsigned i,n,initial_reads=timer_reads,initial_sends=timer_sends;enum pt_paula_song_result r;
    reset();current_task=&own_task;pending_signals=wait_return=wait_mask=wait_calls=wait_change_task=0;wait_advance=0;timer_jump_read=0;timer_jump_ticks=0;timer_read_cost=0;timer_now=1000;timer_rate=mode==5?700001:48000;timer_fail_open=0;timer_hold_abort=0;assert(ed);
    pt_document_init(&doc,&a);assert(pt_document_new(&doc,4,SIZE_MAX)==PT_PROJECT_OK);
    doc.project.samples[0].pcm=(struct pt_pcm){pcm,32,32,8000,1,24};doc.project.samples[0].volume=64;
    doc.project.events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};
    doc.project.events[12].effect=15;
    assert(pt_editor_init(ed,&doc.project));ed->sampler.allocator=a;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    assert(pt_native_paula_transport_attach(&t,ed));assert(!pt_native_paula_transport_attach(&t,ed));
    assert(pt_native_paula_transport_begin(&t,&o,32)==PT_PAULA_SONG_PREPARING);
    i=0;do{r=pt_native_editor_paula_advance(&t.native,NULL);assert(++i<100);}while(r==PT_PAULA_SONG_PREPARING);assert(r==PT_PAULA_SONG_OK);
    if(mode==1)timer_fail_open=UNIT_WAITECLOCK;
    i=0;do {
        r=pt_native_paula_transport_start(&t,1000);assert(++i<2000);
        if(r==PT_PAULA_SONG_PREPARING) {
            assert(!timer_opens && !timer_ports && timer_reads==initial_reads && timer_sends==initial_sends && !(dma&15));
            assert(!t.started && !t.service_clock_ready);
        }
    }while(r==PT_PAULA_SONG_PREPARING);

    if(mode==1){assert(r==PT_PAULA_SONG_CLOCK && !t.started);finish(&t,ed,&doc);return;}
    assert(r==PT_PAULA_SONG_WAITING && timer_sends && !t.can_wait && !pt_native_paula_transport_wait_mask(&t) && pt_native_paula_transport_signal(&t)==((1UL<<7)|(1UL<<8)));
    n=timer_sends;for(i=0;i<100;++i){r=pt_native_paula_transport_service(&t);assert(r==PT_PAULA_SONG_WAITING || r==PT_PAULA_SONG_OK);}
    assert(!t.native.engine.output.held[0] && timer_sends==n);
    assert(t.can_wait && pt_native_paula_transport_wait_mask(&t)==((1UL<<7)|(1UL<<8)));
    if(mode>=13) {
        struct pt_native_paula_pump pump={0};enum pt_native_pump_result pr;unsigned reads,waits,aborts;ULONG abort=1UL<<12;
        assert(pt_native_paula_pump_close(&pump));
        current_task=&foreign_task;assert(!pt_native_paula_pump_bind(&pump,&t) && !pump.active && !t.pump);current_task=&own_task;
        assert(pt_native_paula_pump_bind(&pump,&t) && !pt_native_paula_pump_bind(&pump,&t));
        {struct pt_native_paula_pump duplicate={0},copy=pump;
            assert(!pt_native_paula_pump_bind(&duplicate,&t));
            assert(!pt_native_paula_pump_close(&copy) && pt_native_paula_pump_step(&copy,1UL<<12)==PT_NATIVE_PUMP_INVALID);
            assert(pt_native_paula_transport_begin(&t,&o,32)==PT_PAULA_SONG_INVALID && t.pump==&pump);
        }
        reads=timer_reads;assert(pt_native_paula_pump_step(&pump,0)==PT_NATIVE_PUMP_INVALID);
        assert(pt_native_paula_pump_step(&pump,1UL<<7)==PT_NATIVE_PUMP_INVALID && timer_reads==reads && !wait_calls);
        if(mode==13) {
            wait_return=1UL<<8;wait_advance=128;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_WAKE && timer_reads==reads+3 && timer_now==1128);
            assert(wait_mask==((1UL<<7)|(1UL<<8)|abort));
            wait_advance=0;reads=timer_reads;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_WAKE && timer_reads==reads+4);
        }else if(mode==14) {
            timer_now=2000;assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_DEADLINE);
            reads=timer_reads;
            pr=pt_native_paula_pump_step(&pump,abort);
            assert((pr==PT_NATIVE_PUMP_FAILED || pr==PT_NATIVE_PUMP_HOLD) && !wait_calls && timer_reads==reads);
        }else if(mode==15) {
            pending_signals=abort;reads=timer_reads;
            pr=pt_native_paula_pump_step(&pump,abort);
            assert((pr==PT_NATIVE_PUMP_STOPPED || pr==PT_NATIVE_PUMP_HOLD) && timer_reads==reads && !wait_calls);
        }else if(mode==16) {
            wait_return=abort;pr=pt_native_paula_pump_step(&pump,abort);
            assert((pr==PT_NATIVE_PUMP_STOPPED || pr==PT_NATIVE_PUMP_HOLD) && wait_calls==1);
        }else if(mode==17) {
            current_task=&foreign_task;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_INVALID && !pt_native_paula_pump_close(&pump));
            assert(timer_reads==reads && !wait_calls && pump.active);
            current_task=&own_task;wait_change_task=1;wait_return=1UL<<8;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_HOLD && pump.active && pump.stopping && t.clock.opened);
            assert(!pt_native_paula_pump_close(&pump));current_task=&own_task;
            reads=timer_reads;assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_HOLD && timer_reads==reads);
        }else if(mode==18) {
            pending_signals=abort;timer_hold_abort=1;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_HOLD && pump.active && pump.stopping && t.clock.opened);
            reads=timer_reads;aborts=timer_aborts;waits=wait_calls;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_HOLD && timer_reads==reads && timer_aborts==aborts && wait_calls==waits);
            assert(!pt_editor_dispose(ed));timer_hold_abort=0;
            alarm_ready[alarm_slot((struct IORequest *)t.service_alarm.request)]=1;
        }else if(mode==19) {
            wait_return=1UL<<8;wait_advance=300;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_WAKE);
            waits=wait_calls;reads=timer_reads;
            pr=pt_native_paula_pump_step(&pump,abort);
            assert((pr==PT_NATIVE_PUMP_FAILED || pr==PT_NATIVE_PUMP_HOLD) && wait_calls==waits && timer_reads==reads+1);
            assert(pump.last==PT_PAULA_SONG_DEADLINE);
        }else if(mode==20) {
            for(timer_now=1128;timer_now<2000;timer_now+=128)assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_OK);
            timer_now=2000;reads=timer_reads;
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_WORK && !wait_calls && timer_reads==reads+4);
            i=0;do{assert(++i<100);pr=pt_native_paula_pump_step(&pump,abort);}while(pr==PT_NATIVE_PUMP_WORK);
            assert(pr==PT_NATIVE_PUMP_WAKE && wait_calls==1 && t.native.engine.output.held[0]);
            hold_dma=1;assert(!pt_native_paula_pump_close(&pump) && chip_owned && pump.active);
            hold_dma=0;dma&=~15U;
        }else if(mode==21) {
            wait_return=1UL<<8;wait_advance=UINT64_MAX;i=0;
            do{assert(++i<10000);pr=pt_native_paula_pump_step(&pump,abort);}while(pr==PT_NATIVE_PUMP_WORK || pr==PT_NATIVE_PUMP_WAKE);
            assert((pr==PT_NATIVE_PUMP_DONE || pr==PT_NATIVE_PUMP_HOLD) && pump.last==PT_PAULA_SONG_DONE && wait_calls>100);
        }
        waits=wait_calls;reads=timer_reads;
        for(i=0;i<20 && !pt_native_paula_pump_close(&pump);++i) {
            assert(pump.active && pump.stopping && pump.transport==&t);
            assert(pt_native_paula_pump_step(&pump,abort)==PT_NATIVE_PUMP_HOLD);
        }
        assert(i<20 && !pump.active && !pump.transport && !t.pump && !t.clock.port && wait_calls==waits && timer_reads==reads);
        assert(pt_native_paula_pump_close(&pump));finish(&t,ed,&doc);return;
    }
    if(mode==12) {
        for(timer_now=1128;timer_now<2000;timer_now+=128) {
            r=pt_native_paula_transport_service(&t);assert(r==PT_PAULA_SONG_OK && t.can_wait);
        }
        timer_now=2000;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_WAITING && !t.can_wait && !pt_native_paula_transport_wait_mask(&t));
        n=timer_reads;i=0;do {
            assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_WAITING && ++i<100);
        }while(!t.can_wait);
        assert(timer_reads==n+3*i && pt_native_paula_transport_wait_mask(&t));
        timer_hold_abort=1;
        assert(!pt_editor_prepare_change(ed) && !pt_native_paula_transport_wait_mask(&t));
        alarm_ready[alarm_slot((struct IORequest *)t.service_alarm.request)]=1;
        timer_hold_abort=0;finish(&t,ed,&doc);assert(!t.can_wait && !pt_native_paula_transport_wait_mask(&t));return;
    }
    if(mode==9) {
        timer_now=1128;timer_jump_read=timer_reads+3;timer_jump_ticks=300;n=timer_reads;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_DEADLINE && timer_reads==n+4);
        assert(t.failure_phase==PT_NATIVE_PAULA_POST && t.observation_mask==7);
        assert(t.entry_ticks==1128 && t.core_ticks==1128 && t.post_ticks==1428 && t.notification_deadline==1128);
        finish(&t,ed,&doc);
        assert(t.observation_mask==7 && t.entry_ticks==1128 && t.core_ticks==1128 && t.post_ticks==1428);return;
    }
    if(mode==8) {
        /* Early rearm succeeds, but core work still exceeds the256 watchdog.
         * The pending notification cannot waive the fresh post-work refusal. */
        unsigned reads=timer_reads;
        timer_now=1128;timer_read_cost=90;n=timer_sends;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_DEADLINE);
        assert(timer_reads==reads+4 && t.observation_mask==7);
        assert(t.entry_ticks==1128 && t.core_ticks==1308 && t.post_ticks==1398 && t.notification_deadline==1128);
        assert(t.failure_phase==PT_NATIVE_PAULA_POST && t.failure_frames==398 && t.failure_last_frames==128);
        assert(timer_sends==n+1 && !t.native.engine.output.held[0]);
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_INVALID);
        timer_read_cost=0;finish(&t,ed,&doc);
        assert(t.failure_phase==PT_NATIVE_PAULA_POST && t.failure_frames==398);return;
    }
    if(mode==10) {
        /* Core work ends close enough to the grid that post-work rearm would
         * already be late. Early arm keeps the exact1256 target; no skipped grid. */
        timer_now=1128;timer_read_cost=2;timer_jump_read=timer_reads+3;timer_jump_ticks=120;n=timer_reads;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_OK && timer_reads==n+4);
        assert(t.entry_ticks==1128 && t.core_ticks==1132 && t.post_ticks==1254 && t.observation_mask==7);
        assert(t.service_deadline==1256 && t.service_alarm.observed_ticks==1130 && t.service_alarm.pending);
        timer_read_cost=0;timer_jump_read=0;timer_now=1256;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_OK && t.service_deadline==1384);
        finish(&t,ed,&doc);return;
    }
    if(mode==11) {
        /* Even early arm refuses if the actual counter crosses its exact target;
         * no core clock read or output follows the refusal. */
        timer_now=1255;timer_read_cost=2;n=timer_reads;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_DEADLINE && timer_reads==n+2);
        assert(t.failure_phase==PT_NATIVE_PAULA_PERIODIC_ARM && t.observation_mask==1);
        assert(t.service_alarm.observed_ticks==1257 && t.service_alarm.attempted_deadline==1256);
        assert(!t.native.engine.output.held[0]);timer_read_cost=0;finish(&t,ed,&doc);return;
    }
    if(mode==5) {
        assert(t.service_deadline==1000+(128ULL*timer_rate+47999)/48000);
        timer_now=t.service_deadline;
        r=pt_native_paula_transport_service(&t);
        assert(r==PT_PAULA_SONG_WAITING || r==PT_PAULA_SONG_OK);
        assert(t.service_deadline==1000+(256ULL*timer_rate+47999)/48000);
        assert(!t.native.engine.output.held[0]);finish(&t,ed,&doc);return;
    }
    if(mode==6 || mode==7) {
        if(mode==6)++timer_rate;else --timer_now;
        n=timer_reads;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_CLOCK);
        assert(timer_reads==n+1 && t.observation_mask==1);
        assert(!t.native.engine.output.held[0]);finish(&t,ed,&doc);return;
    }
    if(mode==3) {
        for(timer_now=1128;timer_now<2000;timer_now+=128) {
            r=pt_native_paula_transport_service(&t);assert(r==PT_PAULA_SONG_WAITING || r==PT_PAULA_SONG_OK);
            assert(!t.native.engine.output.held[0]);
        }
        timer_now=2000;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_WAITING);
        assert(t.native.engine.output.held[0] && chip_owned && (dma&15)==1);
        for(i=0;i<32;++i)assert(!chip.bytes[i]);
        n=timer_aborts;hold_dma=1;assert(!pt_editor_prepare_change(ed) && t.native.binding.song && t.clock.opened);
        assert(!pt_editor_dispose(ed) && chip_owned && timer_aborts==n);
        hold_dma=0;dma&=~15U;finish(&t,ed,&doc);return;
    }
    if(mode==2) {
        timer_hold_abort=1;
        for(i=0;i<10 && !t.service_alarm.cancelling;++i)assert(!pt_editor_prepare_change(ed));
        assert(i<10);
        assert(t.native.binding.release_pending && !t.native.binding.song && t.clock.opened && t.alarm.pending);
        n=timer_aborts;assert(!pt_editor_dispose(ed) && !pt_editor_paula_detach(&t.native.binding) && timer_aborts==n);
        alarm_ready[alarm_slot((struct IORequest *)t.service_alarm.request)]=1;timer_hold_abort=0;finish(&t,ed,&doc);return;
    }
    timer_now=mode==4?1257:2001; /* Late wakeup never emits a prepared trigger. */
    n=commands;r=pt_native_paula_transport_service(&t);
    assert(r==PT_PAULA_SONG_DEADLINE && !t.native.engine.output.held[0] && commands>=n);
    assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_INVALID);
    finish(&t,ed,&doc);
}
int main(void){fixture(0);fixture(1);fixture(2);fixture(3);fixture(4);fixture(5);fixture(6);fixture(7);fixture(8);fixture(9);fixture(10);fixture(11);fixture(12);fixture(13);fixture(14);fixture(15);fixture(16);fixture(17);fixture(18);fixture(19);fixture(20);fixture(21);puts("NATIVE PAULA TRANSPORT HOST PASS: dual private alarms, fractional periodic grid, exact prepared start, bounded preparation, starvation/clock refusal, partial open and retained DMA/timer-abort editor barrier; task-owned Wait pump, abort, work debt, late wake, terminal/full song and retained close");return 0;}
