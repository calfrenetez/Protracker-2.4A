#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/sample_file.h"
#include "document.h"
#include "wav.h"
struct budget {size_t live,peak;int fail;};
static void *allocate(void *c,size_t n) {struct budget *b=c;if(b->fail)return NULL;assert(!b->live && n<16384);b->live=n;if(n>b->peak)b->peak=n;return malloc(n);}
static void release(void *c,void *p) {struct budget *b=c;assert(b->live);b->live=0;free(p);}
struct failing {unsigned calls,fail;int change;};
static int generated(void *c,size_t pos,void *out,size_t n)
{struct failing *f=c;(void)pos;memset(out,f->change && f->calls>=2?43:42,n);return ++f->calls!=f->fail;}
int main(int argc,char **argv)
{
    struct budget b={0};struct pt_allocator a={&b,allocate,release};
    struct pt_pcm pcm={0};unsigned bits,channels;size_t i,n,w,peak=0;
    int32_t *data=malloc(20002*sizeof(*data));uint8_t *expected=malloc(70000),*actual=malloc(70000);
    assert(argc==2 && data && expected && actual);pcm.data=data;pcm.capacity=20002;pcm.rate=48000;pcm.frames=10001;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels) {
        FILE *f;pcm.bits=bits;pcm.channels=channels;
        for(i=0;i<(size_t)pcm.frames*channels;++i)data[i]=(i&1)?((1L<<(bits-1))-1):(-(1L<<(bits-1))+1);
        assert(pt_wav_size(&pcm,&n)==PT_WAV_OK && n<70000);
        assert(pt_wav_encode(&pcm,expected,70000,&w)==PT_WAV_OK && w==n);
        b.fail=1;assert(pt_sample_wav_save(argv[1],&pcm,&a)==PT_SAVE_MEMORY);assert(access(argv[1],F_OK)!=0);
        b.fail=0;assert(pt_sample_wav_save(argv[1],&pcm,&a)==PT_SAVE_OK && !b.live);
        if(peak)assert(peak==b.peak);peak=b.peak;
        assert(pt_sample_wav_save(argv[1],&pcm,&a)==PT_SAVE_PUBLISH && !b.live);
        f=fopen(argv[1],"rb");assert(f && fread(actual,1,n,f)==n && fgetc(f)==EOF && !fclose(f));
        assert(!memcmp(expected,actual,n));
        for(i=0;i<(size_t)pcm.frames*channels;++i)assert(data[i]==((i&1)?((1L<<(bits-1))-1):(-(1L<<(bits-1))+1)));
        assert(!unlink(argv[1]));
    }
    {struct failing f={0,2,0};assert(pt_file_save_generated(argv[1],5000,generated,&f,&a)==PT_SAVE_WRITE);
        assert(!b.live && access(argv[1],F_OK)!=0);
        f.calls=0;f.fail=3;assert(pt_file_save_generated(argv[1],5000,generated,&f,&a)==PT_SAVE_VERIFY);
        assert(!b.live && access(argv[1],F_OK)!=0);
        f.calls=0;f.fail=0;f.change=1;assert(pt_file_save_generated(argv[1],5000,generated,&f,&a)==PT_SAVE_VERIFY);
        assert(!b.live && access(argv[1],F_OK)!=0);}
    free(data);free(expected);free(actual);
    printf("SAMPLE WAV STREAM PASS: exact8/16/24 mono/stereo, master preserved, fixed workspace=%lu, failure cleanup and no replacement\n",(unsigned long)peak);return 0;
}
