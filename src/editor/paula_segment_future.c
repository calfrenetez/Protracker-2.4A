#include "paula_segment_future.h"
#include <stdint.h>
#include "../core/elapsed_clock.h"
#include <string.h>
#define FUTURE_MAGIC UINT64_C(0x505447464f555234)
#define FUTURE_SPANS 128U
struct future_command {
    struct pt_paula_future_packet packet;
    const struct pt_readers_event *event;struct pt_readers_event original;
    const struct pt_paula_segment_plan *plan;
    struct pt_readers_key key;struct pt_readers_binding binding;
    uint64_t owner,observed,issued;unsigned attempted,delivered,terminal,cancelled,detached,unknown,resource_quiet;
    enum pt_readers_command result;
};
struct pt_paula_segment_future {
    uint64_t magic;struct pt_paula_segment_future *self;
    struct pt_paula_future_ops ops;struct pt_scheduled_grid grid;struct pt_elapsed_clock clock;uint64_t frame[4],last_tick;unsigned clock_seen;
    struct future_command command[4];unsigned count,detached,failed,busy,closing;
    const struct pt_readers_domain *domain;struct pt_readers_domain original_domain;
    struct pt_scheduled_span spans[FUTURE_SPANS];
    struct pt_readers_key key;struct pt_readers_binding reader_binding;
    unsigned reader_retired,adopted,source_attempted,source_quiet,restore_unknown,reader_unknown,pcm_unknown;
    uint64_t trigger_observed,trigger_issued;
};
struct future_alignment {char byte;struct pt_paula_segment_future value;};
struct ops_alignment {char byte;struct pt_paula_future_ops value;};
struct grid_alignment {char byte;struct pt_scheduled_grid value;};
struct backend_alignment {char byte;struct pt_paula_segment_backend value;};
struct event_alignment {char byte;struct pt_readers_event value;};
struct domain_alignment {char byte;struct pt_readers_domain value;};
struct plan_alignment {char byte;struct pt_paula_segment_plan value;};
struct span_alignment {char byte;struct pt_scheduled_span value;};
struct u64_alignment {char byte;uint64_t value;};
struct u32_alignment {char byte;uint32_t value;};
struct pcm_alignment {char byte;struct pt_pcm value;};
/* Unexposed automatic witness survives prohibited health/reentry edits. The
 * model is serialized across all instances; this is not a native concurrency
 * primitive. Provider callbacks never legitimately modify this owned state.
 * Repair copies only this model's saved holder metadata, no genuine core/cache
 * histories, reference counters, PCM content or provider effects. */
struct future_descriptor {const struct pt_pcm *pointer;struct pt_pcm original;};
struct future_witness {struct pt_paula_segment_future *pool;unsigned fault,count;
    struct future_descriptor source[8];};
static int borrowed_live(const struct pt_paula_segment_future *);
static struct future_witness *active;
static int extent(const void *p,size_t n,size_t alignment)
{uintptr_t a=(uintptr_t)p;return p&&n&&alignment&&a%alignment==0&&a<=UINTPTR_MAX-n;}
static int apart(const void *a,size_t an,const void *b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&x<=UINTPTR_MAX-an&&y<=UINTPTR_MAX-bn&&(x+an<=y||y+bn<=x);}
static int key_equal(const struct pt_readers_key *a,const struct pt_readers_key *b)
{return a->queue==b->queue&&a->session==b->session&&a->generation==b->generation&&a->trigger==b->trigger&&a->owner==b->owner&&a->serial==b->serial&&a->action==b->action&&a->slot==b->slot;}
static int current(const struct pt_paula_segment_future *p)
{return extent(p,sizeof(*p),offsetof(struct future_alignment,value))&&p->magic==FUTURE_MAGIC&&p->self==p;}
static int entry_allowed(struct pt_paula_segment_future *p)
{if(active){active->fault=1;return 0;}return current(p)&&!p->busy&&!p->restore_unknown;}
static int output_safe(const struct pt_paula_segment_future *p,const void *out,size_t bytes)
{unsigned i;
 if(!extent(out,bytes,1)||!apart(p,sizeof(*p),out,bytes)||!apart(p->ops.context,p->ops.context_bytes,out,bytes))return 0;
 /* Numeric original extents remain excluded through disposal. Retired storage
  * is not read and no genuine core ownership or history is restored. */
 if(p->domain&&(!apart(p->domain,sizeof(*p->domain),out,bytes)||
    !apart(p->original_domain.spans,p->original_domain.count*sizeof(*p->spans),out,bytes)||
    !apart(p->reader_binding.context,p->reader_binding.context_bytes,out,bytes)))return 0;
 for(i=0;i<p->original_domain.count;++i)if(!apart(p->spans[i].data,p->spans[i].bytes,out,bytes))return 0;
 for(i=0;i<p->count;++i)if(
    !apart(p->command[i].event,sizeof(*p->command[i].event),out,bytes)||
    !apart(p->command[i].plan,sizeof(*p->command[i].plan),out,bytes)||
    !apart(p->command[i].binding.context,p->command[i].binding.context_bytes,out,bytes))return 0;
 return 1;}
static int saved_equal(struct pt_paula_segment_future *p,const struct pt_paula_segment_future *saved)
{if(memcmp(p,saved,sizeof(*p))){memcpy(p,saved,sizeof(*p));p->failed=1;return 0;}return 1;}
static int callback_begin(struct pt_paula_segment_future *p,struct future_witness *w,struct pt_paula_segment_future *saved)
{unsigned i,j;
 if(active){active->fault=1;p->failed=1;return 0;}
 memcpy(saved,p,sizeof(*saved));w->pool=p;w->fault=0;w->count=0;
 /* The independent reader still retains every sealed source after individual
   * command detachment. Capture at most four packets times two sources from
   * authenticated saved addresses until its genuine positive retirement. Never
   * follow detached event storage or source descriptors after that retirement.
   * Current metadata is captured without certifying a version for activation. */
  if(!p->reader_retired)for(i=0;i<p->count;++i)
    for(j=0;j<p->command[i].packet.geometry.source_count;++j){
       const struct pt_pcm *d=p->command[i].packet.geometry.source[j].descriptor;
       w->source[w->count].pointer=d;memcpy(&w->source[w->count].original,d,sizeof(*d));++w->count;}
 active=w;return 1;}
static int callback_end(struct pt_paula_segment_future *p,struct future_witness *w,const struct pt_paula_segment_future *saved,int *borrowed)
{unsigned i;int same=saved_equal(p,saved);
 if(active!=w||w->pool!=p||w->fault){p->failed=1;same=0;}
 *borrowed=borrowed_live(p);
 for(i=0;i<w->count;++i)if(memcmp(w->source[i].pointer,&w->source[i].original,sizeof(w->source[i].original))){
     *borrowed=0;p->pcm_unknown=1;}
  /* Source PCM is not supplied facade-owned descriptor storage. A forbidden
   * change cannot be repaired here or become retirement proof on a later
   * unchanged callback; keep the independent reader and all real resources. */
 if(!*borrowed){p->failed=1;p->reader_unknown=1;}active=NULL;return same;}
static int enter(struct pt_paula_segment_future *p,enum pt_paula_future_entry kind,uint64_t *token)
{struct future_witness w;struct pt_paula_segment_future saved;int raw,good,borrowed;
 if(!entry_allowed(p))return 0;p->busy=1;
 if(!callback_begin(p,&w,&saved)){p->busy=0;return 0;}
 raw=saved.ops.enter(saved.ops.context,kind,token);good=callback_end(p,&w,&saved,&borrowed);
 /* A valid raw1 entry remains acquired despite a separately detected borrowed
  * mutation. Its exact matching leave is attempted before unknown admission. */
 if(raw==1&&good)return 1;
 if(raw!=0||!good||!borrowed){p->failed=1;p->restore_unknown=1;}p->busy=0;return 0;}
static int leave(struct pt_paula_segment_future *p,enum pt_paula_future_entry kind,uint64_t token)
{struct future_witness w;struct pt_paula_segment_future saved;int raw,good,borrowed;
 if(!callback_begin(p,&w,&saved)){p->busy=0;return 0;}
 raw=saved.ops.leave(saved.ops.context,kind,token);good=callback_end(p,&w,&saved,&borrowed);p->busy=0;
 /* Borrowed failure is sticky, separately from actual outer restoration. No
  * genuine core descriptor or source object is repaired here. */
 if(raw!=1||!good){p->failed=1;p->restore_unknown=1;return 0;}return borrowed?1:2;}
static int clock_locked(struct pt_paula_segment_future *p,uint64_t *tick)
{struct future_witness w;struct pt_paula_segment_future saved;uint64_t t=0;uint32_t f=0;int raw,good,borrowed;
 if(!callback_begin(p,&w,&saved))return 0;
 raw=saved.ops.clock(saved.ops.context,&t,&f);good=callback_end(p,&w,&saved,&borrowed);
 if(raw!=1||!good||!borrowed||f!=p->grid.frequency||t<p->grid.epoch||(p->clock_seen&&t<p->last_tick)){p->failed=1;return 0;}p->last_tick=t;p->clock_seen=1;*tick=t;return 1;}
enum packet_operation {PACKET_ARM,PACKET_REARM,PACKET_APPLY};
static int packet_call(struct pt_paula_segment_future *p,unsigned i,enum packet_operation op)
{struct future_witness w;struct pt_paula_segment_future saved;int raw,good,borrowed;
 if(!callback_begin(p,&w,&saved))return -1;
 if(op==PACKET_ARM)raw=saved.ops.arm(saved.ops.context,&p->command[i].packet);
 else if(op==PACKET_REARM)raw=saved.ops.rearm(saved.ops.context,&p->command[i].packet);
 else raw=saved.ops.apply(saved.ops.context,&p->command[i].packet);
 good=callback_end(p,&w,&saved,&borrowed);return good&&borrowed?raw:-1;}
enum token_operation {TOKEN_ACK,TOKEN_CANCEL,TOKEN_QUIET};
static int token_call(struct pt_paula_segment_future *p,uint64_t ticket,enum token_operation op)
{struct future_witness w;struct pt_paula_segment_future saved;int raw,good,borrowed;
 if(!callback_begin(p,&w,&saved))return -1;
 if(op==TOKEN_ACK)raw=saved.ops.ack(saved.ops.context,ticket);
 else if(op==TOKEN_CANCEL)raw=saved.ops.cancel(saved.ops.context,ticket);
 else raw=saved.ops.ticket_quiet(saved.ops.context,ticket);
 good=callback_end(p,&w,&saved,&borrowed);return good&&borrowed?raw:-1;}
enum source_operation {SOURCE_READER,SOURCE_CLOSE,SOURCE_QUIET};
static int source_call(struct pt_paula_segment_future *p,enum source_operation op)
{struct future_witness w;struct pt_paula_segment_future saved;int raw,good,borrowed;
 if(!callback_begin(p,&w,&saved))return -1;
 if(op==SOURCE_READER)raw=saved.ops.reader_quiet(saved.ops.context);
 else if(op==SOURCE_CLOSE)raw=saved.ops.source_close(saved.ops.context);
 else raw=saved.ops.source_quiet(saved.ops.context);
 good=callback_end(p,&w,&saved,&borrowed);return good&&borrowed?raw:-1;}
static int covered(const struct pt_readers_domain *d,const void *data,size_t bytes)
{unsigned i;uintptr_t a=(uintptr_t)data;
 if(!extent(data,bytes,1))return 0;
 for(i=0;i<d->count;++i){uintptr_t b=(uintptr_t)d->spans[i].data;size_t n=d->spans[i].bytes;
    if(extent(d->spans[i].data,n,1)&&a>=b&&a+bytes<=b+n)return 1;}return 0;}
static int pcm_equal(const struct pt_pcm *a,const struct pt_pcm *b)
{return a->data==b->data&&a->capacity==b->capacity&&a->frames==b->frames&&a->rate==b->rate&&a->channels==b->channels&&a->bits==b->bits;}
static int source_valid(const struct pt_readers_domain *d,const struct pt_paula_segment_source *s)
{unsigned i;size_t bytes;
 if(!extent(s->descriptor,sizeof(*s->descriptor),offsetof(struct pcm_alignment,value))||!s->version||!s->sampler_owner||!s->project_identity||!s->cache_owner||!s->cache_version||
    !s->lease.serial||!s->cache_data||!s->cache_bytes||s->guard_count>PT_PAULA_SEGMENT_SOURCE_GUARDS||
    !s->master.capacity||s->master.capacity>SIZE_MAX/sizeof(*s->master.data))return 0;
 bytes=s->master.capacity*sizeof(*s->master.data);
 if(!covered(d,s->descriptor,sizeof(*s->descriptor))||!covered(d,s->master.data,bytes)||
    !covered(d,s->cache_data,s->cache_bytes))return 0;
 for(i=0;i<s->guard_count;++i)if(!covered(d,s->guard[i].data,s->guard[i].bytes))return 0;
 return 1;}
static int range_valid(const struct pt_paula_segment_range *r,const struct pt_paula_segment_source *source)
{uint64_t end=(uint64_t)r->frame_offset+r->frame_length;
 return r->frame_length&&!(r->frame_offset&1U)&&!(r->frame_length&1U)&&r->frame_offset==r->byte_offset&&
    r->frame_length==r->byte_length&&r->words&&r->byte_length/2U==r->words&&end<=source->master.frames&&end<=source->cache_bytes;}
static int geometry_valid(const struct pt_paula_segment_plan *g,const struct pt_scheduled_action *a)
{unsigned j;
 if(g->volume>64||g->slot!=a->slot)return 0;
 for(j=0;j<g->source_count;++j){const struct pt_paula_segment_source *s=g->source+j;
    if(s->master.channels!=1||(s->master.bits!=8&&s->master.bits!=16&&s->master.bits!=24)||!s->master.rate||
       s->master.frames>s->master.capacity||
       (uint64_t)s->cache_bytes!=(uint64_t)s->master.frames+(s->master.frames&1U)||((uintptr_t)s->cache_data&1U))return 0;}
 if(g->kind==PT_PAULA_SEGMENT_STOP_REQUIREMENTS)return !a->data&&!a->words&&!a->period&&!a->volume;
 if(!g->period||!range_valid(&g->initial,g->source)||!range_valid(&g->repeat,g->source+g->source_count-1))return 0;
 if(g->write_initial)return a->data==g->source[0].cache_data+g->initial.byte_offset&&a->words==g->initial.words;
 return !a->data&&!a->words;}
static int domain_live(const struct pt_paula_segment_future *p)
{if(p->reader_retired||!p->domain)return 0;
 return !memcmp(p->domain,&p->original_domain,sizeof(p->original_domain))&&
    !memcmp(p->original_domain.spans,p->spans,p->original_domain.count*sizeof(p->spans[0]));}
static int borrowed_live(const struct pt_paula_segment_future *p)
{unsigned i;
 if(p->domain&&!p->reader_retired&&!domain_live(p))return 0;
 for(i=0;i<p->count;++i)if(!p->command[i].detached&&
    (memcmp(p->command[i].event,&p->command[i].original,sizeof(p->command[i].original))||
     (!p->reader_retired&&memcmp(p->command[i].plan,&p->command[i].packet.geometry,sizeof(*p->command[i].plan)))))return 0;
 return 1;}
static int command_live(struct pt_paula_segment_future *p,unsigned i)
{struct future_command *c=p->command+i;unsigned j;
 if(!domain_live(p)||!c->event||!c->plan||memcmp(c->event,&c->original,sizeof(c->original))||
    memcmp(c->plan,&c->packet.geometry,sizeof(*c->plan)))return 0;
 for(j=0;j<c->packet.geometry.source_count;++j){const struct pt_paula_segment_source *s=c->packet.geometry.source+j;
    if(!pcm_equal(s->descriptor,&s->master))return 0;}
 return 1;}
static int input(struct pt_paula_segment_future *p,const struct pt_readers_event *e,const struct pt_paula_segment_plan *g)
{const struct pt_readers_domain *d;const struct pt_scheduled_action *a;unsigned i;uint64_t first,last;enum pt_scheduled_kind kind;
 if(!extent(e,sizeof(*e),offsetof(struct event_alignment,value))||!extent(g,sizeof(*g),offsetof(struct plan_alignment,value))||
    !apart(e,sizeof(*e),g,sizeof(*g))||!apart(p,sizeof(*p),e,sizeof(*e))||!apart(p,sizeof(*p),g,sizeof(*g))||
    !apart(p->ops.context,p->ops.context_bytes,e,sizeof(*e))||!apart(p->ops.context,p->ops.context_bytes,g,sizeof(*g)))return 0;
 if(p->count>=4||e->scheduled.batch.count!=1||!e->scheduled.ticket||e->scheduled.first>=e->scheduled.last||
    e->scheduled.batch.frame!=p->frame[p->count]||g->frame!=p->frame[p->count]||e->scheduled.batch.generation!=p->grid.generation)return 0;
 if(e->scheduled.batch.frame==UINT64_MAX||pt_elapsed_clock_deadline(&p->clock,e->scheduled.batch.frame,&first)!=PT_ELAPSED_OK||
    pt_elapsed_clock_deadline(&p->clock,e->scheduled.batch.frame+1,&last)!=PT_ELAPSED_OK||e->scheduled.first!=first||e->scheduled.last!=last)return 0;
 kind=p->count==0?PT_SCHEDULED_TRIGGER:p->count==3?PT_SCHEDULED_STOP:PT_SCHEDULED_CONTROL;
 a=e->scheduled.batch.action;
 if(a->kind!=kind||a->slot!=g->slot||a->period!=g->period||a->volume!=g->volume||g->source_count<1||g->source_count>2||!geometry_valid(g,a))return 0;
 for(i=1;i<PT_READERS_ACTIONS;++i)if(e->reader[i])return 0;
 if((kind==PT_SCHEDULED_TRIGGER&&(g->kind!=PT_PAULA_SEGMENT_INITIAL||!g->write_initial))||
    (kind==PT_SCHEDULED_CONTROL&&(g->kind!=PT_PAULA_SEGMENT_REPROGRAM||g->write_initial))||
    (kind==PT_SCHEDULED_STOP&&(g->kind!=PT_PAULA_SEGMENT_STOP_REQUIREMENTS||g->write_initial||g->write_repeat)))return 0;
 d=e->reader[0];if(!extent(d,sizeof(*d),offsetof(struct domain_alignment,value))||
    !apart(e,sizeof(*e),d,sizeof(*d))||!apart(g,sizeof(*g),d,sizeof(*d))||!apart(p,sizeof(*p),d,sizeof(*d))||
    !apart(p->ops.context,p->ops.context_bytes,d,sizeof(*d)))return 0;
 if(!d->count||d->count>FUTURE_SPANS||!extent(d->spans,d->count*sizeof(*d->spans),offsetof(struct span_alignment,value))||
    !apart(e,sizeof(*e),d->spans,d->count*sizeof(*d->spans))||!apart(g,sizeof(*g),d->spans,d->count*sizeof(*d->spans))||
    !apart(d,sizeof(*d),d->spans,d->count*sizeof(*d->spans))||
    !apart(p,sizeof(*p),d->spans,d->count*sizeof(*d->spans))||!apart(p->ops.context,p->ops.context_bytes,d->spans,d->count*sizeof(*d->spans))||
    d->key.queue!=e->queue||d->key.session!=e->session||d->key.generation!=p->grid.generation||d->key.slot!=a->slot||!d->key.owner||!d->key.serial||
    !extent(d->binding.context,d->binding.context_bytes,1)||!extent(e->binding.context,e->binding.context_bytes,1)||
    !apart(p,sizeof(*p),d->binding.context,d->binding.context_bytes)||!apart(p,sizeof(*p),e->binding.context,e->binding.context_bytes)||
    !apart(p->ops.context,p->ops.context_bytes,d->binding.context,d->binding.context_bytes)||
    !apart(p->ops.context,p->ops.context_bytes,e->binding.context,e->binding.context_bytes))return 0;
 for(i=0;i<d->count;++i)if(!extent(d->spans[i].data,d->spans[i].bytes,1)||
    !apart(e,sizeof(*e),d->spans[i].data,d->spans[i].bytes)||!apart(g,sizeof(*g),d->spans[i].data,d->spans[i].bytes)||
    !apart(d,sizeof(*d),d->spans[i].data,d->spans[i].bytes)||!apart(d->spans,d->count*sizeof(*d->spans),d->spans[i].data,d->spans[i].bytes)||
    !apart(p,sizeof(*p),d->spans[i].data,d->spans[i].bytes)||!apart(p->ops.context,p->ops.context_bytes,d->spans[i].data,d->spans[i].bytes))return 0;
 for(i=0;i<g->source_count;++i)if(!source_valid(d,g->source+i)||!pcm_equal(g->source[i].descriptor,&g->source[i].master))return 0;
 if(p->count&&(!p->adopted||p->reader_retired||p->domain!=d||!key_equal(&p->key,&d->key)||!domain_live(p)))return 0;
 for(i=0;i<p->count;++i)if(p->command[i].packet.ticket==e->scheduled.ticket)return 0;
 return 1;}
static int backend_clock(void *v,uint64_t *tick,uint32_t *frequency)
{struct pt_paula_segment_future *p=v;uint64_t token,t;int good;
 if(!entry_allowed(p)||!extent(tick,sizeof(*tick),offsetof(struct u64_alignment,value))||
    !extent(frequency,sizeof(*frequency),offsetof(struct u32_alignment,value))||!output_safe(p,tick,sizeof(*tick))||!output_safe(p,frequency,sizeof(*frequency))||
    !apart(tick,sizeof(*tick),frequency,sizeof(*frequency))||!enter(p,PT_PAULA_FUTURE_TASK,&token))return 0;
 good=clock_locked(p,&t);if(leave(p,PT_PAULA_FUTURE_TASK,token)!=1)good=0;
 if(good){*tick=t;*frequency=p->grid.frequency;}return good;}
static void no_arm_failure(struct pt_paula_segment_future *p,unsigned i)
{p->command[i].unknown=1;p->command[i].terminal=1;p->command[i].result=PT_READERS_FAILED;
 p->command[i].resource_quiet=1;p->reader_unknown=1;p->failed=1;}
static void forget_clean_attempt(struct pt_paula_segment_future *p,unsigned i)
{--p->count;memset(p->command+i,0,sizeof(p->command[i]));if(!i){p->domain=NULL;memset(&p->original_domain,0,sizeof(p->original_domain));memset(p->spans,0,sizeof(p->spans));memset(&p->key,0,sizeof(p->key));memset(&p->reader_binding,0,sizeof(p->reader_binding));}}
static int backend_submit(void *v,const struct pt_readers_event *e,const struct pt_paula_segment_plan *g)
{struct pt_paula_segment_future *p=v;uint64_t token,now;unsigned i;int raw;
 if(!entry_allowed(p)||p->failed||p->closing||!input(p,e,g))return 0;
 /* Seal every original pointer/count before the first provider callback. This
  * is staged metadata, not a genuine core publication or borrowed arm ticket. */
 i=p->count;memset(p->command+i,0,sizeof(p->command[i]));
 p->command[i].packet.ticket=e->scheduled.ticket;p->command[i].packet.first=e->scheduled.first;p->command[i].packet.last=e->scheduled.last;
 memcpy(&p->command[i].packet.geometry,g,sizeof(*g));memcpy(&p->command[i].original,e,sizeof(*e));
 p->command[i].event=e;p->command[i].owner=e->command_owner;p->command[i].plan=g;p->command[i].key=e->reader[0]->key;p->command[i].binding=e->binding;
 p->command[i].result=PT_READERS_WAITING;
 if(!i){p->domain=e->reader[0];memcpy(&p->original_domain,p->domain,sizeof(p->original_domain));
    memcpy(p->spans,p->original_domain.spans,p->original_domain.count*sizeof(*p->spans));p->key=p->original_domain.key;p->reader_binding=p->original_domain.binding;}
 ++p->count;
 if(!enter(p,PT_PAULA_FUTURE_TASK,&token)){
    if(!p->failed&&!p->restore_unknown){forget_clean_attempt(p,i);return 0;}no_arm_failure(p,i);return -1;}
 if(p->failed||!borrowed_live(p)||!clock_locked(p,&now)||now>=p->command[i].packet.first){
    no_arm_failure(p,i);(void)leave(p,PT_PAULA_FUTURE_TASK,token);return -1;}
 p->command[i].attempted=1;raw=packet_call(p,i,PACKET_ARM);
 if(raw==0)p->command[i].resource_quiet=1;
 if(raw!=0&&raw!=1){p->command[i].unknown=1;p->reader_unknown=1;p->failed=1;}
 if(leave(p,PT_PAULA_FUTURE_TASK,token)!=1){p->command[i].unknown=1;p->reader_unknown=1;return -1;}
 if(p->failed){p->command[i].unknown=1;p->reader_unknown=1;return -1;}
 /* Only unchanged clean0 AND positive outer restoration permit forgetting the
  * never-borrowed attempt. Unknown restoration keeps the original holder. */
 if(raw==0)forget_clean_attempt(p,i);
 return raw==0?0:raw==1?1:-1;}
static int find(struct pt_paula_segment_future *p,uint64_t ticket,unsigned *index)
{unsigned i;for(i=0;i<p->count;++i)if(p->command[i].packet.ticket==ticket&&!p->command[i].detached){*index=i;return 1;}return 0;}
static void cancel_locked(struct pt_paula_segment_future *p,unsigned i)
{struct future_command *c=p->command+i;int raw;
 if(c->cancelled)return;c->cancelled=1;raw=c->attempted&&!c->resource_quiet?token_call(p,c->packet.ticket,TOKEN_CANCEL):1;
 if(raw!=1){c->unknown=1;p->reader_unknown=1;p->failed=1;}
 if(!c->terminal){c->terminal=1;c->result=c->unknown?PT_READERS_FAILED:c->delivered?PT_READERS_CANCELLED_AFTER:PT_READERS_CANCELLED_BEFORE;}
 /* No packet/borrowed descriptor is discarded on a cancellation request. */}
static void command_receipt(struct pt_paula_segment_future *p,unsigned i,struct pt_readers_command_receipt *out)
{struct future_command *c=p->command+i;memset(out,0,sizeof(*out));out->domain=PT_READERS_COMMAND_DOMAIN;out->origin=PT_READERS_BACKEND_ACTUAL;
 out->queue=c->key.queue;out->session=c->key.session;out->generation=c->key.generation;out->ticket=c->packet.ticket;
 out->owner=c->owner;out->count=1;out->event=c->event;out->context=c->binding.context;out->context_bytes=c->binding.context_bytes;
 out->action[0].command=c->result;out->action[0].reader=p->reader_retired?PT_READERS_RETIRED:p->adopted?(p->closing?PT_READERS_DRAINING:PT_READERS_ACTIVE):PT_READERS_RESERVED;
 out->action[0].adoption=p->reader_unknown?(enum pt_readers_adoption)2:p->adopted?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;out->action[0].key=c->key;
 out->action[0].observed=c->observed;out->action[0].issued=c->issued;
 if(c->result==PT_READERS_CANCELLED_BEFORE&&!p->adopted){out->action[0].reader=PT_READERS_NONE;out->action[0].observed=out->action[0].issued=0;}}
static enum pt_readers_reply check_command(void *v,uint64_t ticket,struct pt_readers_command_receipt *out,int cancel)
{struct pt_paula_segment_future *p=v;struct pt_readers_command_receipt receipt;uint64_t token;unsigned i=0;enum pt_readers_reply reply=PT_READERS_UNCERTAIN;int quiet;
 if(!entry_allowed(p)||!output_safe(p,out,sizeof(*out))||!enter(p,PT_PAULA_FUTURE_TASK,&token))return PT_READERS_UNCERTAIN;
 if(!find(p,ticket,&i))goto done;
 if(cancel){p->closing=1;cancel_locked(p,i);}
 if(!p->command[i].terminal){reply=PT_READERS_PENDING;goto done;}
 command_receipt(p,i,&receipt);quiet=p->command[i].resource_quiet?1:token_call(p,ticket,TOKEN_QUIET);
 if(quiet==1){p->command[i].resource_quiet=1;reply=PT_READERS_COMMAND_DETACHED;}
 else if(quiet<0){p->failed=1;reply=PT_READERS_UNCERTAIN;}else reply=PT_READERS_OBSERVATION;
 done:if(leave(p,PT_PAULA_FUTURE_TASK,token)!=1)return PT_READERS_UNCERTAIN;
 /* No recovery anchor is consumed until the exact quiet proof and final
  * matching outer restoration have both actually returned positive. */
 if(reply==PT_READERS_COMMAND_DETACHED){p->command[i].detached=1;++p->detached;}
 if(reply==PT_READERS_OBSERVATION||reply==PT_READERS_COMMAND_DETACHED)memcpy(out,&receipt,sizeof(receipt));return reply;}
static enum pt_readers_reply backend_poll(void *p,uint64_t t,struct pt_readers_command_receipt *r){return check_command(p,t,r,0);}
static enum pt_readers_reply backend_cancel(void *p,uint64_t t,struct pt_readers_command_receipt *r){return check_command(p,t,r,1);}
static enum pt_readers_reply check_reader(void *v,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *out,int cancel)
{struct pt_paula_segment_future *p=v;struct pt_readers_reader_receipt receipt;uint64_t token;unsigned i;int quiet;enum pt_readers_reply reply;
 if(!entry_allowed(p)||!output_safe(p,out,sizeof(*out))||p->reader_retired||p->pcm_unknown||d!=p->domain||!domain_live(p)||!enter(p,PT_PAULA_FUTURE_TASK,&token))return PT_READERS_UNCERTAIN;
  if(p->pcm_unknown){reply=PT_READERS_UNCERTAIN;goto done;}
 if(cancel){p->closing=1;for(i=0;i<p->count;++i)if(!p->command[i].detached)cancel_locked(p,i);}
 memset(&receipt,0,sizeof(receipt));receipt.domain=PT_READERS_READER_DOMAIN;receipt.key=p->key;receipt.reference=p->domain;
 receipt.context=p->reader_binding.context;receipt.context_bytes=p->reader_binding.context_bytes;
 receipt.adoption=p->reader_unknown?(enum pt_readers_adoption)2:p->adopted?PT_READERS_ADOPTED:PT_READERS_UNADOPTED;receipt.observed=p->trigger_observed;receipt.issued=p->trigger_issued;
 /* A currently pending future event forbids reader retirement even if a
  * provider mistakenly reports output quiet. Both proofs are still required. */
 for(i=0;i<p->count;++i)if(!p->command[i].terminal){reply=PT_READERS_PENDING;goto done;}
 /* Reader-first cleanup still requires independent absence of every timer
  * packet which could carry source pointers. It does NOT detach core commands. */
 for(i=0;i<p->count;++i)if(!p->command[i].resource_quiet){quiet=token_call(p,p->command[i].packet.ticket,TOKEN_QUIET);
    if(quiet!=1){if(quiet<0)p->failed=1;reply=quiet<0?PT_READERS_UNCERTAIN:PT_READERS_PENDING;goto done;}
    p->command[i].resource_quiet=1;}
 quiet=source_call(p,SOURCE_READER);
 if(quiet==1){receipt.state=PT_READERS_RETIRED;reply=PT_READERS_READER_RETIRED;}
 else if(quiet<0){p->failed=1;reply=PT_READERS_UNCERTAIN;}
 else{receipt.state=p->adopted?(p->closing?PT_READERS_DRAINING:PT_READERS_ACTIVE):PT_READERS_RESERVED;reply=PT_READERS_OBSERVATION;}
 done:if(leave(p,PT_PAULA_FUTURE_TASK,token)!=1)return PT_READERS_UNCERTAIN;
 if(reply==PT_READERS_READER_RETIRED)p->reader_retired=1;
 if(reply==PT_READERS_OBSERVATION||reply==PT_READERS_READER_RETIRED)memcpy(out,&receipt,sizeof(receipt));return reply;}
static enum pt_readers_reply backend_reader(void *p,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *r){return check_reader(p,d,r,0);}
static enum pt_readers_reply backend_cancel_reader(void *p,const struct pt_readers_domain *d,struct pt_readers_reader_receipt *r){return check_reader(p,d,r,1);}
size_t pt_paula_segment_future_size(void){return sizeof(struct pt_paula_segment_future);}
size_t pt_paula_segment_future_alignment(void){return offsetof(struct future_alignment,value);}
int pt_paula_segment_future_init(void *storage,size_t bytes,const struct pt_paula_future_ops *ops,const struct pt_scheduled_grid *grid,const uint64_t frame[4],struct pt_paula_segment_backend *out)
{struct pt_paula_segment_future *p=storage;struct pt_paula_segment_backend b;struct pt_elapsed_clock clock;uint64_t first,last;size_t i;
 if(active){active->fault=1;return 0;}
 if(bytes!=sizeof(*p)||!extent(storage,bytes,offsetof(struct future_alignment,value))||
    !extent(ops,sizeof(*ops),offsetof(struct ops_alignment,value))||!extent(grid,sizeof(*grid),offsetof(struct grid_alignment,value))||
    !extent(frame,4*sizeof(*frame),offsetof(struct u64_alignment,value))||!extent(out,sizeof(*out),offsetof(struct backend_alignment,value))||
    !apart(storage,bytes,ops,sizeof(*ops))||!apart(storage,bytes,grid,sizeof(*grid))||!apart(storage,bytes,frame,4*sizeof(*frame))||
    !apart(storage,bytes,out,sizeof(*out))||!apart(out,sizeof(*out),ops,sizeof(*ops))||!apart(out,sizeof(*out),grid,sizeof(*grid))||
    !apart(out,sizeof(*out),frame,4*sizeof(*frame)))return 0;
 if(!ops->context||!extent(ops->context,ops->context_bytes,1)||!apart(storage,bytes,ops->context,ops->context_bytes)||
    !apart(out,sizeof(*out),ops->context,ops->context_bytes)||!apart(ops,sizeof(*ops),ops->context,ops->context_bytes)||
    !apart(grid,sizeof(*grid),ops->context,ops->context_bytes)||!apart(frame,4*sizeof(*frame),ops->context,ops->context_bytes)||!ops->enter||!ops->leave||!ops->clock||!ops->arm||!ops->rearm||
    !ops->apply||!ops->ack||!ops->cancel||!ops->ticket_quiet||!ops->reader_quiet||!ops->source_close||!ops->source_quiet||
    !grid->frequency||!grid->rate||!grid->generation||grid->frequency<grid->rate||
    pt_elapsed_clock_init(&clock,grid->frequency,grid->rate,grid->epoch,0)!=PT_ELAPSED_OK)return 0;
 for(i=0;i<4;++i)if(!frame[i]||frame[i]==UINT64_MAX||(i&&frame[i]<=frame[i-1])||
    pt_elapsed_clock_deadline(&clock,frame[i],&first)!=PT_ELAPSED_OK||pt_elapsed_clock_deadline(&clock,frame[i]+1,&last)!=PT_ELAPSED_OK||first>=last)return 0;
 for(i=0;i<bytes;++i)if(((const unsigned char *)storage)[i])return 0;
 memset(&b,0,sizeof(b));p->magic=FUTURE_MAGIC;p->self=p;memcpy(&p->ops,ops,sizeof(*ops));memcpy(&p->grid,grid,sizeof(*grid));memcpy(&p->clock,&clock,sizeof(clock));memcpy(p->frame,frame,sizeof(p->frame));
 b.readers=(struct pt_readers_backend){p,sizeof(*p),{PT_SCHEDULED_REQUIRED,4,1},PT_READERS_VERSION,PT_READERS_REQUIRED,1,
    backend_clock,NULL,backend_poll,backend_cancel,backend_reader,backend_cancel_reader};
 b.plan_version=PT_PAULA_SEGMENT_BACKEND_VERSION;b.plan_flags=PT_PAULA_SEGMENT_BACKEND_REQUIRED;b.submit_plan=backend_submit;
 memcpy(out,&b,sizeof(b));return 1;}
enum pt_paula_future_result pt_paula_segment_future_deliver(struct pt_paula_segment_future *p,uint64_t ticket)
{uint64_t token,observed,issued;unsigned i;int raw;enum pt_paula_future_result result;
 if(!entry_allowed(p))return PT_PAULA_FUTURE_INVALID;
 if(!enter(p,PT_PAULA_FUTURE_DELIVERY,&token))return PT_PAULA_FUTURE_UNKNOWN;
 if(!find(p,ticket,&i)||p->command[i].terminal||p->command[i].unknown){result=PT_PAULA_FUTURE_STALE;goto done;}
 if(!clock_locked(p,&observed)||!command_live(p,i)||p->failed||p->closing){p->command[i].unknown=1;p->reader_unknown=1;p->failed=1;result=PT_PAULA_FUTURE_UNKNOWN;goto done;}
 if(observed<p->command[i].packet.first){raw=packet_call(p,i,PACKET_REARM);if(raw!=1){p->command[i].unknown=1;p->reader_unknown=1;p->failed=1;result=PT_PAULA_FUTURE_UNKNOWN;}else result=PT_PAULA_FUTURE_EARLY;goto done;}
 if(observed>=p->command[i].packet.last){p->command[i].terminal=1;p->command[i].result=PT_READERS_FAILED;p->failed=1;
    (void)token_call(p,ticket,TOKEN_ACK);result=PT_PAULA_FUTURE_LATE;goto done;}
 raw=packet_call(p,i,PACKET_APPLY);
 if(raw!=1||!clock_locked(p,&issued)||!command_live(p,i)||issued<observed||issued>=p->command[i].packet.last){p->command[i].unknown=1;p->reader_unknown=1;p->failed=1;result=PT_PAULA_FUTURE_UNKNOWN;goto done;}
 p->command[i].observed=observed;p->command[i].issued=issued;p->command[i].delivered=1;p->command[i].terminal=1;p->command[i].result=PT_READERS_ISSUED;
 if(i==0){p->adopted=1;p->trigger_observed=observed;p->trigger_issued=issued;}
 if(i==3)p->closing=1;
 raw=token_call(p,ticket,TOKEN_ACK);if(raw!=1){p->command[i].unknown=1;p->reader_unknown=1;p->failed=1;result=PT_PAULA_FUTURE_UNKNOWN;}else result=PT_PAULA_FUTURE_OK;
 done:if(leave(p,PT_PAULA_FUTURE_DELIVERY,token)!=1)return PT_PAULA_FUTURE_UNKNOWN;return result;}
enum pt_paula_future_result pt_paula_segment_future_stop(struct pt_paula_segment_future *p)
{uint64_t token;unsigned i;int raw,quiet=0;enum pt_paula_future_result result=PT_PAULA_FUTURE_PENDING;
 if(!entry_allowed(p)||!enter(p,PT_PAULA_FUTURE_TASK,&token))return PT_PAULA_FUTURE_INVALID;
 p->closing=1;for(i=0;i<p->count;++i)if(!p->command[i].detached)cancel_locked(p,i);
 if(p->detached==p->count&&(!p->count||p->reader_retired)){
    if(!p->source_attempted){p->source_attempted=1;raw=source_call(p,SOURCE_CLOSE);if(raw!=1)p->failed=1;}
    raw=source_call(p,SOURCE_QUIET);if(raw==1){quiet=1;result=PT_PAULA_FUTURE_OK;}else if(raw<0){p->failed=1;result=PT_PAULA_FUTURE_UNKNOWN;}}
 if(leave(p,PT_PAULA_FUTURE_TASK,token)!=1)return PT_PAULA_FUTURE_UNKNOWN;if(quiet)p->source_quiet=1;return result;}
int pt_paula_segment_future_get(const struct pt_paula_segment_future *p,struct pt_paula_future_status *out)
{struct pt_paula_future_status s;if(active){active->fault=1;return 0;}
 if(!current(p)||p->busy||!output_safe(p,out,sizeof(*out)))return 0;
 s=(struct pt_paula_future_status){p->count,p->detached,p->reader_retired,p->adopted,p->closing,p->source_attempted,p->source_quiet,p->failed,p->trigger_observed,p->trigger_issued};
 memcpy(out,&s,sizeof(s));return 1;}
int pt_paula_segment_future_dispose(struct pt_paula_segment_future *p)
{if(!entry_allowed(p)||!p->source_quiet||p->detached!=p->count||(p->count&&!p->reader_retired))return 0;
 memset(p,0,sizeof(*p));return 1;}
