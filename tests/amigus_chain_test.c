#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/amigus_fifo.h"
#include "../src/core/studio_consumer.h"
static unsigned live;
static void *allocate(void *c,size_t n) {void *p;(void)c;p=malloc(n);if(p)++live;return p;}
static void release(void *c,void *p) {(void)c;assert(live);--live;free(p);}
struct output {uint32_t words[800];unsigned n,reads;int reset,fail;};
static int space(void *v) {struct output *o=v;return ++o->reads%4?3:2;}
static int write3(void *v,const uint32_t *p) {struct output *o=v;if(o->fail)return -1;assert(o->n+3<=800);memcpy(o->words+o->n,p,12);o->n+=3;return 1;}
static int reset(void *v) {return ((struct output *)v)->reset;}
int main(void)
{
    int32_t data[514];unsigned i,partition;
    struct pt_allocator a={NULL,allocate,release};
    for(i=0;i<514;++i)data[i]=(int32_t)(i*7919)-2000000;
    for(partition=1;partition<=256;partition=partition==1?17:partition==17?256:257) {
        struct output out={0};struct pt_amigus_fifo fifo={0};struct pt_studio_consumer consumer={0};
        struct pt_amigus_fifo_port port={&out,space,write3,reset};
        struct pt_studio_transport transport={&fifo,pt_amigus_fifo_submit,pt_amigus_fifo_poll,pt_amigus_fifo_cancel};
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);unsigned at=0,steps=0,padding;int finished=0;
        out.reset=1;assert(q && pt_amigus_fifo_init(&fifo,&port));assert(pt_studio_consumer_attach(&consumer,q,&transport));
        while(!finished) {
            enum pt_consumer_result r;assert(++steps<10000);
            if(at<257) {
                unsigned n=257-at<partition?257-at:partition;struct pt_pcm p={data+at*2,n*2,n,48000,2,24};
                enum pt_queue_result qr=pt_studio_queue_push(q,&p);
                assert(qr==PT_QUEUE_OK || qr==PT_QUEUE_FULL);if(qr==PT_QUEUE_OK)at+=n;
                if(at==257)pt_studio_queue_finish(q);
            }
            r=pt_studio_consumer_step(&consumer);assert(r!=PT_CONSUMER_ERROR);finished=r==PT_CONSUMER_FINISHED;
        }
        assert(fifo.pack.held && pt_amigus_fifo_finish(&fifo,&padding)==1 && padding==1);
        while(pt_amigus_fifo_poll(&fifo)!=1)assert(++steps<10000);
        assert(out.n==387);
        for(i=0;i<out.n*4;++i) {
            unsigned want=i<514*3?((uint32_t)data[i/3]>>(16-(i%3)*8))&255:0;
            assert(((out.words[i/4]>>(24-(i%4)*8))&255)==want);
        }
        assert(pt_studio_consumer_detach(&consumer));assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
        /* Host completion does not reset/drain hardware; session must do so. */
        assert(pt_amigus_fifo_cancel(&fifo)==1 && !fifo.pack.held);
    }
    {struct output out={0};struct pt_amigus_fifo fifo={0};struct pt_studio_consumer consumer={0};
        struct pt_amigus_fifo_port port={&out,space,write3,reset};
        struct pt_studio_transport transport={&fifo,pt_amigus_fifo_submit,pt_amigus_fifo_poll,pt_amigus_fifo_cancel};
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);struct pt_pcm p={data,6,3,48000,2,24};
        out.reset=1;assert(q && pt_amigus_fifo_init(&fifo,&port));assert(pt_studio_consumer_attach(&consumer,q,&transport));
        assert(pt_studio_queue_push(q,&p)==PT_QUEUE_OK);assert(pt_studio_consumer_step(&consumer)==PT_CONSUMER_PROGRESS);
        out.fail=1;assert(pt_studio_consumer_step(&consumer)==PT_CONSUMER_ERROR);
        out.reset=0;assert(pt_studio_consumer_stop(&consumer)==PT_CONSUMER_ERROR);
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY && !pt_studio_consumer_detach(&consumer));
        out.reset=-1;assert(!pt_studio_consumer_detach(&consumer));
        out.reset=1;assert(pt_studio_consumer_detach(&consumer));
        assert(!fifo.pack.held && !fifo.count && pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    assert(!live);puts("AMIGUS CHAIN PASS: queue consumer packed byte equivalence, odd partitions, explicit tail finish, failed reset lease retention");return 0;
}
