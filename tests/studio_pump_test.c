#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "studio_pump.h"
static unsigned live;
static void *alloc(void *c,size_t n) {(void)c;++live;return malloc(n);}
static void drop(void *c,void *p) {(void)c;--live;free(p);}
struct source {unsigned pos,calls,stops,fail;int32_t data[512];struct pt_pcm pcm;};
static int32_t value(unsigned i) {return (int32_t)(i*257)-700001;}
static enum pt_render_result pull(void *c,unsigned frames,const struct pt_pcm **out,unsigned *done)
{
    struct source *s=c;unsigned n,i;++s->calls;*out=NULL;*done=0;
    if(s->fail)return PT_RENDER_SAMPLE;
    if(s->pos==2003) {*done=1;return PT_RENDER_OK;}
    if(s->calls%3==0)return PT_RENDER_OK;
    n=2003-s->pos;if(n>frames)n=frames;
    for(i=0;i<n*2;++i)s->data[i]=value(s->pos*2+i);
    s->pcm=(struct pt_pcm){s->data,512,n,48000,2,24};s->pos+=n;*out=&s->pcm;return PT_RENDER_OK;
}
static void stop(void *c) {++((struct source *)c)->stops;}
int main(void)
{
    struct pt_allocator a={NULL,alloc,drop};unsigned size;
    for(size=1;size<=256;size=size==1?17:size==17?256:257) {
        struct source s={0};struct pt_studio_producer producer={&s,pull,stop};struct pt_studio_pump pump;
        struct pt_studio_queue *q=pt_studio_queue_open(&a,2);unsigned received=0,done=0,iterations=0;
        assert(q && pt_studio_pump_init(&pump,&producer,q));
        while(!done) {
            unsigned j;const struct pt_pcm *block;uint64_t ticket;
            for(j=0;j<7;++j) {enum pt_pump_result r=pt_studio_pump_step(&pump,size);assert(r!=PT_PUMP_ERROR);
                if(r==PT_PUMP_BLOCKED) {unsigned calls=s.calls;assert(pt_studio_pump_step(&pump,size)==PT_PUMP_BLOCKED && s.calls==calls);}}
            {enum pt_queue_result r=pt_studio_queue_acquire(q,&block,&ticket);
                if(r==PT_QUEUE_OK) {unsigned i;
                    for(i=0;i<block->frames*2;++i)assert(block->data[i]==value(received*2+i));
                    received+=block->frames;
                    assert(pt_studio_pump_step(&pump,size)!=PT_PUMP_ERROR);
                    assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);
                } else {assert(r==PT_QUEUE_EMPTY || r==PT_QUEUE_DONE);done=r==PT_QUEUE_DONE;}}
            assert(++iterations<10000);
        }
        assert(received==2003 && s.stops==1);assert(pt_studio_queue_close(q)==PT_QUEUE_OK && !live);
    }
    {struct source s={0};struct pt_studio_producer producer={&s,pull,stop};struct pt_studio_pump p;
        struct pt_studio_queue *q=pt_studio_queue_open(&a,1);const struct pt_pcm *out;uint64_t ticket;
        assert(pt_studio_pump_init(&p,&producer,q));assert(pt_studio_pump_step(&p,17)==PT_PUMP_PROGRESS);
        assert(pt_studio_queue_acquire(q,&out,&ticket)==PT_QUEUE_OK);s.fail=1;
        assert(pt_studio_pump_step(&p,17)==PT_PUMP_ERROR && s.stops==1 && out->data[0]==value(0));
        assert(pt_studio_queue_close(q)==PT_QUEUE_BUSY);pt_studio_pump_stop(&p);assert(s.stops==1);
        assert(pt_studio_queue_release(q,ticket)==PT_QUEUE_OK);assert(pt_studio_queue_close(q)==PT_QUEUE_OK && !live);
    }
    puts("STUDIO PUMP PASS: stalled consumers, exact ordered true24 frames, bounded progress, failure lease retention");return 0;
}
