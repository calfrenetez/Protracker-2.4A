#include "paula_delivery4_task.h"
#include "sampler_internal.h"
#include "sampler_paula_internal.h"
#include "../core/elapsed_clock.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define ALIGNMENT(T) offsetof(struct {char byte;T value;},value)
static int object(const void *p,size_t n,size_t a)
{uintptr_t x=(uintptr_t)p;return p&&n&&a&&x%a==0&&n<=UINTPTR_MAX-x;}
static int apart(const void *a,size_t n,const void *b,size_t m)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return object(a,n,1)&&object(b,m,1)&&(x+n<=y||y+m<=x);}
static int pcm_equal(const struct pt_pcm *a,const struct pt_pcm *b)
{return a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&
    a->rate==b->rate&&a->channels==b->channels&&a->bits==b->bits;}
/* Mirror the existing project serialization/validation metadata span guard:
 * tables precede descriptor traversal; full capacities, markers and optional
 * extension payloads are protected without reading PCM/spare values. */
static int array_apart(const void *out,size_t n,const void *source,size_t count,size_t width)
{
    if(!width||count>SIZE_MAX/width)return 0;
    return !count||apart(out,n,source,count*width);
}
static int project_apart(const struct pt_project *p,const void *out,size_t n)
{
    unsigned i;size_t events=p->pattern_count;
    if(events>SIZE_MAX/64U)return 0;
    events*=64U;
    if(p->channels.count&&events>SIZE_MAX/p->channels.count)return 0;
    events*=p->channels.count;
    if(!array_apart(out,n,p,1,sizeof(*p))||
       !array_apart(out,n,p->orders,p->order_count,sizeof(*p->orders))||
       !array_apart(out,n,p->events,events,sizeof(*p->events))||
       !array_apart(out,n,p->samples,p->sample_count,sizeof(*p->samples))||
       !array_apart(out,n,p->extensions,p->extension_count,sizeof(*p->extensions)))return 0;
    for(i=0;i<p->sample_count;++i)
       if(!array_apart(out,n,p->samples[i].pcm.data,p->samples[i].pcm.capacity,
          sizeof(*p->samples[i].pcm.data))||
          !array_apart(out,n,p->samples[i].slices,p->samples[i].slice_count,
          sizeof(*p->samples[i].slices)))return 0;
    for(i=0;i<p->extension_count;++i)
       if(!array_apart(out,n,p->extensions[i].data,p->extensions[i].length,1))return 0;
    return 1;
}
/* Authenticate all three observations against their actual already-held
 * versions, including an unselected/superseded source. This reproduces the
 * genuine lowerer's guard construction; it creates no capability/getter/seal. */
static int source_guards_current(const struct pt_sampler_paula *b,
    const struct pt_paula_segment_source *s,const struct pt_sample_version *pin)
{
    struct pt_sampler_storage_span spans[PT_SAMPLER_VERSION_SPANS];unsigned i,n=0;
    if(!pt_sampler_version_spans(pin,spans,PT_SAMPLER_VERSION_SPANS,&n)||
       n>PT_PAULA_SEGMENT_SOURCE_GUARDS-3U||s->guard_count!=n+3U)return 0;
    for(i=0;i<n;++i)
       if(s->guard[i].data!=spans[i].data||s->guard[i].bytes!=spans[i].bytes)return 0;
    return s->guard[n].data==b&&s->guard[n].bytes==sizeof(*b)&&
       s->guard[n+1U].data==b->sampler&&s->guard[n+1U].bytes==sizeof(*b->sampler)&&
       s->guard[n+2U].data==b->project&&s->guard[n+2U].bytes==sizeof(*b->project);
}
/* Only called for genuine already-held expected pins and an already validated
 * immutable project. No version private layout or cache entry is followed here. */
static int source_current(struct pt_sampler_paula *b,unsigned track,
    const struct pt_paula_segment_source *s,struct pt_sample_version *expected)
{
    struct pt_pcm pcm;struct pt_sample_version *pin=NULL;const uint8_t *data=NULL;
    size_t bytes=0;int same;
    if(!expected||s->version!=expected||s->sampler_owner!=b->sampler||
       s->project_identity!=b->project||s->cache_owner!=b||
       s->cache_version!=b->version||s->generation!=b->sampler->generation||
       s->sample>=b->project->sample_count||
       s->descriptor!=&b->project->samples[s->sample].pcm)return 0;
    if(pt_sampler_pin_current(b->sampler,b->project,s->sample,s->generation,
       expected,&pcm,&pin)!=PT_EDIT_OK)return 0;
    same=pin==expected&&pcm_equal(&pcm,&s->master)&&pcm_equal(s->descriptor,&s->master)&&
       pt_sampler_paula_prepared_location_validated(b,track,s->lease,&data,&bytes)&&
       data==s->cache_data&&bytes==s->cache_bytes&&source_guards_current(b,s,pin);
    /* The caller's original pin remains held, so this temporary unpin cannot
     * end the immutable master lifetime or stand in for reader retirement. */
    pt_sampler_unpin(pin);return same;
}
static int source_apart(const struct pt_paula_segment_source *s,const void *out,size_t n)
{
    unsigned i;size_t bytes;
    if(!object(s->descriptor,sizeof(*s->descriptor),ALIGNMENT(struct pt_pcm))||
       !s->master.capacity||s->master.capacity>SIZE_MAX/sizeof(*s->master.data)||
       s->guard_count>PT_PAULA_SEGMENT_SOURCE_GUARDS)return 0;
    bytes=s->master.capacity*sizeof(*s->master.data);
    if(!apart(out,n,s->descriptor,sizeof(*s->descriptor))||
       !apart(out,n,s->master.data,bytes)||!apart(out,n,s->cache_data,s->cache_bytes))return 0;
    for(i=0;i<s->guard_count;++i)if(!apart(out,n,s->guard[i].data,s->guard[i].bytes))return 0;
    return 1;
}
enum pt_paula_delivery4_snapshot_result pt_paula_delivery4_task_snapshot(
    struct pt_sampler_paula *b,const struct pt_paula_future_packet *q,
    const struct pt_readers_domain *domain,
    const struct pt_paula_segment_source dependency[PT_PAULA_DELIVERY4_DEPENDENCIES],
    struct pt_sample_version *const expected[PT_PAULA_DELIVERY4_DEPENDENCIES],
    const struct pt_scheduled_grid *grid,
    const uint64_t frame[PT_PAULA_DELIVERY4_ACTIONS],unsigned publication,
    struct pt_paula_delivery4_action *out)
{
    struct pt_elapsed_clock clock;struct pt_paula_delivery4_action result;
    uint64_t first,last;unsigned i,j;
    const struct pt_paula_segment_source *selected[2]={NULL,NULL};
    if(publication>=PT_PAULA_DELIVERY4_ACTIONS||
       !object(b,sizeof(*b),ALIGNMENT(struct pt_sampler_paula))||
       !object(q,sizeof(*q),ALIGNMENT(struct pt_paula_future_packet))||
       !object(domain,sizeof(*domain),ALIGNMENT(struct pt_readers_domain))||
       !object(dependency,PT_PAULA_DELIVERY4_DEPENDENCIES*sizeof(*dependency),ALIGNMENT(struct pt_paula_segment_source))||
       !object(expected,PT_PAULA_DELIVERY4_DEPENDENCIES*sizeof(*expected),ALIGNMENT(struct pt_sample_version *))||
       !object(grid,sizeof(*grid),ALIGNMENT(struct pt_scheduled_grid))||
       !object(frame,PT_PAULA_DELIVERY4_ACTIONS*sizeof(*frame),ALIGNMENT(uint64_t))||
       !object(out,sizeof(*out),ALIGNMENT(struct pt_paula_delivery4_action)))
       return PT_PAULA_DELIVERY4_SNAPSHOT_INVALID;
    if(!apart(out,sizeof(*out),b,sizeof(*b))||!apart(out,sizeof(*out),q,sizeof(*q))||
       !apart(out,sizeof(*out),domain,sizeof(*domain))||
       !apart(out,sizeof(*out),dependency,PT_PAULA_DELIVERY4_DEPENDENCIES*sizeof(*dependency))||
       !apart(out,sizeof(*out),expected,PT_PAULA_DELIVERY4_DEPENDENCIES*sizeof(*expected))||
       !apart(out,sizeof(*out),grid,sizeof(*grid))||
       !apart(out,sizeof(*out),frame,PT_PAULA_DELIVERY4_ACTIONS*sizeof(*frame)))
       return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
    if(!object(b->sampler,sizeof(*b->sampler),ALIGNMENT(struct pt_sampler))||
       !object(b->project,sizeof(*b->project),ALIGNMENT(struct pt_project))||
       !apart(out,sizeof(*out),b->sampler,sizeof(*b->sampler))||
       !apart(out,sizeof(*out),b->project,sizeof(*b->project)))
       return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
    if(!domain->count||domain->count>PT_SCHEDULED_SPANS||
       !object(domain->spans,domain->count*sizeof(*domain->spans),ALIGNMENT(struct pt_scheduled_span))||
       !apart(out,sizeof(*out),domain->spans,domain->count*sizeof(*domain->spans))||
       !apart(out,sizeof(*out),domain->binding.context,domain->binding.context_bytes))
       return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
    for(i=0;i<domain->count;++i)
       if(!apart(out,sizeof(*out),domain->spans[i].data,domain->spans[i].bytes))
          return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
    if(!pt_sampler_paula_prepared_current(b))return PT_PAULA_DELIVERY4_SNAPSHOT_STALE;
    if(!project_apart(b->project,out,sizeof(*out))||
       !pt_sampler_output_disjoint(b->sampler,out,sizeof(*out)))
       return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
    if(!q->ticket||q->first>=q->last||q->geometry.frame!=frame[publication]||
       q->geometry.source_count<1||q->geometry.source_count>2||
       q->geometry.track>=b->project->channels.count||
       q->geometry.kind!=(publication==0?PT_PAULA_SEGMENT_INITIAL:
          publication==3?PT_PAULA_SEGMENT_STOP_REQUIREMENTS:PT_PAULA_SEGMENT_REPROGRAM)||
       !grid->generation||
       domain->key.generation!=grid->generation||domain->key.slot!=q->geometry.slot||
       !grid->rate||grid->frequency<grid->rate||
       pt_elapsed_clock_init(&clock,grid->frequency,grid->rate,grid->epoch,0)!=PT_ELAPSED_OK)
       return PT_PAULA_DELIVERY4_SNAPSHOT_INVALID;
    for(i=0;i<PT_PAULA_DELIVERY4_ACTIONS;++i)
       if(!frame[i]||frame[i]==UINT64_MAX||(i&&frame[i]<=frame[i-1]))
          return PT_PAULA_DELIVERY4_SNAPSHOT_INVALID;
    if(pt_elapsed_clock_deadline(&clock,q->geometry.frame,&first)!=PT_ELAPSED_OK||
       pt_elapsed_clock_deadline(&clock,q->geometry.frame+1,&last)!=PT_ELAPSED_OK||
       first!=q->first||last!=q->last)return PT_PAULA_DELIVERY4_SNAPSHOT_INVALID;
    for(i=0;i<PT_PAULA_DELIVERY4_DEPENDENCIES;++i){
       if(!source_apart(dependency+i,out,sizeof(*out)))
          return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
       if(!expected[i]||dependency[i].version!=expected[i])
          return PT_PAULA_DELIVERY4_SNAPSHOT_STALE;
       if(!pt_sampler_version_output_disjoint(expected[i],out,sizeof(*out)))
          return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
       for(j=0;j<i;++j)if(dependency[j].sample==dependency[i].sample)
          return PT_PAULA_DELIVERY4_SNAPSHOT_INVALID;
       if(!source_current(b,q->geometry.track,dependency+i,expected[i]))
          return PT_PAULA_DELIVERY4_SNAPSHOT_STALE;
    }
    for(i=0;i<q->geometry.source_count;++i){
       if(!source_apart(q->geometry.source+i,out,sizeof(*out)))
          return PT_PAULA_DELIVERY4_SNAPSHOT_ALIAS;
       for(j=0;j<PT_PAULA_DELIVERY4_DEPENDENCIES;++j)
          if(q->geometry.source[i].descriptor==dependency[j].descriptor)
             selected[i]=dependency+j;
       if(!selected[i])return PT_PAULA_DELIVERY4_SNAPSHOT_STALE;
    }
    if(q->geometry.source_count==1)selected[1]=selected[0];
    if(!pt_paula_segment_plan_identity_equal(&q->geometry,selected[0],selected[1]))
       return PT_PAULA_DELIVERY4_SNAPSHOT_STALE;
    /* No activation or cached currentness grant is created by this copy. */
    memset(&result,0,sizeof(result));memcpy(&result.packet,q,sizeof(*q));
    result.domain=domain;result.frequency=grid->frequency;result.publication=publication;
    memcpy(out,&result,sizeof(result));return PT_PAULA_DELIVERY4_SNAPSHOT_OK;
}
enum pt_paula_delivery4_window pt_paula_delivery4_window_observe(
    const struct pt_paula_delivery4_action *a,uint64_t ticket,
    uint64_t observed,uint32_t frequency)
{
    if(!object(a,sizeof(*a),ALIGNMENT(struct pt_paula_delivery4_action))||
       a->publication>=PT_PAULA_DELIVERY4_ACTIONS||!a->packet.ticket||
       a->packet.first>=a->packet.last||!frequency||frequency!=a->frequency)
       return PT_PAULA_DELIVERY4_BAD_WINDOW;
    if(ticket!=a->packet.ticket)return PT_PAULA_DELIVERY4_OLD_TICKET;
    if(observed<a->packet.first)return PT_PAULA_DELIVERY4_EARLY;
    return observed<a->packet.last?PT_PAULA_DELIVERY4_IN_WINDOW:PT_PAULA_DELIVERY4_LATE;
}
