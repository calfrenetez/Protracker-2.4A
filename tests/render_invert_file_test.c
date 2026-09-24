#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "render_invert_file.h"
#include "document.h"
struct memory {unsigned calls,fail,live;};
static void *allocate(void *c,size_t n){struct memory *m=c;void *p;if(++m->calls==m->fail)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void release(void *c,void *p){struct memory *m=c;assert(m->live);--m->live;free(p);}
static int cancel(void *c,enum pt_render_phase phase,uint32_t t,uint64_t f){(void)c;(void)t;(void)f;return phase!=PT_RENDER_VERIFY;}
int main(int argc,char **argv)
{
    struct pt_project p={0};struct pt_sample sample={0};struct pt_event events[256]={0};uint16_t order=0;
    int32_t pcm[4]={10,20,30,40};struct pt_render_options o={0};struct pt_render_report r,before;
    struct memory m={0};struct pt_allocator a={&m,allocate,release};enum pt_render_result detail;unsigned i,calls;FILE *f;unsigned char header[44];
    assert(argc==2);pt_channels_init(&p.channels);p.events=events;p.orders=&order;p.order_count=p.pattern_count=p.sample_count=1;p.samples=&sample;p.speed=3;p.bpm=125;
    sample.pcm.data=pcm;sample.pcm.capacity=sample.pcm.frames=4;sample.pcm.rate=48000;sample.pcm.bits=8;sample.pcm.channels=1;sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=4;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;events[0].effect=14;events[0].parameter=255;events[4].effect=15;
    o.rate=48000;o.bits=24;o.tracks=15;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    assert(pt_render_invert_file_new(argv[1],&p,&o,NULL,NULL,&r,&detail,100000,&a)==PT_RENDER_FILE_OK && !m.live);calls=m.calls;
    f=fopen(argv[1],"rb");assert(f && fread(header,1,44,f)==44 && !memcmp(header,"RIFF",4));
    for(i=0;i<2880;++i){unsigned char raw[6];unsigned index=i%4,tick=i/960;int value=(int)(index+1)*10;if(index && index<=tick+1)value=-1-value;
        assert(fread(raw,1,6,f)==6 && raw[0]==0 && raw[1]==0 && raw[2]==(unsigned char)value && !raw[3] && !raw[4] && !raw[5]);}
    assert(fgetc(f)==EOF && !fclose(f));assert(!unlink(argv[1]));
    memset(&before,0x55,sizeof(before));
    for(i=1;i<=calls;++i){memset(&m,0,sizeof(m));m.fail=i;r=before;
        assert(pt_render_invert_file_new(argv[1],&p,&o,NULL,NULL,&r,&detail,100000,&a)!=PT_RENDER_FILE_OK);
        assert(!m.live && access(argv[1],F_OK)!=0 && !memcmp(&before,&r,sizeof(r)) && pcm[0]==10 && pcm[1]==20 && pcm[2]==30 && pcm[3]==40);
    }
    memset(&m,0,sizeof(m));r=before;
    assert(pt_render_invert_file_new(argv[1],&p,&o,cancel,NULL,&r,&detail,100000,&a)==PT_RENDER_FILE_VERIFY && detail==PT_RENDER_CANCELLED && !m.live && access(argv[1],F_OK)!=0);
    puts("INVERT FILE PASS: exact WAV, fresh verification workspace, all allocation failures, verification cancellation and cleanup");return 0;
}
