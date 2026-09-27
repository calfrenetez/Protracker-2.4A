#include "../src/core/render_lookahead.h"
static int lookahead_private_tick(void *ctx,const struct pt_flow *flow)
{(void)ctx;(void)flow;return 1;}
static void lookahead_fixture(const struct pt_allocator *a)
{
    struct pt_project p={0};struct pt_sample sample={0};struct pt_event events[256]={0};uint16_t orders[1]={0};
    struct pt_render_options o={0};struct pt_render_sequence *s=NULL,*oracle=NULL;
    struct pt_render_lookahead *w=malloc(sizeof(*w)),*other=malloc(sizeof(*other));
    struct pt_render_plan *predicted=malloc(sizeof(*predicted)),*expected=malloc(sizeof(*expected));
    struct pt_render_interval span,reference;struct pt_render_snapshot before,after;
    int32_t data[8]={1,2,3,4,5,6,7,8};unsigned tick=0,ready,steps;uint32_t remaining;
    assert(w && other && predicted && expected);memset(w,0,sizeof(*w));memset(other,0,sizeof(*other));pt_channels_init(&p.channels);
    p.samples=&sample;p.sample_count=1;p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.speed=1;p.bpm=131;
    sample.pcm=(struct pt_pcm){data,8,8,48000,1,24};sample.volume=64;sample.loop=PT_LOOP_FORWARD;sample.loop_end=8;
    events[0]=(struct pt_event){428,0,PT_NOTE_PERIOD,1,0,0,0,0};events[4].effect=1;events[4].parameter=1;events[8].effect=15;
    o.rate=48000;o.bits=24;o.tracks=1;o.gain_q16=65536;o.tick_limit=100;o.frame_limit=100000;
    assert(pt_render_sequence_open(&p,&o,a,&s)==PT_RENDER_OK);
    assert(pt_render_sequence_open(&p,&o,a,&oracle)==PT_RENDER_OK);
    assert(pt_render_lookahead_begin(w,s)==PT_RENDER_INVALID);
    do {
        assert(pt_render_sequence_next(s,&span)==PT_RENDER_OK);
        assert(pt_render_sequence_next(oracle,&reference)==PT_RENDER_OK && !memcmp(&span,&reference,sizeof(span)));
        assert(pt_render_sequence_snapshot(s,&before)==PT_RENDER_OK);
        assert(pt_render_sequence_snapshot(oracle,&after)==PT_RENDER_OK && !memcmp(&before,&after,sizeof(before)));
        remaining=span.frames;
        if(remaining){assert(pt_render_sequence_consume(s,1)==PT_RENDER_OK);assert(pt_render_sequence_consume(oracle,1)==PT_RENDER_OK);--remaining;}
        assert(pt_render_lookahead_begin(w,s)==PT_RENDER_OK);
        assert(pt_render_lookahead_begin(w,s)==PT_RENDER_INVALID);
        assert(pt_render_lookahead_begin(other,s)==PT_RENDER_OK);
        ready=7;predicted->count=123;
        assert(pt_render_lookahead_step(w,0,predicted,&ready)==PT_RENDER_INVALID && ready==7 && predicted->count==123);
        assert(pt_render_lookahead_step(w,257,predicted,&ready)==PT_RENDER_INVALID && ready==7 && predicted->count==123);
        assert(pt_render_lookahead_commit(w)==PT_RENDER_INVALID);
        steps=0;
        do {
            uint32_t n=remaining>31?31:remaining;
            assert(pt_render_lookahead_step(w,17,predicted,&ready)==PT_RENDER_OK && ++steps<100);
            if(!ready)assert(predicted->count==123);
            /* Live time consumption interleaves independently of preview progress. */
            if(n){assert(pt_render_sequence_consume(s,n)==PT_RENDER_OK);assert(pt_render_sequence_consume(oracle,n)==PT_RENDER_OK);remaining-=n;}
        }while(!ready);
        assert(!remaining);
        assert(pt_render_lookahead_step(w,256,expected,&ready)==PT_RENDER_OK && ready && !memcmp(predicted,expected,sizeof(*predicted)));
        assert(pt_render_sequence_complete(oracle,expected)==PT_RENDER_OK);
        assert(predicted->count==expected->count && !memcmp(predicted->action,expected->action,predicted->count*sizeof(*predicted->action)));
        if(tick++%2) { /* Ordinary completion invalidates independent previews. */
            assert(pt_render_sequence_complete(s,expected)==PT_RENDER_OK);
            assert(pt_render_lookahead_commit(w)==PT_RENDER_INVALID);
            pt_render_lookahead_cancel(w);
        }else assert(pt_render_lookahead_commit(w)==PT_RENDER_OK);
        ready=7;predicted->count=123;
        assert(pt_render_lookahead_step(other,17,predicted,&ready)==PT_RENDER_INVALID && ready==7 && predicted->count==123);
        assert(pt_render_lookahead_commit(other)==PT_RENDER_INVALID);
        if(!span.end)pt_render_lookahead_cancel(other);
    }while(!span.end);
    assert(pt_render_sequence_rewind(s)==PT_RENDER_OK);
    assert(pt_render_sequence_next(s,&span)==PT_RENDER_OK);
    assert(pt_render_lookahead_step(other,256,predicted,&ready)==PT_RENDER_INVALID); /* No rewind ABA. */
    pt_render_lookahead_cancel(other);
    assert(pt_render_sequence_snapshot(s,&before)==PT_RENDER_OK);
    assert(pt_render_lookahead_begin(w,s)==PT_RENDER_OK);pt_render_lookahead_cancel(w);pt_render_lookahead_cancel(w);
    assert(pt_render_sequence_snapshot(s,&after)==PT_RENDER_OK && !memcmp(&before,&after,sizeof(before)));
    assert(pt_render_lookahead_begin(w,s)==PT_RENDER_OK);pt_render_sequence_close(s);s=NULL;
    pt_render_lookahead_cancel(w); /* Cancel never dereferences a disposed sequence. */
    pt_render_sequence_close(oracle);
    {struct pt_render_mutation mutation={&p,NULL,lookahead_private_tick};
        sample.pcm.bits=8;
        assert(pt_render_mutating_sequence_open(&p,&o,a,&s,&mutation)==PT_RENDER_OK);
        assert(pt_render_sequence_next(s,&span)==PT_RENDER_OK);
        assert(pt_render_lookahead_begin(w,s)==PT_RENDER_INVALID);
        pt_render_sequence_close(s);
    }
    free(expected);free(predicted);free(other);free(w);
    puts("LOOKAHEAD PASS: bounded copied phase/commands, live consume interleave, exact plans, no early commit, stale/rewind/cancel/private-source refusal");
}
