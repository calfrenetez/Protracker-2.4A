#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "invert_sequence.h"
static void *allocate(void *c,size_t n){(void)c;return malloc(n);}
static void release(void *c,void *p){(void)c;free(p);}
static unsigned word(const unsigned char *p){return p[0]*256U+p[1];}
int main(int argc,char **argv)
{
    struct pt_allocator a={NULL,allocate,release};struct pt_document d;struct pt_flow flow;
    struct pt_invert_sequence state;struct pt_invert_pcm bank[31];
    int32_t *copies[31]={0},*original;FILE *f;unsigned char *bytes;long length;
    char line[512];unsigned ticks=0,checked=0,i;unsigned long sample_start=0;int bound=0;
    assert(argc==3);f=fopen(argv[1],"rb");assert(f && !fseek(f,0,SEEK_END));length=ftell(f);assert(length>0);rewind(f);
    bytes=malloc((size_t)length);assert(bytes && fread(bytes,1,(size_t)length,f)==(size_t)length && !fclose(f));
    pt_document_init(&d,&a);assert(pt_document_load(&d,bytes,(size_t)length,SIZE_MAX)==PT_PROJECT_OK);free(bytes);
    assert(d.project.sample_count==31);
    memset(&state,0,sizeof(state));memset(bank,0,sizeof(bank));
    for(i=0;i<31;++i)if(d.project.samples[i].pcm.frames) {
        const struct pt_pcm *pcm=&d.project.samples[i].pcm;
        copies[i]=malloc(pcm->frames*sizeof(int32_t));assert(copies[i]);
        assert(pt_invert_pcm_init(bank+i,pcm,copies[i],pcm->frames)==PT_PCM_OK);
    }
    original=malloc(d.project.samples[0].pcm.frames*sizeof(int32_t));assert(original);
    memcpy(original,d.project.samples[0].pcm.data,d.project.samples[0].pcm.frames*sizeof(int32_t));
    assert(pt_flow_init(&flow,&d.project,PT_FLOW_CLASSIC128,0,100)==PT_FLOW_TICK);
    f=fopen(argv[2],"r");assert(f && fgets(line,sizeof(line),f) && strstr(line,"bytes=164"));
    while(fgets(line,sizeof(line),f) && line[0]=='T') {
        unsigned char r[164];unsigned long cursor;
        assert(strlen(line)==331);
        for(i=0;i<164;++i){char hex[3]={line[2+i*2],line[3+i*2],0};r[i]=(unsigned char)strtoul(hex,NULL,16);}
        assert(pt_flow_tick(&flow)==PT_FLOW_TICK);++ticks;
        if(!flow.active)continue;
        assert(pt_invert_sequence_tick(&state,&flow,15,bank,31)==PT_PCM_OK);
        if(!state.instrument[0])continue;
        /* Trace addresses are relative to metadata, but PCM is a separate
           Chip allocation. Bind once; retain every subsequent cursor delta. */
        if(!bound){sample_start=(unsigned long)word(r+52)*65536+word(r+54);bound=1;}
        assert(sample_start==(unsigned long)word(r+52)*65536+word(r+54));
        cursor=(unsigned long)word(r+140)*65536+word(r+142);
        if(cursor!=((sample_start+state.channel[0].cursor)&0xffffffffUL) || state.channel[0].speed!=(r[144]>>4) || state.channel[0].accumulator!=r[145]) {
            fprintf(stderr,"INVERT clock mismatch tick=%u fresh=%u counter=%u cursor=%lu/%lu speed=%u/%u accumulator=%u/%u\n",ticks,flow.fresh,flow.counter,(unsigned long)(((sample_start+state.channel[0].cursor)&0xffffffffUL)),cursor,state.channel[0].speed,r[144]>>4,state.channel[0].accumulator,r[145]);return 20;
        }
        for(i=0;i<16;++i)if((unsigned char)copies[0][d.project.samples[0].loop_start+i]!=r[148+i]) {
            fprintf(stderr,"INVERT shared PCM mismatch tick=%u index=%u got=%u expected=%u\n",ticks,i,(unsigned char)copies[0][d.project.samples[0].loop_start+i],r[148+i]);return 20;
        }
        assert(!memcmp(original,d.project.samples[0].pcm.data,d.project.samples[0].pcm.frames*sizeof(int32_t)));++checked;
    }
    assert(!flow.active && checked && !strcmp(line,"FLOW PASS dma=0\n"));fclose(f);
    for(i=0;i<31;++i)free(copies[i]);free(original);pt_document_release(&d);
    printf("INVERT ordering native parity PASS: ticks=%u checked=%u immutable master\n",ticks,checked);return 0;
}
