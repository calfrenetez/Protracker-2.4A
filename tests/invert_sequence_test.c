#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "invert_sequence.h"
static unsigned word(const unsigned char *p){return p[0]*256U+p[1];}
int main(int argc,char **argv)
{
    struct pt_project p;struct pt_sample sample;struct pt_event events[64*4];uint16_t order=0;
    struct pt_flow flow;struct pt_invert_sequence state;struct pt_invert_pcm bank;
    int32_t source[16],private_data[16];FILE *f;char line[512];unsigned i,rows=0,previous=99;
    assert(argc==4);memset(&p,0,sizeof(p));memset(&sample,0,sizeof(sample));memset(events,0,sizeof(events));memset(&flow,0,sizeof(flow));memset(&state,0,sizeof(state));
    for(i=0;i<16;++i)source[i]=(int32_t)i;
    sample.pcm.data=source;sample.pcm.capacity=sample.pcm.frames=16;sample.pcm.bits=8;sample.pcm.channels=1;sample.pcm.rate=48000;
    sample.loop=PT_LOOP_FORWARD;sample.loop_end=16;
    p.samples=&sample;p.sample_count=1;p.channels.count=4;p.orders=&order;p.order_count=p.pattern_count=1;p.events=events;
    events[0].instrument=1;events[0].effect=14;events[0].parameter=(uint8_t)(240+atoi(argv[2]));
    if(atoi(argv[3])){events[4].effect=14;events[4].parameter=240;}
    assert(pt_invert_pcm_init(&bank,&sample.pcm,private_data,16)==PT_PCM_OK);flow.project=&p;
    f=fopen(argv[1],"r");assert(f && fgets(line,sizeof(line),f));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        unsigned char r[164];unsigned row;
        for(i=0;i<164;++i){char hex[3]={line[2+i*2],line[3+i*2],0};r[i]=(unsigned char)strtoul(hex,NULL,16);}
        if(!r[14] || !word(r+28))continue;
        row=word(r+6)/16;assert(row<3);rows|=1U<<row;
        flow.played_row=(uint8_t)row;flow.counter=r[10];flow.fresh=row!=previous;previous=row;
        flow.effect[0]=events[row*4].effect;flow.parameter[0]=events[row*4].parameter;
        assert(pt_invert_sequence_tick(&state,&flow,1,&bank,1)==PT_PCM_OK);
        assert(state.channel[0].cursor==((unsigned long)word(r+140)*65536+word(r+142))-2364);
        assert(state.channel[0].speed==(r[144]>>4) && state.channel[0].accumulator==r[145]);
        for(i=0;i<16;++i){assert((unsigned char)private_data[i]==r[148+i]);assert(source[i]==(int32_t)i);}
    }
    fclose(f);assert(rows==7);puts("INVERT sequence native parity PASS");return 0;
}
