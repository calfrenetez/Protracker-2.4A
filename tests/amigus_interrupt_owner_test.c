#define main reservation_fixture_main
#include "amigus_reservation_test.c"
#undef main
#include "amigus_interrupt_owner.h"
struct irq_fixture {
    struct fake library;
    struct pt_amigus_reservation reservation;
    struct pt_amigus_interrupt_owner owner;
    unsigned installs,removes,polls;
    unsigned long code;
    int quiescence, binding, retained;
};
static unsigned long install_irq(void *context,struct pt_amigus_reservation *r,void *binding)
{
    struct irq_fixture *f=context;
    assert(r==&f->reservation && binding==&f->binding && r->interrupt && r->access);
    assert(!pt_amigus_reservation_end(r) && !pt_amigus_reservation_close(r));
    ++f->installs;f->retained=1;return f->code; /* Partial install even on error. */
}
static void remove_irq(void *context,struct pt_amigus_reservation *r)
{
    struct irq_fixture *f=context;
    assert(r==&f->reservation && f->retained && r->interrupt && r->access);
    ++f->removes; /* A request is not a quiescence acknowledgement. */
}
static int quiesce_irq(void *context,struct pt_amigus_reservation *r,void *binding)
{
    struct irq_fixture *f=context;
    assert(r==&f->reservation && binding==&f->binding && f->removes==1);
    assert(r->interrupt && r->access && !pt_amigus_reservation_end(r));
    ++f->polls;if(f->quiescence==1)f->retained=0;return f->quiescence;
}
static void irq_case(enum pt_amigus_resource resource,unsigned long code)
{
    struct irq_fixture f={0};struct pt_amigus_interrupt_owner other={0};
    struct pt_amigus_reservation_api library={&f.library,open_library,close_library,find,supported,reserve,release};
    struct pt_amigus_interrupt_api api={&f,install_irq,remove_irq,quiesce_irq};
    unsigned k;const int pending[]={0,-1,2,0};
    f.library.available=f.library.supported=f.library.count=1;f.code=code;
    assert(pt_amigus_reservation_open_resource(&f.reservation,&library,0,resource)==PT_AMIGUS_RESERVED);
    assert(pt_amigus_interrupt_owner_begin(&f.owner,&f.reservation,&api,&f.binding)==PT_AMIGUS_INTERRUPT_INVALID);
    assert(!f.installs && !f.reservation.interrupt);
    assert(pt_amigus_reservation_begin(&f.reservation));
    api.quiesce=NULL;
    assert(pt_amigus_interrupt_owner_begin(&f.owner,&f.reservation,&api,&f.binding)==PT_AMIGUS_INTERRUPT_INVALID);
    assert(!f.installs && !f.reservation.interrupt);api.quiesce=quiesce_irq;
    assert(pt_amigus_interrupt_owner_begin(&f.owner,&f.reservation,&api,NULL)==PT_AMIGUS_INTERRUPT_INVALID);
    assert(pt_amigus_interrupt_owner_begin(&f.owner,&f.reservation,&api,&f.binding)==
           (code?PT_AMIGUS_INTERRUPT_FAILED:PT_AMIGUS_INTERRUPT_READY));
    assert(f.installs==1 && f.owner.install_code==code && f.retained);
    assert(pt_amigus_interrupt_owner_begin(&f.owner,&f.reservation,&api,&f.binding)==PT_AMIGUS_INTERRUPT_INVALID);
    assert(pt_amigus_interrupt_owner_begin(&other,&f.reservation,&api,&f.binding)==PT_AMIGUS_INTERRUPT_INVALID);
    for(k=0;k<4;++k) {
        f.quiescence=pending[k];assert(!pt_amigus_interrupt_owner_stop(&f.owner));
        assert(f.removes==1 && f.polls==k+1 && f.retained && f.reservation.interrupt);
        assert(!pt_amigus_interrupt_owner_detach(&f.owner));
        assert(!pt_amigus_reservation_end(&f.reservation) && !pt_amigus_reservation_close(&f.reservation));
    }
    f.quiescence=1;assert(pt_amigus_interrupt_owner_stop(&f.owner));
    assert(!f.retained && !f.reservation.interrupt && f.reservation.access);
    assert(pt_amigus_interrupt_owner_stop(&f.owner) && f.removes==1 && f.polls==5);
    assert(!pt_amigus_reservation_close(&f.reservation));
    assert(pt_amigus_interrupt_owner_detach(&f.owner) && !f.owner.reservation);
    assert(pt_amigus_interrupt_owner_detach(&f.owner));
    assert(pt_amigus_reservation_end(&f.reservation) && pt_amigus_reservation_close(&f.reservation));
    assert(f.library.closes==1 && f.library.releases==1);
}
#ifndef PT_AMIGUS_INTERRUPT_OWNER_MAIN
#define PT_AMIGUS_INTERRUPT_OWNER_MAIN main
#endif
int PT_AMIGUS_INTERRUPT_OWNER_MAIN(void)
{
    unsigned resource;assert(reservation_fixture_main()==0);
    for(resource=1;resource<=2;++resource) {
        irq_case((enum pt_amigus_resource)resource,0);
        irq_case((enum pt_amigus_resource)resource,0xf1234567UL);
    }
    puts("AMIGUS INTERRUPT OWNER PASS: partial install retains reservation, removal request is not quiescence, pending/error/invalid status refuses release; injected callbacks only");
    return 0;
}
