#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "render.h"
static unsigned live,refuse;
static void *allocate(void *ctx,size_t bytes)
{
    void *p;(void)ctx;assert(!live && bytes && bytes<65536);
    if(refuse)return NULL;
    p=malloc(bytes);assert(p);live=1;memset(p,0xa5,bytes);return p;
}
static void release(void *ctx,void *p) {(void)ctx;assert(live==1 && p);live=0;free(p);}
static const struct pt_allocator allocator={NULL,allocate,release};
static enum pt_render_result allocated_measure(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_progress progress,void *ctx,struct pt_render_report *out)
{
    enum pt_render_result result;unsigned char before[sizeof(*out)];
    memcpy(before,out,sizeof(before));refuse=1;
    assert(pt_render_measure_allocated(p,o,progress,ctx,out,&allocator)==PT_RENDER_MEMORY);
    assert(!live && !memcmp(before,out,sizeof(before)));refuse=0;
    result=pt_render_measure_allocated(p,o,progress,ctx,out,&allocator);assert(!live);return result;
}
static enum pt_render_result allocated_stream(const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *ctx,struct pt_render_report *out)
{
    enum pt_render_result result;unsigned char before[sizeof(*out)];
    memcpy(before,out,sizeof(before));refuse=1;
    assert(pt_render_stream_allocated(p,o,sink,sink_ctx,progress,ctx,out,&allocator)==PT_RENDER_MEMORY);
    assert(!live && !memcmp(before,out,sizeof(before)));refuse=0;
    result=pt_render_stream_allocated(p,o,sink,sink_ctx,progress,ctx,out,&allocator);assert(!live);return result;
}
#define pt_render_measure allocated_measure
#define pt_render_stream allocated_stream
#include "render_test.c"
