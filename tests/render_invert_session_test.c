#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render_invert.h"
#include "studio_pump.h"
#include "studio_consumer.h"
#ifdef __amigaos__
static void test_failure(const char *condition,unsigned line)
{fprintf(stderr,"INVERT QUEUED FAIL: line=%u condition=%s\n",line,condition);exit(20);}
#undef assert
#define assert(condition) ((condition)?(void)0:test_failure(#condition,__LINE__))
#endif
static int32_t reference[300000];static unsigned used,owned,refuse,attempt,fail_at;
static void *allocate(void *c,size_t n) {(void)c;if(refuse || ++attempt==fail_at)return NULL;++owned;return malloc(n);}
static void release(void *c,void *p) {(void)c;assert(owned);--owned;free(p);}
static int capture(void *c,const struct pt_pcm *p,uint64_t offset)
{(void)c;assert(offset*2==used && used+p->frames*2<=300000);memcpy(reference+used,p->data,p->frames*2*sizeof(int32_t));used+=p->frames*2;return 1;}
#ifndef PT_SONG_FIRST_BLOCK
#define PT_SONG_FIRST_BLOCK 1
#endif
static enum pt_render_result song_pull(void *c,unsigned n,const struct pt_pcm **p,unsigned *done) {return pt_render_invert_pull(c,n,p,done);}
static void song_stop(void *c) {pt_render_invert_stop(c);}
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
    struct pt_project p={0};struct pt_sample sample[2];struct pt_event events[64*4];uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_allocator a={NULL,allocate,release};
    int32_t pcm[2][8]={{17,-93,30,40,1,99,-77,27},{3,17,-27,31,77,-83,127,-128}},original[2][8];unsigned mode,partition;
    memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));pt_channels_init(&p.channels);
    p.samples=sample;p.sample_count=2;p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.speed=3;p.bpm=125;
    sample[0].pcm=(struct pt_pcm){pcm[0],8,8,48000,1,8};sample[0].volume=64;
    sample[1]=sample[0];sample[1].pcm.data=pcm[1];sample[1].loop=PT_LOOP_FORWARD;sample[1].loop_end=8;
    memcpy(original,pcm,sizeof(pcm));
    events[1].kind=PT_NOTE_PERIOD;events[1].pitch=428;events[1].instrument=2;
    events[2].instrument=1;events[2].effect=14;events[2].parameter=0xff;
    events[18].effect=14;events[18].parameter=0xf0;
    events[0].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[4].effect=15;events[4].parameter=131;
    events[8].effect=14;events[8].parameter=0xe1;
    events[12].effect=10;events[12].parameter=1;
    events[16].kind=PT_NOTE_PERIOD;events[16].pitch=320;
    events[20].effect=15;events[20].parameter=0;
    o.rate=48000;o.bits=24;o.tracks=3;o.gain_q16=65536;o.tick_limit=1000;o.frame_limit=1000000;
    for(mode=0;mode<3;++mode)for(partition=PT_SONG_FIRST_BLOCK;partition<=256;partition=partition==1?17:partition==17?256:257) {
        struct pt_render_invert_session *s=NULL;struct pt_render_report report;unsigned emitted=0,done=0;
        const struct pt_pcm *block;
        o.include_lead_in=mode==1;o.pattern_only=o.row_range=mode==2;o.row_first=2;o.row_end=5;
        used=0;assert(pt_render_invert_stream(&p,&o,capture,NULL,NULL,NULL,&report,SIZE_MAX,&a)==PT_RENDER_OK);
        assert(pt_render_invert_open(&p,&o,SIZE_MAX,&a,&s)==PT_RENDER_OK && owned==5);
        assert(pt_render_invert_pull(s,257,&block,&done)==PT_RENDER_INVALID);
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
        assert(emitted==used && emitted==report.frames*2 && owned==1);
        pt_render_invert_close(s);assert(!owned && !memcmp(pcm,original,sizeof(pcm)));
    }
    /* Stop while the actual song's copied audio is held by a failing transport. */
    {struct pt_render_invert_session *s=NULL;struct pt_studio_pump pump;
        struct pt_studio_queue *queue;struct pt_studio_consumer consumer={0};
        struct output output={0};struct pt_studio_transport transport={&output,output_submit,output_poll,output_cancel};
        struct pt_studio_producer producer;unsigned i;int32_t first;
        o.include_lead_in=o.pattern_only=o.row_range=0;
        assert(pt_render_invert_open(&p,&o,SIZE_MAX,&a,&s)==PT_RENDER_OK);
        queue=pt_studio_queue_open(&a,2);producer=(struct pt_studio_producer){s,song_pull,song_stop};
        assert(queue && pt_studio_pump_init(&pump,&producer,queue));
        assert(pt_studio_consumer_attach(&consumer,queue,&transport));
        for(i=0;i<100 && !output.held;++i) {
            assert(pt_studio_pump_step(&pump,17)!=PT_PUMP_ERROR);
            assert(pt_studio_consumer_step(&consumer)!=PT_CONSUMER_ERROR);
        }
        assert(output.held);first=output.held->data[0];
        pt_studio_pump_stop(&pump);pt_render_invert_close(s);assert(owned==1);
        assert(pt_studio_consumer_stop(&consumer)==PT_CONSUMER_ERROR);
        assert(output.held->data[0]==first && pt_studio_queue_close(queue)==PT_QUEUE_BUSY);
        assert(!pt_studio_consumer_detach(&consumer));output.cancel_done=1;
        assert(pt_studio_consumer_detach(&consumer) && !output.held);
        assert(pt_studio_queue_close(queue)==PT_QUEUE_OK && !owned);
    }
    {struct pt_render_invert_session *s=NULL;const struct pt_pcm *block;unsigned done,i;
        for(i=1;i<=5;++i) {attempt=0;fail_at=i;
            assert(pt_render_invert_open(&p,&o,SIZE_MAX,&a,&s)==PT_RENDER_MEMORY && !s && !owned);
            assert(!memcmp(pcm,original,sizeof(pcm)));
        }
        fail_at=0;
        assert(pt_render_invert_open(&p,&o,1,&a,&s)==PT_RENDER_MEMORY && !s && !owned);
        sample[0].pcm.bits=24;
        assert(pt_render_invert_open(&p,&o,SIZE_MAX,&a,&s)==PT_RENDER_SAMPLE && !s && !owned);
        sample[0].pcm.bits=8;
        assert(pt_render_invert_open(&p,&o,SIZE_MAX,&a,&s)==PT_RENDER_OK);
        refuse=1;
        for(i=0;i<30;++i)assert(pt_render_invert_pull(s,17,&block,&done)==PT_RENDER_OK);
        refuse=0;
        assert(owned==5);pt_render_invert_stop(s);assert(owned==1);
        assert(pt_render_invert_pull(s,17,&block,&done)==PT_RENDER_OK && done && !block);
        pt_render_invert_stop(s);pt_render_invert_close(s);assert(!owned);
        assert(!memcmp(pcm,original,sizeof(pcm)));
    }
    puts("INVERT QUEUED PASS: private mixed masters, exact offline parity, tempo/delay, pre-roll, partitions, stalled/cancelled leases, protocol and allocation refusal");return 0;
}
