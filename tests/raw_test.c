#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "raw.h"
static void output_alias_cases(void)
{
 static const uint8_t big8[]={128,255,0,127},unsigned8[]={0,127,128,255};
 static const uint8_t big16[]={128,0,255,255,0,0,127,255},little16[]={0,128,255,255,0,0,255,127};
 static const uint8_t big24[]={128,0,1,255,255,255,0,0,0,127,255,254},little24[]={1,0,128,255,255,255,0,0,0,254,255,127};
 struct source {struct pt_pcm pcm;struct pt_raw_format format;int32_t data[32];} v,old;
 uint8_t bytes[128],saved[128];void *alias[8];size_t n,w,i;uint32_t frames;
 unsigned bits,channels,endian,uns;
 for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)
 for(endian=0;endian<=1;++endian)for(uns=0;uns<=(bits==8);++uns) {
  const uint8_t *wire=bits==8?(uns?unsigned8:big8):bits==16?(endian?little16:big16):(endian?little24:big24);
  memset(&v,0,sizeof(v));v.pcm=(struct pt_pcm){v.data,32,4/channels,48000,(uint8_t)channels,(uint8_t)bits};
  v.format=(struct pt_raw_format){48000,(uint8_t)bits,(uint8_t)channels,(uint8_t)endian,(uint8_t)uns};
  v.data[0]=bits==24?-8388607:-((int32_t)1<<(bits-1));v.data[1]=-1;v.data[2]=0;
  v.data[3]=bits==24?8388606:((int32_t)1<<(bits-1))-1;
  for(i=4;i<32;++i)v.data[i]=INT32_MAX; /* Reserved capacity must not be scanned as active PCM. */
  old=v;n=123;assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_OK && n==4*(bits/8));
  frames=123;assert(pt_raw_frames(n,&v.format,&frames)==PT_RAW_OK && frames==v.pcm.frames);
  assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),&w)==PT_RAW_OK && w==n && !memcmp(bytes,wire,n));
  assert(!memcmp(&v,&old,sizeof(v)));
  memset(bytes,0x55,sizeof(bytes));memcpy(saved,bytes,sizeof(bytes));
  alias[0]=&v.pcm;alias[1]=(uint8_t *)&v.pcm+sizeof(v.pcm)-1;
  alias[2]=&v.format;alias[3]=(uint8_t *)&v.format+sizeof(v.format)-1;
  alias[4]=v.data;alias[5]=v.data+3;alias[6]=v.data+4;alias[7]=(uint8_t *)v.data+sizeof(v.data)-1;
  for(i=0;i<8;++i) {
   assert(pt_raw_size(&v.pcm,&v.format,(size_t *)alias[i])==PT_RAW_ALIAS);
   assert(!memcmp(&v,&old,sizeof(v)) && !memcmp(bytes,saved,sizeof(bytes)));
   assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),(size_t *)alias[i])==PT_RAW_ALIAS);
   assert(!memcmp(&v,&old,sizeof(v)) && !memcmp(bytes,saved,sizeof(bytes)));
   w=123;assert(pt_raw_encode(&v.pcm,&v.format,alias[i],sizeof(bytes),&w)==PT_RAW_ALIAS && w==123);
   assert(!memcmp(&v,&old,sizeof(v)) && !memcmp(bytes,saved,sizeof(bytes)));
  }
  assert(pt_raw_frames(n,&v.format,&v.format.rate)==PT_RAW_ALIAS && !memcmp(&v,&old,sizeof(v)));
  assert(pt_raw_frames(n,&v.format,(uint32_t *)((uint8_t *)&v.format+sizeof(v.format)-1))==PT_RAW_ALIAS);
  assert(!memcmp(&v,&old,sizeof(v)));
  assert(pt_raw_frames(n,&v.format,(uint32_t *)(UINTPTR_MAX-1))==PT_RAW_ALIAS);
  assert(pt_raw_size(&v.pcm,&v.format,(size_t *)(UINTPTR_MAX-1))==PT_RAW_ALIAS);
  w=123;assert(pt_raw_encode(&v.pcm,&v.format,(uint8_t *)(UINTPTR_MAX-1),sizeof(bytes),&w)==PT_RAW_ALIAS && w==123);
  assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),(size_t *)bytes)==PT_RAW_ALIAS);
  assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),(size_t *)(bytes+n-1))==PT_RAW_ALIAS);
  assert(!memcmp(&v,&old,sizeof(v)) && !memcmp(bytes,saved,sizeof(bytes)));
  w=123;assert(pt_raw_encode(&v.pcm,&v.format,bytes,n-1,&w)==PT_RAW_CAPACITY && w==123);
  assert(pt_raw_encode(&v.pcm,&v.format,NULL,sizeof(bytes),&w)==PT_RAW_INVALID && w==123);
  assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),NULL)==PT_RAW_INVALID);
  assert(pt_raw_size(&v.pcm,&v.format,NULL)==PT_RAW_INVALID);
  assert(pt_raw_frames(n,&v.format,NULL)==PT_RAW_INVALID);
  assert(!memcmp(&v,&old,sizeof(v)) && !memcmp(bytes,saved,sizeof(bytes)));
 }
 /* No write occurs through a zero-length encoded byte span, including aliases. */
 v.pcm.frames=0;old=v;w=123;
 assert(pt_raw_encode(&v.pcm,&v.format,NULL,0,&w)==PT_RAW_OK && w==0 && !memcmp(&v,&old,sizeof(v)));
 assert(pt_raw_encode(&v.pcm,&v.format,(uint8_t *)v.data,0,&w)==PT_RAW_OK && w==0 && !memcmp(&v,&old,sizeof(v)));
 assert(pt_raw_encode(&v.pcm,&v.format,NULL,0,(size_t *)v.data)==PT_RAW_ALIAS && !memcmp(&v,&old,sizeof(v)));
 {struct output {uint8_t bytes[16];size_t written;} out;
  v.pcm.frames=2;old=v;memset(&out,0x55,sizeof(out));
  assert(pt_raw_encode(&v.pcm,&v.format,out.bytes,sizeof(out),&out.written)==PT_RAW_OK && out.written==12);
  assert(!memcmp(out.bytes,little24,12) && !memcmp(&v,&old,sizeof(v)));
 }
 /* Invalid geometry/settings retain their existing precedence and output images. */
 n=w=123;frames=123;v.format.rate=0;old=v;
 assert(pt_raw_frames(12,&v.format,&frames)==PT_RAW_INVALID && frames==123);
 assert(pt_raw_frames(12,&v.format,&v.format.rate)==PT_RAW_INVALID && !memcmp(&v,&old,sizeof(v)));
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_INVALID && n==123);
 assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),&w)==PT_RAW_INVALID && w==123);
 v.format.rate=44100;old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_INVALID && n==123);
 assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),(size_t *)v.data)==PT_RAW_INVALID && !memcmp(&v,&old,sizeof(v)));
 v.format.rate=48000;v.format.bits=16;old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_INVALID && n==123 && !memcmp(&v,&old,sizeof(v)));
 v.format.bits=24;v.data[0]=8388608;old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_INVALID && n==123);
 assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),(size_t *)v.data)==PT_RAW_INVALID && !memcmp(&v,&old,sizeof(v)));
 v.data[0]=-8388607;old=v;
 assert(pt_raw_frames(11,&v.format,&frames)==PT_RAW_INVALID && frames==123 && !memcmp(&v,&old,sizeof(v)));
 {uint64_t large=((uint64_t)UINT32_MAX+1)*6;
  if(large<=SIZE_MAX) {
   assert(pt_raw_frames((size_t)large,&v.format,&frames)==PT_RAW_CAPACITY && frames==123);
   assert(pt_raw_frames((size_t)large,&v.format,&v.format.rate)==PT_RAW_CAPACITY && !memcmp(&v,&old,sizeof(v)));
  }
 }
 v.pcm.capacity=SIZE_MAX/sizeof(int32_t)+1;old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_ALIAS && n==123 && !memcmp(&v,&old,sizeof(v)));
 assert(pt_raw_encode(&v.pcm,&v.format,bytes,sizeof(bytes),&w)==PT_RAW_ALIAS && w==123);
 assert(!memcmp(bytes,saved,sizeof(bytes)));
 v.pcm.frames=0;v.pcm.capacity=1;v.pcm.data=(int32_t *)(UINTPTR_MAX-1);old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_ALIAS && n==123 && !memcmp(&v,&old,sizeof(v)));
 assert(pt_raw_encode(&v.pcm,&v.format,NULL,0,&w)==PT_RAW_ALIAS && w==123);
 v.pcm.data=NULL;old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_ALIAS && n==123 && !memcmp(&v,&old,sizeof(v)));
 v.pcm.capacity=0;old=v;
 assert(pt_raw_size(&v.pcm,&v.format,&n)==PT_RAW_OK && n==0);
 assert(pt_raw_encode(&v.pcm,&v.format,NULL,0,&w)==PT_RAW_OK && w==0 && !memcmp(&v,&old,sizeof(v)));
}
int main(void)
{
 const uint8_t big[]={0x80,0,1,0x7f,0xff,0xfe,0xff,0xff,0xff,0,0,1};
 const uint8_t little[]={1,0,0x80,0xfe,0xff,0x7f,0xff,0xff,0xff,1,0,0};
 int32_t data[4]={0},expected[]={-8388607,8388606,-1,1};uint8_t output[16],before[16];size_t w,n;uint32_t frames;
 struct pt_pcm pcm={data,4,2,48000,2,24};struct pt_raw_format fmt={48000,24,2,0,0};
 assert(pt_raw_frames(sizeof(big),&fmt,&frames)==PT_RAW_OK && frames==2);
 assert(pt_raw_decode(big,sizeof(big),&fmt,&pcm)==PT_RAW_OK && !memcmp(data,expected,sizeof(data)));
 assert(pt_raw_encode(&pcm,&fmt,output,sizeof(output),&w)==PT_RAW_OK && w==12 && !memcmp(output,big,12));
 fmt.little_endian=1;assert(pt_raw_decode(little,12,&fmt,&pcm)==PT_RAW_OK && !memcmp(data,expected,sizeof(data)));
 assert(pt_raw_encode(&pcm,&fmt,output,sizeof(output),&w)==PT_RAW_OK && w==12 && !memcmp(output,little,12));
 frames=99;assert(pt_raw_frames(11,&fmt,&frames)==PT_RAW_INVALID && frames==99);
 assert(pt_raw_decode(big,11,&fmt,&pcm)==PT_RAW_INVALID && !memcmp(data,expected,sizeof(data)));
 memset(output,0x55,sizeof(output));memcpy(before,output,sizeof(output));w=99;
 assert(pt_raw_encode(&pcm,&fmt,output,11,&w)==PT_RAW_CAPACITY && w==99 && !memcmp(before,output,sizeof(output)));
 assert(pt_raw_encode(&pcm,&fmt,(uint8_t *)data,12,&w)==PT_RAW_ALIAS);
 assert(pt_raw_decode((uint8_t *)data,12,&fmt,&pcm)==PT_RAW_ALIAS);
 pcm.capacity=3;assert(pt_raw_decode(big,12,&fmt,&pcm)==PT_RAW_CAPACITY);pcm.capacity=4;
 fmt.rate=44100;assert(pt_raw_size(&pcm,&fmt,&n)==PT_RAW_INVALID);fmt.rate=48000;
 fmt.unsigned8=1;assert(pt_raw_frames(12,&fmt,&frames)==PT_RAW_INVALID);fmt.unsigned8=0;
 fmt.bits=pcm.bits=8;fmt.channels=pcm.channels=1;pcm.frames=4;fmt.unsigned8=1;
 {uint8_t values[]={0,127,128,255};assert(pt_raw_decode(values,4,&fmt,&pcm)==PT_RAW_OK);}
 assert(data[0]==-128 && data[1]==-1 && data[2]==0 && data[3]==127);
 assert(pt_raw_encode(&pcm,&fmt,output,4,&w)==PT_RAW_OK && !memcmp(output,"\0\x7f\x80\xff",4));
 fmt.unsigned8=0;assert(pt_raw_encode(&pcm,&fmt,output,4,&w)==PT_RAW_OK && !memcmp(output,"\x80\xff\0\x7f",4));
 {uint8_t values[]={128,255,0,127};assert(pt_raw_decode(values,4,&fmt,&pcm)==PT_RAW_OK);}
 assert(data[0]==-128 && data[1]==-1 && data[2]==0 && data[3]==127);
 fmt.bits=pcm.bits=16;fmt.little_endian=0;
 {uint8_t values[]={128,0,255,255,0,0,127,255};assert(pt_raw_decode(values,8,&fmt,&pcm)==PT_RAW_OK);
 assert(data[0]==-32768 && data[1]==-1 && data[2]==0 && data[3]==32767);
 assert(pt_raw_encode(&pcm,&fmt,output,8,&w)==PT_RAW_OK && !memcmp(output,values,8));}
 fmt.little_endian=2;assert(pt_raw_frames(8,&fmt,&frames)==PT_RAW_INVALID);
 fmt.little_endian=1;pcm.frames=0;assert(pt_raw_decode(NULL,0,&fmt,&pcm)==PT_RAW_OK);
 assert(pt_raw_encode(&pcm,&fmt,NULL,0,&w)==PT_RAW_OK && !w);
 output_alias_cases();
 puts("RAW PASS: explicit mono/stereo 8/16/24-bit signedness/endian, precision, alignment, alias, capacity and no implicit conversion");return 0;
}
