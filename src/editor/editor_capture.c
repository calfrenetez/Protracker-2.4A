#include "editor_capture.h"
#include "sampler_capture.h"
int pt_editor_capture_busy(const struct pt_editor_capture *o)
{return o && (o->device.session.phase!=PT_CS_IDLE || o->device.reservation || o->ready.pcm.data);}
void pt_editor_capture_finish(struct pt_editor_capture *o)
{if(o)pt_amigus_capture_finish(&o->device);}
static int barrier(void *context)
{
    struct pt_editor_capture *o=context;
    pt_editor_capture_finish(o);
    if(o->device.session.phase!=PT_CS_IDLE || o->device.reservation)return 0;
    return !o->ready.pcm.data || o->publishing;
}
static int attached(const struct pt_editor_capture *o)
{return o && o->editor && o->editor->change_ready==barrier && o->editor->before_change_context==o;}
int pt_editor_capture_attach(struct pt_editor_capture *o,struct pt_editor *e)
{
    if(!o || o->editor || pt_editor_capture_busy(o) || !pt_editor_change_barrier(e,barrier,o))return 0;
    o->editor=e;return 1;
}
int pt_editor_capture_start(struct pt_editor_capture *o,struct pt_amigus_reservation *r,const struct pt_capture_input *in,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget)
{
    if(!attached(o) || pt_editor_capture_busy(o) ||
       !pt_amigus_capture_open(&o->device,r,in,&o->editor->sampler.allocator,bits,channels,rate,frames,budget))return 0;
    o->fault=PT_CS_NO_FAULT;return 1;
}
enum pt_capture_poll pt_editor_capture_step(struct pt_editor_capture *o)
{
    enum pt_capture_poll rc;
    if(!o)return PT_CS_ERROR;
    if(o->device.session.phase==PT_CS_IDLE)return attached(o) && !o->fault?PT_CS_COMPLETE:PT_CS_ERROR;
    if(!attached(o))pt_amigus_capture_abort(&o->device);
    rc=pt_amigus_capture_step(&o->device);
    if(o->device.session.fault)o->fault=o->device.session.fault;
    if(o->device.session.phase==PT_CS_DONE && !o->device.reservation) {
        if(rc==PT_CS_COMPLETE)pt_amigus_capture_take(&o->device,&o->ready);
        if(!pt_amigus_capture_close(&o->device))return PT_CS_ERROR;
    }
    return rc;
}
const struct pt_pcm *pt_editor_capture_pcm(const struct pt_editor_capture *o)
{return o?pt_capture_pcm(&o->ready):NULL;}
enum pt_edit_result pt_editor_capture_publish(struct pt_editor_capture *o,const char *name)
{
    enum pt_edit_result result;
    if(!attached(o) || o->device.session.phase!=PT_CS_IDLE || o->device.reservation || !pt_capture_pcm(&o->ready))return PT_EDIT_INVALID;
    o->publishing=1;
    if(!pt_editor_prepare_change(o->editor)) {o->publishing=0;return PT_EDIT_INVALID;}
    o->publishing=0;
    result=pt_sampler_capture_append(&o->editor->sampler,o->editor->project,&o->editor->history,&o->ready,name);
    if(result==PT_EDIT_OK) {
        o->editor->sample=o->editor->project->sample_count;
        pt_editor_sample_all(o->editor);++o->editor->sample_ui;
    }
    pt_editor_sample_result(o->editor,result);return result;
}
int pt_editor_capture_discard(struct pt_editor_capture *o)
{
    if(!o)return 0;
    pt_amigus_capture_abort(&o->device);pt_capture_close(&o->ready);
    if(!pt_amigus_capture_close(&o->device))return 0;
    o->fault=PT_CS_NO_FAULT;return 1;
}
int pt_editor_capture_detach(struct pt_editor_capture *o)
{
    if(!pt_editor_capture_discard(o))return 0;
    if(attached(o))pt_editor_change_barrier(o->editor,NULL,NULL);
    o->editor=NULL;return 1;
}
