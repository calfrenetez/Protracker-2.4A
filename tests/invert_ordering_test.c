#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "invert_sequence.h"
#include "invert_bank.h"
static void *allocate(void *c,size_t n){(void)c;return malloc(n);}
static void release(void *c,void *p){(void)c;free(p);}
static unsigned word(const unsigned char *p){return p[0]*256U+p[1];}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_flow flow;
    struct pt_invert_sequence state;struct pt_invert_bank owned={0};uint8_t selected[31]={0};
    struct pt_invert_pcm *bank;
    int32_t *original[31]={0};FILE *f;unsigned char *bytes;long length;
    char line[512];unsigned ticks=0,checked=0,i;unsigned long sample_start[31]={0};int bound[31]={0};unsigned record_bytes,ch;
    assert(argc==3);f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));length=ftell(f);assert(length>0);rewind(f);
    bytes=malloc((size_t)length);assert(bytes && fread(bytes,1,(size_t)length,f)==(size_t)length && !fclose(f));
    pt_document_init(&d,&a);assert(pt_document_load(&d,bytes,(size_t)length,SIZE_MAX)==PT_PROJECT_OK);free(bytes);
    assert(d.project.sample_count==31);
    memset(&state,0,sizeof(state));
    for(i=0;i<31;++i)selected[i]=d.project.samples[i].pcm.frames!=0;
    assert(pt_invert_bank_open(&owned,d.project.samples,31,selected,SIZE_MAX,&a)==PT_INVERT_BANK_OK);
    bank=owned.entries;
    for(i=0;i<31;++i)if(selected[i]) {
        if(!d.project.samples[i].loop)bank[i].pcm.data[0]=bank[i].pcm.data[1]=0;
        original[i]=malloc(d.project.samples[i].pcm.frames*sizeof(int32_t));assert(original[i]);
        memcpy(original[i],d.project.samples[i].pcm.data,d.project.samples[i].pcm.frames*sizeof(int32_t));
    }
    assert(pt_flow_init(&flow,&d.project,PT_FLOW_CLASSIC128,0,100)==PT_FLOW_TICK);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f) && sscanf(line,"FLOW schema=1 bytes=%u",&record_bytes)==1 && (record_bytes==164 || record_bytes==188));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        unsigned char r[188];unsigned long cursor;unsigned inst;
        assert(strlen(line)==record_bytes*2+3);
        for(i=0;i<record_bytes;++i){char hex[3]={line[2+i*2],line[3+i*2],0};r[i]=(unsigned char)strtoul(hex,NULL,16);}
        assert(pt_flow_tick(&flow)==PT_FLOW_TICK);++ticks;
        if(!flow.active)continue;
        assert(pt_invert_sequence_tick(&state,&flow,15,bank,31)==PT_PCM_OK);
        if(!state.instrument[0])continue;
        for(ch=0;ch<(record_bytes==188?2U:1U);++ch) {
        if(!state.instrument[ch])continue;
        inst=state.instrument[ch]-1;
        /* Trace addresses are relative to metadata, but PCM is a separate
           Chip allocation. Bind its stable loop address, since9xx changes n_start;
           retain every subsequent cursor delta. */
        if(!bound[inst]){sample_start[inst]=(((unsigned long)word(r+58+22*ch)*65536+word(r+60+22*ch))-d.project.samples[inst].loop_start)&0xffffffffUL;bound[inst]=1;}
        assert(((sample_start[inst]+d.project.samples[inst].loop_start)&0xffffffffUL)==(unsigned long)word(r+58+22*ch)*65536+word(r+60+22*ch));
        cursor=(unsigned long)word(r+140+24*ch)*65536+word(r+142+24*ch);
        if(cursor!=((sample_start[inst]+state.channel[ch].cursor)&0xffffffffUL) || state.channel[ch].speed!=(r[144+24*ch]>>4) || state.channel[ch].accumulator!=r[145+24*ch]) {
            fprintf(stderr,"INVERT clock mismatch tick=%u fresh=%u counter=%u cursor=%lu/%lu speed=%u/%u accumulator=%u/%u\n",ticks,flow.fresh,flow.counter,(unsigned long)(((sample_start[inst]+state.channel[ch].cursor)&0xffffffffUL)),cursor,state.channel[ch].speed,r[144+24*ch]>>4,state.channel[ch].accumulator,r[145+24*ch]);return 20;
        }
        for(i=0;i<(d.project.samples[inst].loop?16U:2U);++i)if((unsigned char)bank[inst].pcm.data[d.project.samples[inst].loop_start+i]!=r[148+24*ch+i]) {
            fprintf(stderr,"INVERT shared PCM mismatch tick=%u index=%u got=%u expected=%u\n",ticks,i,(unsigned char)bank[inst].pcm.data[d.project.samples[inst].loop_start+i],r[148+24*ch+i]);return 20;
        }
        }
        for(i=0;i<31;++i)if(selected[i])assert(!memcmp(original[i],d.project.samples[i].pcm.data,d.project.samples[i].pcm.frames*sizeof(int32_t)));++checked;
    }
    assert(!flow.active && checked && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    pt_invert_bank_close(&owned);for(i=0;i<31;++i)free(original[i]);pt_document_release(&d);
    printf("INVERT ordering native parity PASS: ticks=%u checked=%u immutable master\n",ticks,checked);return 0;
}
