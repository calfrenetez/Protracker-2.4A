#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/mod_file.h"
#include "document.h"
static struct pt_event events[65*256];
static int32_t pcm[3][2050],before[3][2050];
static uint8_t gold[80000],output[80000],header[1084];
struct sink_state {size_t pos,fail;};
static int collect(void *context,const uint8_t *data,size_t n)
{struct sink_state *s=context;assert(n<=1084);if(s->pos>=s->fail)return 0;assert(s->pos+n<=sizeof(output));memcpy(output+s->pos,data,n);s->pos+=n;return 1;}
struct budget {size_t live,peak;unsigned refuse;};
static void *allocate(void *context,size_t n)
{struct budget *b=context;void *p;if(b->refuse)return NULL;assert(!b->live && n<16384);p=malloc(n);assert(p);b->live=n;b->peak=n;return p;}
static void release(void *context,void *p)
{struct budget *b=context;assert(b->live);b->live=0;free(p);}
static int mod_stream_fixture(int argc,char **argv)
{
    struct pt_project p;struct pt_sample samples[3];uint16_t orders[1]={0};
    struct pt_extension ext={PT_CLASSIC_HEADER_TAG,1084,1,header};
    struct budget b={0};struct pt_allocator a={&b,allocate,release};
    unsigned legacy,policy,i,j;size_t written;struct sink_state sink;FILE *f;
    assert(argc==2);memset(&p,0,sizeof(p));memset(samples,0,sizeof(samples));
    pt_channels_init(&p.channels);p.bpm=125;p.speed=6;p.mode=PT_MODE_WAVETABLE;
    p.order_count=1;p.pattern_count=65;p.orders=orders;p.events=events;p.samples=samples;p.sample_count=3;
    strcpy(p.title,"stream test");p.extensions=&ext;header[950]=1;header[951]=42;memcpy(header+1080,"M!K!",4);
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[64*256+255].effect=15;events[64*256+255].parameter=125;
    for(legacy=0;legacy<2;++legacy)for(policy=0;policy<3;++policy) {
        p.extension_count=(uint16_t)legacy;
        for(i=0;i<3;++i) {
            unsigned bits=policy?8+8*i:8;
            samples[i].pcm=(struct pt_pcm){pcm[i],2050,2050,PT_CLASSIC_RATE,1,(uint8_t)bits};samples[i].volume=64;
            for(j=0;j<2050;++j)pcm[i][j]=((int32_t)(j%256)-128)*(1L<<(bits-8))+(bits>8?(int32_t)(j%127):0);
            samples[i].loop=i==1?PT_LOOP_FORWARD:PT_LOOP_NONE;samples[i].loop_start=i==1?2:0;samples[i].loop_end=i==1?2048:0;
        }
        memcpy(before,pcm,sizeof(pcm));
        assert((policy==2?pt_mod_export_tpdf8(&p,gold,sizeof(gold),&written):policy?pt_mod_export_round8(&p,gold,sizeof(gold),&written):pt_mod_export_direct(&p,gold,sizeof(gold),&written))==PT_PROJECT_OK);
        sink=(struct sink_state){0,SIZE_MAX};assert(pt_mod_export_stream(&p,policy,collect,&sink)==PT_PROJECT_OK && sink.pos==written && !memcmp(gold,output,written));
        sink=(struct sink_state){0,1084};assert(pt_mod_export_stream(&p,policy,collect,&sink)==PT_PROJECT_INVALID && sink.pos==1084);
        b.refuse=1;assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_MEMORY);b.refuse=0;
        assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_OK && !b.live);
        assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_PUBLISH && !b.live);
        f=fopen(argv[1],"rb");assert(f && fread(output,1,written,f)==written && fgetc(f)==EOF && !fclose(f));
        assert(!memcmp(gold,output,written) && !memcmp(before,pcm,sizeof(pcm)));assert(!unlink(argv[1]));
        sink=(struct sink_state){0,SIZE_MAX};p.samples[0].pcm.rate++;
        assert(pt_mod_export_stream(&p,policy,collect,&sink)==PT_PROJECT_UNSUPPORTED && !sink.pos);
        assert(pt_mod_file_save(argv[1],&p,policy,&a)==PT_SAVE_INVALID && !b.live && access(argv[1],F_OK));p.samples[0].pcm.rate--;
    }
    sink=(struct sink_state){0,SIZE_MAX};assert(pt_mod_export_stream(&p,3,collect,&sink)==PT_PROJECT_INVALID && !sink.pos);
    assert(pt_mod_export_stream(&p,0,collect,&sink)==PT_PROJECT_UNSUPPORTED && !sink.pos);
    printf("MOD STREAM PASS: direct/round8/TPDF exact multi-block bytes, legacy headers, loops, 65 patterns, source and destination protection; workspace=%lu\n",(unsigned long)b.peak);
    return 0;
}
#ifndef PT_MOD_STREAM_NATIVE
int main(int argc,char **argv) {return mod_stream_fixture(argc,argv);}
#endif
