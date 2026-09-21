#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "render.h"
static void *alloc(void *c,size_t n){(void)c;return malloc(n);}
static void drop(void *c,void *p){(void)c;free(p);}
static unsigned word(const unsigned char *p){return p[0]*256U+p[1];}
static unsigned long lng(const unsigned char *p){return (unsigned long)word(p)*65536+word(p+2);}
struct oracle {unsigned char *data;unsigned loop[100],length[100],volume[100],ticks,pos,end;uint64_t frames;};
static int receive(void *ctx,const struct pt_pcm *pcm,uint64_t offset)
{
    struct oracle *o=ctx;unsigned i;assert(offset==o->frames);
    for(i=0;i<pcm->frames;++i) {
        unsigned tick=(unsigned)((offset+i)/960);int value;assert(tick<o->ticks);
        value=o->data[o->pos];if(value>127)value-=256;
        assert(pcm->data[2*i]==value*1024*(int)o->volume[tick] && pcm->data[2*i+1]==0);
        if(++o->pos==o->end){o->pos=o->loop[tick];o->end=o->pos+o->length[tick];}
    }
    o->frames+=pcm->frames;return 1;
}
static int count(void *c,const struct pt_pcm *p,uint64_t n){(void)p;(void)n;++*(unsigned *)c;return 1;}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,alloc,drop};struct pt_document d;struct oracle o;
    struct pt_render_options options;struct pt_render_report report,before;FILE *f;long size;char line[512];unsigned calls=0,i;
    assert(argc==4);memset(&o,0,sizeof(o));memset(&options,0,sizeof(options));
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));size=ftell(f);rewind(f);o.data=malloc(size);assert(o.data && fread(o.data,1,size,f)==(size_t)size);fclose(f);
    pt_document_init(&d,&a);assert(pt_document_load(&d,o.data,size,SIZE_MAX)==PT_PROJECT_OK);
    for(i=0;i<d.project.sample_count;++i)d.project.samples[i].pcm.rate=48000;
    d.project.channels.track[0].pan=0;
    options.rate=48000;options.bits=24;options.gain_q16=65536;options.tracks=1;options.tick_limit=100;options.frame_limit=100000;
    if(atoi(argv[3])) {
        f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f));
        while(fgets(line,sizeof(line),f) && line[0]=='T') {
            unsigned char r[140];for(i=0;i<140;++i){char hex[3]={line[2+i*2],line[3+i*2],0};r[i]=(unsigned char)strtoul(hex,NULL,16);}
            if(!r[14] || !word(r+28))continue;
            assert(o.ticks<100);o.loop[o.ticks]=(unsigned)lng(r+58);o.length[o.ticks]=word(r+62)*2;o.volume[o.ticks++]=r[32];
        }
        fclose(f);o.pos=2108;o.end=3132;
        assert(pt_render_stream(&d.project,&options,receive,&o,NULL,NULL,&report)==PT_RENDER_OK);
        assert(report.frames==o.frames && o.frames==(uint64_t)o.ticks*960);
    } else {
        memset(&report,0xa5,sizeof(report));before=report;
        assert(pt_render_stream(&d.project,&options,count,&calls,NULL,NULL,&report)==PT_RENDER_EFFECT && !calls && !memcmp(&report,&before,sizeof(report)));
    }
    /* Adding a real delayed note must still refuse the cross-sample switch. */
    d.project.events[4].kind=PT_NOTE_PERIOD;d.project.events[4].pitch=428;
    d.project.events[4].effect=14;d.project.events[4].parameter=0xd3;
    calls=0;memset(&report,0xa5,sizeof(report));before=report;
    {
        enum pt_render_result refused=pt_render_stream(&d.project,&options,count,&calls,NULL,NULL,&report);
        assert((refused==PT_RENDER_EFFECT || refused==PT_RENDER_SAMPLE) && !calls && !memcmp(&report,&before,sizeof(report)));
    }
    pt_document_release(&d);free(o.data);puts("HANDOFF renderer PASS");return 0;
}
