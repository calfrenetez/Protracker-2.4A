#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "studio_song.h"
#include "studio_pump.h"
#include "studio_consumer.h"
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
static enum pt_render_result song_pull(void *c,unsigned n,const struct pt_pcm **p,unsigned *done) {return pt_studio_song_pull(c,n,p,done);}
static void song_stop(void *c) {pt_studio_song_stop(c);}
struct output {const struct pt_pcm *held;unsigned emitted,submits,waits;int cancel_done;};
static int output_submit(void *c,const struct pt_pcm *p)
{struct output *o=c;assert(!o->held);if(++o->submits%3==1)return 0;o->held=p;o->waits=0;return 1;}
static int output_poll(void *c)
{struct output *o=c;const struct pt_pcm *p=o->held;assert(p);
    if(++o->waits<3)return 0;
    assert(o->emitted+p->frames*2<=used);
    assert(!memcmp(p->data,reference+o->emitted,p->frames*2*sizeof(int32_t)));
    o->emitted+=p->frames*2;o->held=NULL;return 1;}
static int output_cancel(void *c)
{struct output *o=c;assert(o->held);if(!o->cancel_done)return -1;o->held=NULL;return 1;}
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
        {struct pt_studio_queue *queue=pt_studio_queue_open(&a,2);struct pt_studio_pump pump;
            struct pt_studio_producer producer={s,song_pull,song_stop};assert(queue && pt_studio_pump_init(&pump,&producer,queue));
            struct output output={0};struct pt_studio_consumer consumer={0};
            struct pt_studio_transport transport={&output,output_submit,output_poll,output_cancel};
            unsigned steps=0;assert(pt_studio_consumer_attach(&consumer,queue,&transport));
            while(!done) {
                unsigned i;enum pt_consumer_result cr;assert(++steps<3000000);
                for(i=0;i<7;++i)assert(pt_studio_pump_step(&pump,partition)!=PT_PUMP_ERROR);
                cr=pt_studio_consumer_step(&consumer);assert(cr!=PT_CONSUMER_ERROR);
                done=cr==PT_CONSUMER_FINISHED;
            }
            emitted=output.emitted;assert(pt_studio_consumer_detach(&consumer));
            assert(pt_studio_queue_close(queue)==PT_QUEUE_OK);
        }
        assert(emitted==used && emitted==report.frames*2 && !pins && owned==1);
        pt_studio_song_close(s);assert(!owned);
    }
    /* Stop while the actual song's copied audio is held by a failing transport. */
    {struct pt_studio_song *s=NULL;struct pt_studio_pump pump;
        struct pt_studio_queue *queue;struct pt_studio_consumer consumer={0};
        struct output output={0};struct pt_studio_transport transport={&output,output_submit,output_poll,output_cancel};
        struct pt_studio_producer producer;unsigned i;int32_t first;
        o.include_lead_in=o.pattern_only=o.row_range=0;
        assert(pt_studio_song_open(&p,&o,&a,&provider,&binding,1,&s)==PT_RENDER_OK);
        queue=pt_studio_queue_open(&a,2);producer=(struct pt_studio_producer){s,song_pull,song_stop};
        assert(queue && pt_studio_pump_init(&pump,&producer,queue));
        assert(pt_studio_consumer_attach(&consumer,queue,&transport));
        for(i=0;i<100 && !output.held;++i) {
            assert(pt_studio_pump_step(&pump,17)!=PT_PUMP_ERROR);
            assert(pt_studio_consumer_step(&consumer)!=PT_CONSUMER_ERROR);
        }
        assert(output.held && pins);first=output.held->data[0];
        pt_studio_pump_stop(&pump);pt_studio_song_close(s);assert(!pins);
        assert(pt_studio_consumer_stop(&consumer)==PT_CONSUMER_ERROR);
        assert(output.held->data[0]==first && pt_studio_queue_close(queue)==PT_QUEUE_BUSY);
        assert(!pt_studio_consumer_detach(&consumer));output.cancel_done=1;
        assert(pt_studio_consumer_detach(&consumer) && !output.held);
        assert(pt_studio_queue_close(queue)==PT_QUEUE_OK && !owned);
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
    puts("QUEUED SONG PASS: reference audio, tempo/delay, pre-roll, partition invariance, transport stalls/cancel ownership, protocol and allocation refusal");return 0;
}
