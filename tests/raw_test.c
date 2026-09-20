#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "raw.h"
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
 puts("RAW PASS: explicit mono/stereo 8/16/24-bit signedness/endian, precision, alignment, alias, capacity and no implicit conversion");return 0;
}
