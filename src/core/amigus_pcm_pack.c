#include "amigus_pcm_pack.h"
static void pair(uint32_t *out,int32_t l0,int32_t r0,int32_t l1,int32_t r1)
{
    uint32_t a=(uint32_t)l0&0xffffffU,b=(uint32_t)r0&0xffffffU;
    uint32_t c=(uint32_t)l1&0xffffffU,d=(uint32_t)r1&0xffffffU;
    out[0]=(a<<8)|(b>>16);out[1]=(b<<16)|(c>>8);out[2]=(c<<24)|d;
}
int pt_amigus_pcm_pack_block(struct pt_amigus_pcm_pack *s,const struct pt_pcm *p,uint32_t *out,size_t cap,size_t *count)
{
    size_t needed,n=0;unsigned i=0;
    if(!s || !p || !out || !count || s->held>1 || s->finished ||
       !p->frames || p->frames>256 || p->rate!=48000 || p->bits!=24 || p->channels!=2 || pt_pcm_validate(p)!=PT_PCM_OK)return 0;
    needed=((p->frames+s->held)/2)*3;if(cap<needed)return 0;
    if(s->held) {pair(out,s->tail[0],s->tail[1],p->data[0],p->data[1]);n=3;i=1;s->held=0;}
    for(;i+1<p->frames;i+=2,n+=3)pair(out+n,p->data[i*2],p->data[i*2+1],p->data[i*2+2],p->data[i*2+3]);
    if(i<p->frames) {s->tail[0]=p->data[i*2];s->tail[1]=p->data[i*2+1];s->held=1;}
    *count=n;return 1;
}
int pt_amigus_pcm_pack_finish(struct pt_amigus_pcm_pack *s,uint32_t *out,size_t cap,size_t *count,unsigned *padding)
{
    if(!s || !out || !count || !padding || s->held>1 || (s->held && cap<3))return 0;
    *count=0;*padding=0;
    if(s->held) {pair(out,s->tail[0],s->tail[1],0,0);*count=3;*padding=1;s->held=0;}
    s->finished=1;return 1;
}
