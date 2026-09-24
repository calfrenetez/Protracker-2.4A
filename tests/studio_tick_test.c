#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/core/studio_tick.h"
static unsigned live,refuse,pins;
static int32_t values[]={1,257,-513,1025};
static void *alloc(void *c,size_t n) {void *p;(void)c;if(refuse)return NULL;p=malloc(n);assert(p);++live;return p;}
static void drop(void *c,void *p) {(void)c;assert(live);--live;free(p);}
static int acquire(void *c,uint64_t k,uint64_t v,struct pt_pcm *p,void **t)
{(void)c;(void)k;(void)v;*p=(struct pt_pcm){values,4,4,48000,1,24};*t=values;++pins;return 1;}
static void release(void *c,void *t) {(void)c;assert(t==values && pins);--pins;}
static uint32_t run(unsigned block)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_studio_source src={NULL,acquire,release};
    struct pt_studio_mix *mix=pt_studio_open(&a,&src,1);struct pt_studio_tick *tick;
    struct pt_studio_note note={1,1,1ULL<<32,0,4,0,4,{65536,65536},PT_VOICE_FORWARD,0};
    int32_t data[512];struct pt_pcm out={data,512,1,48000,2,24};uint64_t clips=77,fixed=0,total=0;
    unsigned i,j;uint32_t hash=2166136261U,n;
    assert(mix);refuse=1;assert(!pt_studio_tick_open(&a,mix,1000000));refuse=0;
    tick=pt_studio_tick_open(&a,mix,1000000);assert(tick && live==2);
    assert(pt_studio_trigger(mix,0,&note)==PT_PCM_OK);
    assert(pt_studio_tick_read(tick,&out,&clips)==PT_PCM_INVALID && clips==77);
    for(i=0;i<300;++i) {
        unsigned bpm=i%3==0?127:i%3==1?131:241;
        assert(pt_studio_tick_begin(tick,31)==PT_PCM_INVALID);
        assert(pt_studio_tick_begin(tick,bpm)==PT_PCM_OK);
        fixed+=((uint64_t)48000*5<<31)/bpm;n=(uint32_t)((fixed>>32)-total);total+=n;
        assert(pt_studio_tick_remaining(tick)==n);
        assert(pt_studio_tick_begin(tick,125)==PT_PCM_INVALID);
        out.frames=257;assert(pt_studio_tick_read(tick,&out,&clips)==PT_PCM_INVALID);
        assert(pt_studio_tick_remaining(tick)==n);
        while((n=pt_studio_tick_remaining(tick))) {
            out.frames=n<block?n:block;
            assert(pt_studio_tick_read(tick,&out,&clips)==PT_PCM_OK && !clips);
            for(j=0;j<out.frames*2;++j)hash=(hash^(uint32_t)data[j])*16777619U;
        }
    }
    pt_studio_tick_close(tick);assert(pins==1); /* reader never owns mixer */
    tick=pt_studio_tick_open(&a,mix,960);assert(tick);
    assert(pt_studio_tick_begin(tick,32)==PT_PCM_CAPACITY);
    assert(pt_studio_tick_begin(tick,125)==PT_PCM_OK);
    while((n=pt_studio_tick_remaining(tick))) {out.frames=n>256?256:n;assert(pt_studio_tick_read(tick,&out,&clips)==PT_PCM_OK);}
    assert(pt_studio_tick_begin(tick,125)==PT_PCM_CAPACITY);
    pt_studio_tick_close(tick);pt_studio_close(mix);assert(!live && !pins);return hash;
}
int main(void)
{
    uint32_t hash=run(256);assert(run(17)==hash && run(1)==hash);
    printf("STUDIO TICK PASS: tempo carry, partition hash=%08lx, refusal and cleanup\n",(unsigned long)hash);return 0;
}
