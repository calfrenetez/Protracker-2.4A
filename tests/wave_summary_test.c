#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/editor/wave_summary.h"
#define FRAMES 10001
static int32_t values[FRAMES*2+128],before[FRAMES*2+128];
static struct pt_extension many_extensions[4091];
static struct pt_wave_summary_bin bins[PT_WAVE_SUMMARY_BINS+3],other[PT_WAVE_SUMMARY_BINS+3];
static void source(struct pt_project *p,struct pt_sample *s,unsigned bits,unsigned channels)
{
    unsigned f,c; int32_t magnitude=bits==8 ? 127 : bits==16 ? 32767 : 8388607;
    memset(p,0,sizeof(*p)); memset(s,0,sizeof(*s)); memset(values,0x65,sizeof(values));
    p->samples=s; p->sample_count=1;
    s->pcm.data=values; s->pcm.capacity=sizeof(values)/sizeof(values[0]);
    s->pcm.frames=FRAMES; s->pcm.rate=48000; s->pcm.bits=(uint8_t)bits; s->pcm.channels=(uint8_t)channels;
    for (f=0;f<FRAMES;++f) for (c=0;c<channels;++c) {
        int32_t v=(int32_t)((uint64_t)f*73%(unsigned)magnitude);
        if (bits==24) v|=0x53;
        if ((f+c)&1) v=-v;
        values[f*channels+c]=v;
    }
    memcpy(before,values,sizeof(values));
}
static void reference(const struct pt_sample *s,const struct pt_sample_range *view,const struct pt_wave_summary *summary)
{
    unsigned x,c; uint64_t width=(uint64_t)view->end-view->start;
    for (x=0;x<PT_WAVE_SUMMARY_COLUMNS;++x) for (c=0;c<s->pcm.channels;++c) {
        uint32_t first=view->start+(uint32_t)(x*width/PT_WAVE_SUMMARY_COLUMNS);
        uint32_t last=view->start+(uint32_t)((x+1)*width/PT_WAVE_SUMMARY_COLUMNS),f;
        int32_t lo,hi;
        if (last==first) ++last;
        if (last>view->end) last=view->end;
        lo=hi=s->pcm.data[(size_t)first*s->pcm.channels+c];
        for (f=first+1;f<last;++f) {
            int32_t v=s->pcm.data[(size_t)f*s->pcm.channels+c];
            if (v<lo) lo=v;
            if (v>hi) hi=v;
        }
        assert(summary->bins[x*s->pcm.channels+c].minimum==lo);
        assert(summary->bins[x*s->pcm.channels+c].maximum==hi);
    }
}
static unsigned finish(struct pt_wave_summary_job *j,uint64_t gen)
{
    unsigned ready=77,calls=0,total=0;
    enum pt_wave_summary_result r;
    do {
        r=pt_wave_summary_step(j,gen,&ready); ++calls;
        assert(calls<=6 && (r==PT_WAVE_SUMMARY_OK || r==PT_WAVE_SUMMARY_PENDING));
        assert(j->last_values<=PT_WAVE_SUMMARY_VALUES_PER_STEP);
        total+=j->last_values;
    } while (!ready);
    return total;
}
static void formats(void)
{
    unsigned bits,channels,k;
    const struct pt_sample_range views[]={{0,FRAMES},{13,9187},{42,43},{7,211},{500,4001}};
    for (bits=8;bits<=24;bits+=8) for (channels=1;channels<=2;++channels) {
        struct pt_project p; struct pt_sample s,old; source(&p,&s,bits,channels); old=s;
        for (k=0;k<sizeof(views)/sizeof(views[0]);++k) {
            struct pt_wave_summary_job j; struct pt_wave_summary out; unsigned total; size_t i;
            memset(&j,0x49,sizeof(j)); memset(bins,0x33,sizeof(bins));
            assert(pt_wave_summary_begin(&j,&p,0,17,views+k,bins,PT_WAVE_SUMMARY_BINS+3)==PT_WAVE_SUMMARY_OK);
            total=finish(&j,17);
            assert(total>=620*channels && total<=((views[k].end-views[k].start)+620)*channels);
            assert(pt_wave_summary_take(&j,17,&out)==PT_WAVE_SUMMARY_OK && j.state==0);
            assert(out.columns==620 && out.channels==channels && pt_wave_summary_current(&out,&p,17));
            reference(&s,views+k,&out);
            for (i=(size_t)620*channels;i<PT_WAVE_SUMMARY_BINS+3;++i)
                assert(bins[i].minimum==0x33333333 && bins[i].maximum==0x33333333);
            assert(!memcmp(values,before,sizeof(values)) && !memcmp(&s,&old,sizeof(s)));
            assert(!pt_wave_summary_current(&out,&p,18));
        }
    }
}
static void cancellation_and_stale(void)
{
    struct pt_project p; struct pt_sample s; struct pt_sample_range view={0,FRAMES};
    struct pt_wave_summary_job j; struct pt_wave_summary out,prior; unsigned ready=55,phase;
    source(&p,&s,24,2); memset(&out,0x43,sizeof(out)); prior=out;
    for (phase=0;phase<3;++phase) {
        assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,PT_WAVE_SUMMARY_BINS)==PT_WAVE_SUMMARY_OK);
        if (phase) assert(pt_wave_summary_step(&j,1,&ready)==PT_WAVE_SUMMARY_PENDING);
        if (phase==2) finish(&j,1);
        pt_wave_summary_cancel(&j);
        ready=55;
        assert(pt_wave_summary_step(&j,1,&ready)==PT_WAVE_SUMMARY_INVALID && ready==55);
        assert(pt_wave_summary_take(&j,1,&out)==PT_WAVE_SUMMARY_INVALID && !memcmp(&out,&prior,sizeof(out)));
        assert(!memcmp(values,before,sizeof(values)));
    }
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,PT_WAVE_SUMMARY_BINS)==PT_WAVE_SUMMARY_OK);
    p.samples=NULL; ready=55;
    assert(pt_wave_summary_step(&j,1,&ready)==PT_WAVE_SUMMARY_STALE && ready==55);
    pt_wave_summary_cancel(&j); p.samples=&s;
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,PT_WAVE_SUMMARY_BINS)==PT_WAVE_SUMMARY_OK);
    finish(&j,1); s.pcm.rate=44100;
    assert(pt_wave_summary_take(&j,1,&out)==PT_WAVE_SUMMARY_STALE && !memcmp(&out,&prior,sizeof(out)));
    pt_wave_summary_cancel(&j); s.pcm.rate=48000;
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,PT_WAVE_SUMMARY_BINS)==PT_WAVE_SUMMARY_OK);
    assert(pt_wave_summary_step(&j,2,&ready)==PT_WAVE_SUMMARY_STALE);
    pt_wave_summary_cancel(&j);
}
static void aliases(void)
{
    struct pt_project p,oldp; struct pt_sample s,olds; struct pt_sample_range view={0,FRAMES};
    struct pt_wave_summary_job j,oldj; struct pt_wave_summary out,prior; unsigned ready=77;
    source(&p,&s,16,2); oldp=p; olds=s; memset(&j,0x41,sizeof(j)); oldj=j;
    memset(&out,0x55,sizeof(out)); prior=out; memset(bins,0x63,sizeof(bins)); memcpy(other,bins,sizeof(bins));
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,(struct pt_wave_summary_bin *)(void *)(values+FRAMES*2),1240)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_begin((struct pt_wave_summary_job *)(void *)values,&p,0,1,&view,bins,1240)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,SIZE_MAX)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,619)==PT_WAVE_SUMMARY_CAPACITY);
    assert(!memcmp(&j,&oldj,sizeof(j)) && !memcmp(bins,other,sizeof(bins)));
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,1240)==PT_WAVE_SUMMARY_OK); oldj=j;
    assert(pt_wave_summary_step(&j,1,&j.last_values)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_step(&j,1,(unsigned *)(void *)(values+FRAMES*2+17))==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_step(&j,1,(unsigned *)(void *)&s.pcm.frames)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_step(&j,1,(unsigned *)(void *)bins)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_step(&j,1,(unsigned *)(uintptr_t)(UINTPTR_MAX-1))==PT_WAVE_SUMMARY_ALIAS);
    assert(!memcmp(&j,&oldj,sizeof(j)) && !memcmp(values,before,sizeof(values)) && !memcmp(bins,other,sizeof(bins)));
    finish(&j,1); oldj=j;
    assert(pt_wave_summary_take(&j,1,(struct pt_wave_summary *)(void *)bins)==PT_WAVE_SUMMARY_ALIAS);
    assert(pt_wave_summary_take(&j,1,(struct pt_wave_summary *)(void *)(values+FRAMES*2))==PT_WAVE_SUMMARY_ALIAS);
    assert(!memcmp(&j,&oldj,sizeof(j)) && !memcmp(values,before,sizeof(values)));
    assert(pt_wave_summary_take(&j,1,&out)==PT_WAVE_SUMMARY_OK && memcmp(&out,&prior,sizeof(out)));
    assert(!memcmp(&p,&oldp,sizeof(p)) && !memcmp(&s,&olds,sizeof(s)));
    assert(ready==77);
    /* Identity adapter accepts zero tracks, but rejects out-of-format tables. */
    p.extensions=many_extensions; p.extension_count=4091; memset(&j,0x47,sizeof(j)); oldj=j;
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,1240)==PT_WAVE_SUMMARY_INVALID);
    assert(!memcmp(&j,&oldj,sizeof(j)) && !memcmp(values,before,sizeof(values)));
    p=oldp;
    s.pcm.capacity=SIZE_MAX;
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,1240)==PT_WAVE_SUMMARY_ALIAS);
    s=olds; s.pcm.data=(int32_t *)(uintptr_t)(UINTPTR_MAX-3); s.pcm.capacity=2; s.pcm.frames=1; view.end=1;
    assert(pt_wave_summary_begin(&j,&p,0,1,&view,bins,1240)==PT_WAVE_SUMMARY_ALIAS);
}
int main(void)
{
    formats(); cancellation_and_stale(); aliases();
    puts("WAVE SUMMARY PASS: bounded precision-preserving overview; private staging/current tags");
    return 0;
}
