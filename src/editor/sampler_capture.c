#include "sampler_capture.h"
#include <string.h>
static enum pt_edit_result fill(void *context,struct pt_pcm *out)
{
    const struct pt_pcm *p=context;
    memcpy(out->data,p->data,(size_t)p->frames*p->channels*sizeof(int32_t));return PT_EDIT_OK;
}
enum pt_edit_result pt_sampler_capture_append(struct pt_sampler *s,struct pt_project *p,struct pt_pattern_history *h,struct pt_capture *c,const char *name)
{
    const struct pt_pcm *pcm=pt_capture_pcm(c);enum pt_edit_result result;
    if(!pcm)return PT_EDIT_INVALID;
    result=pt_sampler_append_generated(s,p,h,pcm,name,fill,(void *)pcm);
    if(result==PT_EDIT_OK)pt_capture_close(c);
    return result;
}
