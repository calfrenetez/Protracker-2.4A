#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pitch.h"
#include "document.h"
static void *allocate(void *c,size_t n) {(void)c;return malloc(n);}
static void release(void *c,void *p) {(void)c;free(p);}
static unsigned word(const uint8_t *b) {return (unsigned)b[0]*256+b[1];}
static void boundaries(void)
{
    static const unsigned cases[][5]={{113,255,1,65394,3954},{113,15,1,113,113},{856,255,2,856,856},
        {65394,142,2,0,0},{65535,1,2,0,0},{0,0,1,113,113},{0,0,2,0,0},{0xf071,1,1,0xf071,113}};
    struct pt_project p;struct pt_flow f;struct pt_pitch s;unsigned ch,i;
    memset(&p,0,sizeof(p));pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    memset(&f,0,sizeof(f));f.project=&p;f.counter=1;
    for(ch=0;ch<16;++ch)for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        pt_pitch_init(&s);s.channel[ch].period=(uint16_t)cases[i][0];f.effect[ch]=(uint8_t)cases[i][2];f.parameter[ch]=(uint8_t)cases[i][1];
        pt_pitch_tick(&s,&f,(uint16_t)(1U<<ch));
        assert(s.channel[ch].period==cases[i][3] && s.channel[ch].output==cases[i][4]);
    }
    puts("PITCH boundaries PASS: 16 tracks, stored-word wrap, masked output, limits and zero results");
}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_flow f;struct pt_pitch s;
    FILE *file;uint8_t *data;long length;char line[180];unsigned tick=0,ch;
    if(argc==1) {boundaries();return 0;}
    assert(argc==3);file=fopen(argv[1],"rb");assert(file && !fseek(file,0,SEEK_END));length=ftell(file);assert(length>0 && length<1000000);rewind(file);
    data=malloc((size_t)length);assert(data && fread(data,1,(size_t)length,file)==(size_t)length);fclose(file);
    pt_document_init(&d,&a);assert(pt_document_load(&d,data,(size_t)length,SIZE_MAX)==PT_PROJECT_OK);free(data);
    assert(pt_flow_init(&f,&d.project,PT_FLOW_CLASSIC128,0,100)==PT_FLOW_TICK);pt_pitch_init(&s);
    file=fopen(argv[2],"r");assert(file && fgets(line,sizeof(line),file));assert(strstr(line,"FLOW schema=1 bytes=52 count=") && strstr(line,"reason=native-stop"));
    while(fgets(line,sizeof(line),file) && line[0]=='T') {
        uint8_t record[52];unsigned i;assert(strlen(line)==107);
        for(i=0;i<52;++i) {char hex[3];memcpy(hex,line+2+i*2,2);hex[2]=0;record[i]=(uint8_t)strtoul(hex,NULL,16);}
        assert(pt_flow_tick(&f)==PT_FLOW_TICK);pt_pitch_tick(&s,&f,15);++tick;
        for(ch=0;ch<4;++ch) {
            if(s.channel[ch].period!=word(record+36+2*ch) || s.channel[ch].output!=word(record+44+2*ch)) {
                fprintf(stderr,"PITCH mismatch tick=%u channel=%u stored=%u/%u output=%u/%u\n",tick,ch,s.channel[ch].period,word(record+36+2*ch),s.channel[ch].output,word(record+44+2*ch));return 20;
            }
        }
    }
    assert(!f.active && tick==f.ticks && !strcmp(line,"FLOW PASS dma=0\n"));fclose(file);pt_document_release(&d);
    printf("PITCH native parity PASS: ticks=%u stored words and output registers\n",tick);return 0;
}
