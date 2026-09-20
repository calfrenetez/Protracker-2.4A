#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"
#include "document.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
struct capture {uint8_t volume[256];unsigned ticks,bits,ramp;uint64_t frames;};
static int receive(void *ctx,const struct pt_pcm *block,uint64_t offset)
{
    struct capture *c=ctx;uint32_t i;
    assert(offset==c->frames && block->bits==c->bits && block->channels==2);
    for(i=0;i<block->frames;++i) {
        uint64_t frame=offset+i;unsigned tick=(unsigned)(frame/960);int32_t expected;
        assert(tick<c->ticks);
        /* 8-bit +64 decodes to 2^22; unity gain gives 16384 at 16-bit.
           The seven-frame ramp separately proves silent phase progression. */
        expected=(int32_t)(c->ramp?frame%7+1:65536)*c->volume[tick];
        if(c->bits==16)expected=(expected+128)/256;
        assert(block->data[i*2]==expected && block->data[i*2+1]==0);
    }
    c->frames+=block->frames;return 1;
}
static struct pt_render_options options(void)
{
    struct pt_render_options o;memset(&o,0,sizeof(o));o.rate=48000;o.bits=16;o.gain_q16=65536;
    o.tracks=1;o.tick_limit=1000;o.frame_limit=1000000;return o;
}
static void boundaries(void)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[64*16];uint16_t order=0;
    struct pt_render_options o=options();struct pt_render_report report;
    struct capture c;int32_t pcm[7];unsigned i,ch;
    memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));pt_channels_init(&p.channels);
    assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);p.events=events;p.orders=&order;p.order_count=p.pattern_count=1;
    p.samples=&sample;p.sample_count=1;p.speed=1;p.bpm=125;
    sample.pcm.data=pcm;sample.pcm.capacity=sample.pcm.frames=7;sample.pcm.rate=48000;sample.pcm.channels=1;sample.pcm.bits=24;
    sample.volume=1;sample.loop=PT_LOOP_FORWARD;sample.loop_end=7;
    for(i=0;i<7;++i)pcm[i]=(int32_t)(i+1)*64;
    memset(events,0,sizeof(events));
    for(ch=0;ch<16;++ch) {
        p.channels.track[ch].pan=0;
        events[ch].kind=PT_NOTE_PERIOD;events[ch].instrument=1;events[ch].pitch=428;events[ch].effect=14;events[ch].parameter=0xc0;
        events[16+ch].effect=12;events[16+ch].parameter=(uint8_t)(ch+1);
        events[32+ch].effect=14;events[32+ch].parameter=0xc0;
    }
    events[48].effect=15;
    /* Sum 1..16 = 136. All voices continue through silent tick zero;
       the second pass isolates the upper mask bit without repeating 16 songs. */
    for(i=0;i<2;++i) {
        memset(&c,0,sizeof(c));o.tracks=(uint16_t)(i?0x8000:0xffff);o.bits=c.bits=24;c.ramp=1;c.ticks=3;
        c.volume[1]=(uint8_t)(i?16:136);
        assert(pt_render_stream(&p,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK && c.frames==3*960 && !report.clipped);
    }
    puts("VOLUME boundaries PASS: EC0 and Cxx restore retain silent phase across 16 voices and isolated track 16");
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document doc;struct pt_render_options o=options();
    struct pt_render_report report;struct capture c;FILE *f;long bytes;uint8_t *data;char line[160];unsigned count=0,first=0,stopped=0;
    if(argc==1) {boundaries();return 0;}
    assert(argc==3);memset(&c,0,sizeof(c));c.bits=16;
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));bytes=ftell(f);assert(bytes>0 && bytes<1000000);rewind(f);
    data=malloc((size_t)bytes);assert(data && fread(data,1,(size_t)bytes,f)==(size_t)bytes);fclose(f);
    pt_document_init(&doc,&a);assert(pt_document_load(&doc,data,(size_t)bytes,SIZE_MAX)==PT_PROJECT_OK);free(data);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f));
    assert(strstr(line,"FLOW schema=1 bytes=36 count=") && strstr(line,"reason=native-stop"));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        uint8_t record[36];unsigned i;
        assert(strlen(line)==75 && count<256);
        for(i=0;i<36;++i) {char hex[3];memcpy(hex,line+2+i*2,2);hex[2]=0;record[i]=(uint8_t)strtoul(hex,NULL,16);}
        assert(record[12]==0 && record[13]==125); /* Exactly 960 frames per tick. */
        if(!first && (record[28] || record[29]))first=count+1;
        if(record[14])c.volume[count]=record[32];else {assert(!stopped);stopped=1;}
        ++count;
    }
    assert(stopped && first==6 && count>first && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    c.ticks=count-first;memmove(c.volume,c.volume+first-1,c.ticks);
    /* Constant mono source isolates volume from reference pitch/timing policies. */
    doc.project.channels.track[0].pan=0;doc.project.samples[0].pcm.rate=48000;
    assert(pt_render_stream(&doc.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK);
    assert(c.frames==(uint64_t)c.ticks*960 && report.frames==c.frames && report.ticks==count && !report.clipped);
    printf("VOLUME native-trace PCM PASS: ticks=%u frames=%lu\n",c.ticks,(unsigned long)c.frames);
    pt_document_release(&doc);return 0;
}
