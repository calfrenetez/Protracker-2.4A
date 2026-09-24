#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/core/amigus_session.h"
static void *alloc(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
struct port {int reset,drain;unsigned resets,writes;};
static int space(void *c) {(void)c;return 3;}
static int write3(void *c,const uint32_t *p) {struct port *o=c;assert(p);++o->writes;return 1;}
static int reset(void *c) {struct port *o=c;++o->resets;return o->reset;}
static int drain(void *c) {return ((struct port *)c)->drain;}
int main(void)
{
    struct pt_allocator a={NULL,alloc,release};int32_t data[4]={257,-513,1025,-2049};struct pt_pcm pcm={data,2,1,48000,2,24};unsigned mode;
    for(mode=0;mode<6;++mode) {
        struct pt_amigus_session s={0};struct port o={0};struct pt_amigus_fifo_port p={&o,space,write3,reset};
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);unsigned i;
        o.reset=mode==3?0:1;assert(q);
        assert(pt_amigus_session_open(&s,q,&p,drain,&o)==(mode!=3));
        pcm.frames=mode==5?2:1;pcm.capacity=pcm.frames*2;
        if(mode!=3) {
            assert(pt_studio_queue_push(q,&pcm)==PT_QUEUE_OK);
            assert(pt_amigus_session_step(&s)==PT_CONSUMER_PROGRESS);
            if(mode!=5)assert(pt_amigus_session_step(&s)==PT_CONSUMER_PROGRESS);
            if(mode==5)assert(s.consumer.leased);
            else assert(!s.consumer.leased && s.fifo.pack.held); /* no-lease Stop */
            if(mode==0 || mode==2 || mode==4) {
                pt_studio_queue_finish(q);
                for(i=0;i<10 && s.phase!=PT_AS_DRAIN;++i)assert(pt_amigus_session_step(&s)!=PT_CONSUMER_ERROR);
                assert(s.phase==PT_AS_DRAIN && s.padding==1 && o.writes==1);
                assert(pt_amigus_session_step(&s)==PT_CONSUMER_WAIT);
                assert(!pt_amigus_session_detach(&s));
                o.drain=mode==2?-1:1;
                assert(pt_amigus_session_step(&s)==(mode==2?PT_CONSUMER_ERROR:PT_CONSUMER_PROGRESS));
            } else pt_amigus_session_stop(&s);
        }
        if(mode==4) {
            assert(pt_amigus_session_step(&s)==PT_CONSUMER_FINISHED);
            assert(s.phase==PT_AS_DONE && o.resets==2);
            assert(pt_amigus_session_detach(&s));assert(pt_studio_queue_close(q)==PT_QUEUE_OK);continue;
        }
        o.reset=0;pt_amigus_session_step(&s);assert(!pt_amigus_session_detach(&s));
        o.reset=-1;assert(pt_amigus_session_step(&s)==PT_CONSUMER_ERROR);
        assert(!pt_amigus_session_detach(&s));o.reset=1;
        assert(pt_amigus_session_step(&s)==PT_CONSUMER_ERROR); /* sticky diagnostic */
        assert(s.phase==PT_AS_DONE && !s.fifo.pack.held && o.resets>=4);
        assert(pt_amigus_session_detach(&s));assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    puts("AMIGUS SESSION PASS: tail flush, drain acknowledgement, no-lease Stop, failed-open/reset recovery and safe detach");return 0;
}
