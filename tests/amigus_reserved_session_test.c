#define main reservation_fixture_main
#include "amigus_reservation_test.c"
#undef main
#include <stdlib.h>
#include "amigus_session.h"
static void *allocate_queue(void *c,size_t n) {(void)c;return malloc(n);}
static void free_queue(void *c,void *p) {(void)c;free(p);}
struct reserved_port {struct pt_amigus_reservation *owner;int reset;unsigned writes;};
static void check_access(struct reserved_port *p)
{assert(p->owner->reserved && p->owner->opened && p->owner->access);}
static int port_capacity(void *c) {check_access(c);return 3;}
static int port_write(void *c,const uint32_t *data)
{struct reserved_port *p=c;check_access(p);assert(data);++p->writes;return 1;}
static int port_reset(void *c)
{struct reserved_port *p=c;check_access(p);return p->reset;}
static int port_drain(void *c) {check_access(c);return 1;}
static int reserved_session_fixture_main(void)
{
    unsigned mode;
    assert(reservation_fixture_main()==0);
    for(mode=0;mode<3;++mode) {
        struct fake f={0};struct pt_amigus_reservation r={0};
        struct pt_amigus_reservation_api api={&f,open_library,close_library,find,supported,reserve,release};
        struct pt_allocator allocator={0,allocate_queue,free_queue};
        struct pt_studio_queue *q=pt_studio_queue_open(&allocator,2);
        struct pt_amigus_session s={0};struct reserved_port port={&r,mode==0?0:1,0};
        struct pt_amigus_fifo_port io={&port,port_capacity,port_write,port_reset};
        int32_t samples[4]={1,-1,257,-257};struct pt_pcm pcm={samples,4,2,48000,2,24};
        unsigned i;
        assert(q);f.available=f.supported=f.count=1;
        assert(pt_amigus_reservation_open(&r,&api,0)==PT_AMIGUS_RESERVED);
        assert(pt_amigus_reservation_begin(&r));
        assert(pt_amigus_session_open(&s,q,&io,port_drain,&port)==(mode!=0));
        assert(!pt_amigus_reservation_close(&r));
        if(mode) {
            if(mode==2) {pcm.frames=1;pcm.capacity=2;}
            assert(pt_studio_queue_push(q,&pcm)==PT_QUEUE_OK);
            assert(pt_amigus_session_step(&s)==PT_CONSUMER_PROGRESS);
            if(mode==1) {assert(s.consumer.leased);pt_amigus_session_stop(&s);}
            else {
                pt_studio_queue_finish(q);
                for(i=0;i<12 && s.phase!=PT_AS_RESET;++i)
                    assert(pt_amigus_session_step(&s)!=PT_CONSUMER_ERROR);
                assert(s.phase==PT_AS_RESET && s.padding==1 && port.writes==1);
            }
        }
        port.reset=-1;
        assert(pt_amigus_session_step(&s)==PT_CONSUMER_ERROR);
        assert(!pt_amigus_session_detach(&s));
        assert(!pt_amigus_reservation_close(&r));
        assert(f.owner==&r && f.library && !f.closes && !f.releases);
        port.reset=0;pt_amigus_session_step(&s);
        assert(!pt_amigus_session_detach(&s) && !pt_amigus_reservation_close(&r));
        port.reset=1;assert(pt_amigus_session_step(&s)==PT_CONSUMER_ERROR);
        assert(s.phase==PT_AS_DONE);
        assert(!pt_amigus_reservation_close(&r)); /* lease survives reset itself */
        assert(pt_amigus_session_detach(&s));
        assert(pt_amigus_reservation_end(&r));
        assert(pt_amigus_reservation_close(&r));
        assert(f.releases==1 && f.closes==1 && !f.library);
        assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    puts("AMIGUS RESERVED SESSION PASS: failed open, leased Stop and natural tail retain card through reset recovery");
    return 0;
}

#ifndef PT_RESERVED_SESSION_NATIVE
int main(void) {return reserved_session_fixture_main();}
#endif
