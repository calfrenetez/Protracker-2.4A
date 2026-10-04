#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/editor/sample_range.h"
struct storage { int32_t audio[40]; struct pt_sample_range range; struct pt_sample_loop loop; size_t count; };
union sample_object { struct pt_sample sample; struct pt_sample_range range; struct pt_sample_loop loop; size_t count; };
static void source(struct pt_sample *s,struct storage *m,unsigned bits,unsigned channels)
{
    unsigned i;
    memset(s,0,sizeof(*s)); memset(m,0x59,sizeof(*m));
    for (i=0;i<32;++i) m->audio[i]=(int32_t)i-16;
    s->pcm.data=m->audio; s->pcm.capacity=sizeof(*m)/sizeof(int32_t);
    s->pcm.frames=16; s->pcm.rate=48000; s->pcm.bits=(uint8_t)bits; s->pcm.channels=(uint8_t)channels;
    s->loop=PT_LOOP_FORWARD; s->loop_start=4; s->loop_end=12;
}
static void formats(void)
{
    unsigned bits,channels,k;
    for (bits=8;bits<=24;bits+=8) for (channels=1;channels<=2;++channels) {
        struct storage m,old; struct pt_sample s,before; struct pt_sample_range range={2,14},out={77,88};
        struct pt_sample_loop loop; uint32_t markers[]={0,2,4,8,13,14,15},copy[9]; size_t n=99;
        source(&s,&m,bits,channels); s.slices=markers; s.slice_count=7; old=m; before=s;
        assert(pt_sample_range_validate(&s,&range)==PT_SAMPLE_RANGE_OK);
        assert(pt_sample_range_loop(&s,&out)==PT_SAMPLE_RANGE_OK && out.start==4 && out.end==12);
        for (k=PT_LOOP_FORWARD;k<=PT_LOOP_CROSSFADE;++k) {
            s.loop=(uint8_t)k; s.crossfade=k==PT_LOOP_CROSSFADE ? 2 : 0;
            assert(pt_sample_range_copy_loop(&s,&range,&loop)==PT_SAMPLE_RANGE_OK);
            assert(loop.kind==k && loop.start==2 && loop.end==10 && loop.crossfade==s.crossfade);
        }
        range.start=5;
        assert(pt_sample_range_copy_loop(&s,&range,&loop)==PT_SAMPLE_RANGE_OK);
        assert(loop.kind==PT_LOOP_NONE && !loop.start && !loop.end && !loop.crossfade);
        range.start=2; memset(copy,0x62,sizeof(copy));
        assert(pt_sample_range_copy_markers(&s,&range,copy,9,&n)==PT_SAMPLE_RANGE_OK);
        assert(n==4 && copy[0]==0 && copy[1]==2 && copy[2]==6 && copy[3]==11 && copy[4]==0x62626262UL);
        range.start=3;
        assert(pt_sample_range_copy_markers(&s,&range,copy,9,&n)==PT_SAMPLE_RANGE_OK && n==3 && copy[0]==1);
        range.start=9; range.end=12;
        assert(pt_sample_range_copy_markers(&s,&range,NULL,0,&n)==PT_SAMPLE_RANGE_OK && n==0);
        assert(!memcmp(&m,&old,sizeof(m)));
        s=before; assert(!memcmp(&s,&before,sizeof(s)));
    }
}
static void centre(void)
{
    struct pt_sample_range view={20,40},out={99,100};
    assert(pt_sample_range_centre(100,&view,30,&out)==PT_SAMPLE_RANGE_OK && out.start==20 && out.end==40);
    assert(pt_sample_range_centre(100,&view,0,&out)==PT_SAMPLE_RANGE_OK && out.start==0 && out.end==20);
    assert(pt_sample_range_centre(100,&view,100,&out)==PT_SAMPLE_RANGE_OK && out.start==80 && out.end==100);
    assert(view.start==20 && view.end==40);
    assert(pt_sample_range_centre(100,&view,99,&view)==PT_SAMPLE_RANGE_OK && view.start==80 && view.end==100);
    out.start=99; out.end=100;
    assert(pt_sample_range_centre(100,&view,101,&out)==PT_SAMPLE_RANGE_INVALID && out.start==99);
    assert(pt_sample_range_centre(100,&view,50,(struct pt_sample_range *)(uintptr_t)(UINTPTR_MAX-3))==PT_SAMPLE_RANGE_ALIAS);
}
static void refusals(void)
{
    struct storage m,old; union sample_object so,oldso; struct pt_sample_range range={2,14},out={99,100};
    uint32_t markers[]={2,4,13},oldmarkers[3],copy[3]={77,88,99}; size_t n=55; struct pt_sample_loop loop,oldloop;
    source(&so.sample,&m,24,2); so.sample.slices=markers; so.sample.slice_count=3;
    old=m; oldso=so; memcpy(oldmarkers,markers,sizeof(markers)); memset(&loop,0x43,sizeof(loop)); oldloop=loop;
    assert(pt_sample_range_loop(&so.sample,&m.range)==PT_SAMPLE_RANGE_ALIAS);
    assert(pt_sample_range_loop(&so.sample,&so.range)==PT_SAMPLE_RANGE_ALIAS);
    assert(pt_sample_range_copy_loop(&so.sample,&range,&m.loop)==PT_SAMPLE_RANGE_ALIAS);
    assert(pt_sample_range_copy_loop(&so.sample,&range,&so.loop)==PT_SAMPLE_RANGE_ALIAS);
    assert(pt_sample_range_copy_markers(&so.sample,&range,copy,3,&m.count)==PT_SAMPLE_RANGE_ALIAS);
    assert(pt_sample_range_copy_markers(&so.sample,&range,copy,3,&so.count)==PT_SAMPLE_RANGE_ALIAS);
    assert(pt_sample_range_copy_markers(&so.sample,&range,markers,3,&n)==PT_SAMPLE_RANGE_ALIAS && n==55);
    assert(pt_sample_range_copy_markers(&so.sample,&range,copy,2,&n)==PT_SAMPLE_RANGE_CAPACITY && n==55);
    assert(copy[0]==77 && copy[1]==88 && copy[2]==99);
    assert(!memcmp(&m,&old,sizeof(m)) && !memcmp(&so,&oldso,sizeof(so)) && !memcmp(markers,oldmarkers,sizeof(markers)));
    { struct adjacent { uint32_t copy[3]; uint32_t pad; size_t count; } a;
      memset(&a,0x31,sizeof(a));
      assert(pt_sample_range_copy_markers(&so.sample,&range,a.copy,100,&a.count)==PT_SAMPLE_RANGE_OK && a.count==3);
      assert(a.copy[0]==0 && a.copy[1]==2 && a.copy[2]==11 && a.pad==0x31313131UL); }
    { union paired { size_t count; uint32_t copy[4]; } p,prior;
      memset(&p,0x61,sizeof(p)); prior=p;
      assert(pt_sample_range_copy_markers(&so.sample,&range,p.copy,4,&p.count)==PT_SAMPLE_RANGE_ALIAS);
      assert(!memcmp(&p,&prior,sizeof(p))); }
    so.sample.loop=PT_LOOP_NONE; so.sample.loop_start=so.sample.loop_end=0;
    assert(pt_sample_range_loop(&so.sample,&out)==PT_SAMPLE_RANGE_NO_LOOP && out.start==99);
    so.sample.loop_end=1;
    assert(pt_sample_range_copy_loop(&so.sample,&range,&loop)==PT_SAMPLE_RANGE_INVALID && !memcmp(&loop,&oldloop,sizeof(loop)));
    so=oldso; markers[1]=markers[0];
    assert(pt_sample_range_copy_markers(&so.sample,&range,copy,3,&n)==PT_SAMPLE_RANGE_INVALID && n==55);
    memcpy(markers,oldmarkers,sizeof(markers)); so.sample.pcm.capacity=SIZE_MAX;
    assert(pt_sample_range_loop(&so.sample,&out)==PT_SAMPLE_RANGE_ALIAS && out.start==99);
    so=oldso; so.sample.pcm.data=(int32_t *)(uintptr_t)(UINTPTR_MAX-3); so.sample.pcm.capacity=2; so.sample.pcm.frames=1;
    so.sample.loop=PT_LOOP_NONE; so.sample.loop_start=so.sample.loop_end=0;
    range.start=0; range.end=1;
    assert(pt_sample_range_copy_loop(&so.sample,&range,&loop)==PT_SAMPLE_RANGE_ALIAS && !memcmp(&loop,&oldloop,sizeof(loop)));
    so=oldso; so.sample.pcm.frames=0; so.sample.pcm.data=NULL; so.sample.pcm.capacity=0;
    so.sample.loop=PT_LOOP_NONE; so.sample.loop_start=so.sample.loop_end=0; so.sample.slice_count=0; so.sample.slices=NULL;
    assert(pt_sample_range_loop(&so.sample,&out)==PT_SAMPLE_RANGE_NO_LOOP);
    assert(pt_sample_range_validate(&so.sample,&range)==PT_SAMPLE_RANGE_INVALID);
}
int main(void)
{
    formats(); centre(); refusals();
    puts("SAMPLE RANGE PASS: half-open loops/viewports/contained-copy metadata; source preserved");
    return 0;
}
