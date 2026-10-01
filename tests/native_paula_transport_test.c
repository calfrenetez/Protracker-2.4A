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
#include "../src/native/editor_paula_transport.h"
static struct Device timer={{36}};
static struct IORequest *alarm_io;
static unsigned timer_ports,timer_requests,timer_opens,timer_reads,timer_sends,timer_waits,timer_aborts;
static unsigned timer_ready,timer_hold_abort,timer_fail_open;
static uint64_t timer_now;static ULONG timer_rate=48000;
struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));assert(p);p->mp_SigBit=6;++timer_ports;return p;}
void DeleteMsgPort(struct MsgPort *p){assert(timer_ports);--timer_ports;free(p);}
struct IORequest *CreateIORequest(struct MsgPort *p,ULONG size){struct IORequest *q=calloc(1,size);assert(q && p);++timer_requests;return q;}
void DeleteIORequest(struct IORequest *q){assert(timer_requests && q!=alarm_io);--timer_requests;free(q);}
LONG OpenDevice(const char *name,ULONG unit,struct IORequest *q,ULONG flags)
{assert(!strcmp(name,TIMERNAME) && (unit==UNIT_ECLOCK || unit==UNIT_WAITECLOCK) && !flags);if(timer_fail_open==unit)return -1;q->io_Device=&timer;++timer_opens;return 0;}
void CloseDevice(struct IORequest *q){assert(q->io_Device==&timer && timer_opens && q!=alarm_io);--timer_opens;}
void SendIO(struct IORequest *q){assert(!alarm_io && q->io_Device==&timer);alarm_io=q;timer_ready=0;++timer_sends;}
struct IORequest *CheckIO(struct IORequest *q){assert(q==alarm_io);return timer_ready?q:NULL;}
LONG WaitIO(struct IORequest *q){assert(q==alarm_io && timer_ready);alarm_io=NULL;++timer_waits;return q->io_Error;}
LONG AbortIO(struct IORequest *q){assert(q==alarm_io && !timer_ready);++timer_aborts;if(!timer_hold_abort){timer_ready=1;q->io_Error=IOERR_ABORTED;}return 0;}
ULONG fake_read(struct Device *d,struct EClockVal *v){assert(d==&timer && timer_opens);++timer_reads;v->ev_hi=(ULONG)(timer_now>>32);v->ev_lo=(ULONG)timer_now;return timer_rate;}
static unsigned chip_owned;
ULONG AvailMem(ULONG f){assert(f==MEMF_CHIP);return 512UL*1024+32;}
void *AllocMem(ULONG size,ULONG f){assert(size==32 && f==(MEMF_CHIP|MEMF_PUBLIC) && !chip_owned);chip_owned=1;return chip.bytes;}
void FreeMem(void *p,ULONG size){assert(p==chip.bytes && size==32 && chip_owned && !(dma&15));chip_owned=0;}
static void *fast_alloc(void *c,size_t n){(void)c;return malloc(n);}
static void fast_free(void *c,void *p){(void)c;free(p);}
static void finish(struct pt_native_paula_transport *t,struct pt_editor *ed,struct pt_document *doc)
{unsigned i;for(i=0;i<20 && !pt_editor_paula_stop(&t->native.binding);++i){}assert(i<20);assert(pt_editor_paula_detach(&t->native.binding));assert(pt_editor_dispose(ed));pt_document_release(doc);free(ed);assert(!live && !chip_owned && !timer_opens && !timer_ports && !timer_requests && !alarm_io);}
static void fixture(unsigned mode)
{
    struct pt_allocator a={NULL,fast_alloc,fast_free};struct pt_document doc;
    struct pt_editor *ed=malloc(sizeof(*ed));struct pt_native_paula_transport t={0};
    struct pt_render_options o={0};int32_t pcm[32]={0};unsigned i,n;enum pt_paula_song_result r;
    reset();timer_now=1000;timer_rate=48000;timer_fail_open=0;timer_hold_abort=0;assert(ed);
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
    r=pt_native_paula_transport_start(&t,1000);
    if(mode==1){assert(r==PT_PAULA_SONG_CLOCK && !t.started);finish(&t,ed,&doc);return;}
    assert(r==PT_PAULA_SONG_WAITING && timer_sends && pt_native_paula_transport_signal(&t)==64);
    n=timer_sends;for(i=0;i<100;++i){r=pt_native_paula_transport_service(&t);assert(r==PT_PAULA_SONG_WAITING || r==PT_PAULA_SONG_OK);}
    assert(!t.native.engine.output.held[0] && timer_sends==n);
    if(mode==3) {
        timer_now=2000;timer_ready=1;
        assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_WAITING);
        assert(t.native.engine.output.held[0] && chip_owned && (dma&15)==1);
        for(i=0;i<32;++i)assert(!chip.bytes[i]);
        n=timer_aborts;hold_dma=1;assert(!pt_editor_prepare_change(ed) && t.native.binding.song && t.clock.opened);
        assert(!pt_editor_dispose(ed) && chip_owned && timer_aborts==n);
        hold_dma=0;dma&=~15U;finish(&t,ed,&doc);return;
    }
    if(mode==2) {
        timer_hold_abort=1;
        for(i=0;i<10 && !t.alarm.cancelling;++i)assert(!pt_editor_prepare_change(ed));
        assert(i<10);
        assert(t.native.binding.release_pending && !t.native.binding.song && t.clock.opened && t.alarm.pending);
        n=timer_aborts;assert(!pt_editor_dispose(ed) && !pt_editor_paula_detach(&t.native.binding) && timer_aborts==n);
        timer_ready=1;alarm_io->io_Error=IOERR_ABORTED;finish(&t,ed,&doc);return;
    }
    timer_now=2001;timer_ready=1; /* Late wakeup never emits a prepared trigger. */
    n=commands;r=pt_native_paula_transport_service(&t);
    assert(r==PT_PAULA_SONG_DEADLINE && !t.native.engine.output.held[0] && commands>=n);
    assert(pt_native_paula_transport_service(&t)==PT_PAULA_SONG_INVALID);
    finish(&t,ed,&doc);
}
int main(void){fixture(0);fixture(1);fixture(2);fixture(3);puts("NATIVE PAULA TRANSPORT HOST PASS: private clocks, exact prepared start, bounded preparation, late refusal, partial open and retained DMA/timer-abort editor barrier");return 0;}
