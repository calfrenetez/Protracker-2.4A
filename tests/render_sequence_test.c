#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "studio_plan.h"
static int32_t reference[300000];static unsigned used,owned,pins,refuse;
static struct pt_pcm master;
static void *allocate(void *c,size_t n) {(void)c;if(refuse)return NULL;++owned;return malloc(n);}
static void release(void *c,void *p) {(void)c;assert(owned);--owned;free(p);}
static int acquire(void *c,uint64_t key,uint64_t version,struct pt_pcm *p,void **token)
{(void)c;if(key!=1 || version!=1)return 0;*p=master;*token=&master;++pins;return 1;}
static void unpin(void *c,void *p) {(void)c;assert(p==&master && pins);--pins;}
static int capture(void *c,const struct pt_pcm *p,uint64_t offset)
{(void)c;assert(offset*2==used && used+p->frames*2<=300000);memcpy(reference+used,p->data,p->frames*2*sizeof(int32_t));used+=p->frames*2;return 1;}
static int private_tick(void *context,const struct pt_flow *flow){(void)context;(void)flow;return 1;}
static int sequence_fixture_main(void)
{
    struct pt_project p={0};struct pt_sample sample;struct pt_event events[64*4];uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_allocator a={NULL,allocate,release};
    struct pt_studio_source provider={NULL,acquire,unpin};struct pt_studio_binding binding;
    int32_t pcm[8]={1,257,-513,799,123,991,-777,27};unsigned mode,partition,loop;
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
    for(loop=PT_LOOP_FORWARD;loop<=PT_LOOP_PINGPONG;++loop)for(mode=0;mode<3;++mode)for(partition=1;partition<=256;partition=partition==1?17:partition==17?256:257) {
        struct pt_render_sequence *s=NULL;struct pt_render_interval interval;struct pt_render_plan plan;
        struct pt_studio_mix *mix;struct pt_render_report report;unsigned emitted=0,snapshots=0,fractional=0;struct pt_render_snapshot snapshot,sentinel;
        sample.loop=(uint8_t)loop;o.include_lead_in=mode==1;o.pattern_only=o.row_range=mode==2;o.row_first=2;o.row_end=5;
        used=0;assert(pt_render_stream(&p,&o,capture,NULL,NULL,NULL,&report)==PT_RENDER_OK);
        assert(pt_render_sequence_open(&p,&o,&a,&s)==PT_RENDER_OK && s);
        mix=pt_studio_open(&a,&provider,4);assert(mix);
        assert(pt_render_sequence_consume(s,1)==PT_RENDER_INVALID);
        memset(&sentinel,0x5a,sizeof(sentinel));snapshot=sentinel;
        assert(pt_render_sequence_snapshot(s,&snapshot)==PT_RENDER_INVALID && !memcmp(&snapshot,&sentinel,sizeof(snapshot)));
        do {
            uint32_t remaining;
            assert(pt_render_sequence_next(s,&interval)==PT_RENDER_OK);remaining=interval.frames;
            assert(pt_render_sequence_next(s,&interval)==PT_RENDER_INVALID);
            assert(pt_render_sequence_snapshot(s,&snapshot)==PT_RENDER_OK && snapshot.channels==4);
            assert(pt_render_sequence_snapshot(NULL,&snapshot)==PT_RENDER_INVALID);
            assert(pt_render_sequence_snapshot(s,NULL)==PT_RENDER_INVALID);
            {struct pt_render_snapshot again;assert(pt_render_sequence_snapshot(s,&again)==PT_RENDER_OK && !memcmp(&snapshot,&again,sizeof(snapshot)));}
            if(interval.emit && remaining) {++snapshots;if((uint32_t)snapshot.voice[0].phase)++fractional;}
            if(remaining)assert(pt_render_sequence_complete(s,&plan)==PT_RENDER_INVALID && !plan.count);
            assert(pt_render_sequence_consume(s,257)==PT_RENDER_INVALID);
            while(remaining) {
                int32_t values[512];uint64_t clips;uint32_t n=remaining<partition?remaining:partition;
                struct pt_pcm out={values,512,n,48000,2,24};
                assert(pt_studio_read(mix,&out,&clips)==PT_PCM_OK);
                {int32_t resumed[512];uint64_t resumed_clips;struct pt_pcm restored={resumed,512,n,48000,2,24};
                    assert(pt_voice_mix(snapshot.voice,snapshot.channels,snapshot.gain,&restored,&resumed_clips)==PT_PCM_OK);
                    assert(clips==resumed_clips && !memcmp(values,resumed,n*2*sizeof(int32_t)));}
                if(interval.emit) {assert(emitted+n*2<=used);assert(!memcmp(values,reference+emitted,n*2*sizeof(int32_t)));emitted+=n*2;}
                assert(pt_render_sequence_consume(s,n)==PT_RENDER_OK);remaining-=n;
                {struct pt_render_snapshot refused=sentinel;assert(pt_render_sequence_snapshot(s,&refused)==PT_RENDER_INVALID && !memcmp(&refused,&sentinel,sizeof(refused)));}
            }
            assert(pt_render_sequence_complete(s,&plan)==PT_RENDER_OK);
            {struct pt_render_snapshot refused=sentinel;assert(pt_render_sequence_snapshot(s,&refused)==PT_RENDER_INVALID && !memcmp(&refused,&sentinel,sizeof(refused)));}
            assert(pt_studio_dispatch(mix,4,&plan,&binding,1)==PT_PCM_OK);
        } while(!interval.end);
        assert(emitted==used && emitted==report.frames*2 && snapshots && fractional);
        assert(pt_render_sequence_next(s,&interval)==PT_RENDER_INVALID);
        pt_studio_close(mix);pt_render_sequence_close(s);assert(!owned && !pins);
    }
    {struct pt_render_sequence *s=NULL;refuse=1;assert(pt_render_sequence_open(&p,&o,&a,&s)==PT_RENDER_MEMORY && !s && !owned);refuse=0;
        events[0].effect=14;events[0].parameter=0xf1;
        assert(pt_render_sequence_open(&p,&o,&a,&s)==PT_RENDER_EFFECT && !s && !owned);
    }
    { /* Never publish borrowed private-bank descriptors through snapshots. */
        struct pt_render_sequence *s=NULL;struct pt_render_interval interval;
        struct pt_render_snapshot snapshot,sentinel;struct pt_render_mutation mutation={&p,NULL,private_tick};
        unsigned i;sample.pcm.bits=8;sample.loop=PT_LOOP_FORWARD;events[0].effect=events[0].parameter=0;
        for(i=0;i<8;++i)pcm[i]=(int32_t)i;
        assert(pt_render_mutating_sequence_open(&p,&o,&a,&s,&mutation)==PT_RENDER_OK);
        memset(&sentinel,0x5a,sizeof(sentinel));snapshot=sentinel;
        assert(pt_render_sequence_next(s,&interval)==PT_RENDER_OK);
        assert(pt_render_sequence_snapshot(s,&snapshot)==PT_RENDER_INVALID && !memcmp(&snapshot,&sentinel,sizeof(snapshot)));
        pt_render_sequence_close(s);assert(!owned);
    }
    puts("SEQUENCE PASS: exact snapshot/resume audio, fractional loop phase, tempo/delay, pre-roll, partition invariance, protocol and allocation refusal");return 0;
}

#ifndef PT_RENDER_SEQUENCE_NATIVE
int main(void){return sequence_fixture_main();}
#endif
