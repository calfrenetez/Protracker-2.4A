#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wav.h"
struct input {const uint8_t *data;size_t length;unsigned calls,fail;};
static int read_at(void *context,size_t offset,uint8_t *out,size_t n)
{struct input *in=context;assert(n<=16 && offset<=in->length && n<=in->length-offset);if(++in->calls==in->fail)return 0;memcpy(out,in->data+offset,n);return 1;}
static void put32(uint8_t *p,uint32_t n) {unsigned i;for(i=0;i<4;++i)p[i]=(uint8_t)(n>>(i*8));}
int main(void)
{
    int32_t values[4]={-128,-1,1,127};struct pt_pcm pcm={values,4,4,44100,1,8};
    uint8_t wave[48],chunks[100];size_t n,i;struct pt_wav_info info,sentinel;struct input in;unsigned calls;
    assert(pt_wav_encode(&pcm,wave,sizeof(wave),&n)==PT_WAV_OK && n==48);
    memcpy(chunks,wave,12);memcpy(chunks+12,wave+36,12);memcpy(chunks+24,"JUNK",4);put32(chunks+28,3);
    memcpy(chunks+32,"odd\0",4);memcpy(chunks+36,wave+12,24);put32(chunks+4,52);
    in=(struct input){chunks,60,0,0};assert(pt_wav_inspect_reader(read_at,&in,60,&info)==PT_WAV_OK);
    assert(info.frames==4 && info.data_offset==20 && info.data_bytes==4 && info.bits==8 && info.channels==1 && info.rate==44100);calls=in.calls;
    memset(&sentinel,0xa5,sizeof(sentinel));
    for(i=1;i<=calls;++i) {info=sentinel;in.calls=0;in.fail=(unsigned)i;assert(pt_wav_inspect_reader(read_at,&in,60,&info)==PT_WAV_TRUNCATED && !memcmp(&info,&sentinel,sizeof(info)));}
    for(i=0;i<60;++i) {info=sentinel;in=(struct input){chunks,i,0,0};assert(pt_wav_inspect_reader(read_at,&in,i,&info)==PT_WAV_TRUNCATED && !memcmp(&info,&sentinel,sizeof(info)));}
    in=(struct input){chunks,62,0,0};assert(pt_wav_inspect_reader(read_at,&in,62,&info)==PT_WAV_OK); /* accepted external tail */
    memcpy(chunks+60,wave+12,24);put32(chunks+4,76);in=(struct input){chunks,84,0,0};assert(pt_wav_inspect_reader(read_at,&in,84,&info)==PT_WAV_INVALID);
    puts("WAV READER PASS: bounded metadata reads, data-before-fmt, odd unknown chunk, truncation, duplicate fmt, tail policy and failed-read output preservation");return 0;
}
