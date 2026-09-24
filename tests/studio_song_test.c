#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "studio_song.h"
static int32_t reference[300000];static unsigned used,owned,pins,refuse,attempt,fail_at,fail_pin;
static struct pt_pcm master;
static void *allocate(void *c,size_t n) {(void)c;if(refuse || ++attempt==fail_at)return NULL;++owned;return malloc(n);}
static void release(void *c,void *p) {(void)c;assert(owned);--owned;free(p);}
static int acquire(void *c,uint64_t key,uint64_t version,struct pt_pcm *p,void **token)
{(void)c;if(fail_pin || key!=1 || version!=1)return 0;*p=master;*token=&master;++pins;return 1;}
static void unpin(void *c,void *p) {(void)c;assert(p==&master && pins);--pins;}
static int capture(void *c,const struct pt_pcm *p,uint64_t offset)
{(void)c;assert(offset*2==used && used+p->frames*2<=300000);memcpy(reference+used,p->data,p->frames*2*sizeof(int32_t));used+=p->frames*2;return 1;}
#ifndef PT_SONG_FIRST_BLOCK
#define PT_SONG_FIRST_BLOCK 1
#endif
int main(void)
{
    struct pt_project p={0};struct pt_sample sample;struct pt_event events[64*4];uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_allocator a={NULL,allocate,release};
    struct pt_studio_source provider={NULL,acquire,unpin};struct pt_studio_binding binding;
    int32_t pcm[8]={1,257,-513,799,123,991,-777,27};unsigned mode,partition;
    memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));pt_channels_init(&p.channels);
    p.samples=&sample;p.sample_count=1;p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.speed=3;p.bpm=125;
    sample.pcm=(struct pt_pcm){pcm,8,8,48000,1,24};sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=8;
    master=sample.pcm;binding=(struct pt_studio_binding){&sample.pcm,1,1};
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[4].effect=15;events[4].parameter=131;
    events[8].effect=14;events[8].parameter=0xe1;
    events[12].effect=10;events[12].parameter=1;
    events[16].kind=PT_NOTE_PERIOD;events[16].pitch=320;
    events[20].effect=15;events[20].parameter=0;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=1000;o.frame_limit=1000000;
    for(mode=0;mode<3;++mode)for(partition=PT_SONG_FIRST_BLOCK;partition<=256;partition=partition==1?17:partition==17?256:257) {
        struct pt_studio_song *s=NULL;struct pt_render_report report;unsigned emitted=0,done=0;
        const struct pt_pcm *block;
        o.include_lead_in=mode==1;o.pattern_only=o.row_range=mode==2;o.row_first=2;o.row_end=5;
        used=0;assert(pt_render_stream(&p,&o,capture,NULL,NULL,NULL,&report)==PT_RENDER_OK);
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK && owned==3);
        assert(pt_studio_song_pull(s,257,&block,&done)==PT_RENDER_INVALID);
        while(!done) {
            assert(pt_studio_song_pull(s,partition,&block,&done)==PT_RENDER_OK);
            if(block) {assert(emitted+block->frames*2<=used);assert(!memcmp(block->data,reference+emitted,block->frames*2*sizeof(int32_t)));emitted+=block->frames*2;}
        }
        assert(emitted==used && emitted==report.frames*2 && !pins && owned==1);
        pt_studio_song_close(s);assert(!owned);
    }
    {struct pt_studio_song *s=NULL;const struct pt_pcm *block;unsigned done,i;
        for(i=1;i<=3;++i) {attempt=0;fail_at=i;
            assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_MEMORY && !s && !owned && !pins);
        }
        fail_at=0;
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK);
        for(i=0;i<30 && !pins;++i)assert(pt_studio_song_pull(s,17,&block,&done)==PT_RENDER_OK);
        assert(pins);pt_studio_song_stop(s);assert(!pins && owned==1);
        assert(pt_studio_song_pull(s,17,&block,&done)==PT_RENDER_OK && done && !block);
        pt_studio_song_stop(s);pt_studio_song_close(s);assert(!owned);
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK);
        fail_pin=1;
        for(i=0;i<30;++i)if(pt_studio_song_pull(s,256,&block,&done)!=PT_RENDER_OK)break;
        assert(i<30 && done && !block && !pins && owned==1);
        assert(pt_studio_song_pull(s,256,&block,&done)==PT_RENDER_SAMPLE);
        pt_studio_song_close(s);assert(!owned);fail_pin=0;
    }
    puts("SONG SESSION PASS: reference audio, tempo/delay, pre-roll, partition invariance, protocol and allocation refusal");return 0;
}
