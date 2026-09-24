#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/core/amigus_fifo.h"
struct port {int space,write,reset;unsigned calls,resets,n;uint32_t words[12];};
static int capacity(void *v) {return ((struct port *)v)->space;}
static int write3(void *v,const uint32_t *p) {struct port *s=v;++s->calls;if(s->write==1){memcpy(s->words+s->n,p,12);s->n+=3;}return s->write;}
static int reset(void *v) {struct port *s=v;++s->resets;return s->reset;}
int main(void)
{
    struct pt_amigus_fifo f={0};struct port p={0};struct pt_amigus_fifo_port ops={&p,capacity,write3,reset};
    int32_t data[6]={-8388608,8388607,-1,1,257,-513};struct pt_pcm pcm={data,6,3,48000,2,24};unsigned padding=99;
    assert(!pt_amigus_fifo_init(&f,&ops));assert(pt_amigus_fifo_submit(&f,&pcm)==-1);
    p.reset=1;assert(pt_amigus_fifo_cancel(&f)==1);assert(!pt_amigus_fifo_init(&f,&ops));
    assert(pt_amigus_fifo_submit(&f,&pcm)==1);assert(pt_amigus_fifo_submit(&f,&pcm)==0);
    p.space=2;assert(pt_amigus_fifo_poll(&f)==0 && !p.calls);
    p.space=3;p.write=1;assert(pt_amigus_fifo_poll(&f)==1 && p.calls==1);
    assert(p.words[0]==0x8000007fU && p.words[1]==0xffffffffU && p.words[2]==0xff000001U);
    assert(pt_amigus_fifo_finish(&f,&padding)==1 && padding==1);
    assert(pt_amigus_fifo_poll(&f)==1 && p.n==6);
    assert(p.words[3]==0x000101ffU && p.words[4]==0xfdff0000U && !p.words[5]);
    assert(pt_amigus_fifo_submit(&f,&pcm)==-1);
    assert(pt_amigus_fifo_cancel(&f)==1);assert(pt_amigus_fifo_submit(&f,&pcm)==1);
    p.write=-1;assert(pt_amigus_fifo_poll(&f)==-1);assert(pt_amigus_fifo_poll(&f)==-1 && p.calls==3);
    p.reset=0;assert(pt_amigus_fifo_cancel(&f)==0 && pt_amigus_fifo_submit(&f,&pcm)==-1);
    p.reset=-1;assert(pt_amigus_fifo_cancel(&f)==-1);
    p.reset=1;assert(pt_amigus_fifo_cancel(&f)==1 && !f.pack.held && !f.count);
    assert(pt_amigus_fifo_submit(&f,&pcm)==1);p.space=-1;
    assert(pt_amigus_fifo_poll(&f)==-1 && p.calls==3);
    puts("AMIGUS FIFO PASS: bounded triplets, capacity stalls, odd tail, explicit pad, uncertain writes blocked until confirmed reset");return 0;
}
