#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "flow.h"
static void *allocate(void *ctx,size_t n) {(void)ctx;return malloc(n);}
static void release(void *ctx,void *p) {(void)ctx;free(p);}
static void w16(uint8_t *b,unsigned n) {b[0]=(uint8_t)(n>>8);b[1]=(uint8_t)n;}
static void w32(uint8_t *b,uint32_t n) {w16(b,n>>16);w16(b+2,n&65535);}
static uint8_t *read_file(const char *name,size_t *bytes)
{
    FILE *f=fopen(name,"rb");long length;uint8_t *data;assert(f);
    assert(!fseek(f,0,SEEK_END));length=ftell(f);assert(length>0 && length<1000000);rewind(f);
    data=malloc((size_t)length);assert(data);assert(fread(data,1,(size_t)length,f)==(size_t)length);fclose(f);*bytes=(size_t)length;return data;
}
static void tick_n(struct pt_flow *s,unsigned count)
{while(count--)assert(pt_flow_tick(s)==PT_FLOW_TICK);}
static void boundaries(void)
{
    struct pt_project p;struct pt_flow s,before;struct pt_event events[2*64*16];uint16_t orders[256];unsigned ch;
    memset(&p,0,sizeof(p));memset(events,0,sizeof(events));memset(orders,0,sizeof(orders));
    pt_channels_init(&p.channels);p.events=events;p.orders=orders;p.order_count=2;p.pattern_count=2;p.speed=6;p.bpm=125;
    memset(&s,0x55,sizeof(s));before=s;
    assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,0,0)==PT_FLOW_INVALID && !memcmp(&s,&before,sizeof(s)));
    assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,2,10)==PT_FLOW_INVALID && !memcmp(&s,&before,sizeof(s)));
    assert(pt_flow_init(&s,&p,(enum pt_flow_mode)2,0,10)==PT_FLOW_INVALID && !memcmp(&s,&before,sizeof(s)));
    orders[1]=2;assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,0,10)==PT_FLOW_INVALID);orders[1]=0;
    assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,0,2)==PT_FLOW_TICK);tick_n(&s,2);before=s;
    assert(pt_flow_tick(&s)==PT_FLOW_LIMIT && !memcmp(&s,&before,sizeof(s)));
    s.active=0;before=s;assert(pt_flow_tick(&s)==PT_FLOW_STOPPED && !memcmp(&s,&before,sizeof(s)));
    assert(pt_flow_tick(NULL)==PT_FLOW_INVALID);
    /* All tracks process global effects, including muted/MIDI tracks, in order.
       Extended positions use the full byte instead of native seven-bit wrap. */
    assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);p.order_count=256;
    p.channels.track[15].muted=1;p.channels.track[15].route=PT_MIDI;
    events[0].effect=11;events[0].parameter=128;events[15].effect=13;events[15].parameter=0x0a;
    assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,0,20)==PT_FLOW_INVALID);
    assert(pt_flow_init(&s,&p,PT_FLOW_EXTENDED256,0,20)==PT_FLOW_TICK);tick_n(&s,6);
    assert(s.order==128 && s.row==10 && s.played_order==0 && s.played_row==0);tick_n(&s,6);
    assert(s.played_order==128 && s.played_row==10);
    events[0].parameter=255;events[15].parameter=63; /* D3F = 45 decimal. */
    assert(pt_flow_init(&s,&p,PT_FLOW_EXTENDED256,0,20)==PT_FLOW_TICK);tick_n(&s,6);
    assert(s.order==255 && s.row==45);
    memset(events,0,sizeof(events));p.speed=1;
    assert(pt_flow_init(&s,&p,PT_FLOW_EXTENDED256,255,100)==PT_FLOW_TICK);tick_n(&s,64);
    assert(s.order==0 && s.row==0 && s.fetches==64 && s.played_order==255 && s.played_row==63);
    /* Last track wins speed, but cannot undo an earlier stop. */
    p.speed=6;events[0].effect=15;events[0].parameter=0;events[15].effect=15;events[15].parameter=31;
    assert(pt_flow_init(&s,&p,PT_FLOW_EXTENDED256,0,10)==PT_FLOW_TICK);tick_n(&s,6);
    assert(!s.active && s.speed==31 && s.row==1 && pt_flow_tick(&s)==PT_FLOW_STOPPED);
    /* Supported channel-count range; no audio-route assumption in row flow. */
    memset(events,0,sizeof(events));p.order_count=1;
    for(ch=1;ch<=16;++ch) {
        assert(pt_channels_resize(&p.channels,ch)==PT_CHANNEL_OK);
        assert(pt_flow_init(&s,&p,PT_FLOW_EXTENDED256,0,6)==PT_FLOW_TICK);tick_n(&s,6);assert(s.fresh && s.row==1);
    }
    /* Classic natural wrap at the 127 boundary; repeat order entries are valid. */
    assert(pt_channels_resize(&p.channels,4)==PT_CHANNEL_OK);p.order_count=128;
    events[0].effect=15;events[0].parameter=1;
    assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,127,100)==PT_FLOW_TICK);tick_n(&s,69);
    assert(s.order==0 && s.row==0 && s.played_order==127 && s.played_row==63);
    p.order_count=129;assert(pt_flow_init(&s,&p,PT_FLOW_CLASSIC128,0,100)==PT_FLOW_INVALID);
    puts("FLOW boundaries PASS: atomic init, stop/limit, 1..16 tracks, 128/256 positions, order wrap and ordered globals");
}
int main(int argc,char **argv)
{
    struct pt_allocator allocator={NULL,allocate,release};struct pt_document doc;struct pt_flow s,before;
    uint8_t *input,*trace,b[30];size_t bytes,n,i;unsigned budget;enum pt_flow_result result;
    if(argc==1) {boundaries();return 0;}
    assert(argc==4);budget=(unsigned)strtoul(argv[3],NULL,10);assert(budget);
    input=read_file(argv[1],&bytes);trace=read_file(argv[2],&n);assert(n%36==0);
    pt_document_init(&doc,&allocator);assert(pt_document_load(&doc,input,bytes,SIZE_MAX)==PT_PROJECT_OK);
    assert(pt_flow_init(&s,&doc.project,PT_FLOW_CLASSIC128,0,budget)==PT_FLOW_TICK);
    for(i=0;i<n/36;++i) {
        uint32_t previous=s.fetches;assert(pt_flow_tick(&s)==PT_FLOW_TICK);
        memset(b,0,sizeof(b));w32(b,s.ticks);b[4]=(uint8_t)s.played_order;b[5]=(uint8_t)s.order;
        w16(b+6,s.played_row*16);w16(b+8,s.row*16);b[10]=s.counter;b[11]=s.speed;w16(b+12,s.bpm);
        b[14]=s.active?255:0;b[15]=s.pending_delay;b[16]=s.delay;b[17]=s.break_row;
        b[18]=s.jump?255:0;b[19]=s.loop_break?255:0;memcpy(b+20,s.loop_start,4);memcpy(b+24,s.loop_count,4);w16(b+28,s.fetches&65535);
        if(memcmp(b,trace+i*36,30)) {
            fprintf(stderr,"FLOW mismatch tick %lu in %s\n",(unsigned long)i+1,argv[1]);
            for(bytes=0;bytes<30;++bytes)if(b[bytes]!=trace[i*36+bytes])fprintf(stderr,"byte %lu: host %u native %u\n",(unsigned long)bytes,b[bytes],trace[i*36+bytes]);
            return 20;
        }
        assert(s.fresh==(s.fetches!=previous));assert(!(s.fresh && s.delayed));
    }
    before=s;result=pt_flow_tick(&s);
    assert(result==(trace[n-36+14]?PT_FLOW_LIMIT:PT_FLOW_STOPPED));assert(!memcmp(&s,&before,sizeof(s)));
    printf("FLOW parity PASS ticks=%lu reason=%s\n",(unsigned long)(n/36),result==PT_FLOW_LIMIT?"tick-budget":"native-stop");
    free(input);free(trace);pt_document_release(&doc);return 0;
}
