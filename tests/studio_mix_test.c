#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../src/core/studio_mix.h"
struct master {struct pt_pcm pcm;unsigned pins,retired;};
static int32_t data1[]={1,257,-513,8388607},data2[]={17,65537,-23,42};
static struct master masters[2];
static unsigned allocations,refuse,releases;
static void *allocate(void *ctx,size_t n) {(void)ctx;if(refuse)return NULL;++allocations;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;assert(allocations);--allocations;free(p);}
static int acquire(void *ctx,uint64_t key,uint64_t version,struct pt_pcm *p,void **token)
{
    struct master *m;(void)ctx;if(key!=1 || version<1 || version>2)return 0;
    m=masters+version-1;if(m->retired)return 0;
    ++m->pins;*p=m->pcm;*token=m;return 1;
}
static void unpin(void *ctx,void *token) {struct master *m=token;(void)ctx;assert(m->pins);--m->pins;++releases;}
int main(void)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_studio_source source={NULL,acquire,unpin};
    struct pt_studio_mix *s;struct pt_studio_note note={1,1,1ULL<<32,0,4,0,4,{65536,65536},PT_VOICE_FORWARD,0};
    int32_t out[16],expected[16];struct pt_pcm output={out,16,2,48000,2,24},reference={expected,16,8,48000,2,24};
    struct pt_voice v;uint32_t gains[1][2]={{65536,65536}};uint64_t clips=9;unsigned i,before;
    masters[0].pcm=(struct pt_pcm){data1,4,4,48000,1,24};masters[1].pcm=(struct pt_pcm){data2,4,4,48000,1,24};
    refuse=1;assert(!pt_studio_open(&a,&source,16) && !allocations);refuse=0;
    assert(!pt_studio_open(&a,&source,17));s=pt_studio_open(&a,&source,16);assert(s && allocations==1);
    assert(pt_studio_trigger(s,0,&note)==PT_PCM_OK && masters[0].pins==1);
    assert(pt_voice_init(&v,&masters[0].pcm,0,4,PT_VOICE_FORWARD,0,4,1ULL<<32,0)==PT_PCM_OK);
    assert(pt_voice_mix(&v,1,(const uint32_t (*)[2])gains,&reference,&clips)==PT_PCM_OK);
    /* Retirement prevents new triggers but the old version remains pinned and
       audibly exact across blocks; low24 bits must survive. */
    masters[0].retired=1;assert(pt_studio_trigger(s,1,&note)==PT_PCM_CAPACITY);
    for(i=0;i<4;++i) {
        assert(pt_studio_read(s,&output,&clips)==PT_PCM_OK && !clips);
        assert(!memcmp(out,expected+i*4,4*sizeof(*out)));
    }
    note.version=2;note.end=5;before=releases;
    assert(pt_studio_trigger(s,0,&note)!=PT_PCM_OK && releases==before+1 && !masters[1].pins && masters[0].pins==1);
    output.data=data1;output.capacity=4;clips=99;
    assert(pt_studio_read(s,&output,&clips)==PT_PCM_ALIAS && clips==99 && data1[0]==1);
    output.data=out;output.capacity=16;
    note.end=4;assert(pt_studio_trigger(s,0,&note)==PT_PCM_OK && !masters[0].pins && masters[1].pins==1);
    assert(pt_studio_read(s,&output,&clips)==PT_PCM_OK && out[0]==17 && out[2]==65537);
    pt_studio_stop(s,0);assert(!masters[1].pins);
    note.loop=PT_VOICE_ONCE;note.loop_start=note.loop_end=0;
    assert(pt_studio_trigger(s,15,&note)==PT_PCM_OK);output.frames=4;
    assert(pt_studio_read(s,&output,&clips)==PT_PCM_OK && !masters[1].pins);
    masters[0].retired=0;note.version=1;note.loop=PT_VOICE_FORWARD;note.loop_end=4;
    for(i=0;i<16;++i)assert(pt_studio_trigger(s,i,&note)==PT_PCM_OK);
    assert(masters[0].pins==16);
    assert(pt_studio_read(s,&output,&clips)==PT_PCM_OK && clips==2 && out[6]==8388607);
    pt_studio_close(s);
    assert(!allocations && !masters[0].pins && !masters[1].pins);
    assert(data1[1]==257 && data2[1]==65537);
    puts("STUDIO MIX PASS: pinned versions, transactional trigger, block continuity, true24, alias refusal and cleanup");return 0;
}
