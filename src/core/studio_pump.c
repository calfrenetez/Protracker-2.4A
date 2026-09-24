#include "studio_pump.h"
#include <string.h>
int pt_studio_pump_init(struct pt_studio_pump *p,const struct pt_studio_producer *s,struct pt_studio_queue *q)
{
    if(!p || !s || !s->pull || !s->stop || !q)return 0;
    memset(p,0,sizeof(*p));p->producer=*s;p->queue=q;
    p->pending=(struct pt_pcm){p->data,512,0,48000,2,24};return 1;
}
void pt_studio_pump_stop(struct pt_studio_pump *p)
{
    if(!p || !p->queue)return;
    if(!p->ended)p->producer.stop(p->producer.context);
    p->ended=1;p->held=0;pt_studio_queue_abort(p->queue);
}
enum pt_pump_result pt_studio_pump_step(struct pt_studio_pump *p,unsigned frames)
{
    const struct pt_pcm *block=NULL;unsigned done=0;enum pt_queue_result result;
    if(!p || !p->queue || !frames || frames>256)return PT_PUMP_ERROR;
    if(p->error)return PT_PUMP_ERROR;
    if(p->ended)return PT_PUMP_FINISHED;
    if(!p->held) {
        p->error=p->producer.pull(p->producer.context,frames,&block,&done);
        if(p->error)goto fail;
        if(done) {
            if(block) {p->error=PT_RENDER_INVALID;goto fail;}
            p->producer.stop(p->producer.context);p->ended=1;pt_studio_queue_finish(p->queue);return PT_PUMP_FINISHED;
        }
        if(!block)return PT_PUMP_PROGRESS;
        if(!block->frames || block->frames>frames || block->channels!=2 || block->bits!=24 || block->rate!=48000 || pt_pcm_validate(block)!=PT_PCM_OK) {
            p->error=PT_RENDER_SAMPLE;goto fail;
        }
        memcpy(p->data,block->data,block->frames*2*sizeof(*p->data));p->pending.frames=block->frames;p->held=1;
    }
    result=pt_studio_queue_push(p->queue,&p->pending);
    if(result==PT_QUEUE_FULL)return PT_PUMP_BLOCKED;
    if(result!=PT_QUEUE_OK) {p->error=PT_RENDER_SINK;goto fail;}
    p->held=0;return PT_PUMP_PROGRESS;
fail:
    pt_studio_pump_stop(p);return PT_PUMP_ERROR;
}
