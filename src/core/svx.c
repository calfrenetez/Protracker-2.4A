#include <string.h>
#include "svx.h"
/* FORM 8SVX, VHDR and Fibonacci-delta interpretation:
   https://wiki.amigaos.net/wiki/8SVX_IFF_8-Bit_Sampled_Voice
   Single octave, mono. Preserve trailing PCM after a loop as the pinned 2.3F
   writer does, even when its VHDR one-shot/repeat sum is shorter than BODY. */
static uint32_t u16(const uint8_t *p) {return ((uint32_t)p[0]<<8)|p[1];}
static uint32_t u32(const uint8_t *p) {return (u16(p)<<16)|u16(p+2);}
static void w16(uint8_t *p,uint32_t v) {p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void w32(uint8_t *p,uint32_t v) {w16(p,v>>16);w16(p+2,v);}
static int overlap(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return an && bn && (an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an));
}
enum pt_svx_result pt_svx_inspect(const uint8_t *p,size_t n,struct pt_svx_info *out)
{
 struct pt_svx_info v={0};size_t pos=12,end;unsigned header=0,body=0,chunks=0;uint32_t start=0,repeat=0;
 if(!p || !out)return PT_SVX_INVALID;
 if(n<12)return PT_SVX_TRUNCATED;
 if(memcmp(p,"FORM",4) || memcmp(p+8,"8SVX",4))return PT_SVX_UNSUPPORTED;
 if(u32(p+4)<4)return PT_SVX_INVALID;
 if(u32(p+4)>n-8)return PT_SVX_TRUNCATED;
 end=(size_t)u32(p+4)+8;
 while(pos<end) {
  size_t data;uint32_t bytes;
  if(++chunks>4096)return PT_SVX_INVALID;
  if(end-pos<8)return PT_SVX_TRUNCATED;
  bytes=u32(p+pos+4);data=pos+8;if(bytes>end-data)return PT_SVX_TRUNCATED;
  if(!memcmp(p+pos,"VHDR",4)) {
   if(header++ || body)return PT_SVX_INVALID;
   if(bytes<20)return PT_SVX_TRUNCATED;
   start=u32(p+data);repeat=u32(p+data+4);v.cycles=u32(p+data+8);v.rate=u16(p+data+12);
   v.compression=p[data+15];v.volume=u32(p+data+16);
   if(p[data+14]!=1 || v.compression>1)return PT_SVX_UNSUPPORTED;
   if(!v.rate || v.volume>65536 || repeat>UINT32_MAX-start)return PT_SVX_INVALID;
  } else if(!memcmp(p+pos,"BODY",4)) {
   if(body++ || !header)return PT_SVX_INVALID;
   if(data>UINT32_MAX)return PT_SVX_CAPACITY;
   v.offset=(uint32_t)data;v.bytes=bytes;
  } else if(!memcmp(p+pos,"NAME",4)) {
   size_t length=bytes<31?bytes:31;if(body)return PT_SVX_INVALID;
   memset(v.name,0,sizeof(v.name));memcpy(v.name,p+data,length);
  } else if(!memcmp(p+pos,"CHAN",4)) {
   if(bytes!=4)return PT_SVX_INVALID;
   if(u32(p+data)!=2 && u32(p+data)!=4)return PT_SVX_UNSUPPORTED;
  }
  pos=data+bytes;if(bytes&1) {if(pos==end)return PT_SVX_TRUNCATED;++pos;}
 }
 if(!header || !body)return PT_SVX_INVALID;
 if(v.compression) {
  if(v.bytes<2)return PT_SVX_TRUNCATED;
  if(v.bytes-2>UINT32_MAX/2)return PT_SVX_CAPACITY;
  v.frames=(v.bytes-2)*2;
 } else v.frames=v.bytes;
 if(start>v.frames || repeat>v.frames-start)return PT_SVX_INVALID;
 if(repeat) {v.loop_start=start;v.loop_end=start+repeat;}
 *out=v;return PT_SVX_OK;
}
enum pt_svx_result pt_svx_decode(const uint8_t *p,size_t n,struct pt_pcm *dest)
{
 struct pt_svx_info v;uint32_t i;enum pt_svx_result result=pt_svx_inspect(p,n,&v);
 static const int delta[16]={-34,-21,-13,-8,-5,-3,-2,-1,0,1,2,3,5,8,13,21};uint8_t value;
 if(result!=PT_SVX_OK)return result;
 if(!dest || dest->bits!=8 || dest->channels!=1 || dest->frames!=v.frames || dest->rate!=v.rate || (v.frames && !dest->data))return PT_SVX_INVALID;
 if((uint64_t)v.frames*sizeof(int32_t)>SIZE_MAX || dest->capacity<v.frames)return PT_SVX_CAPACITY;
 if(overlap(p,n,dest->data,(size_t)v.frames*sizeof(int32_t)))return PT_SVX_ALIAS;
 p+=v.offset;value=v.compression?p[1]:0;
 for(i=0;i<v.frames;++i) {
  if(v.compression) {unsigned code=i&1?p[2+i/2]&15:p[2+i/2]>>4;value=(uint8_t)(value+delta[code]);}
  else value=p[i];
  dest->data[i]=value<128?value:(int32_t)value-256;
 }
 return PT_SVX_OK;
}
enum pt_svx_result pt_svx_size(const struct pt_pcm *pcm,const struct pt_svx_info *v,size_t *out)
{
 uint64_t bytes;
 if(!v || !out || pt_pcm_validate(pcm)!=PT_PCM_OK)return PT_SVX_INVALID;
 if(pcm->bits!=8 || pcm->channels!=1 || pcm->rate>65535)return PT_SVX_UNSUPPORTED;
 if(v->volume>65536 || (v->loop_end?(v->loop_start>=v->loop_end || v->loop_end>pcm->frames):v->loop_start!=0))return PT_SVX_INVALID;
 bytes=88+(uint64_t)pcm->frames+(pcm->frames&1);
 if(bytes>SIZE_MAX || bytes>UINT32_MAX)return PT_SVX_CAPACITY;
 *out=(size_t)bytes;return PT_SVX_OK;
}
enum pt_svx_result pt_svx_encode(const struct pt_pcm *pcm,const struct pt_svx_info *v,uint8_t *out,size_t capacity,size_t *written)
{
 size_t size,i;enum pt_svx_result result=pt_svx_size(pcm,v,&size);
 if(result!=PT_SVX_OK)return result;
 if(!out || !written)return PT_SVX_INVALID;
 if(capacity<size)return PT_SVX_CAPACITY;
 if(overlap(out,size,pcm->data,(size_t)pcm->frames*sizeof(int32_t)) || overlap(out,size,v,sizeof(*v)))return PT_SVX_ALIAS;
 memset(out,0,size);memcpy(out,"FORM",4);w32(out+4,(uint32_t)size-8);memcpy(out+8,"8SVXVHDR",8);w32(out+16,20);
 w32(out+20,v->loop_end?v->loop_start:pcm->frames);w32(out+24,v->loop_end?v->loop_end-v->loop_start:0);w32(out+28,v->cycles);
 w16(out+32,pcm->rate);out[34]=1;w32(out+36,v->volume);
 memcpy(out+40,"NAME",4);w32(out+44,32);memcpy(out+48,v->name,32);
 memcpy(out+80,"BODY",4);w32(out+84,pcm->frames);
 for(i=0;i<pcm->frames;++i)out[88+i]=(uint8_t)pcm->data[i];
 *written=size;return PT_SVX_OK;
}
