#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "timeline.h"
int main(void)
{
    struct pt_project p;struct pt_event events[64*4];uint16_t orders[1]={0};
    struct pt_timeline t,before;struct pt_tick_span span,old;unsigned i;
    memset(&p,0,sizeof(p));memset(events,0,sizeof(events));pt_channels_init(&p.channels);
    p.events=events;p.orders=orders;p.order_count=p.pattern_count=1;p.speed=6;p.bpm=125;
    events[0].effect=15;events[0].parameter=32;events[4].effect=15;events[4].parameter=255;
    events[8].effect=15; /* F00 at row 2. */
    assert(pt_timeline_init(&t,&p,PT_FLOW_CLASSIC128,0,48000,100,100000)==PT_TIMELINE_TICK);
    before=t;
    assert(pt_timeline_init(&t,&p,PT_FLOW_CLASSIC128,0,0,100,100000)==PT_TIMELINE_INVALID && !memcmp(&t,&before,sizeof(t)));
    assert(pt_timeline_init(&t,&p,PT_FLOW_CLASSIC128,1,48000,100,100000)==PT_TIMELINE_INVALID && !memcmp(&t,&before,sizeof(t)));
    for(i=0;i<6;++i) {
        assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK && span.frames==960);
        assert(span.start==i*960 && span.end==(i+1)*960);
    }
    assert(t.flow.fresh && t.flow.played_row==0 && t.flow.bpm==32 && span.end==5760);
    for(i=0;i<6;++i)assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK && span.frames==3750);
    assert(t.flow.fresh && t.flow.played_row==1 && t.flow.bpm==255 && span.end==28260);
    for(i=0;i<6;++i)assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK && (span.frames==470 || span.frames==471));
    assert(!t.flow.active && t.flow.played_row==2 && span.end==31083);
    before=t;old=span;assert(pt_timeline_next(&t,&span)==PT_TIMELINE_STOPPED);
    assert(!memcmp(&t,&before,sizeof(t)) && !memcmp(&span,&old,sizeof(span)));
    /* A failed interval does not apply the speed change, fetch the row, move
       the sample boundary or replace a previously returned span. */
    assert(pt_timeline_init(&t,&p,PT_FLOW_CLASSIC128,0,48000,100,5759)==PT_TIMELINE_TICK);
    for(i=0;i<5;++i)assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK);
    before=t;old=span;assert(pt_timeline_next(&t,&span)==PT_TIMELINE_FRAME_LIMIT);
    assert(!memcmp(&t,&before,sizeof(t)) && !memcmp(&span,&old,sizeof(span)) && !t.flow.fetches && t.flow.bpm==125);
    assert(pt_timeline_init(&t,&p,PT_FLOW_CLASSIC128,0,48000,5,10000)==PT_TIMELINE_TICK);
    for(i=0;i<5;++i)assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK);
    before=t;old=span;assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK_LIMIT);
    assert(!memcmp(&t,&before,sizeof(t)) && !memcmp(&span,&old,sizeof(span)));
    assert(pt_timeline_next(&t,(struct pt_tick_span *)&t)==PT_TIMELINE_INVALID && !memcmp(&t,&before,sizeof(t)));
    assert(pt_timeline_next(NULL,&span)==PT_TIMELINE_INVALID && !memcmp(&span,&old,sizeof(span)));
    /* EE2 repeats effect passes without publishing new note rows. */
    memset(events,0,sizeof(events));events[0].effect=14;events[0].parameter=0xe2;events[4].effect=15;
    assert(pt_timeline_init(&t,&p,PT_FLOW_CLASSIC128,0,48000,100,100000)==PT_TIMELINE_TICK);
    for(i=1;i<=24;++i) {
        assert(pt_timeline_next(&t,&span)==PT_TIMELINE_TICK && span.end==i*960);
        assert(t.flow.fresh==(i==6 || i==24));assert(t.flow.delayed==(i==12 || i==18));
    }
    assert(!t.flow.active && t.flow.fetches==2 && span.end==23040);
    puts("TIMELINE PASS: prior-tempo intervals, startup, final F00 boundary, delayed passes and atomic frame/tick limits");return 0;
}
