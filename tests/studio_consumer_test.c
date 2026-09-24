#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/core/studio_consumer.h"
static void *alloc(void *c,size_t n) {(void)c;return malloc(n);}
static void drop(void *c,void *p) {(void)c;free(p);}
struct fake {const struct pt_pcm *held;int accept,poll,cancel;unsigned calls;};
static int submit(void *v,const struct pt_pcm *p) {struct fake *f=v;assert(!f->held);++f->calls;if(f->accept==1)f->held=p;return f->accept;}
static int poll(void *v) {struct fake *f=v;assert(f->held && f->held->data[0]==257);if(f->poll==1)f->held=NULL;return f->poll;}
static int cancel(void *v) {struct fake *f=v;assert(f->held && f->held->data[0]==257);if(f->cancel==1)f->held=NULL;return f->cancel;}
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};int32_t data[2]={257,-513};
    struct pt_pcm pcm={data,2,1,48000,2,24};unsigned mode;
    for(mode=0;mode<6;++mode) {
        struct pt_studio_consumer c={0};struct fake f={0};
        struct pt_studio_transport t={&f,submit,poll,cancel};struct pt_studio_queue *q=pt_studio_queue_open(&a,2);
        assert(q && pt_studio_queue_push(q,&pcm)==PT_QUEUE_OK);
        assert(pt_studio_queue_push(q,&pcm)==PT_QUEUE_OK);pt_studio_queue_finish(q);
        assert(pt_studio_consumer_attach(&c,q,&t));
        assert(!pt_studio_consumer_attach(&c,q,&t));
        assert(pt_studio_consumer_step(&c)==PT_CONSUMER_WAIT);
        assert(pt_studio_consumer_step(&c)==PT_CONSUMER_WAIT && f.calls==2 && !f.held);
        if(mode==0) {assert(pt_studio_consumer_detach(&c));assert(!f.held);}
        else if(mode==1) {f.accept=-1;assert(pt_studio_consumer_step(&c)==PT_CONSUMER_ERROR);assert(pt_studio_consumer_detach(&c));}
        else {
            f.accept=1;assert(pt_studio_consumer_step(&c)==PT_CONSUMER_PROGRESS);
            assert(pt_studio_consumer_step(&c)==PT_CONSUMER_WAIT);
            assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
            if(mode==2) {
                f.poll=1;assert(pt_studio_consumer_step(&c)==PT_CONSUMER_PROGRESS);
                assert(pt_studio_consumer_step(&c)==PT_CONSUMER_PROGRESS);
                assert(pt_studio_consumer_step(&c)==PT_CONSUMER_PROGRESS);
                assert(pt_studio_consumer_step(&c)==PT_CONSUMER_FINISHED && f.calls==4);
            } else if(mode==5) {
                f.cancel=1;assert(pt_studio_consumer_stop(&c)==PT_CONSUMER_FINISHED && !f.held);
            } else {
                f.cancel=mode==3?0:-1;
                assert(pt_studio_consumer_stop(&c)==(mode==3?PT_CONSUMER_WAIT:PT_CONSUMER_ERROR));
                assert(!pt_studio_consumer_detach(&c) && f.held);
                assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);
                f.poll=-1;assert(pt_studio_consumer_step(&c)==PT_CONSUMER_ERROR && f.held);
                f.poll=1;assert(pt_studio_consumer_step(&c)==PT_CONSUMER_ERROR && !f.held);
            }
            assert(pt_studio_consumer_detach(&c));
        }
        assert(pt_studio_queue_close(q)==PT_QUEUE_OK);
    }
    puts("STUDIO CONSUMER PASS: refusal retry, ordered drain, pending/error cancellation retains lease until confirmed completion");return 0;
}
