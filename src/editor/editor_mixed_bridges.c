#include "editor_mixed_bridges.h"
#include "editor_mixed_internal.h"
#include "../core/render_storage_internal.h"
#include <string.h>
/* The established owner protects all four constituent controls. Refuse to
 * compile an ABI with additional inter-control/tail storage outside that union. */
typedef char pt_editor_bridge_guard_coverage[(sizeof(struct pt_editor_mixed_bridges)==
    sizeof(struct pt_sampler_paula)+sizeof(struct pt_sampler_wavetable)+
    sizeof(struct pt_paula_voices)+sizeof(struct pt_wavetable_voices))?1:-1];
static int span(const void *p,size_t n)
{return !n||(p&&n<=UINTPTR_MAX-(uintptr_t)p);}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return span(a,an)&&span(b,bn)&&(!an||!bn||x>=y+bn||y>=x+an);}
static int zero(const void *p,size_t n)
{const unsigned char *b=p;size_t i;for(i=0;i<n;++i)if(b[i])return 0;return 1;}
int pt_editor_mixed_bridges_bind(struct pt_editor_mixed_bridges *out,struct pt_editor_mixed *o,
    const struct pt_editor_mixed_bridge_inputs *in,const struct pt_sampler_storage_span *contexts,unsigned count)
{
    struct pt_editor *e;struct pt_project *p;struct pt_amigus_wavetable_cache *b;
    struct pt_sampler_paula *pb;struct pt_sampler_wavetable *ab;int8_t map[PT_CHANNEL_LIMIT];
    const void *opaque[9];unsigned i;
    if(!out||!in||!pt_editor_mixed_attached(o)||count>PT_EDITOR_BRIDGE_CONTEXTS||
       !span(contexts,count*sizeof(*contexts))||!apart(out,sizeof(*out),in,sizeof(*in))||
       !apart(out,sizeof(*out),o,sizeof(*o))||!apart(out,sizeof(*out),contexts,count*sizeof(*contexts))||
       o->owner||o->transport||o->preparation_close||o->preparation_context||o->owner_finish||o->owner_finish_context)return 0;
    e=o->editor;p=e->project;
    if(!apart(out,sizeof(*out),e,sizeof(*e))||
       !pt_sampler_output_disjoint(&e->sampler,out,sizeof(*out))||
       !pt_render_project_storage_output_disjoint(p,out,sizeof(*out)))return 0;
    for(i=0;i<count;++i)if(!apart(out,sizeof(*out),contexts[i].data,contexts[i].bytes))return 0;
    b=in->backend;
    if(!b||!apart(out,sizeof(*out),b,sizeof(*b))||!b->reservation||
       !apart(out,sizeof(*out),b->reservation,sizeof(*b->reservation))||
       b->closing||b->faulted||b->cache.bytes||!b->reservation->opened||!b->reservation->reserved||
       !b->reservation->access||b->reservation->interrupt||b->reservation->resource!=PT_AMIGUS_WAVETABLE||
       !in->chip_allocate||!in->chip_release||!in->paula.start||!in->paula.stop||
       !in->amigus.start||!in->amigus.stop||!in->paula_quiesce||!in->amigus_quiesce||
       !e->sampler.allocator.allocate||!e->sampler.allocator.release||
       !p->sample_count||p->sample_count>PT_PROJECT_SAMPLES||
       pt_channels_paula_map(&p->channels,NULL,map)!=PT_CHANNEL_OK)return 0;
    for(i=0;i<PT_CACHE_SLOTS;++i)if(b->cache.entry[i].data||b->cache.entry[i].bytes||
        b->cache.entry[i].pins||b->cache.entry[i].valid||b->arena.block[i].reserved)return 0;
    opaque[0]=in->chip_context;opaque[1]=in->paula.context;opaque[2]=in->amigus.context;
    opaque[3]=in->paula_quiesce_context;opaque[4]=in->amigus_quiesce_context;
    opaque[5]=b->context;opaque[6]=b->arena.context;opaque[7]=b->cache.context;opaque[8]=b->reservation->api.context;
    for(i=0;i<9;++i)if(!apart(out,sizeof(*out),opaque[i],opaque[i]?1:0))return 0;
    if(!zero(out,sizeof(*out)))return 0;
    /* No fallible operations or callbacks beyond this publication boundary. */
    pb=&out->paula_cache;ab=&out->amigus_cache;
    pb->sampler=&e->sampler;pb->project=p;pb->table=p->samples;pb->count=p->sample_count;
    pb->generation=e->sampler.generation;pb->channels=p->channels.count;pb->version=1;
    for(i=0;i<PT_CHANNEL_LIMIT;++i)pb->routes[i]=p->channels.track[i].route;
    pt_cache_init(&pb->cache,in->chip_context,in->chip_allocate,in->chip_release,in->chip_budget);
    ab->sampler=&e->sampler;ab->project=p;ab->backend=b;ab->table=p->samples;
    ab->count=p->sample_count;ab->generation=e->sampler.generation;ab->version=1;
    out->paula.bridge=pb;out->paula.api=in->paula;memcpy(out->paula.map,map,sizeof(map));
    for(i=0;i<PT_PAULA_VOICES;++i)out->paula.voice[i].track=-1;
    out->paula.quiesce=in->paula_quiesce;out->paula.quiesce_context=in->paula_quiesce_context;
    out->amigus.bridge=ab;out->amigus.api=in->amigus;
    out->amigus.quiesce=in->amigus_quiesce;out->amigus.quiesce_context=in->amigus_quiesce_context;
    return 1;
}
