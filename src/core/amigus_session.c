#include "amigus_session.h"
#include <string.h>
static int submit(void *v,const struct pt_pcm *p) {struct pt_amigus_session *s=v;s->reset_confirmed=0;return pt_amigus_fifo_submit(&s->fifo,p);}
static int poll(void *v) {return pt_amigus_fifo_poll(&((struct pt_amigus_session *)v)->fifo);}
static int cancel(void *v) {struct pt_amigus_session *s=v;return s->reset_confirmed?1:-1;}
int pt_amigus_session_open(struct pt_amigus_session *s,struct pt_studio_queue *q,const struct pt_amigus_fifo_port *p,int (*drain)(void *),void *context)
{
    struct pt_studio_transport t;int ready;
    if(!s || s->phase!=PT_AS_IDLE || !q || !p || !p->capacity || !p->write3 || !p->reset || !drain)return 0;
    s->drain=drain;s->drain_context=context;ready=pt_amigus_fifo_init(&s->fifo,p);
    t=(struct pt_studio_transport){s,submit,poll,cancel};
    if(!pt_studio_consumer_attach(&s->consumer,q,&t)) {s->phase=PT_AS_RESET;s->failed=1;return 0;}
    s->phase=ready?PT_AS_RUN:PT_AS_RESET;s->failed=!ready;return ready;
}
void pt_amigus_session_stop(struct pt_amigus_session *s)
{
    if(!s || s->phase==PT_AS_IDLE || s->phase==PT_AS_DONE)return;
    pt_studio_queue_abort(s->consumer.queue);s->phase=PT_AS_RESET;
}
static enum pt_consumer_result fail(struct pt_amigus_session *s)
{s->failed=1;pt_amigus_session_stop(s);return PT_CONSUMER_ERROR;}
enum pt_consumer_result pt_amigus_session_step(struct pt_amigus_session *s)
{
    int r;enum pt_consumer_result cr;
    if(!s || s->phase==PT_AS_IDLE)return PT_CONSUMER_ERROR;
    if(s->phase==PT_AS_DONE)return s->failed?PT_CONSUMER_ERROR:PT_CONSUMER_FINISHED;
    if(s->phase==PT_AS_RESET) {
        r=pt_amigus_fifo_cancel(&s->fifo);
        if(r<0)return fail(s);
        if(!r)return s->failed?PT_CONSUMER_ERROR:PT_CONSUMER_WAIT;
        s->reset_confirmed=1;
        if(!pt_studio_consumer_detach(&s->consumer))return fail(s);
        s->phase=PT_AS_DONE;return s->failed?PT_CONSUMER_ERROR:PT_CONSUMER_FINISHED;
    }
    if(s->phase==PT_AS_RUN) {
        cr=pt_studio_consumer_step(&s->consumer);
        if(cr==PT_CONSUMER_ERROR)return fail(s);
        if(cr!=PT_CONSUMER_FINISHED)return cr;
        r=pt_amigus_fifo_finish(&s->fifo,&s->padding);
        if(r!=1)return fail(s);
        s->phase=PT_AS_TAIL;return PT_CONSUMER_PROGRESS;
    }
    if(s->phase==PT_AS_TAIL) {
        r=pt_amigus_fifo_poll(&s->fifo);if(r<0)return fail(s);
        if(r)s->phase=PT_AS_DRAIN;
        return r?PT_CONSUMER_PROGRESS:PT_CONSUMER_WAIT;
    }
    r=s->drain(s->drain_context);if(r<0)return fail(s);
    if(r)s->phase=PT_AS_RESET;
    return r?PT_CONSUMER_PROGRESS:PT_CONSUMER_WAIT;
}
int pt_amigus_session_detach(struct pt_amigus_session *s)
{
    if(!s || (s->phase!=PT_AS_IDLE && s->phase!=PT_AS_DONE))return 0;
    memset(s,0,sizeof(*s));return 1;
}
