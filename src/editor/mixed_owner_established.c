#include "mixed_owner_established.h"
#include "mixed_owner_established_internal.h"
#include "mixed_owner_state_internal.h"
#include "sampler_paula_internal.h"
#include "sampler_wavetable_internal.h"
#include "../core/render_storage_internal.h"
#include <stddef.h>
#include <string.h>
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int source_current(struct pt_mixed_owner *s)
{
    struct pt_mixed_established *c=s->checked_context;
    struct pt_sampler_prepare_project *m=&c->storage;
    struct pt_project saved=m->snapshot;
    saved.channels.selected=m->project->channels.selected;
    return !c->faulted&&!s->checked_faulted&&m->memory.active&&!m->memory.faulted&&
        m->project==s->project&&m->memory.sampler==s->sampler&&
        s->project->channels.selected<s->project->channels.count&&
        !memcmp(&saved,s->project,sizeof(saved))&&
        !memcmp(&m->memory.snapshot,s->sampler,sizeof(*s->sampler))&&
        pt_sampler_paula_prepared_current(s->pb)&&pt_sampler_wavetable_prepared_metadata_current(s->ab);
}
static int output_apart(struct pt_mixed_owner *s,const void *out,size_t n,int handle)
{
    struct pt_mixed_established *c=s->checked_context;
    struct pt_sampler_prepare_project *m=&c->storage;unsigned i;
    /* Exact genuine publisher is protected as a parent, but close may consume it.
     * Cleanup must not traverse stale source arrays; saved parent/child extents
     * still protect any alternate handle. Borrowed source storage stays alive. */
    if(handle && out==c->publisher && n==sizeof(*c->publisher))return 1;
    if(!apart(out,n,c,sizeof(*c)) ||
       !pt_render_project_storage_output_disjoint(&m->snapshot,out,n)||
       !pt_render_project_storage_output_disjoint(m->project,out,n)||
       !pt_sampler_output_disjoint(&m->memory.snapshot,out,n)||
       !pt_sampler_output_disjoint(m->memory.sampler,out,n))return 0;
    for(i=0;i<m->memory.parent_count;++i)
        if(!apart(out,n,m->memory.parents[i].data,m->memory.parents[i].bytes))return 0;
    for(i=0;i<PT_SAMPLER_PREPARE_BLOCKS;++i)
        if(!apart(out,n,m->memory.block[i].data,m->memory.block[i].bytes))return 0;
    return 1;
}
static void cancel(struct pt_mixed_owner *s)
{struct pt_mixed_established *c=s->checked_context;(void)pt_mixed_preflight_setup_cancel(&c->startup);}
static enum pt_mixed_owner_result result_failure(struct pt_mixed_owner *s,enum pt_mixed_owner_result r)
{s->checked_busy=0;return pt_mixed_owner_preparation_fail(s,r);}
static enum pt_mixed_owner_result prepare(struct pt_mixed_owner *s,struct pt_mixed_report *out)
{
    struct pt_mixed_established *c=s->checked_context;
    enum pt_mixed_owner_result r;enum pt_render_setup_result setup;enum pt_mixed_result gate;
    struct pt_pcm pcm;struct pt_sample_version *pin;
    if(s->checked_busy){s->checked_faulted=1;return PT_MIXED_OWNER_STALE;}
    if(out&&!output_apart(s,out,sizeof(*out),0))return PT_MIXED_OWNER_INVALID;
    r=pt_mixed_owner_current(s);if(r!=PT_MIXED_OWNER_OK)return r;
    if(s->schedule_phase||s->clock_armed||s->pending||s->done||s->batch.phase)return PT_MIXED_OWNER_INVALID;
    s->checked_busy=1;
    if(!s->analyzed) {
        if(!c->started) {
            setup=pt_mixed_preflight_setup_begin(s->project,&s->options,s->map,&s->caps,&s->format,
                s->pa.control!=NULL,s->aa.control!=NULL,&s->allocator,c->revision,s->generation,&c->startup);
            if(setup!=PT_RENDER_SETUP_PENDING)return result_failure(s,setup==PT_RENDER_SETUP_CAPACITY?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_STALE);
            c->started=1;
        }else if(c->startup) {
            setup=pt_mixed_preflight_setup_get(c->startup,c->revision,s->generation,NULL);
            if(setup==PT_RENDER_SETUP_PENDING)setup=pt_mixed_preflight_setup_step(c->startup,c->revision,s->generation,c->work);
            else if(setup==PT_RENDER_SETUP_READY)setup=pt_mixed_preflight_setup_transfer(&c->startup,c->revision,s->generation,&s->analysis);
            if(setup!=PT_RENDER_SETUP_PENDING&&setup!=PT_RENDER_SETUP_READY)
                return result_failure(s,setup==PT_RENDER_SETUP_CAPACITY?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_CAPABILITY);
        }else {
            gate=pt_mixed_preflight_step(s->analysis,&s->report);
            if(gate!=PT_MIXED_PENDING&&gate!=PT_MIXED_OK)
                return result_failure(s,gate==PT_MIXED_MEMORY?PT_MIXED_OWNER_MEMORY:PT_MIXED_OWNER_CAPABILITY);
            if(gate==PT_MIXED_OK) {
                if(!pt_mixed_preflight_take(s->analysis,&s->sequence))return result_failure(s,PT_MIXED_OWNER_RENDER);
                pt_mixed_preflight_close(&s->analysis);s->analyzed=1;
            }
        }
        r=PT_MIXED_OWNER_PREPARING;
    }else if(s->ready)r=PT_MIXED_OWNER_OK;
    else {
        while(s->slot<s->project->sample_count&&!s->report.samples[0][s->slot]&&!s->report.samples[1][s->slot])++s->slot;
        if(s->slot==s->project->sample_count){s->ready=1;r=PT_MIXED_OWNER_OK;}
        else {
            if(pt_sampler_pin_current(s->sampler,s->project,s->slot,s->generation,
                s->sampler->current[s->slot],&pcm,&pin)!=PT_EDIT_OK)return result_failure(s,PT_MIXED_OWNER_STALE);
            s->pin[s->slot++]=pin;r=PT_MIXED_OWNER_PREPARING;
        }
    }
    s->checked_busy=0;
    if(pt_mixed_owner_current(s)!=PT_MIXED_OWNER_OK)return s->failure;
    if(out)*out=s->report;
    return r;
}
/* Small exact admission snapshots; backend cache payload is neither copied nor
 * scanned here. Arbitrary destructive callbacks cannot be classified safely. */
static enum pt_mixed_owner_result begin(struct pt_mixed_established *c,
    struct pt_paula_voices *p,struct pt_wavetable_voices *w,
    const struct pt_render_options *o,const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,
    const struct pt_allocator *a,const void *container,size_t container_bytes,
    const struct pt_sampler_storage_span *extra,unsigned count,
    uint32_t revision,unsigned work,struct pt_mixed_owner **out)
{
    struct pt_sampler_storage_span parents[15];struct pt_paula_voices ps;struct pt_wavetable_voices ws;
    struct pt_sampler_paula pb;struct pt_sampler_wavetable ab;
    struct pt_render_options options;struct pt_paula_render_caps limits;struct pt_playback_format format;
    struct pt_amigus_reservation reservation;struct pt_mixed_owner *s;unsigned i,total=12;
    if(!c||!p||!w||!p->bridge||!w->bridge||!w->bridge->backend||!w->bridge->backend->reservation||
       !o||!caps||!f||!a||!a->allocate||!a->release||!out||*out||
       c->storage.memory.active||c->startup||c->starting||count>(container?2U:3U)||!span(extra,count*sizeof(*extra))||
       ((!container)!=(container_bytes==0))||
       !work||work>4096||p->song_owner||w->song_owner||p->closing||w->closing||
       p->bridge->sampler!=w->bridge->sampler||p->bridge->project!=w->bridge->project||
       !pt_sampler_paula_prepared_current(p->bridge)||!pt_sampler_wavetable_prepared_metadata_current(w->bridge)||
       !pt_paula_render_caps_valid(caps)||(f->bits!=8&&f->bits!=16)||f->channel||f->word_pad||f->little_endian>1||
       w->bridge->backend->closing||w->bridge->backend->faulted||w->bridge->backend->reservation->interrupt)
        return PT_MIXED_OWNER_INVALID;
    for(i=0;i<PT_PAULA_VOICES;++i)if(p->voice[i].held)return PT_MIXED_OWNER_INVALID;
    for(i=0;i<PT_WAVETABLE_VOICES;++i)if(w->voice[i].held)return PT_MIXED_OWNER_INVALID;
    parents[0]=(struct pt_sampler_storage_span){p,sizeof(*p)};
    parents[1]=(struct pt_sampler_storage_span){w,sizeof(*w)};
    parents[2]=(struct pt_sampler_storage_span){p->bridge,sizeof(*p->bridge)};
    parents[3]=(struct pt_sampler_storage_span){w->bridge,sizeof(*w->bridge)};
    parents[4]=(struct pt_sampler_storage_span){w->bridge->backend,sizeof(*w->bridge->backend)};
    parents[5]=(struct pt_sampler_storage_span){w->bridge->backend->reservation,sizeof(reservation)};
    parents[6]=(struct pt_sampler_storage_span){o,sizeof(*o)};
    parents[7]=(struct pt_sampler_storage_span){caps,sizeof(*caps)};
    parents[8]=(struct pt_sampler_storage_span){f,sizeof(*f)};
    parents[9]=(struct pt_sampler_storage_span){a,sizeof(*a)};
    parents[10]=(struct pt_sampler_storage_span){out,sizeof(*out)};
    parents[11]=(struct pt_sampler_storage_span){c,sizeof(*c)};
    for(i=0;i<11;++i)if(!apart(c,sizeof(*c),parents[i].data,parents[i].bytes))return PT_MIXED_OWNER_INVALID;
    for(i=0;i<10;++i)if(!apart(out,sizeof(*out),parents[i].data,parents[i].bytes))return PT_MIXED_OWNER_INVALID;
    if(container) {
        if(!span(container,container_bytes)||container_bytes<sizeof(*out)||
           (uintptr_t)out<(uintptr_t)container||(uintptr_t)out-(uintptr_t)container>container_bytes-sizeof(*out)||
           !apart(c,sizeof(*c),container,container_bytes)||
           !pt_render_project_storage_output_disjoint(p->bridge->project,container,container_bytes)||
           !pt_sampler_output_disjoint(p->bridge->sampler,container,container_bytes))return PT_MIXED_OWNER_INVALID;
        for(i=0;i<10;++i)if(!apart(container,container_bytes,parents[i].data,parents[i].bytes))return PT_MIXED_OWNER_INVALID;
        parents[total++]=(struct pt_sampler_storage_span){container,container_bytes};
    }
    for(i=0;i<count;++i){parents[total+i]=extra[i];
        if(!apart(out,sizeof(*out),extra[i].data,extra[i].bytes)||
           !apart(c,sizeof(*c),extra[i].data,extra[i].bytes))return PT_MIXED_OWNER_INVALID;}
    if(!pt_render_project_storage_output_disjoint(p->bridge->project,out,sizeof(*out))||
       !pt_sampler_output_disjoint(p->bridge->sampler,out,sizeof(*out))||
       !apart(c,sizeof(*c),extra,count*sizeof(*extra))||
       !pt_sampler_prepare_project_begin(&c->storage,p->bridge->sampler,p->bridge->project,a,parents,total+count))return PT_MIXED_OWNER_INVALID;
    memcpy(&ps,p,sizeof(ps));memcpy(&ws,w,sizeof(ws));
    memcpy(&pb,p->bridge,sizeof(pb));memcpy(&ab,w->bridge,sizeof(ab));
    memcpy(&reservation,ab.backend->reservation,sizeof(reservation));
    options=*o;limits=*caps;format=*f;
    c->starting=1;c->publisher=out;c->revision=revision;c->work=work;c->started=c->faulted=0;
    s=c->storage.memory.allocator.allocate(c->storage.memory.allocator.context,sizeof(*s));
    if(!s){c->starting=0;pt_sampler_prepare_project_finish(&c->storage);return PT_MIXED_OWNER_MEMORY;}
    if(*out||memcmp(&ps,p,sizeof(ps))||memcmp(&ws,w,sizeof(ws))||
       memcmp(&pb,p->bridge,sizeof(pb))||memcmp(&ab,w->bridge,sizeof(ab))||
       memcmp(&reservation,ab.backend->reservation,sizeof(reservation))||
       memcmp(&options,o,sizeof(options))||memcmp(&limits,caps,sizeof(limits))||memcmp(&format,f,sizeof(format))) {
        c->storage.memory.allocator.release(c->storage.memory.allocator.context,s);
        c->starting=0;pt_sampler_prepare_project_finish(&c->storage);return PT_MIXED_OWNER_STALE;
    }
    memset(s,0,sizeof(*s));s->allocator=c->storage.memory.allocator;s->paula=p;s->amigus=w;s->pb=p->bridge;s->ab=w->bridge;
    s->sampler=pb.sampler;s->project=pb.project;memcpy(&s->snapshot,s->project,sizeof(s->snapshot));s->backend=ab.backend;s->reservation=ab.backend->reservation;
    s->pa=p->api;s->aa=w->api;s->pq=p->quiesce;s->aq=w->quiesce;s->pc=p->quiesce_context;s->ac=w->quiesce_context;
    s->options=options;s->caps=limits;s->format=format;s->generation=s->sampler->generation;s->pv=pb.version;s->av=ab.version;
    memcpy(s->map,p->map,sizeof(s->map));s->checked_context=c;s->checked_prepare=prepare;
    s->checked_current=source_current;s->checked_output=output_apart;s->checked_cancel=cancel;
    s->report.result=PT_MIXED_PENDING;
    c->starting=0;p->song_owner=s;w->song_owner=s;*out=s;return PT_MIXED_OWNER_PREPARING;
}
enum pt_mixed_owner_result pt_mixed_owner_established_begin(struct pt_mixed_established *c,
    struct pt_paula_voices *p,struct pt_wavetable_voices *w,const struct pt_render_options *o,
    const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,const struct pt_allocator *a,
    const struct pt_sampler_storage_span *extra,unsigned count,uint32_t revision,unsigned work,struct pt_mixed_owner **out)
{return begin(c,p,w,o,caps,f,a,NULL,0,extra,count,revision,work,out);}
enum pt_mixed_owner_result pt_mixed_owner_established_begin_bound(struct pt_mixed_established *c,
    struct pt_paula_voices *p,struct pt_wavetable_voices *w,const struct pt_render_options *o,
    const struct pt_paula_render_caps *caps,const struct pt_playback_format *f,const struct pt_allocator *a,
    const void *container,size_t bytes,const struct pt_sampler_storage_span *extra,unsigned count,
    uint32_t revision,unsigned work,struct pt_mixed_owner **out)
{return container&&bytes?begin(c,p,w,o,caps,f,a,container,bytes,extra,count,revision,work,out):PT_MIXED_OWNER_INVALID;}
int pt_mixed_owner_established_finish(struct pt_mixed_established *c)
{
    if(!c)return 0;
    if(c->starting){c->faulted=1;c->storage.memory.faulted=1;return 0;}
    if(c->startup)return 0;
    return pt_sampler_prepare_project_finish(&c->storage);
}
