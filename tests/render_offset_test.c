#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"
#include "document.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static unsigned word(const unsigned char *p) {return (unsigned)p[0]*256+p[1];}
static uint32_t dword(const unsigned char *p) {return ((uint32_t)word(p)<<16)|word(p+2);}
struct capture {uint32_t start[128],length[128],loop[128],repeat[128];uint8_t reset[128],active[128],volume[128];unsigned ticks,looping;uint64_t elapsed,frames;};
static int receive(void *ctx,const struct pt_pcm *b,uint64_t offset)
{
    struct capture *c=ctx;unsigned i;assert(offset==c->frames && b->bits==24);
    for(i=0;i<b->frames;++i) {
        unsigned t=(unsigned)((offset+i)/960);uint64_t position;int32_t expected=0;assert(t<c->ticks);
        if((offset+i)%960==0 && c->reset[t])c->elapsed=0;
        if(c->active[t]) {
            if(c->elapsed<c->length[t])position=c->start[t]+c->elapsed;
            else if(c->looping)position=c->loop[t]+(c->elapsed-c->length[t])%c->repeat[t];
            else position=UINT64_MAX;
            if(position!=UINT64_MAX)expected=(int32_t)(position%127)*1024*c->volume[t];
            ++c->elapsed;
        }
        assert(b->data[i*2]==expected && b->data[i*2+1]==0);
    }
    c->frames+=b->frames;return 1;
}
static int counted(void *ctx,const struct pt_pcm *b,uint64_t n) {(void)b;(void)n;++*(unsigned *)ctx;return 1;}
static void safety(void)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[64*16];uint16_t order=0;
    struct pt_render_options o;struct pt_render_report result,before;struct capture c;int32_t data[1024];unsigned i,calls=0;
    memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.samples=&sample;p.sample_count=1;p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;p.speed=1;p.bpm=125;
    for(i=0;i<1024;++i)data[i]=(int32_t)(i%127);
    sample.pcm.data=data;sample.pcm.frames=sample.pcm.capacity=1024;sample.pcm.bits=8;sample.pcm.channels=1;sample.pcm.rate=48000;sample.volume=64;
    events[15].kind=PT_NOTE_PERIOD;events[15].pitch=428;events[15].instrument=1;events[15].effect=9;events[15].parameter=1;events[16].effect=15;p.channels.track[15].pan=0;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=0x8000;o.tick_limit=100;o.frame_limit=200000;
    memset(&c,0,sizeof(c));c.ticks=1;c.start[0]=256;c.length[0]=768;c.active[0]=c.reset[0]=1;c.volume[0]=64;
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&result)==PT_RENDER_OK && result.frames==960);
    p.speed=6;events[15].effect=14;events[15].parameter=0x92;
    memset(&c,0,sizeof(c));c.ticks=6;
    for(i=0;i<6;++i) {c.length[i]=1024;c.active[i]=1;c.reset[i]=(uint8_t)(i%2==0);c.volume[i]=64;}
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&result)==PT_RENDER_OK && result.frames==5760);
    events[15].effect=9;events[15].parameter=1;events[16].effect=0;
    events[31].effect=14;events[31].parameter=0x92;events[32].effect=15;
    memset(&c,0,sizeof(c));c.ticks=12;
    for(i=0;i<12;++i) {
        c.start[i]=i<6?256:512;c.length[i]=i<6?768:512;c.active[i]=1;
        c.reset[i]=(uint8_t)(i==0 || (i>=6 && i%2==0));c.volume[i]=64;
    }
    assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&result)==PT_RENDER_OK && result.frames==11520);
    memset(events+31,0,2*sizeof(*events));events[16].effect=15;events[15].effect=14;events[15].parameter=0x92;
    sample.pcm.bits=16;calls=0;memset(&result,0x55,sizeof(result));before=result;
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&result)==PT_RENDER_SAMPLE && !calls && !memcmp(&before,&result,sizeof(result)));
    p.speed=1;events[15].effect=9;events[15].parameter=1;
    memset(&result,0x55,sizeof(result));before=result;sample.pcm.bits=16;
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&result)==PT_RENDER_SAMPLE && !calls && !memcmp(&before,&result,sizeof(result)));
    sample.pcm.bits=8;sample.pcm.frames=1023;
    assert(pt_render_measure(&p,&o,NULL,NULL,&result)==PT_RENDER_SAMPLE && !memcmp(&before,&result,sizeof(result)));
    sample.pcm.frames=1024;sample.loop=PT_LOOP_PINGPONG;sample.loop_start=0;sample.loop_end=1024;
    assert(pt_render_measure(&p,&o,NULL,NULL,&result)==PT_RENDER_SAMPLE && !memcmp(&before,&result,sizeof(result)));
    sample.loop=PT_LOOP_NONE;sample.loop_end=0;sample.pcm.channels=2;sample.pcm.frames=512;
    assert(pt_render_measure(&p,&o,NULL,NULL,&result)==PT_RENDER_SAMPLE && !memcmp(&before,&result,sizeof(result)));
    sample.pcm.channels=1;sample.pcm.frames=1024;
    {uint32_t marker=0;sample.slices=&marker;sample.slice_count=1;events[15].slice=1;
     assert(pt_render_measure(&p,&o,NULL,NULL,&result)==PT_RENDER_EFFECT && !memcmp(&before,&result,sizeof(result)));}
    puts("OFFSET safety PASS: isolated track16 PCM, non-byte/stereo/odd/pingpong/slice refusal before output/report changes");
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_render_options o;struct pt_render_report report;
    struct capture c;FILE *f;long n;unsigned char *data;char line[320];unsigned count=0,first=0,ch,previous=0;
    if(argc==1) {safety();return 0;}
    assert(argc==4);ch=(unsigned)atoi(argv[3]);assert(ch<4);memset(&c,0,sizeof(c));memset(&o,0,sizeof(o));
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);rewind(f);data=malloc((size_t)n);assert(data && fread(data,1,(size_t)n,f)==(size_t)n);fclose(f);
    pt_document_init(&d,&a);assert(pt_document_load(&d,data,(size_t)n,SIZE_MAX)==PT_PROJECT_OK);free(data);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f) && strstr(line,"bytes=140"));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        unsigned char r[140];unsigned i,t=c.ticks,base=52+22*ch;assert(strlen(line)==283 && t<128);
        for(i=0;i<140;++i) {char h[3];memcpy(h,line+2+i*2,2);h[2]=0;r[i]=(unsigned char)strtoul(h,NULL,16);}
        if(!first && word(r+28))first=count+1;
        if(first && r[14]) {
            unsigned triggers=word(r+base+20);c.active[t]=(uint8_t)(triggers!=0);c.reset[t]=(uint8_t)(triggers && triggers!=previous);previous=triggers;
            if(triggers) {assert(word(r+44+2*ch)==428);c.start[t]=dword(r+base+14)-2108;c.length[t]=word(r+base+18)*2;c.loop[t]=dword(r+base+6)-2108;c.repeat[t]=word(r+base+10)*2;}
            c.volume[t]=r[32+ch];++c.ticks;
        }
        ++count;
    }
    assert(first==6 && c.ticks==count-first && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    c.looping=d.project.samples[0].loop==PT_LOOP_FORWARD;d.project.samples[0].pcm.rate=48000;d.project.channels.track[ch].pan=0;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=(uint16_t)(1U<<ch);o.tick_limit=100;o.frame_limit=200000;
    assert(pt_render_stream(&d.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK);
    assert(report.frames==(uint64_t)c.ticks*960 && !report.clipped);
    printf("OFFSET PCM PASS: channel=%u ticks=%u frames=%lu\n",ch,c.ticks,(unsigned long)report.frames);
    pt_document_release(&d);return 0;
}
