#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/platform/sample_file.h"
#include "document.h"
struct budget {size_t live,peak;int fail;};
static void *allocate(void *c,size_t n) {struct budget *b=c;if(b->fail)return NULL;assert(!b->live && n<16384);b->live=n;if(n>b->peak)b->peak=n;return malloc(n);}
static void release(void *c,void *p) {struct budget *b=c;assert(b->live);b->live=0;free(p);}
int main(int argc,char **argv)
{
    struct budget b={0};struct pt_allocator a={&b,allocate,release};struct pt_raw_format fmt={0};
    struct pt_pcm pcm={0};unsigned bits,channels,little,uns,cases=0;size_t i,n,w,peak=0;
    int32_t *data=malloc(20002*sizeof(*data));uint8_t *expected=malloc(70000),*actual=malloc(70000);
    assert(argc==2 && data && expected && actual);pcm.data=data;pcm.capacity=20002;pcm.rate=48000;pcm.frames=10001;
    for(bits=8;bits<=24;bits+=8)for(channels=1;channels<=2;++channels)
    for(little=0;little<2;++little)for(uns=0;uns<(bits==8?2U:1U);++uns) {
        FILE *f;pcm.bits=bits;pcm.channels=channels;
        fmt.rate=pcm.rate;fmt.bits=bits;fmt.channels=channels;fmt.little_endian=little;fmt.unsigned8=uns;
        for(i=0;i<(size_t)pcm.frames*channels;++i)data[i]=(i&1)?((1L<<(bits-1))-1):(-(1L<<(bits-1))+1);
        assert(pt_raw_encode(&pcm,&fmt,expected,70000,&w)==PT_RAW_OK);n=w;
        b.fail=1;assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_MEMORY);
        assert(access(argv[1],F_OK)!=0);b.fail=0;
        assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_OK && !b.live);
        if(peak)assert(peak==b.peak);peak=b.peak;
        assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_PUBLISH && !b.live);
        f=fopen(argv[1],"rb");assert(f && fread(actual,1,n,f)==n && fgetc(f)==EOF && !fclose(f));
        assert(!memcmp(expected,actual,n));
        for(i=0;i<(size_t)pcm.frames*channels;++i)assert(data[i]==((i&1)?((1L<<(bits-1))-1):(-(1L<<(bits-1))+1)));
        assert(!unlink(argv[1]));++cases;
    }
    fmt.bits=8;assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_INVALID);
    fmt.bits=pcm.bits;fmt.rate=22050;assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_INVALID);
    fmt.rate=pcm.rate;fmt.channels=1;assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_INVALID);
    fmt.channels=2;fmt.unsigned8=1;assert(pt_sample_raw_save(argv[1],&pcm,&fmt,&a)==PT_SAVE_INVALID);
    assert(!b.live && access(argv[1],F_OK)!=0);
    free(data);free(expected);free(actual);
    printf("SAMPLE RAW STREAM PASS: %u formats, exact bytes, master preserved, fixed workspace=%lu and no replacement\n",cases,(unsigned long)peak);return 0;
}
