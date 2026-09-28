#define main capture_session_base_fixture
#include "capture_session_test.c"
#undef main
#include "../src/core/amigus_capture.h"
#include "../src/core/amigus_interrupt_owner.h"
struct resources {unsigned opened,reserved,releases,closes,installs,removes;int quiesce;};
static int open_library(void *ctx){struct resources *r=ctx;assert(!r->opened);r->opened=1;return 1;}
static void close_library(void *ctx){struct resources *r=ctx;assert(r->opened && !r->reserved && !device_owned);r->opened=0;++r->closes;}
static void *find_card(void *ctx,void *previous){return previous?NULL:ctx;}
static int supported(void *ctx,void *card,enum pt_amigus_resource block){assert(ctx==card);return block==PT_AMIGUS_PCM || block==PT_AMIGUS_WAVETABLE;}
static unsigned long reserve_card(void *ctx,void *card,enum pt_amigus_resource block,void *owner)
{struct resources *r=ctx;(void)block;assert(card==ctx && owner && r->opened && !r->reserved);r->reserved=1;return 0;}
static void release_card(void *ctx,void *card,enum pt_amigus_resource block,void *owner)
{struct resources *r=ctx;struct pt_amigus_reservation *o=owner;(void)block;assert(card==ctx && r->reserved && !o->access && !o->interrupt && !device_owned);r->reserved=0;++r->releases;}
static unsigned long install(void *ctx,struct pt_amigus_reservation *r,void *binding)
{struct resources *f=ctx;assert(r->access && r->interrupt && binding);++f->installs;return 0;}
static void remove_irq(void *ctx,struct pt_amigus_reservation *r)
{struct resources *f=ctx;assert(r->access && r->interrupt);++f->removes;}
static int quiesce_irq(void *ctx,struct pt_amigus_reservation *r,void *binding)
{struct resources *f=ctx;assert(r->access && r->interrupt && binding);return f->quiesce;}
static struct pt_amigus_reservation_api api(struct resources *r)
{struct pt_amigus_reservation_api a={r,open_library,close_library,find_card,supported,reserve_card,release_card};return a;}
static void reservation_refusals(void)
{
    struct resources f={0};struct fake in={0};struct pt_capture_input p=input(&in);
    struct pt_amigus_reservation r={0};struct pt_amigus_capture o={0},other={0};struct pt_amigus_reservation_api a=api(&f);
    assert(!pt_amigus_capture_open(&o,&r,&p,&allocator,24,2,48000,2,16));
    assert(pt_amigus_reservation_open_resource(&r,&a,0,PT_AMIGUS_WAVETABLE)==PT_AMIGUS_RESERVED);
    assert(!pt_amigus_capture_open(&o,&r,&p,&allocator,24,2,48000,2,16));
    assert(!r.access && pt_amigus_reservation_close(&r));
    assert(pt_amigus_reservation_open(&r,&a,0)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_reservation_begin(&r));
    assert(!pt_amigus_capture_open(&o,&r,&p,&allocator,24,2,48000,2,16));
    assert(r.access && pt_amigus_reservation_end(&r));
    fail_allocate=1;assert(!pt_amigus_capture_open(&o,&r,&p,&allocator,24,2,48000,2,16));fail_allocate=0;
    assert(!r.access && !o.reservation && !live && !in.starts);
    assert(!pt_amigus_capture_open(&o,&r,&p,&allocator,32,2,48000,2,16) && !r.access);
    assert(pt_amigus_capture_open(&o,&r,&p,&allocator,24,2,48000,2,16));
    assert(!pt_amigus_capture_open(&other,&r,&p,&allocator,24,2,48000,2,16));
    assert(!pt_amigus_reservation_begin(&r) && !pt_amigus_reservation_close(&r));
    pt_amigus_capture_abort(&o);in.stop_rc=1;
    assert(pt_amigus_capture_step(&o)==PT_CS_COMPLETE && !r.access && !o.reservation);
    assert(pt_amigus_capture_close(&o) && pt_amigus_capture_close(&other));
    assert(pt_amigus_reservation_close(&r) && f.releases==2 && f.closes==2 && !live);
}
static void retained_interrupt(void)
{
    unsigned mode;
    for(mode=0;mode<4;++mode) {
        struct resources f={0};struct fake in={0};struct pt_capture_input p=input(&in);
        struct pt_amigus_reservation r={0};struct pt_amigus_capture o={0};struct pt_capture c={0};
        struct pt_amigus_reservation_api a=api(&f);struct pt_amigus_interrupt_owner irq={0};
        struct pt_amigus_interrupt_api ia={&f,install,remove_irq,quiesce_irq};size_t before;
        assert(pt_amigus_reservation_open(&r,&a,0)==PT_AMIGUS_RESERVED);
        assert(pt_amigus_capture_open(&o,&r,&p,&allocator,24,2,48000,2,16));
        in.start_rc=in.read_rc=1;in.frames=1;before=allocations;
        assert(pt_amigus_capture_step(&o)==PT_CS_PENDING);
        assert(pt_amigus_interrupt_owner_begin(&irq,&r,&ia,&in)==PT_AMIGUS_INTERRUPT_READY);
        assert(pt_amigus_capture_step(&o)==PT_CS_PENDING);
        if(mode==1)pt_amigus_capture_abort(&o);
        else if(mode==2){in.read_rc=-2;assert(pt_amigus_capture_step(&o)==PT_CS_ERROR);}
        else pt_amigus_capture_finish(&o);
        assert(pt_amigus_capture_step(&o)==PT_CS_PENDING && device_owned && live==1);
        if(mode==3){in.stop_rc=2;assert(pt_amigus_capture_step(&o)==PT_CS_ERROR);}
        in.stop_rc=1;assert(pt_amigus_capture_step(&o)==PT_CS_PENDING);
        assert(!device_owned && r.access && r.interrupt && live==1 && allocations==before);
        assert(!pt_amigus_capture_take(&o,&c) && !pt_amigus_capture_close(&o));
        assert(!pt_amigus_reservation_end(&r) && !pt_amigus_reservation_close(&r));
        f.quiesce=2;assert(!pt_amigus_interrupt_owner_stop(&irq));
        assert(pt_amigus_capture_step(&o)==PT_CS_PENDING && live==1 && !f.releases);
        f.quiesce=0;assert(!pt_amigus_interrupt_owner_stop(&irq));
        f.quiesce=1;assert(pt_amigus_interrupt_owner_stop(&irq) && f.removes==1);
        assert(pt_amigus_interrupt_owner_detach(&irq));
        assert(pt_amigus_capture_step(&o)==(mode>1?PT_CS_ERROR:PT_CS_COMPLETE));
        assert(!r.access && !o.reservation && !r.interrupt && !f.releases);
        if(!mode) {assert(pt_amigus_capture_take(&o,&c));assert(c.pcm.data[0]==-8388608 && c.pcm.frames==1);}
        else assert(!pt_amigus_capture_take(&o,&c) && !live);
        assert(pt_amigus_capture_close(&o) && pt_amigus_reservation_close(&r));
        assert(f.releases==1 && f.closes==1);pt_capture_close(&c);assert(!live);
    }
}
#ifndef AMIGUS_CAPTURE_ENTRY
#define AMIGUS_CAPTURE_ENTRY main
#endif
int AMIGUS_CAPTURE_ENTRY(void)
{
    assert(capture_session_base_fixture()==0);reservation_refusals();retained_interrupt();
    assert(!live && !device_owned);
    puts("AMIGUS CAPTURE PASS: exclusive PCM lease, busy/format/allocation refusal, retained interrupt quiescence, exact stopped recording transfer and resource release; injected only");return 0;
}
