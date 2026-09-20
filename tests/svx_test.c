#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "svx.h"
static void be32(uint8_t *p,uint32_t v) {p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
/* Hand-authored canonical single-octave fixture, not produced by our encoder. */
static const uint8_t fixture[]={
 'F','O','R','M',0,0,0,58,'8','S','V','X',
 'V','H','D','R',0,0,0,20, 0,0,0,1,0,0,0,3,0,0,0,2, 0x20,0x5f,1,0,0,0,0x80,0,
 'N','A','M','E',0,0,0,3,'O','D','D',0,
 'B','O','D','Y',0,0,0,5,128,255,0,127,42,0};
int main(void)
{
 uint8_t bytes[512],saved[512];struct pt_svx_info v,untouched;int32_t data[8]={0},original[5]={-128,-1,0,127,42};
 struct pt_pcm pcm={data,8,5,8287,1,8};size_t i,n,w;uint32_t random=1;
 assert(sizeof(fixture)==66);
 assert(pt_svx_inspect(fixture,sizeof(fixture),&v)==PT_SVX_OK);
 assert(v.frames==5 && v.rate==8287 && v.loop_start==1 && v.loop_end==4 && v.volume==32768 && v.cycles==2 && !strcmp(v.name,"ODD"));
 assert(pt_svx_decode(fixture,sizeof(fixture),&pcm)==PT_SVX_OK && !memcmp(data,original,sizeof(original)));
 assert(pt_svx_size(&pcm,&v,&n)==PT_SVX_OK && n==94);
 assert(pt_svx_encode(&pcm,&v,bytes,sizeof(bytes),&w)==PT_SVX_OK && w==n && bytes[n-1]==0);
 assert(!memcmp(bytes,"FORM\0\0\0\x56""8SVX",12));
 assert(!memcmp(bytes+12,fixture+12,28));assert(!memcmp(bytes+88,fixture+60,5));
 assert(pt_svx_inspect(bytes,n,&v)==PT_SVX_OK && v.frames==5 && v.loop_start==1 && v.loop_end==4);
 memset(&untouched,0x55,sizeof(untouched));
 for(i=0;i<sizeof(fixture);++i) {v=untouched;assert(pt_svx_inspect(fixture,i,&v)!=PT_SVX_OK);assert(!memcmp(&v,&untouched,sizeof(v)));}
 memcpy(bytes,fixture,sizeof(fixture));be32(bytes+4,56);be32(bytes+20,4);be32(bytes+24,0);bytes[35]=1;be32(bytes+56,4);
 bytes[60]=0;bytes[61]=120;bytes[62]=255;bytes[63]=8;pcm.frames=4;
 assert(pt_svx_decode(bytes,64,&pcm)==PT_SVX_OK);
 assert(data[0]==-115 && data[1]==-94 && data[2]==-128 && data[3]==-128);
 /* Explicit endian chunk handling, unknown odd chunks, mono CHAN extension. */
 memcpy(bytes,fixture,52);memcpy(bytes+52,"ANNO",4);be32(bytes+56,1);bytes[60]='x';bytes[61]=0;
 memcpy(bytes+62,"CHAN",4);be32(bytes+66,4);be32(bytes+70,2);memcpy(bytes+74,fixture+52,14);be32(bytes+4,80);
 assert(pt_svx_inspect(bytes,88,&v)==PT_SVX_OK && v.frames==5);
 be32(bytes+70,6);assert(pt_svx_inspect(bytes,88,&v)==PT_SVX_UNSUPPORTED);
 be32(bytes+70,4);assert(pt_svx_inspect(bytes,88,&v)==PT_SVX_OK);
 memcpy(bytes,fixture,sizeof(fixture));bytes[34]=2;assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_UNSUPPORTED);
 bytes[34]=1;bytes[35]=2;assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_UNSUPPORTED);
 bytes[35]=0;be32(bytes+24,5);assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_INVALID);
 be32(bytes+24,3);be32(bytes+36,65537);assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_INVALID);
 memcpy(bytes,fixture,sizeof(fixture));bytes[32]=bytes[33]=0;assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_INVALID);
 memcpy(bytes,fixture,sizeof(fixture));be32(bytes+56,UINT32_MAX);assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_TRUNCATED);
 memcpy(bytes,fixture,sizeof(fixture));be32(bytes+4,57);assert(pt_svx_inspect(bytes,65,&v)==PT_SVX_TRUNCATED);
 /* Duplicate critical chunks and BODY before header are refused. */
 memcpy(bytes,fixture,52);memcpy(bytes+52,fixture+12,28);memcpy(bytes+80,fixture+52,14);be32(bytes+4,86);
 assert(pt_svx_inspect(bytes,94,&v)==PT_SVX_INVALID);
 memcpy(bytes,fixture,12);memcpy(bytes+12,fixture+52,14);memcpy(bytes+26,fixture+12,40);
 assert(pt_svx_inspect(bytes,66,&v)==PT_SVX_INVALID);
 pcm.frames=5;pcm.capacity=4;memset(data,0x55,sizeof(data));memcpy(original,data,sizeof(original));
 assert(pt_svx_decode(fixture,66,&pcm)==PT_SVX_CAPACITY && !memcmp(original,data,sizeof(original)));
 pcm.capacity=8;pcm.bits=16;assert(pt_svx_decode(fixture,66,&pcm)==PT_SVX_INVALID);
 pcm.bits=8;memcpy(bytes,fixture,66);pcm.data=(int32_t *)bytes;assert(pt_svx_decode(bytes,66,&pcm)==PT_SVX_ALIAS);pcm.data=data;
 assert(pt_svx_decode(fixture,66,&pcm)==PT_SVX_OK);assert(pt_svx_inspect(fixture,66,&v)==PT_SVX_OK);
 memset(bytes,0x55,sizeof(bytes));memcpy(saved,bytes,sizeof(bytes));w=123;
 assert(pt_svx_encode(&pcm,&v,bytes,93,&w)==PT_SVX_CAPACITY && w==123 && !memcmp(bytes,saved,sizeof(bytes)));
 assert(pt_svx_encode(&pcm,&v,(uint8_t *)data,512,&w)==PT_SVX_ALIAS);
 pcm.bits=16;assert(pt_svx_size(&pcm,&v,&n)==PT_SVX_UNSUPPORTED);pcm.bits=8;
 pcm.rate=65536;assert(pt_svx_size(&pcm,&v,&n)==PT_SVX_UNSUPPORTED);pcm.rate=8287;
 pcm.channels=2;pcm.frames=2;assert(pt_svx_size(&pcm,&v,&n)==PT_SVX_UNSUPPORTED);pcm.channels=1;pcm.frames=5;
 /* Bounded mutation corpus: successful parses must be decodable within bounds. */
 for(i=0;i<4096;++i) {
  size_t j;memcpy(bytes,fixture,66);random=random*1664525u+1013904223u;j=random%66;
  bytes[j]^=(uint8_t)(random>>24);
  if(pt_svx_inspect(bytes,66,&v)==PT_SVX_OK) {
   assert(v.frames<=8);pcm.frames=v.frames;pcm.rate=v.rate;
   assert(pt_svx_decode(bytes,66,&pcm)==PT_SVX_OK);
  }
 }
 puts("8SVX PASS: independent signed PCM fixture, odd chunks, loops, Fibonacci wrap, malformed bounds, alias and capacity");
 return 0;
}
