#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/raw_import.h"
#include "../src/core/svx.h"
static size_t live,calls,fail;
static void *allocate(void *ctx,size_t n) {(void)ctx;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p) {(void)ctx;if(p) {assert(live);--live;free(p);}}
static int refuse(void *ctx,int32_t *data,uint32_t frames,const struct pt_raw_format *f)
{(void)ctx;(void)frames;(void)f;data[0]=0;return 0;}
static int32_t values[4102];static uint8_t bytes[12360];
int main(int argc,char **argv)
{
    unsigned compressed,loop,named,i,cases=0;struct pt_allocator a={NULL,allocate,release};
    assert(argc==2);
    for(compressed=0;compressed<2;++compressed)for(loop=0;loop<2;++loop)for(named=0;named<2;++named){
        unsigned frames=compressed?4098:4097;
        struct pt_raw_format f={22050,8,1,0,0};
        struct pt_svx_info info={0};struct pt_pcm pcm={values,4102,frames,22050,1,8};size_t n,owned;
        struct pt_document d;struct pt_sampler s;struct pt_pattern_history h;
        struct pt_pattern_command commands[4];struct pt_event_change changes[4];FILE *file;struct pt_sample old;
        for(i=0;i<frames;++i)values[i]=(int32_t)(i%256)-128;
        info.volume=named?32767:65536;info.loop_start=loop?2:0;info.loop_end=loop?2048:0;
        if(named)strcpy(info.name,"From IFF");
        assert(pt_svx_encode(&pcm,&info,bytes,sizeof(bytes),&n)==PT_SVX_OK);
        if(compressed) {
            size_t body=2+frames/2;n=88+body+(body&1);bytes[35]=1;
            bytes[4]=(uint8_t)((n-8)>>24);bytes[5]=(uint8_t)((n-8)>>16);bytes[6]=(uint8_t)((n-8)>>8);bytes[7]=(uint8_t)(n-8);
            bytes[84]=0;bytes[85]=0;bytes[86]=(uint8_t)(body>>8);bytes[87]=(uint8_t)body;
            bytes[88]=0;bytes[89]=127;for(i=2;i<body;++i)bytes[88+i]=(uint8_t)(i*37);bytes[n-1]=0;
            assert(pt_svx_decode(bytes,n,&pcm)==PT_SVX_OK);
        }
        file=fopen(argv[1],"wb");assert(file && fwrite(bytes,1,n,file)==n && !fclose(file));
        pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        pt_sampler_init(&s,&a,100000);assert(pt_pattern_history_init(&h,&d.project,commands,4,changes,4)==PT_EDIT_OK);
        assert(pt_svx_file_candidate(argv[1]));
        old=d.project.samples[0];owned=live;
        for(i=1;i<=3;++i) {fail=calls+i;assert(pt_svx_file_import(argv[1],n,&s,&d.project,&h,0,"fallback")==PT_EDIT_CAPACITY);assert(live==owned && !h.count && !memcmp(&old,&d.project.samples[0],sizeof(old)));}
        fail=0;
        assert(pt_svx_file_import(argv[1],n-1,&s,&d.project,&h,0,"fallback")==PT_EDIT_CAPACITY && !h.count);
        assert(pt_sampler_import_raw_fill(&s,&d.project,&h,0,frames,"raw",&f,refuse,NULL)==PT_EDIT_INVALID && live==owned && !h.count);
        assert(pt_svx_file_import(argv[1],n,&s,&d.project,&h,0,"fallback")==PT_EDIT_OK);
        assert(!memcmp(d.project.samples[0].pcm.data,values,frames*sizeof(int32_t)));
        assert(!strcmp(d.project.samples[0].name,named?"From IFF":"fallback"));
        assert(d.project.samples[0].volume==(named?32:64) && d.project.samples[0].loop_start==info.loop_start && d.project.samples[0].loop_end==info.loop_end);
        assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !d.project.samples[0].pcm.frames);
        assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && !memcmp(d.project.samples[0].pcm.data,values,frames*sizeof(int32_t)));
        {uint32_t revision=h.revision;int32_t *master=d.project.samples[0].pcm.data;
            bytes[34]=2;file=fopen(argv[1],"wb");assert(file && fwrite(bytes,1,n,file)==n && !fclose(file));
            assert(pt_svx_file_import(argv[1],n,&s,&d.project,&h,0,"bad")==PT_EDIT_UNSUPPORTED);
            assert(h.revision==revision && d.project.samples[0].pcm.data==master && !memcmp(master,values,frames*sizeof(int32_t)));
        }
        pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);pt_document_release(&d);assert(!live);++cases;
    }
    assert(!unlink(argv[1]));printf("SVX IMPORT STREAM PASS: %u explicit formats, multi-block exact masters, allocation/fill/limit refusal, undo/redo and zero leaks\n",cases);return 0;
}
