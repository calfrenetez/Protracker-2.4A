#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"
#include "document.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
struct capture {uint16_t period[128];uint8_t volume[128];unsigned ticks;uint64_t phase,frames;};
static int receive(void *ctx,const struct pt_pcm *block,uint64_t offset)
{
    struct capture *c=ctx;unsigned i;assert(offset==c->frames && block->channels==2 && block->bits==24);
    for(i=0;i<block->frames;++i) {
        unsigned tick=(unsigned)((offset+i)/960);int32_t expected;
        assert(tick<c->ticks && c->period[tick]);
        expected=(int32_t)((c->phase>>32)%64)*1024*c->volume[tick];
        assert(block->data[i*2]==expected && block->data[i*2+1]==0);
        c->phase+=(428ULL<<32)/c->period[tick];
    }
    c->frames+=block->frames;return 1;
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_render_options o;
    struct pt_render_report report,before;struct capture c;FILE *f;long n;uint8_t *data;char line[180];unsigned count=0,zero=0,first=0;
    assert(argc==3);memset(&c,0,sizeof(c));memset(&o,0,sizeof(o));o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=200000;
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>0 && n<1000000);rewind(f);
    data=malloc((size_t)n);assert(data && fread(data,1,(size_t)n,f)==(size_t)n);fclose(f);
    pt_document_init(&d,&a);assert(pt_document_load(&d,data,(size_t)n,SIZE_MAX)==PT_PROJECT_OK);free(data);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f));assert(strstr(line,"FLOW schema=1 bytes=52 count=") && strstr(line,"reason=native-stop"));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        uint8_t r[52];unsigned i;assert(strlen(line)==107 && count<128);
        for(i=0;i<52;++i) {char h[3];memcpy(h,line+2+i*2,2);h[2]=0;r[i]=(uint8_t)strtoul(h,NULL,16);}
        assert(r[12]==0 && r[13]==125);
        if(!first && (r[28] || r[29]))first=count+1;
        if(first && r[14]) {c.period[c.ticks]=(uint16_t)((unsigned)r[44]*256+r[45]);c.volume[c.ticks]=r[32];if(!c.period[c.ticks])zero=1;++c.ticks;}
        ++count;
    }
    assert(first==6 && c.ticks==count-first && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    d.project.channels.track[0].pan=0;d.project.samples[0].pcm.rate=48000;
    memset(&report,0x55,sizeof(report));before=report;
    if(zero) {
        assert(pt_render_measure(&d.project,&o,NULL,NULL,&report)==PT_RENDER_EFFECT && !memcmp(&report,&before,sizeof(report)));
        assert(pt_render_stream(&d.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_EFFECT && !c.frames && !memcmp(&report,&before,sizeof(report)));
        puts("PITCH PCM PASS: zero period refused in preflight before sink/report mutation");
    } else {
        assert(pt_render_stream(&d.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK);
        assert(c.frames==(uint64_t)c.ticks*960 && report.frames==c.frames && !report.clipped);
        printf("PITCH PCM native-trace PASS: ticks=%u frames=%lu\n",c.ticks,(unsigned long)c.frames);
    }
    pt_document_release(&d);return 0;
}
