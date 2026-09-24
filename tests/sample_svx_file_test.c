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
    struct budget b={0};struct pt_allocator a={&b,allocate,release};struct pt_svx_info info={0},decoded;
    struct pt_pcm pcm={0};unsigned mode;size_t i,n,w,peak=0;
    int32_t *data=malloc(10002*sizeof(*data));uint8_t *expected=malloc(11000),*actual=malloc(11000);
    assert(argc==2 && data && expected && actual);pcm.data=data;pcm.capacity=10002;pcm.rate=65535;pcm.bits=8;pcm.channels=1;
    for(i=0;i<10002;++i)data[i]=(int32_t)(i%256)-128;
    for(mode=0;mode<4;++mode) {
        FILE *f;pcm.frames=10001+(mode&1);info.volume=mode*16384;info.cycles=257;
        memset(info.name,'A'+mode,32);info.loop_start=mode>=2?3:0;info.loop_end=mode>=2?10000:0;
        assert(pt_svx_encode(&pcm,&info,expected,11000,&w)==PT_SVX_OK);n=w;
        b.fail=1;assert(pt_sample_svx_save(argv[1],&pcm,&info,&a)==PT_SAVE_MEMORY);
        assert(access(argv[1],F_OK)!=0);b.fail=0;
        assert(pt_sample_svx_save(argv[1],&pcm,&info,&a)==PT_SAVE_OK && !b.live);
        if(peak)assert(peak==b.peak);peak=b.peak;
        assert(pt_sample_svx_save(argv[1],&pcm,&info,&a)==PT_SAVE_PUBLISH && !b.live);
        f=fopen(argv[1],"rb");assert(f && fread(actual,1,n,f)==n && fgetc(f)==EOF && !fclose(f));
        assert(!memcmp(expected,actual,n));assert(pt_svx_inspect(actual,n,&decoded)==PT_SVX_OK);
        assert(decoded.loop_start==info.loop_start && decoded.loop_end==info.loop_end && decoded.volume==info.volume && decoded.cycles==info.cycles);
        for(i=0;i<10002;++i)assert(data[i]==(int32_t)(i%256)-128);
        assert(!unlink(argv[1]));
    }
    pcm.bits=24;assert(pt_sample_svx_save(argv[1],&pcm,&info,&a)==PT_SAVE_INVALID);
    pcm.bits=8;pcm.rate=65536;assert(pt_sample_svx_save(argv[1],&pcm,&info,&a)==PT_SAVE_INVALID);
    pcm.rate=48000;info.loop_end=pcm.frames+1;assert(pt_sample_svx_save(argv[1],&pcm,&info,&a)==PT_SAVE_INVALID);
    assert(!b.live && access(argv[1],F_OK)!=0);free(data);free(expected);free(actual);
    printf("SAMPLE SVX STREAM PASS: exact metadata, loop and odd padding, master preserved, fixed workspace=%lu\n",(unsigned long)peak);return 0;
}
