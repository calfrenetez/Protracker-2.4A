#include "render_invert_file.h"
#include "render_invert.h"
static int discard(void *ctx,const struct pt_pcm *pcm,uint64_t offset)
{(void)ctx;(void)pcm;(void)offset;return 1;}
static enum pt_render_result run(void *ctx,const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
    struct pt_render_report *out,const struct pt_allocator *a)
{
    return pt_render_invert_stream(p,o,sink?sink:discard,sink_ctx,progress,progress_ctx,out,*(const size_t *)ctx,a);
}
enum pt_render_file_result pt_render_invert_file_new(const char *path,const struct pt_project *p,
    const struct pt_render_options *o,pt_render_progress progress,void *ctx,struct pt_render_report *out,
    enum pt_render_result *detail,size_t budget,const struct pt_allocator *a)
{
    struct pt_render_file_engine engine={&budget,run};
    return pt_render_file_engine_new(path,p,o,progress,ctx,out,detail,a,&engine);
}
