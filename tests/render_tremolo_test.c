#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"
#include "document.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
struct capture {uint16_t period[128];uint8_t volume[128],active[128],reset[128];unsigned ticks;uint64_t phase,frames;};
static int receive(void *ctx,const struct pt_pcm *b,uint64_t offset)
{
    struct capture *c=ctx;unsigned i;assert(offset==c->frames);
    for(i=0;i<b->frames;++i) {
        unsigned t=(unsigned)((offset+i)/960);int32_t expected=0;assert(t<c->ticks);
        if((offset+i)%960==0 && c->reset[t])c->phase=0;
        if(c->active[t]) {
            assert(c->period[t]);expected=(int32_t)(((c->phase>>32)%2048)%127)*1024*c->volume[t];
            c->phase+=(428ULL<<32)/c->period[t];
        }
        assert(b->data[i*2]==expected && b->data[i*2+1]==0);
    }
    c->frames+=b->frames;return 1;
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_render_options o;
    struct pt_render_report report;struct capture c;FILE *f;long n;uint8_t *data;char line[180];
    unsigned count=0,first=0,fetches=0,active=0,ch;
    assert(argc==4);ch=(unsigned)atoi(argv[3]);assert(ch<4);memset(&c,0,sizeof(c));memset(&o,0,sizeof(o));
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);rewind(f);data=malloc((size_t)n);
    assert(data && fread(data,1,(size_t)n,f)==(size_t)n);fclose(f);pt_document_init(&d,&a);
    assert(pt_document_load(&d,data,(size_t)n,SIZE_MAX)==PT_PROJECT_OK);free(data);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f) && strstr(line,"bytes=76"));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        uint8_t r[76];unsigned i,fetched,start,t=c.ticks;assert(strlen(line)==155 && t<128);
        for(i=0;i<76;++i) {char h[3];memcpy(h,line+2+i*2,2);h[2]=0;r[i]=(uint8_t)strtoul(h,NULL,16);}
        fetched=(unsigned)r[28]*256+r[29];if(!first && fetched)first=count+1;
        start=fetched!=fetches && (r[31]&(1U<<ch));if(start)active=1;fetches=fetched;
        if(first && r[14]) {
            c.active[t]=(uint8_t)active;c.reset[t]=(uint8_t)(start!=0);c.volume[t]=r[32+ch];
            c.period[t]=(uint16_t)((unsigned)r[44+ch*2]*256+r[45+ch*2]);++c.ticks;
        }
        ++count;
    }
    assert(first==6 && c.ticks==count-first && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    /* Extend the immutable ramp into a full-sample reference loop, so every
       later volume tick stays audible. Native register traces remain inputs;
       this is reference PCM policy, not an analogue output comparison. */
    assert(d.project.samples[0].pcm.frames==2048);d.project.samples[0].pcm.rate=48000;
    d.project.samples[0].loop=PT_LOOP_FORWARD;d.project.samples[0].loop_start=0;d.project.samples[0].loop_end=2048;
    d.project.channels.track[ch].pan=0;o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=(uint16_t)(1U<<ch);o.tick_limit=100;o.frame_limit=200000;
    assert(pt_render_stream(&d.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK);
    assert(report.frames==(uint64_t)c.ticks*960 && !report.clipped);
    printf("TREMOLO PCM PASS: channel=%u ticks=%u frames=%lu\n",ch,c.ticks,(unsigned long)report.frames);
    pt_document_release(&d);return 0;
}
