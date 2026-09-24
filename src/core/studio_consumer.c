#include "studio_consumer.h"
#include <string.h>
int pt_studio_consumer_attach(struct pt_studio_consumer *c,struct pt_studio_queue *q,const struct pt_studio_transport *t)
{
    if(!c || c->queue || !q || !t || !t->submit || !t->poll || !t->cancel)return 0;
    memset(c,0,sizeof(*c));c->queue=q;c->transport=*t;return 1;
}
static enum pt_consumer_result confirmed(struct pt_studio_consumer *c,int result)
{
    if(result==1) {
        if(pt_studio_queue_release(c->queue,c->ticket)!=PT_QUEUE_OK) {c->failed=1;return PT_CONSUMER_ERROR;}
        c->leased=0;c->submitted=0;c->pcm=NULL;
    } else if(result!=0) {c->failed=1;c->stopped=1;pt_studio_queue_abort(c->queue);}
    if(c->failed)return PT_CONSUMER_ERROR;
    if(c->leased)return PT_CONSUMER_WAIT;
    return c->stopped?PT_CONSUMER_FINISHED:PT_CONSUMER_PROGRESS;
}
enum pt_consumer_result pt_studio_consumer_step(struct pt_studio_consumer *c)
{
    enum pt_queue_result qr;int accepted;
    if(!c || !c->queue)return PT_CONSUMER_ERROR;
    if(c->submitted)return confirmed(c,c->transport.poll(c->transport.context));
    if(c->failed)return PT_CONSUMER_ERROR;
    if(c->stopped)return PT_CONSUMER_FINISHED;
    if(!c->leased) {
        qr=pt_studio_queue_acquire(c->queue,&c->pcm,&c->ticket);
        if(qr==PT_QUEUE_EMPTY)return PT_CONSUMER_WAIT;
        if(qr==PT_QUEUE_DONE) {c->stopped=1;return PT_CONSUMER_FINISHED;}
        if(qr!=PT_QUEUE_OK) {c->failed=1;return PT_CONSUMER_ERROR;}
        c->leased=1;
    }
    accepted=c->transport.submit(c->transport.context,c->pcm);
    if(accepted==1) {c->submitted=1;return PT_CONSUMER_PROGRESS;}
    if(accepted==0)return PT_CONSUMER_WAIT;
    c->failed=1;c->stopped=1;pt_studio_queue_abort(c->queue);
    return confirmed(c,1);
}
enum pt_consumer_result pt_studio_consumer_stop(struct pt_studio_consumer *c)
{
    if(!c || !c->queue)return PT_CONSUMER_ERROR;
    c->stopped=1;pt_studio_queue_abort(c->queue);
    if(c->leased)return confirmed(c,c->submitted?c->transport.cancel(c->transport.context):1);
    return c->failed?PT_CONSUMER_ERROR:PT_CONSUMER_FINISHED;
}
int pt_studio_consumer_detach(struct pt_studio_consumer *c)
{
    if(!c)return 0;
    if(!c->queue)return 1;
    pt_studio_consumer_stop(c);if(c->leased)return 0;
    memset(c,0,sizeof(*c));return 1;
}
