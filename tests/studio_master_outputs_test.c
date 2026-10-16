#include "../src/core/studio_mix.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct master {struct pt_pcm pcm;unsigned pins;};
struct fixture {
    struct master master[2];
    void *mix_storage;
    size_t mix_bytes;
    unsigned allocations,acquires,releases;
};
static void *allocate(void *opaque,size_t bytes)
{
    struct fixture *f=opaque;void *p=malloc(bytes);
    if(p){assert(!f->allocations);f->mix_storage=p;f->mix_bytes=bytes;++f->allocations;}
    return p;
}
static void release(void *opaque,void *p)
{
    struct fixture *f=opaque;assert(f->allocations==1 && p==f->mix_storage);
    --f->allocations;free(p);
}
static int acquire(void *opaque,uint64_t key,uint64_t version,struct pt_pcm *pcm,void **token)
{
    struct fixture *f=opaque;struct master *m;
    if(key<1 || key>2 || version!=1)return 0;
    m=&f->master[key-1];++m->pins;++f->acquires;*pcm=m->pcm;*token=m;return 1;
}
static void unpin(void *opaque,void *token)
{
    struct fixture *f=opaque;struct master *m=token;
    assert((m==&f->master[0] || m==&f->master[1]) && m->pins);
    --m->pins;++f->releases;
}
static int32_t value(unsigned bits,unsigned frame,unsigned side)
{
    static const int32_t low[4]={1,2,-3,4};
    static const int32_t wide[4]={1,257,-513,1025};
    int32_t n=bits==8?low[frame]:wide[frame];return side?-n:n;
}
static void one(unsigned bits,unsigned channels,unsigned which)
{
    struct fixture f;struct pt_studio_mix *mix;unsigned i,j,source=which/3;
    struct pt_allocator a;struct pt_studio_source s;
    struct pt_studio_note note={1,1,1ULL<<32,0,4,0,4,{65536,65536},PT_VOICE_FORWARD,0};
    int32_t audio[4]={111,222,333,444},saved_audio[4];
    struct pt_pcm output={audio,4,1,48000,2,24},*out=&output;
    uint64_t clips=77,*clipped=&clips;unsigned char before[2][384];void *mix_before;
    unsigned acquired,released;enum pt_pcm_result result;
    memset(&f,0,sizeof(f));a=(struct pt_allocator){&f,allocate,release};
    s=(struct pt_studio_source){&f,acquire,unpin};
    for(i=0;i<2;++i){
        f.master[i].pcm=(struct pt_pcm){malloc(384),96,4,48000,channels,bits};
        assert(f.master[i].pcm.data);memset(f.master[i].pcm.data,0x23,384);
        for(j=0;j<4*channels;++j)f.master[i].pcm.data[j]=value(bits,j/channels,j%channels);
    }
    /* Output descriptors and counters occupy genuine unused capacity before
       either immutable source is pinned. Audio frames remain valid. */
    if(which<6){
        unsigned kind=which%3;unsigned char *base=(unsigned char *)f.master[source].pcm.data;
        if(kind==0)output.data=(int32_t *)(void *)(base+64);
        else if(kind==1){out=(struct pt_pcm *)(void *)(base+128);memcpy(out,&output,sizeof(output));}
        else{clipped=(uint64_t *)(void *)(base+96);memcpy(clipped,&clips,sizeof(clips));}
    }
    mix=pt_studio_open(&a,&s,1);assert(mix && f.allocations==1);
    assert(pt_studio_trigger(mix,0,&note)==PT_PCM_OK);
    assert(pt_studio_repeat(mix,0,2,1,0,4)==PT_PCM_OK);
    assert(f.master[0].pins==1 && f.master[1].pins==1);
    if(which==6)output.data=f.mix_storage;
    if(which==7)clipped=f.mix_storage;
    mix_before=malloc(f.mix_bytes);assert(mix_before);
    memcpy(mix_before,f.mix_storage,f.mix_bytes);memcpy(saved_audio,audio,sizeof(audio));
    for(i=0;i<2;++i)memcpy(before[i],f.master[i].pcm.data,384);
    acquired=f.acquires;released=f.releases;
    result=pt_studio_read(mix,out,clipped);
    if(result!=PT_PCM_ALIAS){printf("STUDIO MASTER OUTPUT REFUSAL FAILED:%u:%u:%u result=%u\n",bits,channels,which,(unsigned)result);fflush(stdout);}
    assert(result==PT_PCM_ALIAS);
    assert(!memcmp(mix_before,f.mix_storage,f.mix_bytes));
    for(i=0;i<2;++i)assert(!memcmp(before[i],f.master[i].pcm.data,384) && f.master[i].pins==1);
    assert(f.acquires==acquired && f.releases==released && !memcmp(audio,saved_audio,sizeof(audio)));
    /* Refusal neither advances phase nor drops the pending handoff. */
    output=(struct pt_pcm){audio,4,1,48000,2,24};clips=77;
    assert(pt_studio_read(mix,&output,&clips)==PT_PCM_OK && !clips);
    assert(audio[0]==value(bits,0,0)*(int32_t)(1U<<(24-bits)));
    assert(audio[1]==value(bits,0,channels==2)*(int32_t)(1U<<(24-bits)));
    assert(f.master[0].pins==1 && f.master[1].pins==1 && f.releases==released);
    pt_studio_close(mix);assert(!f.allocations && !f.master[0].pins && !f.master[1].pins && f.releases==released+2);
    for(i=0;i<2;++i){assert(!memcmp(before[i],f.master[i].pcm.data,384));free(f.master[i].pcm.data);}
    free(mix_before);
}
int main(void)
{
    unsigned bits,channels,which;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)for(which=0;which<8;++which)one(bits,channels,which);
    puts("STUDIO MASTER OUTPUT PASS:48 refusals/48 controls;8/16/24 mono/stereo;current/pending full capacity and opaque owner;phase/pins/bytes preserved");
    return 0;
}
