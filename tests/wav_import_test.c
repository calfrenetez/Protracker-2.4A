#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/raw_import.h"
#include "../src/core/wav.h"
static size_t live,calls,fail;
static void *allocate(void *ctx,size_t n) {(void)ctx;void *p;if(++calls==fail)return NULL;p=malloc(n);if(p)++live;return p;}
static void release(void *ctx,void *p) {(void)ctx;if(p) {assert(live);--live;free(p);}}
static int refuse(void *ctx,int32_t *data,uint32_t frames,const struct pt_raw_format *f)
{(void)ctx;(void)frames;(void)f;data[0]=0;return 0;}
static int32_t values[4102];static uint8_t bytes[12360];
int main(int argc,char **argv)
{
    unsigned bits,ch,i,cases=0;struct pt_allocator a={NULL,allocate,release};
    assert(argc==2);
    for(bits=8;bits<=24;bits+=8)for(ch=1;ch<=2;++ch){
        struct pt_raw_format f={44100,(uint8_t)bits,(uint8_t)ch,1,bits==8};
        struct pt_pcm pcm={values,4102,2051,44100,(uint8_t)ch,(uint8_t)bits};size_t n,owned;
        struct pt_document d;struct pt_sampler s;struct pt_pattern_history h;
        struct pt_pattern_command commands[4];struct pt_event_change changes[4];FILE *file;struct pt_sample old;
        for(i=0;i<2051*ch;++i)values[i]=(int32_t)((i*7919U)%((uint32_t)1<<bits))-((int32_t)1<<(bits-1));
        assert(pt_wav_encode(&pcm,bytes,sizeof(bytes),&n)==PT_WAV_OK);
        file=fopen(argv[1],"wb");assert(file && fwrite(bytes,1,n,file)==n && !fclose(file));
        pt_document_init(&d,&a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        pt_sampler_init(&s,&a,100000);assert(pt_pattern_history_init(&h,&d.project,commands,4,changes,4)==PT_EDIT_OK);
        assert(pt_wav_file_candidate(argv[1]));
        old=d.project.samples[0];owned=live;
        for(i=1;i<=3;++i) {fail=calls+i;assert(pt_wav_file_import(argv[1],n,&s,&d.project,&h,0,"wav")==PT_EDIT_CAPACITY);assert(live==owned && !h.count && !memcmp(&old,&d.project.samples[0],sizeof(old)));}
        fail=0;
        assert(pt_wav_file_import(argv[1],n-1,&s,&d.project,&h,0,"wav")==PT_EDIT_CAPACITY && !h.count);
        assert(pt_sampler_import_raw_fill(&s,&d.project,&h,0,(size_t)2051*ch*(bits/8),"raw",&f,refuse,NULL)==PT_EDIT_INVALID && live==owned && !h.count);
        assert(pt_wav_file_import(argv[1],n,&s,&d.project,&h,0,"wav")==PT_EDIT_OK);
        assert(!memcmp(d.project.samples[0].pcm.data,values,2051*ch*sizeof(int32_t)));
        assert(pt_pattern_undo(&d.project,&h,-1)==PT_EDIT_OK && !d.project.samples[0].pcm.frames);
        assert(pt_pattern_undo(&d.project,&h,1)==PT_EDIT_OK && !memcmp(d.project.samples[0].pcm.data,values,2051*ch*sizeof(int32_t)));
        {uint32_t revision=h.revision;int32_t *master=d.project.samples[0].pcm.data;
            bytes[32]^=1;file=fopen(argv[1],"wb");assert(file && fwrite(bytes,1,n,file)==n && !fclose(file));
            assert(pt_wav_file_import(argv[1],n,&s,&d.project,&h,0,"bad")==PT_EDIT_UNSUPPORTED);
            assert(h.revision==revision && d.project.samples[0].pcm.data==master && !memcmp(master,values,2051*ch*sizeof(int32_t)));
        }
        pt_pattern_history_release(&h);pt_sampler_release(&s);assert(!s.bytes);pt_document_release(&d);assert(!live);++cases;
    }
    assert(!unlink(argv[1]));printf("WAV IMPORT STREAM PASS: %u explicit formats, multi-block exact masters, allocation/fill/limit refusal, undo/redo and zero leaks\n",cases);return 0;
}
