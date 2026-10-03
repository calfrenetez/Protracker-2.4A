#include "editor_studio.h"
static int output_overlaps(const void *out,size_t bytes,const void *owner,size_t size)
{
    uintptr_t x=(uintptr_t)out,y=(uintptr_t)owner;
    if(!bytes || !size)return 0;
    if(bytes>UINTPTR_MAX-x || size>UINTPTR_MAX-y)return 1;
    return x<y+size && y<x+bytes;
}
static int fixed_output_disjoint(const struct pt_editor_studio *o,const void *out,size_t bytes)
{
    return out && bytes && bytes<=UINTPTR_MAX-(uintptr_t)out && (!o ||
        (!output_overlaps(out,bytes,o,sizeof(*o)) && (!o->editor ||
        (!output_overlaps(out,bytes,o->editor,sizeof(*o->editor)) &&
         !output_overlaps(out,bytes,o->editor->project,sizeof(*o->editor->project))))));
}
static int source_owner_matches(const struct pt_editor_studio *o)
{
    return (!o->song || pt_sampler_song_matches_owner(o->song,&o->editor->sampler,o->editor->project)) &&
        (!o->invert_song || pt_sampler_invert_song_matches_owner(o->invert_song,&o->editor->sampler,o->editor->project));
}
static int source_output_disjoint(struct pt_editor_studio *o,const void *out,size_t bytes)
{
    return (!o->song || pt_sampler_song_output_disjoint(o->song,out,bytes)) &&
        (!o->invert_song || pt_sampler_invert_song_output_disjoint(o->invert_song,out,bytes));
}
static void close_song(struct pt_editor_studio *o) {pt_sampler_song_close(o->song);o->song=NULL;pt_sampler_invert_song_close(o->invert_song);o->invert_song=NULL;}
void pt_editor_studio_stop(struct pt_editor_studio *owner)
{if(owner) {void (*stop)(void *)=owner->output_stop;void *context=owner->output_context;owner->output_stop=NULL;owner->output_context=NULL;if(owner->queue) {pt_studio_queue_abort(owner->queue);owner->queue=NULL;owner->pump.held=0;owner->pump.ended=1;}close_song(owner);if(stop)stop(context);}}
static void stop_guard(void *context) {pt_editor_studio_stop(context);}
static int attached(const struct pt_editor_studio *o)
{return o && o->editor && o->editor->before_change==stop_guard && o->editor->before_change_context==o;}
int pt_editor_studio_attach(struct pt_editor_studio *o,struct pt_editor *e)
{
    if(!o || !e || o->editor || o->song || o->invert_song || o->queue || e->before_change || e->change_ready)return 0;
    o->editor=e;pt_editor_change_guard(e,stop_guard,o);return 1;
}
void pt_editor_studio_detach(struct pt_editor_studio *o)
{
    if(!o)return;
    pt_editor_studio_stop(o);
    if(attached(o))pt_editor_change_guard(o->editor,NULL,NULL);
    o->editor=NULL;
}
enum pt_render_result pt_editor_studio_start(struct pt_editor_studio *o,const struct pt_render_options *options)
{
    if(!attached(o))return PT_RENDER_INVALID;
    pt_editor_studio_stop(o);
    return pt_sampler_song_open(&o->editor->sampler,o->editor->project,options,&o->editor->sampler.allocator,&o->song);
}
enum pt_render_result pt_editor_studio_begin(struct pt_editor_studio *o,const struct pt_render_options *options)
{
    if(!attached(o))return PT_RENDER_INVALID;
    pt_editor_studio_stop(o);
    return pt_sampler_song_begin(&o->editor->sampler,o->editor->project,options,&o->editor->sampler.allocator,&o->song);
}
enum pt_render_result pt_editor_studio_prepare(struct pt_editor_studio *o,unsigned *ready)
{
    enum pt_render_result result;
    if(!fixed_output_disjoint(o,ready,sizeof(*ready)))return PT_RENDER_INVALID;
    if(!attached(o) || (!o->song && !o->invert_song))return PT_RENDER_INVALID;
    if((o->song && o->invert_song) || !source_owner_matches(o)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    if(!source_output_disjoint(o,ready,sizeof(*ready)))return PT_RENDER_INVALID;
    result=o->invert_song?pt_sampler_invert_song_prepare(o->invert_song,ready):pt_sampler_song_prepare(o->song,ready);
    if(result!=PT_RENDER_OK)pt_editor_studio_stop(o);
    return result;
}
enum pt_render_result pt_editor_studio_start_invert(struct pt_editor_studio *o,const struct pt_render_options *options,size_t budget)
{
    if(!attached(o))return PT_RENDER_INVALID;
    pt_editor_studio_stop(o);
    return pt_sampler_invert_song_open(&o->editor->sampler,o->editor->project,options,budget,&o->editor->sampler.allocator,&o->invert_song);
}
enum pt_render_result pt_editor_studio_begin_invert(struct pt_editor_studio *o,const struct pt_render_options *options,size_t budget)
{
    if(!attached(o))return PT_RENDER_INVALID;
    pt_editor_studio_stop(o);
    return pt_sampler_invert_song_begin(&o->editor->sampler,o->editor->project,options,budget,&o->editor->sampler.allocator,&o->invert_song);
}
static enum pt_render_result pull_song(struct pt_editor_studio *o,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    if(o->invert_song)return pt_sampler_invert_song_pull(o->invert_song,frames,pcm,done);
    if(o->song)return pt_sampler_song_pull(o->song,frames,pcm,done);
    *pcm=NULL;*done=1;return PT_RENDER_OK;
}
enum pt_render_result pt_editor_studio_pull(struct pt_editor_studio *o,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    enum pt_render_result result;
    if(!pcm || !done || !frames || frames>256 ||
       !fixed_output_disjoint(o,pcm,sizeof(*pcm)) || !fixed_output_disjoint(o,done,sizeof(*done)) ||
       output_overlaps(pcm,sizeof(*pcm),done,sizeof(*done)))return PT_RENDER_INVALID;
    if(!attached(o)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    if((o->song && o->invert_song) || !source_owner_matches(o)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    if(!source_output_disjoint(o,pcm,sizeof(*pcm)) || !source_output_disjoint(o,done,sizeof(*done)))return PT_RENDER_INVALID;
    if(o->queue)return PT_RENDER_INVALID;
    result=pull_song(o,frames,pcm,done);
    if(result!=PT_RENDER_OK || *done)pt_editor_studio_stop(o);
    return result;
}

static enum pt_render_result producer_pull(void *context,unsigned frames,const struct pt_pcm **pcm,unsigned *done)
{
    struct pt_editor_studio *o=context;
    return pull_song(o,frames,pcm,done);
}
static void producer_stop(void *context) {close_song(context);}
static enum pt_render_result start_queued(struct pt_editor_studio *o,const struct pt_render_options *options,struct pt_studio_queue *queue,unsigned invert,size_t budget,unsigned preparing)
{
    struct pt_studio_producer source;enum pt_render_result result;
    if(!queue || (o && queue==o->queue))return PT_RENDER_INVALID;
    if(invert)result=preparing?pt_editor_studio_begin_invert(o,options,budget):pt_editor_studio_start_invert(o,options,budget);
    else result=preparing?pt_editor_studio_begin(o,options):pt_editor_studio_start(o,options);
    if(result!=PT_RENDER_OK)return result;
    source=(struct pt_studio_producer){o,producer_pull,producer_stop};
    if(!pt_studio_pump_init(&o->pump,&source,queue)) {pt_editor_studio_stop(o);return PT_RENDER_INVALID;}
    o->queue=queue;return PT_RENDER_OK;
}
enum pt_render_result pt_editor_studio_start_queued(struct pt_editor_studio *o,const struct pt_render_options *options,struct pt_studio_queue *queue)
{return start_queued(o,options,queue,0,0,0);}
enum pt_render_result pt_editor_studio_start_invert_queued(struct pt_editor_studio *o,const struct pt_render_options *options,size_t budget,struct pt_studio_queue *queue)
{return start_queued(o,options,queue,1,budget,0);}
enum pt_render_result pt_editor_studio_begin_queued(struct pt_editor_studio *o,const struct pt_render_options *options,struct pt_studio_queue *queue)
{return start_queued(o,options,queue,0,0,1);}
enum pt_render_result pt_editor_studio_begin_invert_queued(struct pt_editor_studio *o,const struct pt_render_options *options,size_t budget,struct pt_studio_queue *queue)
{return start_queued(o,options,queue,1,budget,1);}
enum pt_pump_result pt_editor_studio_step(struct pt_editor_studio *o,unsigned frames)
{
    enum pt_pump_result result;
    if(!attached(o)) {pt_editor_studio_stop(o);return PT_PUMP_ERROR;}
    if(!o->queue)return PT_PUMP_FINISHED;
    if((o->song && o->invert_song) || !source_owner_matches(o)) {pt_editor_studio_stop(o);return PT_PUMP_ERROR;}
    result=pt_studio_pump_step(&o->pump,frames);
    if(result==PT_PUMP_ERROR && o->pump.error)pt_editor_studio_stop(o);
    return result;
}

int pt_editor_studio_bind_output_stop(struct pt_editor_studio *o,void (*stop)(void *),void *context)
{
    if(!attached(o) || !o->queue || !stop || o->output_stop)return 0;
    o->output_stop=stop;o->output_context=context;return 1;
}


static void output_session_stop(void *context) {pt_amigus_session_stop(context);}
int pt_editor_studio_output_attach(struct pt_editor_studio_output *o,struct pt_editor *e)
{
    if(!o || pt_editor_studio_output_busy(o) || o->session.phase!=PT_AS_IDLE)return 0;
    return pt_editor_studio_attach(&o->producer,e);
}
void pt_editor_studio_output_stop(struct pt_editor_studio_output *o)
{
    if(!o)return;
    pt_editor_studio_stop(&o->producer);
    /* Also covers initial-reset failure before the stop hook was bound. */
    pt_amigus_session_stop(&o->session);
}
int pt_editor_studio_output_bind_prefill(struct pt_editor_studio_output *o,int (*start)(void *),void *context,unsigned triplets)
{
    if(!o || !attached(&o->producer) || pt_editor_studio_output_busy(o) || o->session.phase!=PT_AS_IDLE || (start && !triplets))return 0;
    o->start=start;o->start_context=start?context:NULL;o->prefill_triplets=start?triplets:0;return 1;
}
int pt_editor_studio_output_bind_start(struct pt_editor_studio_output *o,int (*start)(void *),void *context)
{return pt_editor_studio_output_bind_prefill(o,start,context,1);}
static int start_output(struct pt_editor_studio_output *o,const struct pt_render_options *options,unsigned blocks,const struct pt_amigus_fifo_port *port,int (*drain)(void *),void *context)
{
    if(!o || !attached(&o->producer) || o->queue || o->session.phase!=PT_AS_IDLE ||
       !port || !port->capacity || !port->write3 || !port->reset || !drain || blocks<1 || blocks>8)return 0;
    o->failed=0;o->quiesced=0;
    o->queue=pt_studio_queue_open(&o->producer.editor->sampler.allocator,blocks);
    if(!o->queue) {o->failed=1;return 0;}
    if(pt_editor_studio_begin_queued(&o->producer,options,o->queue)!=PT_RENDER_OK) {
        pt_studio_queue_close(o->queue);o->queue=NULL;o->failed=1;return 0;
    }
    if(!(o->start?pt_amigus_session_open_prefilled(&o->session,o->queue,port,drain,context,o->start,o->start_context,o->prefill_triplets):
         pt_amigus_session_open(&o->session,o->queue,port,drain,context)) ||
       !pt_editor_studio_bind_output_stop(&o->producer,output_session_stop,&o->session)) {
        o->failed=1;pt_editor_studio_output_stop(o);return 0;
    }
    return 1;
}
int pt_editor_studio_output_busy(const struct pt_editor_studio_output *o)
{return o && (o->queue || o->reservation);}
int pt_editor_studio_output_start(struct pt_editor_studio_output *o,const struct pt_render_options *options,unsigned blocks,const struct pt_amigus_fifo_port *port,int (*drain)(void *),void *context)
{
    if(!o || o->reservation)return 0;
    return start_output(o,options,blocks,port,drain,context);
}
int pt_editor_studio_output_start_reserved(struct pt_editor_studio_output *o,const struct pt_render_options *options,unsigned blocks,const struct pt_amigus_fifo_port *port,int (*drain)(void *),void *context,struct pt_amigus_reservation *r,int (*quiesce)(void *),void *qc)
{
    if(!o || !attached(&o->producer) || pt_editor_studio_output_busy(o) ||
       o->session.phase!=PT_AS_IDLE || !port || !port->capacity || !port->write3 || !port->reset ||
       !drain || !quiesce || blocks<1 || blocks>8 || !r || !r->opened || !r->card ||
       r->resource!=PT_AMIGUS_PCM || !pt_amigus_reservation_begin(r))return 0;
    o->reservation=r;o->quiesce=quiesce;o->quiesce_context=qc;
    /* Even failed preparation keeps access until the adapter confirms quiescence. */
    return start_output(o,options,blocks,port,drain,context);
}
static int quiesce_access(struct pt_editor_studio_output *o)
{
    int result;
    if(!o->reservation || o->quiesced)return 1;
    result=o->quiesce(o->quiesce_context);
    if(result<0)o->failed=1;
    if(result!=1 || o->reservation->interrupt)return 0;
    o->quiesced=1;return 1;
}
static enum pt_consumer_result finish_access(struct pt_editor_studio_output *o)
{
    if(!o->reservation)return o->failed?PT_CONSUMER_ERROR:PT_CONSUMER_FINISHED;
    if(!quiesce_access(o))return o->failed?PT_CONSUMER_ERROR:PT_CONSUMER_WAIT;
    if(!pt_amigus_reservation_end(o->reservation)) {o->failed=1;return PT_CONSUMER_ERROR;}
    o->reservation=NULL;o->quiesce=NULL;o->quiesce_context=NULL;
    return o->failed?PT_CONSUMER_ERROR:PT_CONSUMER_FINISHED;
}
enum pt_consumer_result pt_editor_studio_output_step(struct pt_editor_studio_output *o,unsigned frames)
{
    enum pt_consumer_result result;
    if(!o || !frames || frames>256)return PT_CONSUMER_ERROR;
    if(!o->queue)return finish_access(o);
    if(o->session.phase==PT_AS_DONE) {
        /* Reset releases the consumer lease, not arbitrary callback references
         * to queue/session storage. Keep both until adapter quiescence. */
        if(!quiesce_access(o))return o->failed?PT_CONSUMER_ERROR:PT_CONSUMER_WAIT;
        pt_editor_studio_stop(&o->producer);
        if(pt_studio_queue_close(o->queue)!=PT_QUEUE_OK) {o->failed=1;return PT_CONSUMER_ERROR;}
        o->queue=NULL;pt_amigus_session_detach(&o->session);
        return finish_access(o);
    }
    if(o->session.phase==PT_AS_RUN && o->producer.queue &&
       pt_editor_studio_step(&o->producer,frames)==PT_PUMP_ERROR) {
        o->failed=1;pt_editor_studio_output_stop(o);
    }
    result=pt_amigus_session_step(&o->session);
    if(result==PT_CONSUMER_ERROR) {o->failed=1;pt_editor_studio_output_stop(o);}
    if(o->session.phase==PT_AS_DONE) {
        pt_editor_studio_stop(&o->producer);
        return o->failed?PT_CONSUMER_ERROR:PT_CONSUMER_PROGRESS;
    }
    return o->failed?PT_CONSUMER_ERROR:result;
}
int pt_editor_studio_output_detach(struct pt_editor_studio_output *o)
{
    if(!o)return 0;
    pt_editor_studio_output_stop(o);
    if(pt_editor_studio_output_busy(o) || !pt_amigus_session_detach(&o->session))return 0;
    pt_editor_studio_detach(&o->producer);o->start=NULL;o->start_context=NULL;o->prefill_triplets=0;return 1;
}
