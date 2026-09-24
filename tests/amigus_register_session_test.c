#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/core/amigus_session.h"
#include "../src/core/amigus_register_port.h"
static unsigned live;
static void *alloc(void *c,size_t n) {void *p;(void)c;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;assert(live);--live;free(p);}
struct bus {uint16_t rate,mask,irq,used;unsigned reset_delay,resets,writes,fail_write,owned;uint32_t words[6];};
static int owned(void *v) {return ((struct bus *)v)->owned;}
static int read16(void *v,unsigned a,uint16_t *x) {struct bus *b=v;*x=a==6?b->rate:a==2?b->mask:b->used;return 1;}
static int write16(void *v,unsigned a,uint16_t x) {
    struct bus *b=v;
    if(a==6)b->rate=x;
    else if(a==0)b->irq=x&0x8000?b->irq|(x&0x7fff):b->irq&~x;
    else if(a==2)b->mask=x&0x8000?b->mask|(x&0x7fff):b->mask&~x;
    else {assert(a==8 && !x);++b->resets;if(b->reset_delay)--b->reset_delay;else b->used=0;}
    return 1;
}
static int write32(void *v,unsigned a,uint32_t x) {
    struct bus *b=v;assert(a==12 && b->used+2<=12 && b->writes<6);
    b->words[b->writes++]=x;b->used+=2;return b->writes!=b->fail_write;
}
int main(void)
{
    struct pt_allocator a={NULL,alloc,release};int32_t data[6]={1,257,-513,1025,-1,8388607};
    struct pt_pcm pcm={data,6,3,48000,2,24};unsigned mode;
    for(mode=0;mode<3;++mode) {
        struct bus b={0};struct pt_amigus_register_port registers;struct pt_amigus_session session={0};
        struct pt_amigus_register_io io={&b,owned,read16,write16,write32};
        struct pt_amigus_fifo_port port={&registers,pt_amigus_register_capacity,pt_amigus_register_write3,pt_amigus_register_reset};
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);unsigned i;
        b.owned=1;b.rate=0x8007;b.mask=b.irq=0x17;
        assert(q && pt_amigus_register_port_init(&registers,&io,12));
        assert(pt_amigus_session_open(&session,q,&port,pt_amigus_register_drain,&registers));
        assert(!b.rate && b.mask==0x10 && b.irq==0x10); /* preserve capture bits */
        assert(pt_studio_queue_push(q,&pcm)==PT_QUEUE_OK);pt_studio_queue_finish(q);
        assert(pt_amigus_session_step(&session)==PT_CONSUMER_PROGRESS);
        if(mode) {
            b.fail_write=2;assert(pt_amigus_session_step(&session)==PT_CONSUMER_ERROR);
            assert(b.used==4 && !pt_amigus_session_detach(&session));
            if(mode==2) {b.owned=0;i=b.resets;assert(pt_amigus_session_step(&session)==PT_CONSUMER_ERROR && b.resets==i);b.owned=1;}
            b.reset_delay=2;
            for(i=0;i<2;++i) {assert(pt_amigus_session_step(&session)==PT_CONSUMER_ERROR);assert(!pt_amigus_session_detach(&session));assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);}
            assert(pt_amigus_session_step(&session)==PT_CONSUMER_ERROR && session.phase==PT_AS_DONE);
            assert(!b.used && b.writes==2);
        } else {
            for(i=0;i<10 && session.phase!=PT_AS_DRAIN;++i)assert(pt_amigus_session_step(&session)!=PT_CONSUMER_ERROR);
            assert(session.phase==PT_AS_DRAIN && b.used==12 && b.writes==6 && session.padding==1);
            assert(pt_amigus_session_step(&session)==PT_CONSUMER_WAIT);
            b.used=1;assert(pt_amigus_session_step(&session)==PT_CONSUMER_WAIT);
            b.used=0;assert(pt_amigus_session_step(&session)==PT_CONSUMER_PROGRESS);
            assert(pt_amigus_session_step(&session)==PT_CONSUMER_FINISHED && b.resets==2);
            for(i=0;i<24;++i) {unsigned want=i<18?((uint32_t)data[i/3]>>(16-(i%3)*8))&255:0;
                assert(((b.words[i/4]>>(24-(i%4)*8))&255)==want);}
        }
        assert(pt_amigus_session_detach(&session));assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    assert(!live);puts("AMIGUS REGISTER SESSION PASS: exact packed stream, IRQ bit isolation, drain wait, partial-write/reset delays, ownership loss cleanup");return 0;
}
