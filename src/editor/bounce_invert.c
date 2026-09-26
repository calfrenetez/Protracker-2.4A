#include "bounce_invert.h"
#include "render_invert.h"
static int discard(void *ctx,const struct pt_pcm *pcm,uint64_t offset)
{(void)ctx;(void)pcm;(void)offset;return 1;}
static enum pt_render_result run(void *ctx,const struct pt_project *p,const struct pt_render_options *o,
    pt_render_sink sink,void *sink_ctx,pt_render_progress progress,void *progress_ctx,
    struct pt_render_report *out,const struct pt_allocator *a)
{
    return pt_render_invert_stream(p,o,sink?sink:discard,sink_ctx,progress,progress_ctx,out,*(const size_t *)ctx,a);
}
enum pt_edit_result pt_sampler_bounce_invert(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_render_options *o,const char *name,
    pt_render_progress progress,void *ctx,struct pt_render_report *out,enum pt_render_result *detail,size_t budget)
{
    const struct pt_bounce_engine engine={&budget,run};
    return pt_sampler_bounce_engine(s,p,h,o,name,progress,ctx,out,detail,&engine);
}
