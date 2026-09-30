#include "amigus_capture.h"
#include <string.h>
static int owned(struct pt_amigus_capture *o)
{
    struct pt_amigus_reservation *r=o->reservation;
    return r && r->opened && r->reserved && r->card && r->access && r->resource==PT_AMIGUS_PCM;
}
static int start_input(void *context,unsigned bits,unsigned channels,uint32_t rate)
{
    struct pt_amigus_capture *o=context;
    return owned(o)?o->input.start(o->input.context,bits,channels,rate):-1;
}
static int read_input(void *context,int32_t *data,unsigned limit,unsigned *frames)
{
    struct pt_amigus_capture *o=context;
    return owned(o)?o->input.read(o->input.context,data,limit,frames):-1;
}
static int stop_input(void *context)
{
    struct pt_amigus_capture *o=context;int rc;
    if(!owned(o))return -1;
    rc=o->input.stop(o->input.context);
    return rc==1 && o->reservation->interrupt?0:rc;
}
int pt_amigus_capture_open(struct pt_amigus_capture *o,struct pt_amigus_reservation *r,const struct pt_capture_input *in,const struct pt_allocator *a,unsigned bits,unsigned channels,uint32_t rate,uint32_t frames,size_t budget)
{
    struct pt_capture_input guarded;
    if(!o || o->reservation || o->session.phase!=PT_CS_IDLE || !r || !r->opened || !r->card ||
       r->resource!=PT_AMIGUS_PCM || !in || !in->start || !in->read || !in->stop ||
       !pt_capture_input_accepts(in,bits,channels,rate) ||
       !pt_amigus_reservation_begin(r))return 0;
    guarded=*in;guarded.context=o;guarded.start=start_input;guarded.read=read_input;guarded.stop=stop_input;
    o->reservation=r;o->input=*in;
    if(pt_capture_session_open(&o->session,a,&guarded,bits,channels,rate,frames,budget)!=PT_CAPTURE_OK) {
        /* No input callback/interrupt installation has happened yet. */
        pt_amigus_reservation_end(r);o->reservation=NULL;memset(&o->input,0,sizeof(o->input));return 0;
    }
    return 1;
}
enum pt_capture_poll pt_amigus_capture_step(struct pt_amigus_capture *o)
{
    enum pt_capture_poll rc;
    if(!o)return PT_CS_ERROR;
    rc=pt_capture_session_step(&o->session);
    if(o->session.phase==PT_CS_DONE && o->reservation) {
        if(!pt_amigus_reservation_end(o->reservation))return PT_CS_ERROR;
        o->reservation=NULL;
    }
    return rc;
}
void pt_amigus_capture_finish(struct pt_amigus_capture *o)
{if(o)pt_capture_session_finish(&o->session);}
void pt_amigus_capture_abort(struct pt_amigus_capture *o)
{if(o)pt_capture_session_abort(&o->session);}
int pt_amigus_capture_take(struct pt_amigus_capture *o,struct pt_capture *c)
{return o && !o->reservation && pt_capture_session_take(&o->session,c);}
int pt_amigus_capture_close(struct pt_amigus_capture *o)
{
    if(!o || o->reservation || !pt_capture_session_close(&o->session))return 0;
    memset(o,0,sizeof(*o));return 1;
}
