#include "capture_session.h"
#include <string.h>
static enum pt_capture_poll fail(struct pt_capture_session *s,enum pt_capture_fault fault)
{
    if(!s->fault)s->fault=fault;
    s->phase=PT_CS_STOP;
    return PT_CS_ERROR;
}
enum pt_capture_result pt_capture_session_open(struct pt_capture_session *s,const struct pt_allocator *a,const struct pt_capture_input *in,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget)
{
    enum pt_capture_result result;
    if(!s || s->phase!=PT_CS_IDLE || !in || !in->start || !in->read || !in->stop)return PT_CAPTURE_INVALID;
    result=pt_capture_open(&s->capture,a,bits,channels,rate,frames,budget);
    if(result!=PT_CAPTURE_OK)return result;
    s->input=*in;s->fault=PT_CS_NO_FAULT;s->cancelled=0;s->phase=PT_CS_START;
    return PT_CAPTURE_OK;
}
enum pt_capture_poll pt_capture_session_step(struct pt_capture_session *s)
{
    int rc;unsigned frames=0,limit;struct pt_pcm chunk;
    if(!s || s->phase==PT_CS_IDLE)return PT_CS_ERROR;
    if(s->phase==PT_CS_DONE)return s->fault?PT_CS_ERROR:PT_CS_COMPLETE;
    if(s->phase==PT_CS_START) {
        rc=s->input.start(s->input.context,s->capture.pcm.bits,s->capture.pcm.channels,s->capture.pcm.rate);
        if(rc==1)s->phase=PT_CS_RECORD;
        else if(rc!=0)return fail(s,PT_CS_INPUT_FAULT);
        return PT_CS_PENDING;
    }
    if(s->phase==PT_CS_RECORD) {
        limit=s->capture.limit-s->capture.pcm.frames;if(limit>256)limit=256;
        rc=s->input.read(s->input.context,s->scratch,limit,&frames);
        if(rc==0 && !frames)return PT_CS_PENDING;
        if(rc==-2)return fail(s,PT_CS_OVERRUN);
        if(rc!=1)return fail(s,PT_CS_INPUT_FAULT);
        if(!frames || frames>limit)return fail(s,PT_CS_DATA_FAULT);
        chunk=(struct pt_pcm){s->scratch,512,frames,s->capture.pcm.rate,s->capture.pcm.channels,s->capture.pcm.bits};
        if(pt_capture_append(&s->capture,&chunk)!=PT_CAPTURE_OK)return fail(s,PT_CS_DATA_FAULT);
        if(s->capture.pcm.frames==s->capture.limit)s->phase=PT_CS_STOP;
        return PT_CS_PENDING;
    }
    if(s->phase!=PT_CS_STOP)return PT_CS_ERROR;
    rc=s->input.stop(s->input.context);
    if(rc==0)return PT_CS_PENDING;
    if(rc!=1)return fail(s,PT_CS_STOP_FAULT);
    if(s->cancelled || s->fault)pt_capture_close(&s->capture);
    else if(pt_capture_finish(&s->capture)!=PT_CAPTURE_OK) {
        s->fault=PT_CS_DATA_FAULT;pt_capture_close(&s->capture);
    }
    s->phase=PT_CS_DONE;
    return s->fault?PT_CS_ERROR:PT_CS_COMPLETE;
}
void pt_capture_session_finish(struct pt_capture_session *s)
{if(s && (s->phase==PT_CS_START || s->phase==PT_CS_RECORD))s->phase=PT_CS_STOP;}
void pt_capture_session_abort(struct pt_capture_session *s)
{
    if(!s || s->phase==PT_CS_IDLE)return;
    s->cancelled=1;pt_capture_session_finish(s);
    if(s->phase==PT_CS_DONE)pt_capture_close(&s->capture);
}
int pt_capture_session_take(struct pt_capture_session *s,struct pt_capture *c)
{
    if(!s || s->phase!=PT_CS_DONE || s->fault || s->cancelled || !pt_capture_pcm(&s->capture) ||
       !c || c==&s->capture || c->pcm.data || c->active || c->finished || c->failed)return 0;
    *c=s->capture;memset(&s->capture,0,sizeof(s->capture));return 1;
}
int pt_capture_session_close(struct pt_capture_session *s)
{
    if(!s)return 0;
    if(s->phase!=PT_CS_IDLE && s->phase!=PT_CS_DONE)return 0;
    pt_capture_close(&s->capture);memset(s,0,sizeof(*s));return 1;
}
