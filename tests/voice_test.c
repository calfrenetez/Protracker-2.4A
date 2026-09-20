#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "voice.h"
#define Q (1ULL<<32)
static void sequence(struct pt_voice *v,const int32_t *values,unsigned n)
{
    unsigned i;int32_t out[2];
    for(i=0;i<n;++i) {assert(pt_voice_frame(v,out)==PT_PCM_OK);assert(out[0]==values[i] && out[1]==values[i]);}
}
static void traversal(void)
{
    int32_t data[]={0,100,200,300};struct pt_pcm pcm={data,4,4,48000,1,24};struct pt_voice v,before;int32_t out[2]={9,9};
    static const int32_t once[]={0,100,200,300,0,0},slice[]={100,200,0};
    static const int32_t forward[]={0,100,200,300,200,300,200,300};
    static const int32_t ping[]={0,100,200,300,200,100,200,300,200,100};
    static const int32_t half[]={0,50,100,150,200,250,300,250,200,150,100,150};
    static const int32_t one[]={0,100,200,200,200,200};
    static const int32_t skip[]={0,300,200,300,200};
    static const int32_t skip_ping[]={0,100,200,300,200,100};
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);sequence(&v,once,6);assert(!v.active);
    assert(pt_voice_init(&v,&pcm,1,3,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);sequence(&v,slice,3);
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_FORWARD,2,4,Q,0)==PT_PCM_OK);sequence(&v,forward,8);
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_PINGPONG,1,4,Q,0)==PT_PCM_OK);sequence(&v,ping,10);
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_PINGPONG,1,4,Q/2,1)==PT_PCM_OK);sequence(&v,half,12);
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_PINGPONG,2,3,Q,1)==PT_PCM_OK);sequence(&v,one,6);
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_FORWARD,2,4,Q*9,0)==PT_PCM_OK);sequence(&v,skip,5);
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_PINGPONG,1,4,Q*9,0)==PT_PCM_OK);sequence(&v,skip_ping,6);
    /* Interpolation blends across a forward-loop seam, never the sample tail. */
    assert(pt_voice_init(&v,&pcm,2,4,PT_VOICE_FORWARD,2,4,Q/2,1)==PT_PCM_OK);
    {const int32_t seam[]={200,250,300,250,200};sequence(&v,seam,5);}
    before=v;assert(pt_voice_frame(&v,data)==PT_PCM_ALIAS && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init(&v,&pcm,1,4,PT_VOICE_FORWARD,0,4,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_ONCE,0,0,0,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init(&v,&pcm,0,4,PT_VOICE_ONCE,1,2,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    assert(pt_voice_init(&v,&pcm,0,4,(enum pt_voice_loop)-1,0,0,Q,0)==PT_PCM_INVALID && !memcmp(&v,&before,sizeof(v)));
    pcm.frames=0;pcm.capacity=0;pcm.data=NULL;
    assert(pt_voice_init(&v,&pcm,0,0,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);
    assert(pt_voice_frame(&v,out)==PT_PCM_OK && !out[0] && !out[1] && !v.active);
    puts("VOICE traversal PASS: selected ranges, forward/pingpong seams, fractional steps, one-frame loops and large steps");
}
static void mixing(void)
{
    int32_t data[]={-8388608,8388607,1,-1,128,-128,8388607,8388607};
    int32_t buffer[32],original[32];struct pt_pcm pcm={data,8,4,48000,2,24},out={buffer,32,4,48000,2,24};
    struct pt_voice v[16],before[16];uint32_t gain[16][2];uint64_t clipped;unsigned i;
    memset(v,0,sizeof(v));memset(gain,0,sizeof(gain));gain[0][0]=gain[0][1]=65536;
    assert(pt_voice_init(v,&pcm,0,4,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);
    assert(pt_voice_mix(v,1,gain,&out,&clipped)==PT_PCM_OK && !clipped && !memcmp(data,buffer,8*sizeof(*data)));
    assert(pt_voice_init(v,&pcm,0,4,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);out.bits=16;
    assert(pt_voice_mix(v,1,gain,&out,&clipped)==PT_PCM_OK && clipped==3);
    {const int32_t expected[]={-32768,32767,0,0,1,-1,32767,32767};assert(!memcmp(buffer,expected,sizeof(expected)));}
    out.bits=24;
    for(i=0;i<16;++i) {assert(pt_voice_init(v+i,&pcm,0,4,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);gain[i][0]=gain[i][1]=65536;}
    assert(pt_voice_mix(v,16,gain,&out,&clipped)==PT_PCM_OK && clipped==4);
    assert(buffer[0]==-8388608 && buffer[1]==8388607 && buffer[2]==16 && buffer[3]==-16 && buffer[4]==2048 && buffer[5]==-2048);
    /* Zero side gains do not stop voice progression; empty slots are silent. */
    memset(v,0,sizeof(v));memset(gain,0,sizeof(gain));gain[0][0]=65536;
    assert(pt_voice_init(v,&pcm,0,4,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);
    assert(pt_voice_mix(v,16,gain,&out,&clipped)==PT_PCM_OK && !clipped && !v[0].active);
    for(i=0;i<4;++i)assert(buffer[i*2]==data[i*2] && !buffer[i*2+1]);
    memset(buffer,0x55,sizeof(buffer));memcpy(original,buffer,sizeof(buffer));memcpy(before,v,sizeof(v));clipped=123;
    gain[15][0]=65537;
    assert(pt_voice_mix(v,16,gain,&out,&clipped)==PT_PCM_INVALID && clipped==123 && !memcmp(v,before,sizeof(v)) && !memcmp(buffer,original,sizeof(buffer)));
    gain[15][0]=0;out.capacity=7;
    assert(pt_voice_mix(v,16,gain,&out,&clipped)==PT_PCM_CAPACITY && clipped==123 && !memcmp(v,before,sizeof(v)) && !memcmp(buffer,original,sizeof(buffer)));
    out.capacity=32;out.data=data;
    assert(pt_voice_mix(v,16,gain,&out,&clipped)==PT_PCM_ALIAS && clipped==123 && !memcmp(v,before,sizeof(v)));
    out.data=buffer;
    assert(pt_voice_mix(v,16,gain,&out,(uint64_t *)&v[0])==PT_PCM_ALIAS && !memcmp(v,before,sizeof(v)));
    assert(pt_voice_mix(NULL,0,NULL,&out,&clipped)==PT_PCM_OK && !clipped);
    for(i=0;i<8;++i)assert(!buffer[i]);
    /* Source 8/16-bit ranges scale exactly without losing sign. */
    {int32_t small[]={-128,127};int32_t stereo[2];struct pt_pcm source={small,2,2,8287,1,8};
        assert(pt_voice_init(v,&source,0,2,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);
        assert(pt_voice_frame(v,stereo)==PT_PCM_OK && stereo[0]==-8388608 && stereo[1]==-8388608);
        assert(pt_voice_frame(v,stereo)==PT_PCM_OK && stereo[0]==8323072);
        small[0]=-32768;small[1]=32767;source.bits=16;
        assert(pt_voice_init(v,&source,0,2,PT_VOICE_ONCE,0,0,Q,0)==PT_PCM_OK);
        assert(pt_voice_frame(v,stereo)==PT_PCM_OK && stereo[0]==-8388608);
        assert(pt_voice_frame(v,stereo)==PT_PCM_OK && stereo[0]==8388352);
    }
    puts("VOICE mix PASS: true24 stereo, single quantization, clipping, 16 voices, mute progression, capacity and alias refusal");
}
static void partition(void)
{
    int32_t data[64],a[2048],b[2048];struct pt_pcm pcm={data,64,32,44100,2,24};
    struct pt_pcm dest={a,2048,1024,48000,2,24};struct pt_voice one[3],many[3];uint32_t gain[3][2]={{65536,0},{0,65536},{32768,32768}};
    uint64_t clipped,total=0,chunk_clips;unsigned i,offset;uint32_t hash=2166136261UL;
    for(i=0;i<64;++i)data[i]=(int32_t)(i*123457UL%16000000UL)-8000000;
    assert(pt_voice_init(one,&pcm,1,30,PT_VOICE_FORWARD,7,29,UINT64_MAX,1)==PT_PCM_OK);
    assert(pt_voice_init(one+1,&pcm,0,31,PT_VOICE_PINGPONG,2,31,Q*9+Q/3,1)==PT_PCM_OK);
    assert(pt_voice_init(one+2,&pcm,4,21,PT_VOICE_ONCE,0,0,Q/7,1)==PT_PCM_OK);memcpy(many,one,sizeof(one));
    assert(pt_voice_mix(one,3,gain,&dest,&clipped)==PT_PCM_OK);
    for(offset=0;offset<1024;) {
        unsigned count=(offset%17)+1;if(count>1024-offset)count=1024-offset;
        dest.data=b+offset*2;dest.capacity=count*2;dest.frames=count;
        assert(pt_voice_mix(many,3,gain,&dest,&chunk_clips)==PT_PCM_OK);total+=chunk_clips;offset+=count;
    }
    assert(clipped==total && !memcmp(a,b,sizeof(a)) && !memcmp(one,many,sizeof(one)));
    for(i=0;i<64;++i)assert(data[i]==(int32_t)(i*123457UL%16000000UL)-8000000);
    for(i=0;i<2048;++i) {unsigned byte;for(byte=0;byte<4;++byte)hash=(hash^((uint32_t)a[i]>>(byte*8)&255))*16777619UL;}
    printf("VOICE partition PASS hash=%08lx clips=%lu\n",(unsigned long)hash,(unsigned long)clipped);
}
int main(void) {traversal();mixing();partition();return 0;}
