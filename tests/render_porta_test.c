#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"
#include "pitch.h"
#include "document.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
struct capture {uint16_t period[128];uint8_t volume[128],active[128],reset[128];unsigned ticks;uint64_t phase,frames;};
static int receive(void *ctx,const struct pt_pcm *block,uint64_t offset)
{
    struct capture *c=ctx;unsigned i;assert(offset==c->frames && block->channels==2 && block->bits==24);
    for(i=0;i<block->frames;++i) {
        unsigned tick=(unsigned)((offset+i)/960);int32_t expected;
        assert(tick<c->ticks);
        if((offset+i)%960==0 && c->reset[tick])c->phase=0;
        if(!c->active[tick]) {assert(!block->data[i*2] && !block->data[i*2+1]);continue;}
        assert(c->period[tick]);
        expected=(int32_t)((c->phase>>32)%64)*1024*c->volume[tick];
        assert(block->data[i*2]==expected && block->data[i*2+1]==0);
        c->phase+=(428ULL<<32)/c->period[tick];
    }
    c->frames+=block->frames;return 1;
}
static int counted(void *ctx,const struct pt_pcm *b,uint64_t offset)
{unsigned *n=ctx;(void)b;(void)offset;++*n;return 1;}
static void boundaries(void)
{
    struct pt_project p;struct pt_sample samples[2];struct pt_event events[64*16];uint16_t order=0;
    struct pt_render_options o;struct pt_render_report report,before;int32_t data[2]={1,2};uint32_t marker=0;unsigned calls=0,i;
    memset(&p,0,sizeof(p));memset(samples,0,sizeof(samples));memset(events,0,sizeof(events));memset(&o,0,sizeof(o));
    pt_channels_init(&p.channels);p.events=events;p.samples=samples;p.sample_count=2;p.orders=&order;p.order_count=p.pattern_count=1;p.speed=1;p.bpm=125;
    for(i=0;i<2;++i) {
        samples[i].pcm.data=data;samples[i].pcm.frames=samples[i].pcm.capacity=2;samples[i].pcm.bits=24;samples[i].pcm.channels=1;samples[i].pcm.rate=48000;
        samples[i].volume=64;samples[i].loop=PT_LOOP_FORWARD;samples[i].loop_end=2;
    }
    events[0].kind=events[4].kind=PT_NOTE_PERIOD;events[0].pitch=428;events[0].instrument=1;
    events[4].pitch=214;events[4].instrument=2;events[4].effect=3;events[4].parameter=7;events[8].effect=15;
    o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=200000;
    memset(&report,0x55,sizeof(report));before=report;
    assert(pt_project_validate(&p,NULL)==PT_PROJECT_OK);
    assert(pt_render_measure(&p,&o,NULL,NULL,&report)==PT_RENDER_EFFECT && !memcmp(&report,&before,sizeof(report)));
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&report)==PT_RENDER_EFFECT && !calls && !memcmp(&report,&before,sizeof(report)));
    events[4].instrument=1;events[4].slice=1;samples[0].slices=&marker;samples[0].slice_count=1;
    assert(pt_project_validate(&p,NULL)==PT_PROJECT_OK);
    assert(pt_render_stream(&p,&o,counted,&calls,NULL,NULL,&report)==PT_RENDER_EFFECT && !calls && !memcmp(&report,&before,sizeof(report)));
    {
        struct pt_flow f;struct pt_pitch state;unsigned ch,tick;
        assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);memset(events,0,sizeof(events));memset(&f,0,sizeof(f));
        f.project=&p;f.fresh=1;pt_pitch_init(&state);
        for(ch=0;ch<16;++ch) {
            state.channel[ch].period=state.channel[ch].output=428;state.channel[ch].instrument=state.channel[ch].sounding=1;
            events[ch].kind=PT_NOTE_PERIOD;events[ch].pitch=214;events[ch].effect=f.effect[ch]=3;events[ch].parameter=f.parameter[ch]=(uint8_t)(ch+1);
        }
        pt_pitch_tick(&state,&f,65535);
        for(ch=0;ch<16;++ch)assert(state.channel[ch].period==428 && state.channel[ch].target==214 && state.channel[ch].up);
        f.fresh=0;f.counter=1;pt_pitch_tick(&state,&f,65535);
        for(ch=0;ch<16;++ch)assert(state.channel[ch].period==427-ch && state.channel[ch].speed==ch+1);
        memset(events,0,sizeof(events));f.fresh=1;f.counter=0;
        for(ch=0;ch<16;++ch) {events[ch].effect=f.effect[ch]=5;events[ch].parameter=f.parameter[ch]=0xf1;}
        pt_pitch_tick(&state,&f,65535);f.fresh=0;f.counter=1;pt_pitch_tick(&state,&f,65535);
        for(ch=0;ch<16;++ch)assert(state.channel[ch].period==426-2*ch && state.channel[ch].speed==ch+1);
        for(tick=0;tick<256;++tick)pt_pitch_tick(&state,&f,65535);
        for(ch=0;ch<16;++ch)assert(state.channel[ch].period==214 && !state.channel[ch].target);
        f.fresh=1;f.counter=0;
        for(ch=0;ch<16;++ch) {
            events[ch].kind=PT_NOTE_PERIOD;events[ch].pitch=428;events[ch].effect=f.effect[ch]=3;events[ch].parameter=f.parameter[ch]=0;
        }
        pt_pitch_tick(&state,&f,65535);f.fresh=0;f.counter=1;pt_pitch_tick(&state,&f,65535);
        for(ch=0;ch<16;++ch)assert(state.channel[ch].period==215+ch && state.channel[ch].speed==ch+1 && !state.channel[ch].up);
    }
    puts("PORTA 16-track state PASS: independent speed memory, target arrival and direction reversal");
    puts("PORTA boundaries PASS: cross-sample glide and slice-target refusal before sink/report mutation");
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_render_options o;
    struct pt_render_report report,before;struct capture c;FILE *f;long n;uint8_t *data;char line[180];unsigned count=0,zero=0,first=0,fetches=0,active=0;
    if(argc==1) {boundaries();return 0;}
    assert(argc==3);memset(&c,0,sizeof(c));memset(&o,0,sizeof(o));o.rate=48000;o.bits=24;o.gain_q16=65536;o.tracks=1;o.tick_limit=100;o.frame_limit=200000;
    f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));n=ftell(f);assert(n>0 && n<1000000);rewind(f);
    data=malloc((size_t)n);assert(data && fread(data,1,(size_t)n,f)==(size_t)n);fclose(f);
    pt_document_init(&d,&a);assert(pt_document_load(&d,data,(size_t)n,SIZE_MAX)==PT_PROJECT_OK);free(data);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f));assert(strstr(line,"FLOW schema=1 bytes=52 count=") && strstr(line,"reason=native-stop"));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        uint8_t r[52];unsigned i;assert(strlen(line)==107 && count<128);
        for(i=0;i<52;++i) {char h[3];memcpy(h,line+2+i*2,2);h[2]=0;r[i]=(uint8_t)strtoul(h,NULL,16);}
        assert(r[12]==0 && r[13]==125);
        if(!first && (r[28] || r[29]))first=count+1;
        {
            unsigned fetched=(unsigned)r[28]*256+r[29],start=fetched!=fetches && (r[31]&1);
            if(start)active=1;
            c.active[c.ticks]=(uint8_t)active;c.reset[c.ticks]=(uint8_t)start;fetches=fetched;
        }
        if(first && r[14]) {c.period[c.ticks]=(uint16_t)((unsigned)r[44]*256+r[45]);c.volume[c.ticks]=r[32];if(active && !c.period[c.ticks])zero=1;++c.ticks;}
        ++count;
    }
    assert(first==6 && c.ticks==count-first && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    d.project.channels.track[0].pan=0;d.project.samples[0].pcm.rate=48000;
    memset(&report,0x55,sizeof(report));before=report;
    if(zero) {
        assert(pt_render_measure(&d.project,&o,NULL,NULL,&report)==PT_RENDER_EFFECT && !memcmp(&report,&before,sizeof(report)));
        assert(pt_render_stream(&d.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_EFFECT && !c.frames && !memcmp(&report,&before,sizeof(report)));
        puts("PORTA PCM PASS: zero period refused in preflight before sink/report mutation");
    } else {
        assert(pt_render_stream(&d.project,&o,receive,&c,NULL,NULL,&report)==PT_RENDER_OK);
        assert(c.frames==(uint64_t)c.ticks*960 && report.frames==c.frames && !report.clipped);
        printf("PORTA PCM native-trace PASS: ticks=%u frames=%lu\n",c.ticks,(unsigned long)c.frames);
    }
    pt_document_release(&d);return 0;
}
