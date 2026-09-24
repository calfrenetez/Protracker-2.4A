#define main project_baseline_main
#include "project_test.c"
#undef main
#include "../src/platform/project_file.h"
#include "document.h"
#include <unistd.h>
struct sink_state {size_t offset,fail;};
static int collect(void *context,const uint8_t *data,size_t n)
{struct sink_state *s=context;assert(n<=1024);if(s->offset>=s->fail)return 0;assert(s->offset+n<=sizeof(rewritten));memcpy(rewritten+s->offset,data,n);s->offset+=n;return 1;}
struct budget {size_t live,peak;int fail;};
static void *allocate_stream(void *context,size_t n)
{struct budget *b=context;if(b->fail)return NULL;assert(!b->live && n<16384);b->live=n;b->peak=n;return malloc(n);}
static void release_stream(void *context,void *p)
{struct budget *b=context;assert(b->live);b->live=0;free(p);}
struct bad_producer {unsigned mode,calls;};
static int bad_produce(void *context,pt_file_sink sink,void *sink_context)
{
    struct bad_producer *p=context;uint8_t bytes[9]={0};++p->calls;
    if(p->mode==0)return sink(sink_context,bytes,7); /* short successful producer */
    if(p->mode==1) {sink(sink_context,bytes,9);return 1;} /* ignored overflow */
    if(p->mode==2 && p->calls==2)bytes[0]=1; /* changed verification content */
    if(p->mode==3)return 0;
    return sink(sink_context,bytes,8);
}
static int project_stream_fixture(int argc,char **argv)
{
    struct pt_project p;struct sink_state sink={0,SIZE_MAX};struct budget b={0};
    struct pt_allocator a={&b,allocate_stream,release_stream};size_t n,w,i;FILE *f;
    assert(argc==2);fixture(&p);
    assert(pt_project_encode(&p,encoded,sizeof(encoded),&n)==PT_PROJECT_OK);
    assert(pt_project_stream(&p,collect,&sink,&w)==PT_PROJECT_OK && w==n && sink.offset==n);
    assert(!memcmp(encoded,rewritten,n));
    for(i=0;i<n;i+=1024) {sink.offset=0;sink.fail=i;w=123;assert(pt_project_stream(&p,collect,&sink,&w)==PT_PROJECT_INVALID && w==123);}
    sink.offset=0;sink.fail=SIZE_MAX;w=123;
    assert(pt_project_stream(&p,collect,&sink,(size_t *)p.samples[0].pcm.data)==PT_PROJECT_ALIAS && !sink.offset);
    {uint8_t speed=p.speed;p.speed=0;
        assert(pt_project_stream(&p,collect,&sink,&w)==PT_PROJECT_INVALID && !sink.offset && w==123);
        p.speed=speed;
    }
    b.fail=1;assert(pt_project_file_save(argv[1],&p,&a)==PT_SAVE_MEMORY);assert(access(argv[1],F_OK)!=0);
    b.fail=0;assert(pt_project_file_save(argv[1],&p,&a)==PT_SAVE_OK && !b.live);
    assert(pt_project_file_save(argv[1],&p,&a)==PT_SAVE_PUBLISH && !b.live);
    f=fopen(argv[1],"rb");assert(f && fread(rewritten,1,n,f)==n && fgetc(f)==EOF && !fclose(f));
    assert(!memcmp(encoded,rewritten,n));assert(pt_project_probe(rewritten,n,&(struct pt_project_requirements){0})==PT_PROJECT_OK);
    assert(pt_project_encode(&p,rewritten,sizeof(rewritten),&w)==PT_PROJECT_OK && !memcmp(encoded,rewritten,n));
    assert(!unlink(argv[1]));
    {struct bad_producer bad;unsigned mode;
        for(mode=0;mode<4;++mode) {
            bad.mode=mode;bad.calls=0;
            assert(pt_file_save_streamed(argv[1],8,bad_produce,&bad,&a)==(mode==2?PT_SAVE_VERIFY:PT_SAVE_WRITE));
            assert(!b.live && access(argv[1],F_OK)!=0);
        }
    }
    printf("PROJECT STREAM PASS: exact golden layout/CRC, mixed masters/loops/slices/extensions, sink refusal, bounded workspace=%lu\n",(unsigned long)b.peak);
    return 0;
}

#ifndef PT_PROJECT_STREAM_NATIVE
int main(int argc,char **argv) {return project_stream_fixture(argc,argv);}
#endif
