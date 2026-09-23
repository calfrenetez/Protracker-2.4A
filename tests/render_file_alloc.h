#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "../src/platform/stem_file.h"
static unsigned allocated_live,allocation_refused;
static void *file_allocate(void *ctx,size_t bytes)
{
    void *p;(void)ctx;assert(allocated_live<3 && bytes && bytes<65536);
    if(allocation_refused)return NULL;
    p=malloc(bytes);assert(p);++allocated_live;memset(p,0xa5,bytes);return p;
}
static void file_release(void *ctx,void *p) {(void)ctx;assert(allocated_live && p);--allocated_live;free(p);}
static const struct pt_allocator file_allocator={NULL,file_allocate,file_release};
#ifdef PT_TEST_STEMS
static enum pt_render_file_result allocated_file(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,unsigned grouped,pt_render_progress progress,void *ctx,
    struct pt_stem_report *out,enum pt_render_result *detail)
{
    enum pt_render_file_result result;
    result=pt_stem_file_new_allocated(path,p,o,grouped,progress,ctx,out,detail,&file_allocator);
    assert(!allocated_live);return result;
}
#define pt_stem_file_new allocated_file
#else
static enum pt_render_file_result allocated_file(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,pt_render_progress progress,void *ctx,
    struct pt_render_report *out,enum pt_render_result *detail)
{
    enum pt_render_file_result result;unsigned char before[sizeof(*out)];
    memcpy(before,out,sizeof(before));allocation_refused=1;
    assert(pt_render_file_new_allocated(path,p,o,progress,ctx,out,detail,&file_allocator)==PT_RENDER_FILE_RENDER);
    assert(*detail==PT_RENDER_MEMORY && !allocated_live && !memcmp(before,out,sizeof(before)));
    allocation_refused=0;
    result=pt_render_file_new_allocated(path,p,o,progress,ctx,out,detail,&file_allocator);
    assert(!allocated_live);return result;
}
#define pt_render_file_new allocated_file
#endif
