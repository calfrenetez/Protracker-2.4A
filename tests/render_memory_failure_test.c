#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "document.h"
#include "../src/platform/stem_file.h"
static unsigned calls,fail_at,live;
static void *allocate(void *ctx,size_t n)
{
    void *p;(void)ctx;assert(!live && n && n<65536);
    if(++calls==fail_at)return NULL;
    p=malloc(n);assert(p);++live;return p;
}
static void release(void *ctx,void *p) {(void)ctx;assert(live==1 && p);--live;free(p);}
static void absent(const char *path,const char *suffix)
{
    char staging[1500];unsigned i;
    assert(access(path,F_OK)!=0);
    for(i=0;i<32;++i) {
        snprintf(staging,sizeof(staging),"%s.%s-%lu-%u",path,suffix,(unsigned long)getpid(),i);
        assert(access(staging,F_OK)!=0);
    }
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_project p,original;
    struct pt_sample sample,original_sample;struct pt_event events[256],original_events[256];
    uint16_t order=0;int32_t pcm[2]={0x123457,-0x345679};
    struct pt_render_options options;struct pt_render_report report,before;
    struct pt_stem_report stems,stems_before;enum pt_render_result detail;
    char path[1200];unsigned i;enum pt_render_file_result result;
    assert(argc==2 && strlen(argv[1])<1000);
    memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&options,0,sizeof(options));
    pt_channels_init(&p.channels);p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;
    p.samples=&sample;p.sample_count=1;p.speed=1;p.bpm=125;
    sample.pcm=(struct pt_pcm){pcm,2,1,48000,2,24};sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=1;
    for(i=0;i<2;++i) {events[i].kind=PT_NOTE_PERIOD;events[i].pitch=428;events[i].instrument=1;}
    events[4].effect=15;
    options.rate=48000;options.bits=24;options.gain_q16=32768;options.tracks=3;options.tick_limit=100;options.frame_limit=10000;
    memset(&report,0x5a,sizeof(report));before=report;memset(&stems,0x3c,sizeof(stems));stems_before=stems;
    original=p;original_sample=sample;memcpy(original_events,events,sizeof(events));
    snprintf(path,sizeof(path),"%s/memory.wav",argv[1]);
    /* Measurement, mixing after WAV header staging, and verification after render. */
    for(i=1;i<=3;++i) {
        calls=0;fail_at=i;
        result=pt_render_file_new_allocated(path,&p,&options,NULL,NULL,&report,&detail,&a);
        assert(result==(i==3?PT_RENDER_FILE_VERIFY:PT_RENDER_FILE_RENDER));
        assert(detail==PT_RENDER_MEMORY && calls==i && !live && !memcmp(&report,&before,sizeof(report)));
        absent(path,"pttmp");
    }
    snprintf(path,sizeof(path),"%s/memory-stems",argv[1]);
    /* Two stem preflights then measure/mix/verify per stem. Later failures must
       remove already verified earlier stems along with the owned staging tree. */
    for(i=1;i<=8;++i) {
        calls=0;fail_at=i;
        result=pt_stem_file_new_allocated(path,&p,&options,0,NULL,NULL,&stems,&detail,&a);
        assert(result==((i==5 || i==8)?PT_RENDER_FILE_VERIFY:PT_RENDER_FILE_RENDER));
        assert(detail==PT_RENDER_MEMORY && calls==i && !live && !memcmp(&stems,&stems_before,sizeof(stems)));
        absent(path,"ptstems");
    }
    assert(!memcmp(&p,&original,sizeof(p)) && !memcmp(&sample,&original_sample,sizeof(sample)));
    assert(!memcmp(events,original_events,sizeof(events)) && order==0 && pcm[0]==0x123457 && pcm[1]==-0x345679);
    puts("RENDER MEMORY PASS: all3 WAV and8 stem allocation failures preserve masters/reports and remove owned staging");return 0;
}
