#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "paula_sync.h"
struct capture {uint8_t *data;size_t bytes,capacity,calls,fail;};
static int capture(void *context,const uint8_t *data,size_t n)
{
    struct capture *c=context;
    assert(n<=1084);++c->calls;
    if(c->calls==c->fail)return 0;
    assert(n<=c->capacity-c->bytes);memcpy(c->data+c->bytes,data,n);c->bytes+=n;return 1;
}
static void padded_samples(void)
{
    enum {PREFIX=2108,MAX=131072,TOTAL=PREFIX+2*MAX};
    const unsigned lengths[]={1,3,1023,1025,131069,131070};
    struct pt_project p={0},saved;struct pt_sample samples[2]={{0}};
    struct pt_event events[256]={{0}};uint16_t order=0;
    struct pt_mod_export_report r;struct pt_paula_cache_plan plan;
    int32_t *pcm=malloc(MAX*sizeof(*pcm));uint8_t *a=malloc(TOTAL),*b=malloc(TOTAL);
    uint8_t workspace[PREFIX];unsigned bits,k,i,j;size_t n,w,pos;
    assert(pcm && a && b);pt_channels_init(&p.channels);
    p.speed=6;p.bpm=125;p.orders=&order;p.order_count=p.pattern_count=1;
    p.events=events;p.samples=samples;p.sample_count=2;
    events[0].instrument=1;events[1].instrument=2;
    samples[0].pcm.data=pcm;samples[0].pcm.capacity=MAX;samples[0].pcm.channels=1;
    samples[0].pcm.rate=PT_CLASSIC_RATE;samples[0].volume=64;
    for(bits=8;bits<=24;bits+=8)for(k=0;k<sizeof(lengths)/sizeof(*lengths);++k) {
        int32_t scale=1L<<(bits-8);struct capture c={b,0,TOTAL,0,0};
        samples[0].pcm.bits=(uint8_t)bits;samples[0].pcm.frames=lengths[k];
        samples[1]=samples[0];samples[1].pcm.frames=3;saved=p;
        for(i=0;i<MAX;++i)pcm[i]=((int32_t)(i%255)-127)*scale;
        assert(pt_mod_playback_analyse(&p,&r)==PT_PROJECT_OK);
        assert(r.issues==(uint32_t)(PT_EXPORT_PADDING|(bits>8?PT_EXPORT_PRECISION:0)));
        assert(r.classification==PT_CONVERSION_CONVERTED);
        n=PREFIX+lengths[k]+(lengths[k]&1)+4;assert(r.bytes==n);
        memset(a,0xa5,TOTAL);w=123;
        assert(pt_mod_playback_encode(&p,a,n-1,&w)==PT_PROJECT_CAPACITY && w==123 && a[0]==0xa5);
        assert(pt_mod_playback_encode(&p,a,TOTAL,&w)==PT_PROJECT_OK && w==n && a[n]==0xa5);
        assert(pt_mod_playback_stream(&p,capture,&c)==PT_PROJECT_OK && c.bytes==n && !memcmp(a,b,n));
        assert(pt_paula_cache_plan(a,n,&plan) && plan.bytes==n && plan.instruments==3);
        pos=PREFIX;
        for(j=0;j<2;++j) {
            unsigned frames=samples[j].pcm.frames,words=(frames+1)/2;
            assert(a[42+j*30]==(words>>8) && a[43+j*30]==(words&255));
            for(i=0;i<frames;++i)assert(a[pos++]==(uint8_t)((int)(i%255)-127));
            if(frames&1)assert(a[pos++]==0);
        }
        assert(pos==n && !memcmp(&p,&saved,sizeof(p)));
        for(i=0;i<MAX;++i)assert(pcm[i]==((int32_t)(i%255)-127)*scale);
        assert(pt_paula_sync_prepare(&p,a,n,workspace,sizeof(workspace)));
        pcm[lengths[k]-1]+=scale;
        assert(!pt_paula_sync_prepare(&p,a,n,workspace,sizeof(workspace)));
        pcm[lengths[k]-1]-=scale;
        /* User-facing exports still refuse odd frames without a sink write. */
        assert(pt_mod_export_analyse_round8(&p,&r)==PT_PROJECT_OK && (r.issues&PT_EXPORT_LIMITS) && !r.bytes);
        assert(pt_mod_export_round8(&p,b,TOTAL,&w)==PT_PROJECT_UNSUPPORTED);
        c.bytes=c.calls=0;
        assert(pt_mod_export_stream(&p,1,capture,&c)==PT_PROJECT_UNSUPPORTED && !c.calls);
        assert(pt_mod_export_stream(&p,3,capture,&c)==PT_PROJECT_INVALID && !c.calls);
        c.fail=3;assert(pt_mod_playback_stream(&p,capture,&c)==PT_PROJECT_INVALID && c.calls==3);
    }
    /* Padding never relaxes the maximum length or odd loop endpoints. */
    samples[0].pcm.frames=131071;
    assert(pt_mod_playback_analyse(&p,&r)==PT_PROJECT_OK && (r.issues&PT_EXPORT_LIMITS) && !r.bytes);
    samples[0].pcm.frames=5;samples[0].loop=PT_LOOP_FORWARD;samples[0].loop_end=5;
    assert(pt_mod_playback_encode(&p,a,TOTAL,&w)==PT_PROJECT_UNSUPPORTED);
    samples[0].loop_end=4;
    assert(pt_mod_playback_encode(&p,a,TOTAL,&w)==PT_PROJECT_OK);
    {uint8_t header[1084]={0};struct pt_extension e={PT_CLASSIC_HEADER_TAG,1084,1,header};
        header[950]=1;memcpy(header+1080,"M.K.",4);header[47]=2;header[49]=1;
        p.extensions=&e;p.extension_count=1;samples[0].loop=PT_LOOP_NONE;samples[0].loop_end=0;
        /* Old start2/repeat1 is unsafe in5 source frames, even though padding
         * would create a sixth byte. Reject before publishing anything. */
        assert(pt_mod_playback_analyse(&p,&r)==PT_PROJECT_OK && (r.issues&PT_EXPORT_LOOPS) && !r.bytes);
        p.extensions=NULL;p.extension_count=0;
    }
    assert(pt_mod_playback_encode(&p,(uint8_t *)pcm,MAX*sizeof(*pcm),&w)==PT_PROJECT_ALIAS);
    free(a);free(b);free(pcm);
    puts("PAULA PADDING PASS: 8/16/24-bit odd masters, exact silent bytes and stream parity, strict exports, loop/length limits, bounded sync and immutable source");
}
int main(void)
{
    enum { PREFIX=1084+1024, FRAMES=131070, TOTAL=PREFIX+FRAMES+4 };
    struct pt_project p={0};struct pt_sample samples[2]={{0}};
    struct pt_event events[256]={0};uint16_t order=0;
    int32_t *pcm=malloc(FRAMES*sizeof(*pcm));
    uint8_t *before=malloc(TOTAL),*after=malloc(TOTAL),workspace[PREFIX+1];
    size_t written,i;unsigned mutation,precision;
    assert(pcm && before && after);
    for(i=0;i<FRAMES;++i)pcm[i]=(int32_t)(i%256)-128;
    pt_channels_init(&p.channels);p.bpm=125;p.speed=6;p.orders=&order;p.order_count=1;
    p.pattern_count=1;p.events=events;p.samples=samples;p.sample_count=2;
    samples[0].pcm=(struct pt_pcm){0};
    samples[0].pcm.data=pcm;samples[0].pcm.capacity=samples[0].pcm.frames=FRAMES;
    samples[0].pcm.channels=1;samples[0].pcm.bits=8;samples[0].pcm.rate=PT_CLASSIC_RATE;
    samples[0].volume=64;samples[1]=samples[0];samples[1].pcm.frames=4;
    events[0].instrument=1;events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;
    for(precision=8;precision<=24;precision+=8) {
    int32_t scale=1L<<(precision-8);
    samples[0].pcm.bits=samples[1].pcm.bits=(uint8_t)precision;
    for(i=0;i<FRAMES;++i)pcm[i]=((int32_t)(i%256)-128)*scale;
    assert(pt_mod_export_round8(&p,before,TOTAL,&written)==PT_PROJECT_OK && written==TOTAL);
    /* Compare the bounded decision with the original full-export oracle. */
    for(mutation=0;mutation<7;++mutation) {
        int expected,result;
        if(mutation==1)events[0].pitch=404;
        if(mutation==2)events[0].instrument=2;
        if(mutation==3)pcm[0]+=scale;
        if(mutation==4)pcm[FRAMES-1]-=scale;
        if(mutation==5)samples[0].volume=32;
        if(mutation==6)strcpy(p.title,"Renamed");
        assert(pt_mod_export_round8(&p,after,TOTAL,&written)==PT_PROJECT_OK);
        expected=pt_paula_cache_compatible(before,after,TOTAL);
        memset(workspace,0xa5,sizeof(workspace));
        result=pt_paula_sync_prepare(&p,before,TOTAL,workspace,PREFIX);
        assert(result==expected && workspace[PREFIX]==0xa5);
        if(result)assert(!memcmp(workspace,after,PREFIX));
        events[0].pitch=428;events[0].instrument=1;pcm[0]=-128*scale;
        pcm[FRAMES-1]=((FRAMES-1)%256-128)*scale;samples[0].volume=64;memset(p.title,0,sizeof(p.title));
    }
    memset(workspace,0xa5,sizeof(workspace));
    assert(!pt_paula_sync_prepare(&p,before,TOTAL,workspace,PREFIX-1) && workspace[0]==0xa5);
    assert(!pt_paula_sync_prepare(&p,before,TOTAL,before,PREFIX));
    samples[0].pcm.rate=48000;
    assert(!pt_paula_sync_prepare(&p,before,TOTAL,workspace,PREFIX) && workspace[0]==0xa5);
    samples[0].pcm.rate=PT_CLASSIC_RATE;
    /* Sink offsets may split headers/events/sample boundaries arbitrarily. */
    for(i=1;i<=1084;i+=31) {
        struct pt_paula_sync_stream stream={before,workspace,TOTAL,PREFIX,0};size_t offset;
        memset(workspace,0xa5,sizeof(workspace));
        for(offset=0;offset<TOTAL;) {
            size_t n=TOTAL-offset;if(n>i)n=i;
            assert(pt_paula_sync_sink(&stream,before+offset,n));offset+=n;
        }
        assert(stream.offset==TOTAL && !memcmp(workspace,before,PREFIX) && workspace[PREFIX]==0xa5);
        assert(!pt_paula_sync_sink(&stream,before,1));
    }
    assert(pt_mod_export_round8(&p,after,TOTAL,&written)==PT_PROJECT_OK && !memcmp(before,after,TOTAL));
    for(i=0;i<FRAMES;++i)assert(pcm[i]==((int32_t)(i%256)-128)*scale);
    }
    free(pcm);free(before);free(after);padded_samples();
    puts("PAULA SYNC PASS: bounded prefix, full-export parity, sample/header refusal, split blocks, guards and unchanged masters");
    return 0;
}
