#include <stdlib.h>
#include "../src/platform/recent_file.h"
#include <assert.h>
static unsigned live,calls,refuse;
static void *allocate(void *ctx,size_t n)
{void *p;(void)ctx;++calls;if(refuse)return NULL;assert(!live);p=malloc(n);assert(p);live=1;return p;}
static void release(void *ctx,void *p)
{(void)ctx;assert(live && p);free(p);live=0;}
static const struct pt_allocator allocator={NULL,allocate,release};
static int load_bounded(const char *path,struct pt_recent *r)
{return pt_recent_file_load_allocated(path,r,&allocator);}
static int save_bounded(const char *path,const struct pt_recent *r)
{return pt_recent_file_save_allocated(path,r,&allocator);}
#define pt_recent_file_load load_bounded
#define pt_recent_file_save save_bounded
#define main fixture_main
#include "recent_file_test.c"
#undef main
int main(int argc,char **argv)
{
    struct pt_recent expected;unsigned was;int result=fixture_main(argc,argv);
    assert(!live && calls);assert(load_bounded(argv[1],&expected));
    before=expected;refuse=1;was=calls;
    assert(!load_bounded(argv[1],&expected) && !memcmp(&expected,&before,sizeof(expected)));
    assert(!save_bounded(argv[1],&b));assert(calls==was+2 && !live);
    refuse=0;assert(load_bounded(argv[1],&expected) && !memcmp(&expected,&before,sizeof(expected)));
    assert(!pt_recent_file_load_allocated(argv[1],&expected,NULL));
#ifdef PT_IO_FAULTS
    {extern int pt_test_write_failure;pt_test_write_failure=3;
     assert(!save_bounded(argv[1],&b));pt_test_write_failure=0;
     assert(load_bounded(argv[1],&expected) && !memcmp(&expected,&before,sizeof(expected)));}
#endif
    assert(!live);puts("RECENT MEMORY PASS");return result;
}
