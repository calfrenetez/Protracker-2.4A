#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/core/amigus_register_port.h"
struct bus {unsigned calls,fail,owned,n;unsigned addr[32];uint32_t val[32];uint16_t used,rate,mask;};
static int owner(void *v) {return ((struct bus *)v)->owned;}
static int read16(void *v,unsigned a,uint16_t *out) {struct bus *b=v;if(++b->calls==b->fail)return 0;*out=a==0x10?b->used:a==6?b->rate:b->mask;return 1;}
static int write16(void *v,unsigned a,uint16_t x) {struct bus *b=v;b->addr[b->n]=a;b->val[b->n++]=x;return ++b->calls!=b->fail;}
static int write32(void *v,unsigned a,uint32_t x) {struct bus *b=v;b->addr[b->n]=a;b->val[b->n++]=x;b->used+=2;return ++b->calls!=b->fail;}
int main(void)
{
    struct bus b={0};struct pt_amigus_register_port p;struct pt_amigus_register_io io={&b,owner,read16,write16,write32};unsigned i;uint32_t words[3]={0x12345678,0x90abcdef,0xfedcba98};
    assert(!pt_amigus_register_port_init(&p,&io,5));assert(pt_amigus_register_port_init(&p,&io,16));
    assert(pt_amigus_register_reset(&p)==-1 && !b.calls);b.owned=1;
    for(i=1;i<=7;++i) {b.calls=b.n=0;b.fail=i;assert(pt_amigus_register_reset(&p)==-1 && !p.aligned);}
    b.fail=0;b.calls=b.n=0;b.rate=0x8000;assert(pt_amigus_register_reset(&p)==0);
    b.n=0;b.rate=0;b.mask=7;assert(pt_amigus_register_reset(&p)==0);
    b.n=0;b.mask=0;b.used=1;assert(pt_amigus_register_reset(&p)==0);
    b.n=0;b.used=0;assert(pt_amigus_register_reset(&p)==1);
    assert(b.n==4 && b.addr[0]==6 && !b.val[0] && b.addr[1]==0 && b.val[1]==7 && b.addr[2]==2 && b.val[2]==7 && b.addr[3]==8 && !b.val[3]);
    b.used=11;assert(pt_amigus_register_capacity(&p)==2);b.used=10;assert(pt_amigus_register_capacity(&p)==3);
    b.n=0;assert(pt_amigus_register_write3(&p,words)==1 && b.n==3 && b.used==16);
    for(i=0;i<3;++i)assert(b.addr[i]==12 && b.val[i]==words[i]);
    assert(!pt_amigus_register_drain(&p));b.used=0;assert(pt_amigus_register_drain(&p)==1);
    for(i=2;i<=4;++i) {b.used=0;b.n=b.calls=0;b.fail=0;assert(pt_amigus_register_reset(&p)==1);
        b.n=b.calls=0;b.fail=i;assert(pt_amigus_register_write3(&p,words)==-1 && !p.aligned);
        assert(pt_amigus_register_capacity(&p)==-1);}
    b.n=0;b.fail=0;b.used=0;assert(pt_amigus_register_reset(&p)==1);
    b.used=17;assert(pt_amigus_register_capacity(&p)==-1);
    b.used=0;b.n=0;assert(pt_amigus_register_reset(&p)==1);b.owned=0;i=b.calls;
    assert(pt_amigus_register_write3(&p,words)==-1 && b.calls==i);
    puts("AMIGUS REGISTER PASS: owned access, word capacity, ordered triplets, bounded reset/readback, partial-write poison");return 0;
}
