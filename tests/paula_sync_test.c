#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "paula_sync.h"
int main(void)
{
    enum { PREFIX=1084+1024, FRAMES=131070, TOTAL=PREFIX+FRAMES+4 };
    struct pt_project p={0};struct pt_sample samples[2]={{0}};
    struct pt_event events[256]={0};uint16_t order=0;
    int32_t *pcm=malloc(FRAMES*sizeof(*pcm));
    uint8_t *before=malloc(TOTAL),*after=malloc(TOTAL),workspace[PREFIX+1];
    size_t written,i;unsigned mutation;
    assert(pcm && before && after);
    for(i=0;i<FRAMES;++i)pcm[i]=(int32_t)(i%256)-128;
    pt_channels_init(&p.channels);p.bpm=125;p.speed=6;p.orders=&order;p.order_count=1;
    p.pattern_count=1;p.events=events;p.samples=samples;p.sample_count=2;
    samples[0].pcm=(struct pt_pcm){0};
    samples[0].pcm.data=pcm;samples[0].pcm.capacity=samples[0].pcm.frames=FRAMES;
    samples[0].pcm.channels=1;samples[0].pcm.bits=8;samples[0].pcm.rate=PT_CLASSIC_RATE;
    samples[0].volume=64;samples[1]=samples[0];samples[1].pcm.frames=4;
    events[0].instrument=1;events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;
    assert(pt_mod_export_direct(&p,before,TOTAL,&written)==PT_PROJECT_OK && written==TOTAL);
    /* Compare the bounded decision with the original full-export oracle. */
    for(mutation=0;mutation<7;++mutation) {
        int expected,result;
        if(mutation==1)events[0].pitch=404;
        if(mutation==2)events[0].instrument=2;
        if(mutation==3)pcm[0]++;
        if(mutation==4)pcm[FRAMES-1]--;
        if(mutation==5)samples[0].volume=32;
        if(mutation==6)strcpy(p.title,"Renamed");
        assert(pt_mod_export_direct(&p,after,TOTAL,&written)==PT_PROJECT_OK);
        expected=pt_paula_cache_compatible(before,after,TOTAL);
        memset(workspace,0xa5,sizeof(workspace));
        result=pt_paula_sync_prepare(&p,before,TOTAL,workspace,PREFIX);
        assert(result==expected && workspace[PREFIX]==0xa5);
        if(result)assert(!memcmp(workspace,after,PREFIX));
        events[0].pitch=428;events[0].instrument=1;pcm[0]=-128;
        pcm[FRAMES-1]=(FRAMES-1)%256-128;samples[0].volume=64;memset(p.title,0,sizeof(p.title));
    }
    memset(workspace,0xa5,sizeof(workspace));
    assert(!pt_paula_sync_prepare(&p,before,TOTAL,workspace,PREFIX-1) && workspace[0]==0xa5);
    assert(!pt_paula_sync_prepare(&p,before,TOTAL,before,PREFIX));
    samples[0].pcm.bits=24;
    assert(!pt_paula_sync_prepare(&p,before,TOTAL,workspace,PREFIX) && workspace[0]==0xa5);
    samples[0].pcm.bits=8;
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
    assert(pt_mod_export_direct(&p,after,TOTAL,&written)==PT_PROJECT_OK && !memcmp(before,after,TOTAL));
    free(pcm);free(before);free(after);
    puts("PAULA SYNC PASS: bounded prefix, full-export parity, sample/header refusal, split blocks, guards and unchanged masters");
    return 0;
}
