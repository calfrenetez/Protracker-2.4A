#include <string.h>
#include "bounce.h"
struct bounce {
    const struct pt_project *project;
    const struct pt_render_options *options;
    struct pt_pcm *pcm;
    struct pt_render_report plan,report;
    enum pt_render_result detail;
    pt_render_progress progress;
    void *context;
    uint64_t frames;
};
static int receive(void *context,const struct pt_pcm *block,uint64_t offset)
{
    struct bounce *b=context;struct pt_pcm *p=b->pcm;
    if(offset!=b->frames || offset>p->frames || block->frames>p->frames-offset ||
       block->channels!=p->channels || block->bits!=p->bits || block->rate!=p->rate)return 0;
    memcpy(p->data+(size_t)offset*p->channels,block->data,(size_t)block->frames*p->channels*sizeof(int32_t));
    b->frames+=block->frames;return 1;
}
static enum pt_edit_result fill(void *context,struct pt_pcm *pcm)
{
    struct bounce *b=context;b->pcm=pcm;
    b->detail=pt_render_stream(b->project,b->options,receive,b,b->progress,b->context,&b->report);
    if(b->detail!=PT_RENDER_OK)return b->detail==PT_RENDER_CANCELLED?PT_EDIT_CANCELLED:PT_EDIT_UNSUPPORTED;
    if(b->frames!=b->plan.frames || b->report.frames!=b->plan.frames || b->report.ticks!=b->plan.ticks || b->report.end!=b->plan.end) {
        b->detail=PT_RENDER_INVALID;return PT_EDIT_INVALID;
    }
    if(b->progress && !b->progress(b->context,PT_RENDER_MIX,b->report.ticks,b->frames)) {
        b->detail=PT_RENDER_CANCELLED;return PT_EDIT_CANCELLED;
    }
    return PT_EDIT_OK;
}
enum pt_edit_result pt_sampler_bounce(struct pt_sampler *s,struct pt_project *p,
    struct pt_pattern_history *h,const struct pt_render_options *o,const char *name,
    pt_render_progress progress,void *context,struct pt_render_report *out,enum pt_render_result *detail)
{
    struct bounce b;struct pt_pcm format;enum pt_edit_result result;
    if(detail)*detail=PT_RENDER_INVALID;
    if(!s || !h || !o || !out || !detail || !name)return PT_EDIT_INVALID;
    memset(&b,0,sizeof(b));b.project=p;b.options=o;b.progress=progress;b.context=context;
    b.detail=pt_render_measure(p,o,progress,context,&b.plan);*detail=b.detail;
    if(b.detail!=PT_RENDER_OK)return b.detail==PT_RENDER_CANCELLED?PT_EDIT_CANCELLED:PT_EDIT_UNSUPPORTED;
    if(!b.plan.frames || b.plan.frames>UINT32_MAX)return PT_EDIT_CAPACITY;
    memset(&format,0,sizeof(format));format.frames=(uint32_t)b.plan.frames;format.channels=2;format.bits=o->bits;format.rate=o->rate;
    result=pt_sampler_append_generated(s,p,h,&format,name,fill,&b);*detail=b.detail;
    if(result==PT_EDIT_OK)*out=b.report;
    return result;
}
